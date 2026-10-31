/**
 ******************************************************************************
 * @file      main.c
 * @brief     慧灌智控 V1 主程序：非阻塞调度主循环
 *
 * 调度顺序（每轮）：
 *   串口命令 -> 传感器采集 -> 数据诊断 -> 控制决策 -> 显示刷新 -> 日志 -> 状态灯
 *
 * @note  系统节拍由 TIM5 提供（tick.c），不使用 delay_ms() 作为节拍；
 *        delay_us() 仅供 DHT11 时序使用。
 ******************************************************************************
 */
#include "sys.h"
#include "delay.h"
#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"
#include "stdio.h"

#include "usart.h"
#include "app_config.h"
#include "tick.h"
#include "agri_data.h"
#include "sensor_port.h"
#include "diag.h"
#include "control.h"
#include "pump.h"
#include "actuator.h"
#include "serial_protocol.h"
#include "logger.h"
#include "ui.h"

/* 各任务的基准时刻 */
static uint32_t s_t_sensor = 0;
static uint32_t s_t_diag   = 0;
static uint32_t s_t_ctrl   = 0;
static uint32_t s_t_ui     = 0;
static uint32_t s_t_log    = 0;

int main(void)
{
    AgriData  *data;
    FaultCode  fault = FAULT_NONE;

    /* 中断优先级分组 */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    /* 基础外设 */
    Tick_Init();                    /* TIM5 1 ms 节拍 */
    uart_init(SERIAL_BAUDRATE);     /* USART1 PA9/PA10，printf 已重定向 */
    SerialProtocol_Init();

    /* 应用层 */
    SensorPort_Init();              /* ADC(PA0/PA1) + DHT11(PB0) + BH1750(PB8/PB9) */
    Pump_Init();                    /* PC0，上电默认关闭 */
    Actuator_Init();                /* PC1 蜂鸣器、PC2 状态灯 */
    Control_Init();
    Diag_Init();
    UI_Init();
    Log_Init();

    printf("HuiGuan Smart Irrigation V1 ready\r\n");
    printf("Type HELP for commands\r\n");

    data = SensorPort_Get();

    while (1) {
        uint32_t now = Tick_GetMs();

        /* 1. 串口命令（每轮） */
        SerialProtocol_Process(now);

        /* 2. 传感器采集 */
        if (Tick_Due(now, &s_t_sensor, PERIOD_SENSOR_PROCESS)) {
            SensorPort_Process(now);
        }

        /* 3. 数据诊断 */
        if (Tick_Due(now, &s_t_diag, PERIOD_DIAG)) {
            fault = Diag_Process(data, now);
        }

        /* 4. 控制决策 */
        if (Tick_Due(now, &s_t_ctrl, PERIOD_CONTROL)) {
            Control_Process(data, fault, now);
        }

        /* 5. 显示刷新 */
        if (Tick_Due(now, &s_t_ui, PERIOD_UI)) {
            UI_Update(data, Control_GetState(), Control_GetReason(),
                      Pump_IsOn(), now);
        }

        /* 6. 运行日志 */
        if (Tick_Due(now, &s_t_log, PERIOD_LOG)) {
            Log_Write(data, Control_GetFault(), Control_GetState(),
                      Control_GetScore(), Pump_IsOn(), now);
        }

        /* 7. 状态灯 */
        Actuator_LedBlink(now, Control_GetState());
    }
}
