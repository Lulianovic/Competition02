/**
 * @file    diag.h
 * @brief   数据诊断层：范围检查、有效性标记、超时检测与恢复确认
 *
 * 规则（依据项目书 2.2 / 6.4）：
 *   - 超出合理范围 -> 标记无效
 *   - 长时间未更新 -> 超时，关键量超时进入安全保护
 *   - 恢复需连续 N 次有效采样
 */
#ifndef __DIAG_H
#define __DIAG_H

#include "stdint.h"
#include "agri_data.h"

/** @brief 复位内部恢复计数器 */
void      Diag_Init(void);

/**
 * @brief 执行一次诊断
 * @param d        环境数据（会被写入 *_valid 标志）
 * @param now_ms   当前系统毫秒数
 * @return 当前故障码（FAULT_NONE 表示正常）
 */
FaultCode Diag_Process(AgriData *d, uint32_t now_ms);

/** @brief 关键数据是否齐备（土壤有效且无故障），用于允许自动灌溉 */
uint8_t   Diag_IsReady(void);

/** @brief 获取连续有效采样计数值（调试用） */
uint8_t   Diag_GetRecoverCount(void);

#endif /* __DIAG_H */
