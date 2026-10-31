/**
 * @file    control.h
 * @brief   决策层：灌溉需求评估 + 状态机 + 安全约束
 *
 * 两级判断（项目书 6.3）：
 *   第一级 安全约束：土壤湿度、数据有效性、运行时长、冷却状态
 *   第二级 需求评估：固定阈值 或 多参数评分
 * 辅助参数（温湿度、光照）不得绕过土壤湿度约束。
 */
#ifndef __CONTROL_H
#define __CONTROL_H

#include "stdint.h"
#include "agri_data.h"

/** @brief 初始化控制状态（默认 IDLE，水泵关闭） */
void          Control_Init(void);

/**
 * @brief 执行一次控制决策
 * @param d       环境数据（只读）
 * @param f       诊断层给出的故障码
 * @param now_ms  当前毫秒数
 */
void          Control_Process(const AgriData *d, FaultCode f, uint32_t now_ms);

/** @brief 当前控制状态 */
ControlState  Control_GetState(void);

/** @brief 当前决策原因（字符串，用于显示与日志） */
const char   *Control_GetReason(void);

/** @brief 当前灌溉需求评分（0~100） */
float         Control_GetScore(void);

/** @brief 当前控制层持有的故障码（含水泵运行超时等） */
FaultCode     Control_GetFault(void);

/** @brief 切换控制策略 */
void          Control_SetStrategy(Strategy s);

/** @brief 当前策略 */
Strategy      Control_GetStrategy(void);

/** @brief 人工/串口确认后清除故障（用于解除水泵运行超时等需确认的故障） */
void          Control_ClearFault(void);

/** @brief 故障是否处于锁定状态（需外部确认才能恢复） */
uint8_t       Control_IsFaultLatched(void);

#endif /* __CONTROL_H */
