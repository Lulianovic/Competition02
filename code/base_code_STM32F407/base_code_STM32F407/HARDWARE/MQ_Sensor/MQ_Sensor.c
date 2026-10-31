/**
 ******************************************************************************
 * @file    MQ_Sensor.c
 * @brief   多种 MQ 系列气体传感器数据采集与浓度换算驱动
 *
 * 支持以下传感器类型：
 * - MQ-8：氢气（H₂）
 * - MQ-135：空气质量（CO₂、NH₃、NOx、苯等）
 * - MQ-136 / MQ-137 / MQ-138：硫化氢（H₂S）、氨气（NH₃）、丙酮（PA）等
 *
 * @note    - 所有传感器共用同一 ADC 通道（通道1）
 *          - 输出值为模拟电压（V），内部计算 ppm 仅用于调试打印
 *          - 换算公式基于典型灵敏度曲线拟合，实际精度受环境温湿度、校准状态影响
 *          - 使用前需确保传感器已预热（通常 >60 秒）
 ******************************************************************************
 */

/**
 * @brief  存储最近一次 ADC 采样原始值（12位，0～4095）
 *
 * 该变量为静态局部变量，仅供本文件内函数使用。
 */
#include "MQ_Sensor.h"


/**
 * @brief MQ-4 传感器校准函数（计算 R0）
 * @param RS: 当前传感器在已知浓度下的等效电阻（单位：kΩ）
 * @details
 *   - 根据 MQ-4 典型灵敏度曲线拟合公式：
 *        log(PPM) = a * log(RS/R0) + b
 *     整理得：
 *        R0 = RS / (PPM / A)^(1/B)
 *   - 本代码采用厂商/社区常用参数：
 *        A = 613.9, B = -2.074 （适用于甲烷）
 *   - CAL_PPM = 20 ppm（校准时假设环境浓度为 20ppm）
 *
 * @note
 *   - R0 是传感器在 **洁净空气（或已知浓度）中** 的基准电阻
 *   - 实际应用中，应在 **标准气体环境** 下校准（如 100ppm CH₄）
 *   - 此处用 20ppm 仅为示例，精度有限！
 */
void MQ4_PPM_Calibration(float RS);

// 全局标志：是否需要执行校准（上电后仅校准一次）
int Calibration = 1;

// 校准时假设的环境气体浓度（单位：PPM）
// 注意：此值应为实际校准环境浓度（如洁净空气≈0，但 MQ 系列通常用 100~1000ppm 标定）
#define CAL_PPM     20  

// 负载电阻 RL（连接在 VCC 与传感器输出之间，单位：kΩ）
// 典型值：RL = 10kΩ ~ 47kΩ，需与硬件电路一致
#define RL          10.0f   

// 传感器基准电阻 R0（校准后存储，单位：kΩ）
static float R0;  


/**
 * @brief 执行 MQ-4 校准（计算 R0）
 * @param RS: 当前测得的传感器电阻（kΩ）
 * @formula
 *   R0 = RS / (CAL_PPM / A)^(1/B)
 *   其中 A=613.9, B=-2.074 → 来自 MQ-4 灵敏度曲线拟合
 */
void MQ4_PPM_Calibration(float RS)
{
    // 根据反向公式计算 R0：
    //   PPM = A * (RS/R0)^B  →  R0 = RS / (PPM/A)^(1/B)
    R0 = RS / pow(CAL_PPM / 613.9f, 1.0f / -2.074f);
}

/**
 * @brief 获取 MQ-4 传感器输出（电压）并计算 PPM 浓度
 * @return: 传感器分压点电压（单位：V），用于调试
 * @details
 *   1. 读取 ADC 值（通道1 → PA1）
 *   2. 转换为电压：volt = ADC * 3.3 / 4095
 *   3. 计算传感器电阻：RS = (VCC - volt) / volt * RL
 *   4. 若首次运行，执行校准（更新 R0）
 *   5. 计算当前 PPM：PPM = A * (RS/R0)^B
 *
 * @hardware 连接：
 *   - MQ-4 AO 引脚 → PA1（ADC1_IN1）
 *   - 电路结构：VCC — [RL] — AO — [MQ-4] — GND
 *                ↑
 *              ADC 采样点
 *
 * @warning
 *   - 校准值 CAL_PPM=20 不准确！洁净空气中甲烷≈0ppm，
 *     但 MQ 系列在 0ppm 时 R0 极大，通常用 **100~1000ppm 标气** 校准
 *   - 建议：校准阶段置于已知浓度环境（如 1000ppm CH₄），设 CAL_PPM=1000
 */

float GetMQ4Out(void)
{
    float RS;           // 传感器当前等效电阻（kΩ）
    double volt = 0;    // 传感器分压点电压（V）
    double ppm = 0;     // 计算出的气体浓度（PPM）

    delay_ms(100);

    // 1. 读取 ADC 值（12位，0~4095）
    adc_value = Get_Adc(1);
    printf("adcvalue = %d.\n", adc_value);

    // 防止 volt == 0（ADC 读数为0时跳过计算）
    if (adc_value == 0) {
        printf("Error: ADC=0 → volt=0, skip calculation.\n");
        return 0.0f;
    }


    // 2. 转换为电压（假设 VREF = 3.3V）
    volt = (double)adc_value * 3.3 / 4095;
    printf("volt = %.6f V.\n", volt);

    // 防止 volt 接近 0（避免除零）
    if (volt < 0.01f) {  // 小于 10mV 认为无效
        printf("Warning: volt too low (%.3fV), skip.\n", volt);
        return volt;
    }


    // 3. 计算传感器电阻 RS（根据分压公式）
    RS = (3.3f - volt) / volt * RL;
    printf("RS = %.2f kΩ.\n", RS);

    // 4. 执行一次性校准（仅上电后第一次调用）
    if (Calibration) 
    {
        // 确保 RS 有效后再校准
        if (RS > 0 && RS < 1000.0f) {  // 合理范围：1kΩ ~ 1000kΩ
            MQ4_PPM_Calibration(RS);
            printf("MQ4_PPM_Calibration: R0 = %.2f kΩ\n", R0);
            Calibration = 0;
        } else {
            printf("Calibration skipped: RS invalid (%.2f kΩ)\n", RS);
        }
    }

    // 确保 R0 已有效初始化
    if (R0 <= 0 || isinf(R0) || isnan(R0)) {
        printf("Error: R0 invalid! Using default R0 = 20.0kΩ\n");
        R0 = 20.0f;  // 设默认值（根据典型值）
    }

    // 5. 计算当前气体浓度（PPM）
    //    公式：PPM = A * (RS / R0)^B
    ppm = 613.9f * pow(RS / R0, -2.074f);
    printf("ppm = %.2f ppm.\n\n", ppm);

    return (float)volt;
}

/**
 * @brief  获取 MQ-8 传感器输出电压（氢气 H₂）
 * @return float 传感器分压输出电压（单位：V），范围约 0.0 ～ 3.3V
 *
 * @details
 * - 内部执行 20ms 延时以稳定读数
 * - 通过 ADC 通道1 采集原始值，并转换为电压
 * - 使用经验公式估算 H₂ 浓度（ppm），仅用于串口调试输出
 * - 实际返回值为 **电压值**，非 ppm
 *
 * @note   公式来源（典型拟合）：
 *         \f$ V_{out} = \frac{3.65 \cdot (ppm)^{0.6406}}{34.88 + (ppm)^{0.6406}} + 0.6 \f$
 *         反解得：
 *         \f$ ppm = 10^{\frac{ \log_{10}\left( \frac{34.88 \cdot (V_{out} - 0.6)}{3.65 - (V_{out} - 0.6)} \right) }{0.6406}} \f$
 *
 * @warning 返回值为电压，若需 ppm 值，应修改函数返回类型或新增接口。
 */
// static unsigned int adc_value = 0; //保存ADC数值

//MQ8数据获取
float GetMQ8Out(void)
{
    double volt = 0;
    double ppm = 0;
    
    delay_ms(20);
    
    //adc数值
    adc_value = Get_Adc(1);
    printf("adcvalue = %d.\n",adc_value);

    
    //adc输出电压
    volt=(double)adc_value*3.3/4095;
    printf("volt = %f.\n",volt);
    
    //根据电压值换算成被检测气体的污染指数

    // MQ135 MQ8 H2 空气质量 通用 
    // volt = ((3.65*pow(ppm,0.3203*2))/(34.88+pow(ppm,0.3203*2))+0.6);
    ppm = pow(10, (log10(34.88*(volt-0.6)/(3.65-volt+0.6)))/(0.3203*2));

    printf("ppm = %f.\n\n",ppm);
    return volt;
}

/**
 * @brief  获取 MQ-135 传感器输出电压（空气质量）
 * @return float 传感器分压输出电压（单位：V）
 *
 * @details
 * - 功能与 \ref GetMQ8Out 类似，但针对 MQ-135 的灵敏度特性
 * - 可检测 CO₂、NH₃、苯、酒精等多种气体（混合响应）
 * - 同样返回电压，ppm 为估算值（以 CO₂ 为参考）
 *
 * @note   使用与 MQ-8 相同的经验公式，适用于通用空气质量评估。
 */
float GetMQ135Out(void)
{
    double volt = 0;
    double ppm = 0;
    
    delay_ms(20);
    
    //adc数值
    adc_value = Get_Adc(1);
    printf("adcvalue = %d.\n",adc_value);

    
    //adc输出电压
    volt=(double)adc_value*3.3/4095;
    printf("volt = %f.\n",volt);
    
    //根据电压值换算成被检测气体的污染指数

    // MQ135 MQ8 H2 空气质量 通用 
    // volt = ((3.65*pow(ppm,0.3203*2))/(34.88+pow(ppm,0.3203*2))+0.6);
    ppm = pow(10, (log10(34.88*(volt-0.6)/(3.65-volt+0.6)))/(0.3203*2));

    printf("ppm = %f.\n\n",ppm);
    return volt;
}

/**
 * @brief  获取 MQ-136 传感器输出电压（H₂S / NH₃ / 丙酮）
 * @return float 传感器分压输出电压（单位：V）
 *
 * @details
 * - 针对 MQ-136/137/138 系列优化的换算公式
 * - 公式中引入系数 4，反映其对特定气体的更高灵敏度
 *
 * @note   公式来源：
 *         \f$ V_{out} = \frac{2.3 \cdot (4 \cdot ppm)^{0.6406}}{34.88 + (4 \cdot ppm)^{0.6406}} + 0.6 \f$
 *         反解后除以 4 得到 ppm。
 */
float GetMQ136Out(void)
{
    double volt = 0;
    double ppm = 0;
    
    delay_ms(20);
    
    //adc数值
    adc_value = Get_Adc(1);
    printf("adcvalue = %d.\n",adc_value);

    
    //adc输出电压
    volt=(double)adc_value*3.3/4095;
    printf("volt = %f.\n",volt);
    
    //根据电压值换算成被检测气体的污染指数
    //MQ136~138 H2S NH3 PA 通用
    // volt = ((2.3*pow(4*ppm,0.3203*2))/(34.88+pow(4*ppm,0.3203*2))+0.6); 
    ppm = (pow(10, (log10(34.88*(volt-0.6)/(2.3-volt+0.6)))/(0.3203*2)))/4;


    printf("ppm = %f.\n\n",ppm);
    return volt;
}

/**
 * @brief  获取 MQ-137 传感器输出电压（H₂S / NH₃ / 丙酮）
 * @return float 传感器分压输出电压（单位：V）
 *
 * @copydoc GetMQ136Out
 */
float GetMQ137Out(void)
{
    double volt = 0;
    double ppm = 0;
    
    delay_ms(20);
    
    //adc数值
    adc_value = Get_Adc(1);
    printf("adcvalue = %d.\n",adc_value);

    
    //adc输出电压
    volt=(double)adc_value*3.3/4095;
    printf("volt = %f.\n",volt);
    
    //根据电压值换算成被检测气体的污染指数
    //MQ136~138 H2S NH3 PA 通用
    // volt = ((2.3*pow(4*ppm,0.3203*2))/(34.88+pow(4*ppm,0.3203*2))+0.6); 
    ppm = (pow(10, (log10(34.88*(volt-0.6)/(2.3-volt+0.6)))/(0.3203*2)))/4;


    printf("ppm = %f.\n\n",ppm);
    return volt;
}

/**
 * @brief  获取 MQ-138 传感器输出电压（H₂S / NH₃ / 丙酮）
 * @return float 传感器分压输出电压（单位：V）
 *
 * @copydoc GetMQ136Out
 */
float GetMQ138Out(void)
{
    double volt = 0;
    double ppm = 0;
    
    delay_ms(20);
    
    //adc数值
    adc_value = Get_Adc(1);
    printf("adcvalue = %d.\n",adc_value);

    
    //adc输出电压
    volt=(double)adc_value*3.3/4095;
    printf("volt = %f.\n",volt);
    
    //根据电压值换算成被检测气体的污染指数
    //MQ136~138 H2S NH3 PA 通用
    // volt = ((2.3*pow(4*ppm,0.3203*2))/(34.88+pow(4*ppm,0.3203*2))+0.6); 
    ppm = (pow(10, (log10(34.88*(volt-0.6)/(2.3-volt+0.6)))/(0.3203*2)))/4;


    printf("ppm = %f.\n\n",ppm);
    return volt;
}