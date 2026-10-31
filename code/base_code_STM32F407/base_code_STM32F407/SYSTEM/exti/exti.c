/**
 * @file    exti.c
 * @brief   STM32F40x 外部中断（EXTI）配置：PA0/PA1 上升沿触发
 *
 * 本文件实现将 GPIOA 的 PA0 和 PA1 配置为外部中断输入，用于按键检测。
 * - 触发条件：**上升沿**（适用于按键一端接地，另一端接 PAx，按下时从低→高）
 * - 中断线：EXTI0（PA0）、EXTI1（PA1）
 * - 中断优先级：抢占优先级 2，子优先级分别为 0（EXTI0）和 1（EXTI1）
 *
 * @note
 * - 默认假设按键硬件连接为：PA0/PA1 → 按键 → GND，内部下拉（IPD）使空闲时为低电平；
 * - 按键按下时引脚被拉高，产生上升沿，触发中断；
 * - 若硬件使用上拉电阻（按键接 VCC），应改为下降沿触发（EXTI_Trigger_Falling）；
 * - 当前中断服务函数仅清除标志位，用户需在其中添加实际处理逻辑。
 *
 * @warning
 * - PA0/PA1 必须映射到 EXTI0/1（STM32F40x 中默认 AFIO 映射即为此）；
 * - 若使用其他 GPIO（如 PB0），需额外配置 AFIO_EXTICR 寄存器。
 */
#include "exti.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_syscfg.h"



/*------------------ 外部中断初始化 ------------------*/

/**
 * @brief 外部中断初始化函数：配置 PA1 和 PA2 为上升沿触发的外部中断输入
 * @details
 *   - 使用 EXTI_Line1（对应 PA1）和 EXTI_Line2（对应 PA2）
 *   - 触发方式：上升沿（按键松开时触发，需配合下拉电阻）
 *   - 中断优先级：EXTI1 > EXTI2（子优先级 2 < 3）
 *   - 需在 stm32f4xx_it.c 中实现 EXTI1_IRQHandler() 和 EXTI2_IRQHandler()
 *
 * @hardware 要求：
 *   - 按键一端接 PA1/PA2，另一端接 VCC（高电平有效）
 *   - GPIO 配置为下拉输入 → 无按键时为低电平，按下后为高电平
 *   - 上升沿触发 → 按键松开瞬间产生中断（若需按下触发，应改用下降沿）
 *
 */
void EXTI_KEY_Init(void)
{
    NVIC_InitTypeDef   NVIC_InitStructure;
    EXTI_InitTypeDef   EXTI_InitStructure;
    GPIO_InitTypeDef   GPIO_InitStructure;

    /* 使能 GPIOA 时钟（PA1/PA2 属于 GPIOA） */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    /* 使能 SYSCFG 时钟（EXTI 映射需通过 SYSCFG） */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

    /* 将 EXTI 中断线 1 映射到 GPIOA 的 Pin1 */
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource1); // PA1 → EXTI1

    /* 将 EXTI 中断线 2 映射到 GPIOA 的 Pin2 */
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource2); // PA2 → EXTI2

    /* ------------------ 配置 PA1 和 PA2 为下拉输入 ------------------ */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;   // 选择 PA1 和 PA2
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;             // 输入模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;       // 速度（对输入无实质影响）
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;             // 下拉电阻（默认低电平）
    GPIO_Init(GPIOA, &GPIO_InitStructure);                   // 初始化 GPIOA

    /* ------------------ 配置 EXTI_Line1 (PA1) ------------------ */
    EXTI_InitStructure.EXTI_Line = EXTI_Line1;               // 选择中断线 1
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;      // 中断模式（非事件）
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;   // 上升沿触发（低→高）
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;                // 使能该中断线
    EXTI_Init(&EXTI_InitStructure);

    /* ------------------ 配置 EXTI_Line2 (PA2) ------------------ */
    EXTI_InitStructure.EXTI_Line = EXTI_Line2;               // 选择中断线 2
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    /* ------------------ 配置 EXTI1 中断 NVIC ------------------ */
    NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;         // 中断通道：EXTI1
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00; // 抢占优先级 0（最高）
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x02;        // 子优先级 2
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* ------------------ 配置 EXTI2 中断 NVIC ------------------ */
    NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQn;         // 中断通道：EXTI2
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00; // 同抢占优先级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x03;        // 子优先级 3（低于 EXTI1）
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

