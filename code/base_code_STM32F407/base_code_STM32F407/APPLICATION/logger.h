/**
 * @file    logger.h
 * @brief   运行日志（CSV，通过 printf 输出到 USART1）
 */
#ifndef __LOGGER_H
#define __LOGGER_H

#include "stdint.h"
#include "agri_data.h"

/** @brief 打印 CSV 表头 */
void Log_Init(void);

/**
 * @brief 输出一条运行日志
 * @param d        环境数据
 * @param f        当前故障码
 * @param st       当前控制状态
 * @param score    当前灌溉需求评分
 * @param pump_on  水泵是否开启
 * @param now_ms   当前毫秒数
 */
void Log_Write(const AgriData *d, FaultCode f, ControlState st,
               float score, uint8_t pump_on, uint32_t now_ms);

#endif /* __LOGGER_H */
