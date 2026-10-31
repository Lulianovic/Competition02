/**
 * @file led.h
 * @brief LED 控制模块头文件（PA0）
 */
#ifndef __LED_H
#define __LED_H	 

#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"
#include "sys.h"


/** @defgroup LED_Macros LED 宏定义
 *  @{ */

/**
 * @brief 控制 LED 的输出宏
 * @note 【本项目改造】原实现为 PA0（与 DHT11 冲突），状态灯已改到 PC2；
 *       原宏写成 PFout(9) 与实现不一致，此处统一为 PCout(2)。
 */
#define LED PCout(2) /**< PC2 引脚控制状态灯 */

/** @} */

/** @defgroup LED_Functions LED 函数
 *  @{ */

/**
 * @brief 初始化 PA0 用于控制 LED
 *
 * 初始化 PA0 为推挽输出并设置为初始高电平。
 */
void LED_Init(void);

/** @} */

#endif
