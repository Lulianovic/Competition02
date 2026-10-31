/**
 * @file    logger.c
 * @brief   运行日志实现
 *
 * CSV 字段（项目书 9.2）：
 *   timestamp,source,soil_pct,air_c,air_rh,light_lux,valid,mode,
 *   strategy,score,pump,state,fault,reason
 */
#include "logger.h"
#include "serial_protocol.h"
#include "control.h"
#include "stdio.h"

void Log_Init(void)
{
    printf("timestamp_ms,source,soil_pct,soil_raw,spare_raw,air_c,air_rh,light_lux,"
           "soil_v,air_v,light_v,mode,strategy,score,pump,state,fault,reason\r\n");
}

void Log_Write(const AgriData *d, FaultCode f, ControlState st,
               float score, uint8_t pump_on, uint32_t now_ms)
{
    if (d == 0) {
        return;
    }

    printf("%lu,%s,%.1f,%u,%u,%.1f,%.1f,%.0f,%u,%u,%u,%s,%s,%.1f,%u,%s,%s,%s\r\n",
           (unsigned long)now_ms,
           DataSource_Name(d->source),
           d->soil_moisture_pct,
           (unsigned)d->soil_adc_raw,          /* 标定用：土壤通道原始值 */
           (unsigned)d->spare_adc_raw,         /* 标定用：备用通道原始值 */
           d->air_temperature_c,
           d->air_humidity_pct,
           d->light_lux,
           d->soil_valid  ? 1u : 0u,           /* 有效位拆成三列，便于阅读 */
           d->air_valid   ? 1u : 0u,
           d->light_valid ? 1u : 0u,
           (SerialProtocol_GetMode() == MODE_TEST) ? "TEST" : "AUTO",
           (Control_GetStrategy() == STRATEGY_SCORE) ? "SCORE" : "THRESHOLD",
           score,
           pump_on ? 1u : 0u,
           ControlState_Name(st),
           FaultCode_Name(f),
           Control_GetReason());
}
