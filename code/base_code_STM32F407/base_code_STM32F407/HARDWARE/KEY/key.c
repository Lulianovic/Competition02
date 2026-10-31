/**
 ******************************************************************************
 * @file    key.c
 * @brief   按键输入驱动实现（支持下拉输入模式）
 *
 * 实现单个或多个独立按键的初始化与扫描功能。
 * 默认示例使用 PA1 作为按键输入引脚，配置为 **下拉输入**：
 * - 按键未按下：PA1 = 低电平（0）
 * - 按键按下：PA1 = 高电平（1，通过外部上拉或 VCC 直连）
 *
 * @note    - 按键需一端接地，另一端接 PA1 和 VCC（或通过上拉电阻接 VCC）
 *          - 扫描函数 \c Key_Scan() 包含简单软件消抖（等待释放）
 *          - 适用于短按检测，不支持长按/连按
 ******************************************************************************
 */

#include "key.h"

/**
 * @brief  初始化按键 GPIO 引脚（默认 PB0）
 *
 * 配置 PB0 为下拉输入模式（GPIO_Mode_IPD）：
 * - 内部弱下拉确保无按键时为低电平
 * - 按键按下时，引脚被拉高至 VCC（需外部电路支持）
 *
 * @note   若使用其他引脚或上拉模式，需修改此函数或扩展为参数化版本。
 */
void KEY_Init(void)
{
	GPIO_InitTypeDef   GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);//使能GPIOB时钟

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;   // 对应引脚PB0
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;//普通输入模式
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100M
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN ;//下拉
	GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化GPIOB0
}
