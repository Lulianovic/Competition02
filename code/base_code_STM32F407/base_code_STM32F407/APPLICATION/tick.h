/**
 * @file    tick.h
 * @brief   系统毫秒节拍
 *
 * 使用 TIM5 产生 1 ms 更新中断维护全局毫秒计数。
 *
 * @note    不使用 SysTick：基础工程的 delay_us()/delay_ms() 内部会重配并在结束时
 *          关闭 SysTick，若与其共用会导致节拍停摆（DHT11 时序依赖 delay_us）。
 */
#ifndef __TICK_H
#define __TICK_H

#include "stdint.h"

/** @brief 初始化 TIM5 节拍（1 ms 中断） */
void     Tick_Init(void);

/** @brief 获取系统启动以来的毫秒数 */
uint32_t Tick_GetMs(void);

/**
 * @brief 判断周期任务是否到期（到期则自动刷新基准时刻）
 * @param now      当前 Tick_GetMs() 值
 * @param last     上一次执行的时刻（会被本函数更新）
 * @param period   周期（ms）
 * @return 1 = 到期，0 = 未到期
 */
uint8_t  Tick_Due(uint32_t now, uint32_t *last, uint32_t period);

#endif /* __TICK_H */
