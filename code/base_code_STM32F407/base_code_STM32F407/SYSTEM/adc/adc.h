#ifndef __ADC_H
#define __ADC_H

#include "sys.h"  // 包含 STM32 寄存器定义及常用类型

/* =============================================================================
 * ADC 硬件资源配置（根据实际电路修改）
 * =========================================================================== */

// 【ADC 模块】
#define ADC_CLK        RCC_APB2Periph_ADC1     // ADC1 时钟使能位（APB2 总线）

// 【模拟输入通道】——本项目使用双通道
#define ADC_CHANNEL      ADC_Channel_1        // 兼容旧定义：通道 1（PA1，备用模拟量）
#define ADC_CHANNEL_SOIL ADC_Channel_0        // 通道 0（PA0，土壤湿度 YL-69）
#define ADC_CHANNEL_SPARE ADC_Channel_1       // 通道 1（PA1，备用可调电压源）

// 【GPIO 引脚配置】
#define ADC_PIN        GPIO_Pin_0             // PA0 引脚（ADC1_IN0，土壤湿度）
#define ADC_PIN2       GPIO_Pin_1             // PA1 引脚（ADC1_IN1，备用模拟量）
#define ADC_GPIO_PORT  GPIOA                  // 所属端口
#define ADC_GPIO_CLK   RCC_AHB1Periph_GPIOA   // GPIOA 时钟使能位（AHB1 总线）

/* =============================================================================
 * 全局变量声明
 * 用于在其他文件（如 main.c）中访问 ADC 转换结果
 * =========================================================================== */
extern unsigned int adc_value;  // 存储 12 位 ADC 转换结果（范围 0~4095）

/* =============================================================================
 * 函数声明
 * =========================================================================== */

/**
 * @brief 初始化 ADC1 模块
 * @details
 *   - 使能 GPIOA 和 ADC1 时钟
 *   - 配置 PA1 为模拟输入模式
 *   - 设置 ADC 工作参数（分辨率、对齐方式、采样时间等）
 *   - 执行 ADC 校准 基本配置（STM32F4）
 * @note 必须在调用 Get_Adc() 前执行一次
 */
void ADC_Config(void);

/**
 * @brief 获取指定 ADC 通道的单次转换值
 * @param ch: ADC 通道号（如 ADC_Channel_1）
 * @return: 12 位 ADC 结果（0 ~ 4095）
 * @note
 *   - 内部会重新配置规则通道并启动一次软件转换
 *   - 适用于低频、单通道采样场景
 *   - 若需高精度，建议增加采样时间或多次平均
 */
uint16_t Get_Adc(uint8_t ch);

#endif /* __ADC_H */