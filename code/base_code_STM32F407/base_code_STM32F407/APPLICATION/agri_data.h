/**
 * @file    agri_data.h
 * @brief   统一环境数据结构与枚举定义
 *
 * 采集层只负责填充本结构；诊断层标记有效性；决策层只读不写。
 */
#ifndef __AGRI_DATA_H
#define __AGRI_DATA_H

#include "stdint.h"

/** 数据来源：区分真实传感器与串口注入，防止测试数据污染实验结论 */
typedef enum {
    SRC_SENSOR = 0,     /**< 真实传感器 */
    SRC_SERIAL = 1      /**< 串口注入（仅测试模式） */
} DataSource;

/** 运行模式 */
typedef enum {
    MODE_AUTO = 0,      /**< 自动模式：只使用真实传感器（默认） */
    MODE_TEST = 1       /**< 测试模式：允许串口注入 */
} RunMode;

/** 控制策略 */
typedef enum {
    STRATEGY_THRESHOLD = 0,   /**< 固定阈值基准策略 */
    STRATEGY_SCORE     = 1    /**< 多参数需求评分策略 */
} Strategy;

/** 控制状态机状态 */
typedef enum {
    ST_IDLE       = 0,  /**< 待机：水泵关闭，持续监测 */
    ST_IRRIGATING = 1,  /**< 灌溉中 */
    ST_COOLDOWN   = 2,  /**< 冷却中：禁止立即再次启动 */
    ST_FAULT      = 3   /**< 故障：安全状态，禁止自动灌溉 */
} ControlState;

/** 故障码 */
typedef enum {
    FAULT_NONE          = 0,  /**< 无故障 */
    FAULT_SOIL_RANGE    = 1,  /**< 土壤湿度超出有效范围 */
    FAULT_SOIL_TIMEOUT  = 2,  /**< 土壤数据超时 */
    FAULT_SENSOR_TIMEOUT= 3,  /**< 其它传感器超时 */
    FAULT_PUMP_OVERTIME = 4,  /**< 水泵运行超时 */
    FAULT_SERIAL_FORMAT = 5   /**< 串口命令格式错误（仅提示，不进安全状态） */
} FaultCode;

/** 环境数据 */
typedef struct {
    /* 工程单位值 */
    float    soil_moisture_pct;   /**< 土壤湿度 0~100 % */
    float    air_temperature_c;   /**< 空气温度 ℃ */
    float    air_humidity_pct;    /**< 空气相对湿度 %RH */
    float    light_lux;           /**< 光照强度 lux */

    /* 原始值，便于标定与日志回溯 */
    uint16_t soil_adc_raw;        /**< 土壤通道 ADC 原始值 0~4095 */
    uint16_t spare_adc_raw;       /**< 备用通道 ADC 原始值 0~4095 */

    /* 有效性标志（由诊断层填写） */
    uint8_t  soil_valid;
    uint8_t  air_valid;
    uint8_t  light_valid;

    /* 各量最近一次成功更新的时刻（ms） */
    uint32_t soil_update_ms;
    uint32_t air_update_ms;
    uint32_t light_update_ms;

    /* 本组数据来源 */
    DataSource source;
} AgriData;

/** @brief 将环境数据结构清零（有效位全部置 0，更新时间置 0） */
void AgriData_Init(AgriData *d);

/** @brief 状态转字符串，用于显示与日志 */
const char *ControlState_Name(ControlState st);

/** @brief 故障码转字符串 */
const char *FaultCode_Name(FaultCode f);

/** @brief 数据来源转字符串 */
const char *DataSource_Name(DataSource s);

#endif /* __AGRI_DATA_H */
