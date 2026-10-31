/**
 * @file    sensor_port.c
 * @brief   数据采集层实现
 */
#include "sensor_port.h"
#include "app_config.h"
#include "tick.h"

#include "adc.h"
#include "dht11.h"
#include "bh1750.h"

static AgriData  s_data;
static uint32_t  s_last_adc_ms     = 0;
static uint32_t  s_last_dht_ms     = 0;
static uint32_t  s_last_bh_ms      = 0;
static RunMode   s_mode            = MODE_AUTO;
/* 分项注入标志：注入的数据每轮刷新时间戳，避免被误判为"超时" */
static uint8_t   s_inj_soil        = 0;
static uint8_t   s_inj_air         = 0;
static uint8_t   s_inj_light       = 0;

/* ---------------- 工具函数 ---------------- */

static float clampf(float v, float lo, float hi)
{
    if (v < lo) { return lo; }
    if (v > hi) { return hi; }
    return v;
}

float SensorPort_RawToSoilPct(uint16_t raw)
{
    float dry = (float)SOIL_ADC_DRY_RAW;
    float wet = (float)SOIL_ADC_WET_RAW;
    float pct;

    if (dry == wet) {
        return 0.0f;
    }
    /* raw == WET_RAW -> 100 %，raw == DRY_RAW -> 0 % */
    pct = ((float)raw - dry) * 100.0f / (wet - dry);
    return clampf(pct, 0.0f, 100.0f);
}

/* ---------------- 对外接口 ---------------- */

void SensorPort_Init(void)
{
    AgriData_Init(&s_data);

    ADC_Config();       /* 双通道：PA0(ADC1_IN0) + PA1(ADC1_IN1) */
    DHT11_Init();       /* PB0 */
    BH1750_Init();      /* PB8/PB9，地址 0x23 */
}

void SensorPort_SetMode(RunMode mode)
{
    s_mode = mode;
    if (mode == MODE_AUTO) {
        SensorPort_ClearInjection();
    }
}

void SensorPort_Process(uint32_t now_ms)
{
    int   t_raw = 0;
    int   h_raw = 0;
    float lux   = 0.0f;
    /* 测试模式下，被注入的量每轮刷新时间戳（注入值保持不变），
     * 否则会被诊断层判定为"数据超时"，有效位全部变 0。 */
    uint8_t inj_soil  = (uint8_t)((s_mode == MODE_TEST) && s_inj_soil);
    uint8_t inj_air   = (uint8_t)((s_mode == MODE_TEST) && s_inj_air);
    uint8_t inj_light = (uint8_t)((s_mode == MODE_TEST) && s_inj_light);

    /* ---- ADC：土壤湿度 + 备用通道 ---- */
    if ((uint32_t)(now_ms - s_last_adc_ms) >= PERIOD_ADC_READ) {
        s_last_adc_ms = now_ms;

        s_data.soil_adc_raw  = Get_Adc(SOIL_ADC_CHANNEL);
        s_data.spare_adc_raw = Get_Adc(SPARE_ADC_CHANNEL);

        if (inj_soil) {
            s_data.soil_update_ms = now_ms;      /* 注入值保持，时间戳刷新 */
            s_data.source         = SRC_SERIAL;
        } else {
            s_data.soil_moisture_pct = SensorPort_RawToSoilPct(s_data.soil_adc_raw);
            s_data.soil_update_ms    = now_ms;
            s_data.source            = SRC_SENSOR;
        }
    }

    /* ---- DHT11：空气温湿度（转换需 >1 s 间隔） ---- */
    if ((uint32_t)(now_ms - s_last_dht_ms) >= PERIOD_DHT_READ) {
        s_last_dht_ms = now_ms;

        if (inj_air) {
            s_data.air_update_ms = now_ms;           /* 注入值保持，时间戳刷新 */
            s_data.source        = SRC_SERIAL;
        } else if (DHT11_Read_Data(&t_raw, &h_raw) == 0) {   /* 0 = 成功 */
            s_data.air_temperature_c = (float)t_raw / 10.0f;   /* 0.1 ℃ */
            s_data.air_humidity_pct  = (float)h_raw / 10.0f;   /* 0.1 %RH */
            s_data.air_update_ms     = now_ms;
            s_data.source            = SRC_SENSOR;
        }
        /* 读取失败且不处于注入状态时不刷新，交由诊断层按超时处理 */
    }

    /* ---- BH1750：光照 ---- */
    if ((uint32_t)(now_ms - s_last_bh_ms) >= PERIOD_BH1750_READ) {
        s_last_bh_ms = now_ms;

        if (inj_light) {
            s_data.light_update_ms = now_ms;         /* 注入值保持，时间戳刷新 */
            s_data.source          = SRC_SERIAL;
        } else if (BH1750_Read(&lux) != 0) {
            s_data.light_lux       = lux;
            s_data.light_update_ms = now_ms;
            s_data.source          = SRC_SENSOR;
        }
    }

    /* 统一决定数据来源，避免多个分支各自赋值导致 SENSOR/SERIAL 来回跳变 */
    s_data.source = (inj_soil || inj_air || inj_light) ? SRC_SERIAL : SRC_SENSOR;
}

AgriData *SensorPort_Get(void)
{
    return &s_data;
}

void SensorPort_InjectSoil(float pct)
{
    s_data.soil_moisture_pct = clampf(pct, 0.0f, 100.0f);
    s_data.soil_update_ms    = Tick_GetMs();
    s_data.source            = SRC_SERIAL;
    s_inj_soil               = 1;
}

void SensorPort_InjectAir(float temp_c, float humi_pct)
{
    s_data.air_temperature_c = clampf(temp_c,    TEMP_VALID_MIN, TEMP_VALID_MAX);
    s_data.air_humidity_pct  = clampf(humi_pct,  HUMI_VALID_MIN, HUMI_VALID_MAX);
    s_data.air_update_ms     = Tick_GetMs();
    s_data.source            = SRC_SERIAL;
    s_inj_air                = 1;
}

void SensorPort_InjectLight(float lux)
{
    s_data.light_lux     = clampf(lux, LIGHT_VALID_MIN, LIGHT_VALID_MAX);
    s_data.light_update_ms = Tick_GetMs();
    s_data.source        = SRC_SERIAL;
    s_inj_light          = 1;
}

void SensorPort_SimulateSoilTimeout(void)
{
    /* 把更新时间回拨到超时窗口之外，使诊断层下一轮判定为超时 */
    s_data.soil_update_ms = Tick_GetMs() - (SENSOR_TIMEOUT_MS + 1000u);
}

void SensorPort_ClearInjection(void)
{
    s_inj_soil      = 0;
    s_inj_air       = 0;
    s_inj_light     = 0;
    s_data.source   = SRC_SENSOR;
    /* 立即用真实数据刷新一次时间戳，避免被判定为超时 */
    s_data.soil_update_ms  = Tick_GetMs();
    s_data.air_update_ms   = Tick_GetMs();
    s_data.light_update_ms = Tick_GetMs();
}
