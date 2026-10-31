/**
 * @file    pwm.c
 * @brief   STM32F407x 定时器3（TIM3）四通道 PWM 输出驱动
 *
 * 本文件配置 TIM3 生成 4 路 PWM 信号，输出到以下引脚：
 * - CH1 → PA6
 * - CH2 → PA7
 * - CH3 → PB0
 * - CH4 → PB1
 *
 * @note
 * - 使用 **PWM 模式 2**（TIM_OCMode_PWM2）：  
 *   当计数器 < CCR 时输出 **低电平**，否则输出 **高电平**（即占空比 = (ARR - CCR) / ARR）；
 *   若需常规 PWM（CCR 越大占空比越高），请改用 TIM_OCMode_PWM1；
 * - 定时器配置：
 *   - 预分频器（PSC）= 167 → 计数频率 = 168MHz / (167+1) = 1000 kHz
 *   - 自动重装载值（ARR）= 999 → PWM 周期 = 1000 / 1000 kHz = **1 ms**（ 1 kHz）
 * - 所有通道默认占空比为 0%（因 CCR 初始为 0）；
 * - 用户可通过 `TIM_SetCompareX(TIM3, value)` 动态调整占空比。
 *
 * @warning
 * - PA6/PA7/PB0/PB1 必须配置为复用推挽输出（AF_PP）；
 * - AFIO 时钟必须使能（用于重映射，但本例使用默认映射）；
 * - 当前未启用 TIM3 更新中断（如需周期回调，请取消注释相关代码）。
 */
#include "sys.h"
#include "pwm.h"
#include "stm32f4xx_conf.h"


/*------------------ PWM 初始化 ------------------*/

/**
 * @brief 初始化 TIM3 为单通道 PWM 输出（PA6 → TIM3_CH1）
 * @details
 * - 使用 PA6 引脚（复用为 TIM3 通道 1）
 * - 配置 GPIO 为复用推挽输出（50 MHz）
 * - 定时器时基配置：
 *     • 预分频器 (PSC) = 84 - 1 = 83
 *     • 自动重装载值 (ARR) = 1000 - 1 = 999
 *   → 计数频率 = APB1_CLK / (PSC+1) = 84 MHz / 84 = 1 MHz
 *   → PWM 频率 = 1 MHz / (ARR+1) = 1 MHz / 1000 = **1 kHz**
 * - 使用 PWM 模式 1（高电平有效：CNT < CCR 时输出高）
 * - 使能 ARR 和 CCR 寄存器的预装载
 *
 * @note
 * - 此配置适用于 **通用控制场景**，如：
 *     • 直流电机调速
 *     • 简单的 LED 亮度调节（1kHz 略低于人眼舒适区，可能有轻微频闪）
 * - 初始占空比为 0%（TIM_Pulse = 0）
 * - 动态设置占空比：调用 `TIM_SetCompare1(TIM3, value)`，
 *   其中 value ∈ [0, 999]（对应 0% ~ 99.9% 占空比）
 *   示例：
 *     TIM_SetCompare1(TIM3, 500); // 50% 占空比 (500/1000)
 *     TIM_SetCompare1(TIM3, 250); // 25% 占空比
 *
 * @warning
 * - 若用于舵机控制（通常需 50Hz），此频率过高（1kHz），会导致舵机无法工作！
 *   舵机请使用 ARR=19999。
 */
void PWM_Init(void)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    TIM_OCInitTypeDef        TIM_OCInitStructure;
    GPIO_InitTypeDef         GPIO_InitStructure;  

    // 1. 使能 TIM3 和 GPIOA 时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    // 2. 将 PA6 映射到 TIM3 的通道 1（AF2 功能）
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_TIM3);

    // 3. 配置 PA6 为复用推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;          // 复用功能模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;     // 最高输出速度 50MHz
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;        // 推挽输出（驱动能力强）
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;      // 无上下拉
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // ------------------ 4. 配置 TIM3 时基（决定 PWM 频率） ------------------
    // 假设系统 APB1 时钟 = 84 MHz（STM32F4 默认配置）
    // 计数器时钟 = 84 MHz / (83 + 1) = 1 MHz → 计数周期 = 1 μs
    // PWM 周期 = (999 + 1) * 1 μs = 1000 μs = 1 ms → 频率 = 1 kHz
    TIM_TimeBaseStructure.TIM_Period = 1000 - 1;          // ARR = 999（计数 0~999）
    TIM_TimeBaseStructure.TIM_Prescaler = 84 - 1;         // PSC = 83（分频 84 倍）
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;          // 不分频
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; // 向上计数
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    // ------------------ 5. 配置通道 1 为 PWM 模式 ------------------
    TIM_OCStructInit(&TIM_OCInitStructure);               // 初始化结构体
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;     // PWM 模式 1：
                                                          //   - CNT < CCR 时，输出有效电平（高）
                                                          //   - CNT >= CCR 时，输出无效电平（低）
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; // 使能输出
    TIM_OCInitStructure.TIM_Pulse = 0;                    // 初始比较值 = 0 → 占空比 0%
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; // 有效电平为高
    TIM_OC1Init(TIM3, &TIM_OCInitStructure);              // 应用到通道 1

    // 6. 使能通道 1 比较寄存器预装载
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);

    // 7. 使能自动重装载寄存器（ARR）预装载
    TIM_ARRPreloadConfig(TIM3, ENABLE);

    // 8. 启动定时器
    TIM_Cmd(TIM3, ENABLE);

    // 9. 触发一次更新事件，使 ARR 和 CCR 立即生效
    TIM_GenerateEvent(TIM3, TIM_EventSource_Update);
}