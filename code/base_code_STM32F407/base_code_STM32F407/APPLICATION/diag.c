/**
 * @file    diag.c
 * @brief   数据诊断层实现
 */
#include "diag.h"
#include "app_config.h"

static uint8_t   s_recover_cnt = 0;    /* 连续有效采样计数 */
static uint8_t   s_ready       = 0;    /* 关键数据是否齐备 */
static FaultCode s_last_fault  = FAULT_NONE;

/** 判断是否超时：update_ms 为 0 表示上电后从未更新，以 0 时刻为基准 */
static uint8_t is_timeout(uint32_t now_ms, uint32_t update_ms)
{
    return ((uint32_t)(now_ms - update_ms) > SENSOR_TIMEOUT_MS);
}

/** 浮点范围检查 */
static uint8_t in_range(float v, float lo, float hi)
{
    return ((v >= lo) && (v <= hi));
}

void Diag_Init(void)
{
    s_recover_cnt = 0;
    s_ready       = 0;
    s_last_fault  = FAULT_NONE;
}

FaultCode Diag_Process(AgriData *d, uint32_t now_ms)
{
    FaultCode fault = FAULT_NONE;

    if (d == 0) {
        return FAULT_NONE;
    }

    /* ---------- 1. 土壤湿度（关键量） ---------- */
    if (!in_range(d->soil_moisture_pct, SOIL_VALID_MIN, SOIL_VALID_MAX)) {
        d->soil_valid = 0;
        fault = FAULT_SOIL_RANGE;
    } else if (is_timeout(now_ms, d->soil_update_ms)) {
        d->soil_valid = 0;
        fault = FAULT_SOIL_TIMEOUT;
    } else {
        d->soil_valid = 1;
    }

    /* ---------- 2. 空气温湿度（辅助量，异常不直接判故障） ---------- */
    d->air_valid = (uint8_t)(in_range(d->air_temperature_c, TEMP_VALID_MIN, TEMP_VALID_MAX) &&
                             in_range(d->air_humidity_pct,  HUMI_VALID_MIN, HUMI_VALID_MAX) &&
                             !is_timeout(now_ms, d->air_update_ms));

    /* ---------- 3. 光照（辅助量） ---------- */
    d->light_valid = (uint8_t)(in_range(d->light_lux, LIGHT_VALID_MIN, LIGHT_VALID_MAX) &&
                               !is_timeout(now_ms, d->light_update_ms));

    /* ---------- 4. 恢复确认 ---------- */
    if ((fault == FAULT_NONE) && d->soil_valid) {
        if (s_recover_cnt < 255) { s_recover_cnt++; }
    } else if (fault != FAULT_NONE) {
        s_recover_cnt = 0;
    }

    if (fault == FAULT_NONE && s_recover_cnt >= DIAG_RECOVER_COUNT) {
        s_ready = 1;
    } else if (fault != FAULT_NONE) {
        s_ready = 0;
    }

    /* 水泵运行超时由 control.c 传入并覆盖，此处仅记录自身诊断结果 */
    if (fault != FAULT_NONE) {
        s_last_fault = fault;
    } else if (s_last_fault != FAULT_PUMP_OVERTIME) {
        s_last_fault = FAULT_NONE;
    }
    /* FAULT_PUMP_OVERTIME 由控制层持有，诊断层不清除 */

    return fault;
}

uint8_t Diag_IsReady(void)
{
    return s_ready;
}

uint8_t Diag_GetRecoverCount(void)
{
    return s_recover_cnt;
}
