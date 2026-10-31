/**
 * @file    actuator.h
 * @brief   蜂鸣器（PC1）与状态指示灯（PC2）
 */
#ifndef __ACTUATOR_H
#define __ACTUATOR_H

#include "stdint.h"
#include "agri_data.h"

/** @brief 初始化 PC1、PC2 为推挽输出并置为关闭状态 */
void Actuator_Init(void);

/** @brief 蜂鸣器开关（1 = 响） */
void Actuator_Buzzer(uint8_t on);

/** @brief 状态灯直接开关 */
void Actuator_Led(uint8_t on);

/**
 * @brief 按控制状态自动闪烁
 * @param now_ms 当前毫秒数
 * @param st     当前控制状态
 * @note  待机慢闪 / 灌溉快闪 / 故障急闪 / 冷却慢闪
 */
void Actuator_LedBlink(uint32_t now_ms, ControlState st);

#endif /* __ACTUATOR_H */
