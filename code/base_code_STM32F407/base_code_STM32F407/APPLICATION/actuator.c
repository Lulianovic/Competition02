/**
 * @file    actuator.c
 * @brief   蜂鸣器与状态灯实现（PC1 / PC2）
 */
#include "actuator.h"
#include "app_config.h"
#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"

static uint32_t s_last_toggle_ms = 0;
static uint8_t  s_led_phase      = 0;

void Actuator_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

    gpio.GPIO_Pin   = BUZZER_PIN | LED_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_OUT;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_PuPd  = GPIO_PuPd_DOWN;
    GPIO_Init(GPIOC, &gpio);

    Actuator_Buzzer(0);
    Actuator_Led(0);
}

void Actuator_Buzzer(uint8_t on)
{
    uint8_t level = BUZZER_ACTIVE_LEVEL ? (on ? 1 : 0) : (on ? 0 : 1);
    if (level) {
        GPIO_SetBits(BUZZER_PORT, BUZZER_PIN);
    } else {
        GPIO_ResetBits(BUZZER_PORT, BUZZER_PIN);
    }
}

void Actuator_Led(uint8_t on)
{
    uint8_t level = LED_ACTIVE_LEVEL ? (on ? 1 : 0) : (on ? 0 : 1);
    if (level) {
        GPIO_SetBits(LED_PORT, LED_PIN);
    } else {
        GPIO_ResetBits(LED_PORT, LED_PIN);
    }
}

void Actuator_LedBlink(uint32_t now_ms, ControlState st)
{
    uint32_t period;

    switch (st) {
        case ST_IRRIGATING: period = LED_BLINK_IRRIGATE; break;
        case ST_FAULT:      period = LED_BLINK_FAULT;    break;
        case ST_COOLDOWN:   period = LED_BLINK_IDLE;     break;
        default:            period = LED_BLINK_IDLE;     break;
    }

    if ((uint32_t)(now_ms - s_last_toggle_ms) >= period) {
        s_last_toggle_ms = now_ms;
        s_led_phase ^= 1;
        Actuator_Led(s_led_phase);
    }
}
