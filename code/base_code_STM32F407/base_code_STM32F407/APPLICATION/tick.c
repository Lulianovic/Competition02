/**
 * @file    tick.c
 * @brief   系统毫秒节拍实现（TIM5，1 ms）
 *
 * 时钟说明：
 *   APB1 = 42 MHz，APB1 定时器时钟 = 42 MHz x 2 = 84 MHz
 *   PSC = 84-1  -> 计数时钟 1 MHz（1 us）
 *   ARR = 1000-1 -> 更新周期 1 ms
 */
#include "tick.h"
#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"

/** 全局毫秒计数 */
static volatile uint32_t s_tick_ms = 0;

void Tick_Init(void)
{
    TIM_TimeBaseInitTypeDef tim;
    NVIC_InitTypeDef        nvic;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);

    tim.TIM_Prescaler     = 84 - 1;
    tim.TIM_CounterMode   = TIM_CounterMode_Up;
    tim.TIM_Period        = 1000 - 1;
    tim.TIM_ClockDivision = TIM_CKD_DIV1;
    tim.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM5, &tim);

    TIM_ITConfig(TIM5, TIM_IT_Update, ENABLE);
    TIM_ClearFlag(TIM5, TIM_FLAG_Update);

    nvic.NVIC_IRQChannel                   = TIM5_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1;
    nvic.NVIC_IRQChannelSubPriority        = 1;
    nvic.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&nvic);

    TIM_Cmd(TIM5, ENABLE);
}

uint32_t Tick_GetMs(void)
{
    return s_tick_ms;
}

uint8_t Tick_Due(uint32_t now, uint32_t *last, uint32_t period)
{
    if ((uint32_t)(now - *last) >= period) {
        *last = now;
        return 1;
    }
    return 0;
}

/**
 * @brief TIM5 更新中断：毫秒计数 +1
 * @note  中断内只做计数，不做任何业务处理
 */
void TIM5_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM5, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM5, TIM_IT_Update);
        s_tick_ms++;
    }
}
