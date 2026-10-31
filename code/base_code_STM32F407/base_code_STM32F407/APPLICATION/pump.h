/**
 * @file    pump.h
 * @brief   执行层：继电器与水泵控制
 *
 * 只负责"输出电平"和"运行时间统计"，不决定是否灌溉（由 control.c 决策）。
 */
#ifndef __PUMP_H
#define __PUMP_H

#include "stdint.h"

/** @brief 初始化 PC0 为推挽输出，并强制关闭水泵（上电默认关闭） */
void     Pump_Init(void);

/** @brief 打开/关闭水泵 */
void     Pump_Set(uint8_t on);

/** @brief 水泵当前是否开启 */
uint8_t  Pump_IsOn(void);

/** @brief 本次连续运行时间（ms），未运行时返回 0 */
uint32_t Pump_GetRunMs(uint32_t now_ms);

/** @brief 累计运行时间（ms），用于模拟用水量统计 */
uint32_t Pump_GetTotalRunMs(uint32_t now_ms);

/** @brief 启动次数，用于对照实验统计 */
uint32_t Pump_GetStartCount(void);

#endif /* __PUMP_H */
