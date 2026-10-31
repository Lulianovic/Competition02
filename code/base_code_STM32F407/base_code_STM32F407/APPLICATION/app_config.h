/**
 * @file    app_config.h
 * @brief   慧灌智控 V1 —— 全局配置（唯一配置入口）
 *
 * 所有阈值、周期、权重、引脚与模式开关集中在本文件。
 * 修改本文件即可完成标定与调参，不应在其它模块中硬编码这些数值。
 *
 * 依据：《项目书.txt》v1.3、《代码实现计划.md》
 */
#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H

#include "stdint.h"
#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"   /* 提供 ADC_Channel_x / GPIO_Pin_x 等外设定义 */

/* ==========================================================================
 * 1. 引脚分配（冻结，随原理图接线确定，禁止随意改动）
 * ========================================================================== */

/* 土壤湿度 YL-69：AO -> PA0 / ADC1_IN0 */
#define SOIL_ADC_PORT        GPIOA
#define SOIL_ADC_PIN         GPIO_Pin_0
#define SOIL_ADC_CHANNEL     ADC_Channel_0

/* 备用模拟量：可调电压源 -> PA1 / ADC1_IN1 */
#define SPARE_ADC_PORT       GPIOA
#define SPARE_ADC_PIN        GPIO_Pin_1
#define SPARE_ADC_CHANNEL    ADC_Channel_1

/* 空气温湿度 DHT11：DATA -> PB0 */
#define DHT_PORT             GPIOB
#define DHT_PIN              GPIO_Pin_0

/* I2C 总线（BH1750 + OLED 共用）：SCL -> PB8，SDA -> PB9 */
#define I2C_PORT             GPIOB
#define I2C_SCL_PIN          GPIO_Pin_8
#define I2C_SDA_PIN          GPIO_Pin_9

/* 继电器（水泵）：DQ -> PC0 */
#define RELAY_PORT           GPIOC
#define RELAY_PIN            GPIO_Pin_0

/* 蜂鸣器：+ -> PC1 */
#define BUZZER_PORT          GPIOC
#define BUZZER_PIN           GPIO_Pin_1

/* 状态指示灯：PC2 -> 330R -> LED -> GND */
#define LED_PORT             GPIOC
#define LED_PIN              GPIO_Pin_2

/* ==========================================================================
 * 2. 执行器有效电平（上电实测后只需改这三行）
 * ========================================================================== */
/* 继电器模块为"高电平触发"（DQ 高电平吸合线圈）。
 * 因水泵接在 NC（常闭）触点：开泵=PC0 低电平=线圈释放=NC 闭合=水泵转；
 *                          关泵=PC0 高电平=线圈吸合=NC 断开=水泵停。
 * （若改接 NO，RELAY_ACTIVE_LEVEL 应改回 1。）
 */
#define RELAY_ACTIVE_LEVEL     0     /* 1：高电平吸合；0：低电平吸合 */
#define BUZZER_ACTIVE_LEVEL    1     /* 1：高电平响 */
#define LED_ACTIVE_LEVEL       1     /* 1：高电平亮 */

/* ==========================================================================
 * 3. 土壤湿度标定
 *
 * 标定步骤（在仿真中操作 YL-69 组件的"土壤湿度"参数）：
 *   1) 设为 0   %，读 ADC 原始值 -> 填入 SOIL_ADC_DRY_RAW
 *   2) 设为 100 %，读 ADC 原始值 -> 填入 SOIL_ADC_WET_RAW
 *   3) 若方向相反，直接对调两个值即可，无需改换算公式
 * 供电固定 3.3V，AO 满量程 = ADC 参考 = 3.3V，RAW 满量程 = 4095
 * ========================================================================== */
/* 【已实测标定 2026-10-30】仿真模型为线性：raw = 4095 x (湿度%/100)
 *   实测：参数 40 % -> raw 1638；参数 10 % -> raw 410
 *   故：0 % -> 0，100 % -> 4095
 */
#define SOIL_ADC_DRY_RAW    0        /* 干燥（0%）时 ADC 原始值 */
#define SOIL_ADC_WET_RAW    4095     /* 湿润（100%）时 ADC 原始值 */
#define SOIL_ADC_MAX        4095
#define SOIL_RAW_MIN        0
#define SOIL_RAW_MAX        4095

/* ==========================================================================
 * 4. 固定阈值基准策略（MODE,THRESHOLD）
 * ========================================================================== */
#define SOIL_START_THRESHOLD    30.0f   /* % 低于此值可申请灌溉 */
#define SOIL_STOP_THRESHOLD     40.0f   /* % 达到此值停止灌溉  */
#define PUMP_MAX_RUN_MS         10000   /* 最长连续运行 10 s   */
#define PUMP_COOLDOWN_MS        5000    /* 灌溉后冷却 5 s      */

/* ==========================================================================
 * 5. 诊断参数
 * ========================================================================== */
#define SENSOR_TIMEOUT_MS       3000    /* 数据超时 3 s              */
#define DIAG_RECOVER_COUNT      3       /* 连续 N 次有效才解除故障   */
#define SOIL_VALID_MIN          0.0f    /* 土壤湿度有效范围          */
#define SOIL_VALID_MAX          100.0f
#define TEMP_VALID_MIN          -20.0f  /* 空气温度有效范围          */
#define TEMP_VALID_MAX          80.0f
#define HUMI_VALID_MIN          0.0f    /* 空气湿度有效范围          */
#define HUMI_VALID_MAX          100.0f
#define LIGHT_VALID_MIN         0.0f    /* 光照有效范围              */
#define LIGHT_VALID_MAX         65535.0f

/* ==========================================================================
 * 6. 多参数需求评分（MODE,SCORE）
 *    S = wM*M + wT*T + wH*H + wL*L，权重之和为 1
 * ========================================================================== */
#define SCORE_START_THRESHOLD   60.0f   /* 达到该评分才启动灌溉 */
#define SCORE_W_M   0.70f               /* 土壤湿度（主导）     */
#define SCORE_W_T   0.10f               /* 空气温度             */
#define SCORE_W_H   0.10f               /* 空气湿度             */
#define SCORE_W_L   0.10f               /* 光照                 */

/* 评分归一化区间 */
#define SCORE_TEMP_MIN  20.0f
#define SCORE_TEMP_MAX  40.0f
#define SCORE_HUMI_MIN  20.0f
#define SCORE_HUMI_MAX  60.0f
#define SCORE_LUX_MAX   20000.0f

/* ==========================================================================
 * 7. 传感器采样周期（ms）
 * ========================================================================== */
#define PERIOD_ADC_READ         100     /* ADC 每 100 ms            */
#define PERIOD_DHT_READ         2000    /* DHT11 至少间隔 1~2 s     */
#define PERIOD_BH1750_READ      200     /* BH1750 每 200 ms         */

/* ==========================================================================
 * 8. 任务调度周期（ms）
 * ========================================================================== */
#define PERIOD_SENSOR_PROCESS   100
#define PERIOD_DIAG             200
#define PERIOD_CONTROL          500
#define PERIOD_UI               300
#define PERIOD_LOG              500

/* ==========================================================================
 * 9. 串口
 * ========================================================================== */
#define SERIAL_BAUDRATE         115200
#define SERIAL_LINE_MAX         96      /* 单行命令最大长度，防止越界 */

/* ==========================================================================
 * 10. 状态灯闪烁节奏（ms）
 * ========================================================================== */
#define LED_BLINK_IDLE          1000    /* 待机慢闪 */
#define LED_BLINK_IRRIGATE      200     /* 灌溉快闪 */
#define LED_BLINK_FAULT         100     /* 故障急闪 */

#endif /* __APP_CONFIG_H */
