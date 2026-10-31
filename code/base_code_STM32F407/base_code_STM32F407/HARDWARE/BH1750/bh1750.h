/**
 * @file    bh1750.h
 * @brief   BH1750 光照传感器驱动（软件 I²C，与 OLED 共用 PB8/PB9）
 *
 * - 器件地址：ADDR 接地 -> 0x23
 * - 工作模式：连续高分辨率模式 1（1 lx，典型转换 120 ms）
 * - 换算关系：lux = raw / 1.2
 *
 * @note 本驱动自带轻量软件 I²C，不依赖 OLED 驱动；
 *       每次传输结束都会把 SDA 恢复为推挽输出，避免影响 OLED 的后续操作。
 */
#ifndef __BH1750_H
#define __BH1750_H

#include "stdint.h"

/** @brief 初始化 BH1750（GPIO + 上电 + 连续高分辨率模式） */
void     BH1750_Init(void);

/**
 * @brief 读取光照强度
 * @param lux 输出：光照强度（lux）
 * @return 1 = 成功，0 = 失败（器件无响应）
 */
uint8_t  BH1750_Read(float *lux);

/**
 * @brief 读取原始 16 位寄存器值
 * @param raw 输出：原始值
 * @return 1 = 成功，0 = 失败
 */
uint8_t  BH1750_ReadRaw(uint16_t *raw);

#endif /* __BH1750_H */
