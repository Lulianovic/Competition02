#include "adc.h"

// 全局变量定义（在 adc.h 中 extern 声明）
// extern unsigned int adc_value = 0;
unsigned int adc_value = 0;  // 存储 12 位 ADC 转换结果（范围 0~4095）
/**
 * @brief ADC1 初始化函数（非中断、非DMA模式）
 * @details
 *   - 配置指定 GPIO 为模拟输入（无上下拉）
 *   - 设置 ADC 为独立模式、12 位分辨率、右对齐
 *   - 使用软件触发、单通道、连续转换模式（但由 Get_Adc() 控制单次启动）
 *
 * @warning
 *   - STM32F4 ADC 最大时钟频率为 **36MHz（VDDA≥2.7V）或 30MHz（VDDA=2.4~3.6V）**
 *     但推荐 ≤14MHz 以保证精度（数据手册建议）
 */
void ADC_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;
    ADC_CommonInitTypeDef ADC_CommonInitStructure;

    // 使能 GPIO 和 ADC 时钟
    RCC_AHB1PeriphClockCmd(ADC_GPIO_CLK, ENABLE);   // 如 GPIOA
    RCC_APB2PeriphClockCmd(ADC_CLK, ENABLE);        // ADC1

    /* ------------------ GPIO 配置 ------------------ */
    /* 本项目改造：同时初始化 PA0(ADC1_IN0) 与 PA1(ADC1_IN1) 两路模拟输入 */
    GPIO_InitStructure.GPIO_Pin = ADC_PIN | ADC_PIN2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;   // 必须为模拟输入
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;// 无上下拉（外部电路提供偏置）
    GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed; // 高速（对模拟输入无实质影响）
    GPIO_Init(ADC_GPIO_PORT, &GPIO_InitStructure);

    /* ------------------ ADC 基本配置 ------------------ */
    /* 【本项目改造】改为单次转换模式。
     * 原为连续转换模式：切换通道后软件再触发，读回的常是上一个通道的残留结果，
     * 导致 PA0 与 PA1 串扰（表现为两个通道读到同一个值）。 */
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;    // 单次转换模式
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right; // 右对齐（12位结果在低12位）
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1; // 虽设为 TIM1 触发，但...
    ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None; // 实际禁用外部触发
    ADC_InitStructure.ADC_NbrOfConversion = 1;             // 仅1个规则通道
    ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b; // 12位精度
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;          // 单通道，无需扫描
    ADC_Init(ADC1, &ADC_InitStructure);

    /* ------------------ ADC 公共配置（多ADC同步用） ------------------ */
    ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled; // 不使用 DMA
    ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;                // 独立模式
    ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;             
    ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_20Cycles;
    ADC_CommonInit(&ADC_CommonInitStructure);

    /* ------------------ 启动 ADC ------------------ */
    ADC_Cmd(ADC1, ENABLE);            // 使能 ADC1
    // 注意：此处启动了一次转换，但后续由 Get_Adc() 控制，此行可省略
    // ADC_SoftwareStartConv(ADC1);
}

/**
 * @brief 获取指定 ADC 通道的单次转换值（阻塞式）
 * @param ch: ADC 通道号（如 ADC_Channel_1）
 * @return: 12 位 ADC 结果（0 ~ 4095）
 * @note
 *   - 每次调用会重新配置规则通道（灵活性高，但效率低）
 *   - 采样时间设为 56 cycles（适用于中等源阻抗）
 *   - 使用轮询 EOC 标志，非中断方式
 *
 * @example
 *   uint16_t val = Get_Adc(ADC_Channel_1);
 */
uint16_t Get_Adc(uint8_t ch)
{
    uint16_t val;

    // 配置规则通道：ch 为第 1 个（也是唯一一个）转换，采样时间 56 cycles
    ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_56Cycles);

    /* 【本项目改造】切换通道后先做一次"丢弃转换"。
     * 刚切换通道时采样电容上仍残留上一通道的电荷，
     * 直接取第一次结果会偏向上一个通道，因此丢弃第一次、取第二次。 */
    ADC_SoftwareStartConv(ADC1);
    while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
    val = ADC_GetConversionValue(ADC1);      /* 丢弃 */

    // 第二次转换，作为有效结果
    ADC_SoftwareStartConv(ADC1);
    while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
    val = ADC_GetConversionValue(ADC1);

    return val;
}