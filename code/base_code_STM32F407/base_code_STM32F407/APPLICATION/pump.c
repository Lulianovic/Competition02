/**
 * @file    pump.c
 * @brief   执行层实现（PC0 -> 继电器 DQ）
 */
#include "pump.h"
#include "app_config.h"
#include "tick.h"
#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"

static uint8_t  s_on        = 0;
static uint32_t s_start_ms  = 0;
static uint32_t s_total_ms  = 0;
static uint32_t s_start_cnt = 0;

void Pump_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

    gpio.GPIO_Pin   = RELAY_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_OUT;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    /* 上电默认电平 = 关泵电平。低电平吸合时关泵为高电平，故用上拉；
     * 高电平吸合时关泵为低电平，故用下拉。避免上电瞬间误吸合。 */
    gpio.GPIO_PuPd  = RELAY_ACTIVE_LEVEL ? GPIO_PuPd_DOWN : GPIO_PuPd_UP;
    GPIO_Init(RELAY_PORT, &gpio);

    s_on       = 0;
    s_start_ms = 0;
    s_total_ms = 0;
    Pump_Set(0);
}

void Pump_Set(uint8_t on)
{
    uint8_t level;

    if (on && !s_on) {
        s_start_ms  = Tick_GetMs();
        s_start_cnt++;
    } else if (!on && s_on) {
        s_total_ms += (uint32_t)(Tick_GetMs() - s_start_ms);
    }
    s_on = on;

    level = RELAY_ACTIVE_LEVEL ? (on ? 1 : 0) : (on ? 0 : 1);
    if (level) {
        GPIO_SetBits(RELAY_PORT, RELAY_PIN);
    } else {
        GPIO_ResetBits(RELAY_PORT, RELAY_PIN);
    }
}

uint8_t Pump_IsOn(void)
{
    return s_on;
}

uint32_t Pump_GetRunMs(uint32_t now_ms)
{
    return s_on ? (uint32_t)(now_ms - s_start_ms) : 0u;
}

uint32_t Pump_GetTotalRunMs(uint32_t now_ms)
{
    return s_total_ms + Pump_GetRunMs(now_ms);
}

uint32_t Pump_GetStartCount(void)
{
    return s_start_cnt;
}
