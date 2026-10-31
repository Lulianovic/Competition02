/**
 * @file    sensor_port.h
 * @brief   数据采集层：ADC / DHT11 / BH1750 的读数与单位换算
 *
 * 职责：
 *   - 读取原始值并换算为工程单位（% / ℃ / %RH / lux）
 *   - 记录每个量最近一次成功更新的时刻
 *   - 在测试模式下接受串口注入的数据
 *
 * 不负责判断有效性（由 diag.c 完成），也不参与控制决策。
 */
#ifndef __SENSOR_PORT_H
#define __SENSOR_PORT_H

#include "stdint.h"
#include "agri_data.h"

/** @brief 初始化 ADC、DHT11 与 BH1750 */
void           SensorPort_Init(void);

/**
 * @brief 周期性采集（按各传感器自身周期节流）
 * @param now_ms 当前系统毫秒数
 */
void           SensorPort_Process(uint32_t now_ms);

/**
 * @brief 获取当前环境数据
 * @note  返回的指针指向模块内部数据；诊断层会写入其中的有效性标志，
 *        其余模块应视为只读。
 */
AgriData *SensorPort_Get(void);

/** @brief 设置运行模式：测试模式下传感器数据不再覆盖注入值 */
void           SensorPort_SetMode(RunMode mode);

/** @brief 注入土壤湿度（仅测试模式），值限定在 0~100 */
void           SensorPort_InjectSoil(float pct);

/** @brief 注入空气温湿度（仅测试模式） */
void           SensorPort_InjectAir(float temp_c, float humi_pct);

/** @brief 注入光照（仅测试模式） */
void           SensorPort_InjectLight(float lux);

/** @brief 清除注入，恢复使用真实传感器数据 */
void           SensorPort_ClearInjection(void);

/** @brief ADC 原始值换算为土壤湿度百分比（供标定与调试打印使用） */
float          SensorPort_RawToSoilPct(uint16_t raw);

/** @brief 模拟土壤数据超时（把更新时间回拨到超时窗口之外），用于故障测试 */
void           SensorPort_SimulateSoilTimeout(void);

#endif /* __SENSOR_PORT_H */
