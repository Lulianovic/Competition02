/**
 * @file    serial_protocol.h
 * @brief   串口命令协议（调试与测试入口）
 *
 * 设计要点（项目书 7.3）：
 *   - 固定长度接收缓冲区，防止越界
 *   - 中断中只做字节搬运与完成标记
 *   - 主循环中完成解析、范围检查与命令处理
 *   - 数据注入仅在测试模式（MODE,TEST）生效
 */
#ifndef __SERIAL_PROTOCOL_H
#define __SERIAL_PROTOCOL_H

#include "stdint.h"
#include "agri_data.h"

/** @brief 初始化接收缓冲状态（需在 uart_init 之后调用） */
void SerialProtocol_Init(void);

/**
 * @brief 主循环中处理已接收的命令
 * @param now_ms 当前毫秒数
 */
void SerialProtocol_Process(uint32_t now_ms);

/** @brief 获取当前运行模式 */
RunMode SerialProtocol_GetMode(void);

#endif /* __SERIAL_PROTOCOL_H */
