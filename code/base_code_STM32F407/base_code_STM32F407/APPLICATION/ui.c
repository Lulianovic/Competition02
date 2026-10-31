/**
 * @file    ui.c
 * @brief   OLED 显示实现（y 为页地址 0~7，每页 8 像素）
 */
#include "ui.h"
#include "app_config.h"
#include "serial_protocol.h"
#include "control.h"
#include "iic_oled.h"
#include "stdio.h"

void UI_Init(void)
{
    IIC_OLED_Init();
    IIC_OLED_Clear();

    IIC_OLED_ShowString(0, 0, "HuiGuan Irrigation");
    IIC_OLED_ShowString(0, 2, "Booting...");
    IIC_OLED_ShowString(0, 4, "V1.0 STM32F407");
}

void UI_Update(const AgriData *d, ControlState st,
               const char *reason, uint8_t pump_on, uint32_t now_ms)
{
    char buf[24];

    (void)now_ms;

    if (d == 0) {
        return;
    }

    IIC_OLED_Clear();

    /* 第 0 页：标题 */
    IIC_OLED_ShowString(0, 0, "HuiGuan Irrigation");

    /* 第 1 页：模式与策略 */
    snprintf(buf, sizeof(buf), "Mode:%s %s",
             (SerialProtocol_GetMode() == MODE_TEST) ? "TEST" : "AUTO",
             (Control_GetStrategy() == STRATEGY_SCORE) ? "SCORE" : "THR");
    IIC_OLED_ShowString(0, 1, buf);

    /* 第 2 页：土壤湿度 */
    snprintf(buf, sizeof(buf), "Soil :%5.1f %%", d->soil_moisture_pct);
    IIC_OLED_ShowString(0, 2, buf);

    /* 第 3 页：空气温湿度 */
    snprintf(buf, sizeof(buf), "Air  :%4.1fC %4.1f%%",
             d->air_temperature_c, d->air_humidity_pct);
    IIC_OLED_ShowString(0, 3, buf);

    /* 第 4 页：光照 */
    snprintf(buf, sizeof(buf), "Light:%6.0f lx", d->light_lux);
    IIC_OLED_ShowString(0, 4, buf);

    /* 第 5 页：水泵与状态 */
    snprintf(buf, sizeof(buf), "Pump:%s St:%s",
             pump_on ? "ON " : "OFF", ControlState_Name(st));
    IIC_OLED_ShowString(0, 5, buf);

    /* 第 6 页：决策原因 */
    if (reason != 0) {
        snprintf(buf, sizeof(buf), "R:%s", reason);
        IIC_OLED_ShowString(0, 6, buf);
    }

    /* 第 7 页：ADC 原始值（供标定观察） */
    snprintf(buf, sizeof(buf), "ADC:%u S:%u", d->soil_adc_raw, d->spare_adc_raw);
    IIC_OLED_ShowString(0, 7, buf);
}
