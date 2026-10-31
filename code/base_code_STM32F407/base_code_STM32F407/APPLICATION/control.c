/**
 * @file    control.c
 * @brief   决策层实现
 */
#include "control.h"
#include "app_config.h"
#include "tick.h"
#include "diag.h"
#include "pump.h"
#include "actuator.h"

static ControlState s_state      = ST_IDLE;
static uint32_t     s_enter_ms   = 0;
static FaultCode    s_fault      = FAULT_NONE;
static uint8_t      s_latched    = 0;      /* 故障锁定，需 Control_ClearFault() */
static const char  *s_reason     = "BOOT";
static Strategy     s_strategy   = STRATEGY_THRESHOLD;
static float        s_score      = 0.0f;

/* ---------------- 工具 ---------------- */

static float clamp01(float v)
{
    if (v < 0.0f) { return 0.0f; }
    if (v > 1.0f) { return 1.0f; }
    return v;
}

/**
 * @brief 计算多参数灌溉需求评分
 * @note 土壤湿度项 M 为主项；辅助量为 0 时该项按 0 参与（保守处理）
 */
static float calc_score(const AgriData *d)
{
    float m, t, h, l;

    /* M：土壤越干分越高；以启动/停止阈值区间归一化 */
    m = clamp01((SOIL_STOP_THRESHOLD - d->soil_moisture_pct) /
                (SOIL_STOP_THRESHOLD - SOIL_START_THRESHOLD)) * 100.0f;

    /* T：温度越高蒸腾越强 */
    t = d->air_valid ?
        clamp01((d->air_temperature_c - SCORE_TEMP_MIN) /
                (SCORE_TEMP_MAX - SCORE_TEMP_MIN)) * 100.0f : 0.0f;

    /* H：空气越干分越高 */
    h = d->air_valid ?
        clamp01((SCORE_HUMI_MAX - d->air_humidity_pct) /
                (SCORE_HUMI_MAX - SCORE_HUMI_MIN)) * 100.0f : 0.0f;

    /* L：光照越强蒸腾越强 */
    l = d->light_valid ?
        clamp01(d->light_lux / SCORE_LUX_MAX) * 100.0f : 0.0f;

    return SCORE_W_M * m + SCORE_W_T * t + SCORE_W_H * h + SCORE_W_L * l;
}

/** 是否满足"需要灌溉" */
static uint8_t need_irrigation(const AgriData *d)
{
    if (d->soil_moisture_pct >= SOIL_STOP_THRESHOLD) {
        return 0;                    /* 安全约束：已足够湿润 */
    }
    if (s_strategy == STRATEGY_SCORE) {
        return (uint8_t)(s_score >= SCORE_START_THRESHOLD);
    }
    return (uint8_t)(d->soil_moisture_pct < SOIL_START_THRESHOLD);
}

static void enter_state(ControlState st, uint32_t now_ms, const char *reason)
{
    s_state    = st;
    s_enter_ms = now_ms;
    s_reason   = reason;
}

/* ---------------- 对外接口 ---------------- */

void Control_Init(void)
{
    s_state   = ST_IDLE;
    s_fault   = FAULT_NONE;
    s_latched = 0;
    s_reason  = "INIT";
    s_score   = 0.0f;
    Pump_Set(0);
    Actuator_Buzzer(0);
}

void Control_Process(const AgriData *d, FaultCode f, uint32_t now_ms)
{
    uint8_t ready;

    if (d == 0) {
        return;
    }

    s_score = calc_score(d);
    ready   = Diag_IsReady();

    /* ---------- 1. 故障优先 ---------- */
    if (f != FAULT_NONE && s_state != ST_FAULT) {
        s_fault   = f;
        s_latched = (f == FAULT_PUMP_OVERTIME) ? 1 : 0;
        Pump_Set(0);
        Actuator_Buzzer(1);
        enter_state(ST_FAULT, now_ms, FaultCode_Name(f));
        return;
    }

    /* ---------- 2. 水泵最长运行时间保护 ---------- */
    if (Pump_IsOn() && Pump_GetRunMs(now_ms) > PUMP_MAX_RUN_MS) {
        s_fault   = FAULT_PUMP_OVERTIME;
        s_latched = 1;
        Pump_Set(0);
        Actuator_Buzzer(1);
        enter_state(ST_FAULT, now_ms, "PUMP_OVERTIME");
        return;
    }

    /* ---------- 3. 状态机 ---------- */
    switch (s_state) {

        case ST_FAULT:
            /* 需确认的故障不自动恢复 */
            if (s_latched) {
                s_reason = "FAULT_LATCHED";
                break;
            }
            if (f == FAULT_NONE && ready) {
                Actuator_Buzzer(0);
                s_fault = FAULT_NONE;
                enter_state(ST_IDLE, now_ms, "RECOVERED");
            }
            break;

        case ST_IDLE:
            if (!ready) {
                s_reason = "WAITING_DATA";
                break;
            }
            if (d->soil_moisture_pct >= SOIL_START_THRESHOLD &&
                s_strategy == STRATEGY_THRESHOLD) {
                s_reason = "SOIL_OK";
                break;
            }
            if (need_irrigation(d)) {
                Pump_Set(1);
                enter_state(ST_IRRIGATING, now_ms,
                            (s_strategy == STRATEGY_SCORE) ? "SCORE_HIGH" : "SOIL_LOW");
            } else {
                s_reason = "WAITING";
            }
            break;

        case ST_IRRIGATING:
            if (!ready || f != FAULT_NONE) {
                s_fault   = (f != FAULT_NONE) ? f : FAULT_SOIL_TIMEOUT;
                s_latched = 0;
                Pump_Set(0);
                Actuator_Buzzer(1);
                enter_state(ST_FAULT, now_ms, "DATA_INVALID");
                break;
            }
            /* 停止条件：达到停止阈值（安全约束优先于评分） */
            if (d->soil_moisture_pct >= SOIL_STOP_THRESHOLD) {
                Pump_Set(0);
                enter_state(ST_COOLDOWN, now_ms, "SOIL_OK");
            } else {
                s_reason = "IRRIGATING";
            }
            break;

        case ST_COOLDOWN:
            s_reason = "COOLING";
            if ((uint32_t)(now_ms - s_enter_ms) >= PUMP_COOLDOWN_MS) {
                enter_state(ST_IDLE, now_ms, "WAITING");
            }
            break;

        default:
            enter_state(ST_IDLE, now_ms, "WAITING");
            break;
    }
}

ControlState Control_GetState(void)
{
    return s_state;
}

const char *Control_GetReason(void)
{
    return s_reason;
}

float Control_GetScore(void)
{
    return s_score;
}

FaultCode Control_GetFault(void)
{
    return s_fault;
}

void Control_SetStrategy(Strategy s)
{
    s_strategy = s;
}

Strategy Control_GetStrategy(void)
{
    return s_strategy;
}

void Control_ClearFault(void)
{
    s_fault   = FAULT_NONE;
    s_latched = 0;
    Actuator_Buzzer(0);
    Pump_Set(0);
    enter_state(ST_IDLE, Tick_GetMs(), "FAULT_CLEARED");
}

uint8_t Control_IsFaultLatched(void)
{
    return s_latched;
}
