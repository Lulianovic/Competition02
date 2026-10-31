/**
 ******************************************************************************
 * @file    usart3.h
 * @brief   STM32 USART3 驱动头文件（专用于 ESP8266 模块通信）
 * @details
 *          - 定义 USART3 接收/发送缓冲区大小及状态标志
 *          - 声明初始化函数与 printf 重定向接口
 *          - 为 ESP8266 AT 指令交互提供底层串口支持
 *
 * @note
 *          - USART3 硬件连接：PB10 (TX), PB11 (RX)
 *          - 接收采用 **中断 + 超时判定帧结束** 机制（依赖 TIM4）
 *          - 所有接收到的数据存入 \c USART3_RX_BUF，由 \c USART3_RX_STA 标记状态
 *          - \c u3_printf() 可直接向 ESP8266 发送格式化 AT 指令
 *
 * @warning
 *          - 缓冲区大小限制为 600 字节，超长应答（如 CWLAP 扫描结果）可能被截断
 *          - 使用前必须调用 \c usart3_init() 初始化，并确保 TIM4 已配置为 10ms 超时定时器
 ******************************************************************************
 */

#ifndef __USART3_H
#define __USART3_H

/* 标准库与驱动依赖 */
#include "stdio.h"      // 用于 FILE 结构（若启用 semihosting，但此处已禁用）
#include "stdarg.h"     // 支持可变参数（用于 u3_printf）
#include "string.h"     // 提供 strlen 等字符串操作
#include "tim.h"        // 依赖定时器驱动（TIM4 用于接收超时检测）
#include "sys.h"        // 包含 STM32 类型定义（如 u8, vu16）
#include "esp8266.h"    // 与 ESP8266 应用层关联


/* ======================================================================== */
/** @defgroup USART3_Configuration USART3 配置参数
 *  @{
 *  @brief 定义串口通信的关键缓冲区大小与使能选项。
 */
/* ======================================================================== */

#define USART3_MAX_RECV_LEN     600     ///< 接收缓冲区最大字节数（建议 ≥512 以容纳完整 AT 应答）
#define USART3_MAX_SEND_LEN     600     ///< 发送缓冲区最大字节数（用于 u3_printf 格式化输出）
#define USART3_RX_EN            1       ///< 接收使能开关：1=启用接收中断，0=禁用（当前未使用）



/** @} */ // End of group USART3_Configuration


/* ======================================================================== */
/** @defgroup USART3_Global_Variables 全局变量声明
 *  @{
 *  @brief 供外部模块（如 esp8266.c）访问的接收/发送缓冲区及状态标志。
 */
/* ======================================================================== */

/**
 * @brief USART3 接收数据缓冲区
 * @details
 *          - 存储从 ESP8266 接收到的原始字节流
 *          - 最大长度由 \c USART3_MAX_RECV_LEN 限制
 *          - 主程序需通过 \c USART3_RX_STA 判断是否包含有效数据
 */
extern u8 USART3_RX_BUF[USART3_MAX_RECV_LEN];

/**
 * @brief USART3 发送数据缓冲区（临时使用）
 * @details
 *          - 仅在 \c u3_printf() 内部使用，用于暂存格式化后的字符串
 *          - 不用于常规数据发送
 */
extern u8 USART3_TX_BUF[USART3_MAX_SEND_LEN];

/**
 * @brief USART3 接收状态标志（volatile 保证中断可见性）
 * @details
 *          - bit15 (0x8000): 帧完成标志
 *                - 0: 正在接收或空闲
 *                - 1: 已接收到完整一帧数据（主程序可处理）
 *          - bit14~0: 当前已接收的有效字节数（0 ~ 32767）
 *
 * @usage 示例：
 *        if (USART3_RX_STA & 0x8000) {
 *            len = USART3_RX_STA & 0x7FFF;
 *            process_data(USART3_RX_BUF, len);
 *            USART3_RX_STA = 0; // 清空状态，准备下一次接收
 *        }
 */
extern vu16 USART3_RX_STA;

/** @} */ // End of group USART3_Global_Variables


/* ======================================================================== */
/** @defgroup USART3_Function_Interface 函数接口声明
 *  @{
 */
/* ======================================================================== */

/**
 * @brief 初始化 USART3 串口
 * @param bound 波特率（典型值：115200，需与 ESP8266 一致）
 * @details
 *          - 配置 GPIO：PB10 (TX, 复用推挽), PB11 (RX, 浮空输入)
 *          - 设置 8N1 数据格式，无硬件流控
 *          - 使能接收中断（依赖 TIM4 实现超时帧检测）
 * @note 必须在调用任何通信函数前执行此初始化
 */
void usart3_init(u32 bound);


/**
 * @brief 重定向 printf 到 USART3（用于发送 AT 指令）
 * @param fmt 格式化字符串（同标准 printf）
 * @param ... 可变参数列表
 * @details
 *          - 将格式化后的字符串通过 USART3 发送给 ESP8266
 *          - 自动添加 "\r\n"（由 \c vsprintf 和调用方式决定）
 *          - **阻塞式发送**：直到所有字符发送完毕才返回
 *
 * @usage 示例：
 *        u3_printf("AT+CWJAP=\"%s\",\"%s\"", ssid, password);
 *
 * @warning
 *          - 输出长度不可超过 \c USART3_MAX_SEND_LEN
 *          - 不适用于高实时性场景（因阻塞等待）
 */
void u3_printf(char* fmt, ...);

/** @} */ // End of group USART3_Function_Interface

#endif /* __USART3_H */