/**
 * @file    ui.h
 * @brief   OLED 显示界面（128×64，英文）
 */
#ifndef __UI_H
#define __UI_H

#include "stdint.h"
#include "agri_data.h"

/** @brief 初始化 OLED 并显示开机画面 */
void UI_Init(void);

/**
 * @brief 刷新显示内容
 * @param d        环境数据
 * @param st       当前控制状态
 * @param reason   决策原因字符串
 * @param pump_on  水泵是否开启
 * @param now_ms   当前毫秒数
 */
void UI_Update(const AgriData *d, ControlState st,
               const char *reason, uint8_t pump_on, uint32_t now_ms);

#endif /* __UI_H */
