/**
 * @file    stepper_motor.c
 * @brief   四相混合式步进电机驱动实现（四相八拍模式）
 * @version V1.0
 *
 * 本文件实现了基于 GPIO 控制的四相步进电机驱动，采用 **四相八拍** 工作模式，
 * 支持正转、反转及电机关闭功能。
 *
 * @note
 * - 使用引脚：PB12 (IN1), PB13 (IN2), PB14 (IN3), PB15 (IN4)
 * - 电机类型：四相双极性/单极性混合式步进电机
 * - 控制模式：四相八拍（Half-Step），每周期 8 拍，步距角减半，运行更平稳
 * - 转速控制：通过调节每拍延时 `n`（单位：微秒）实现，`n` 越大，转速越慢
 * - 引脚宏定义（需在 stepper_motor.h 中定义）：
 *     - IN1_H / IN1_L → 控制 PB12
 *     - IN2_H / IN2_L → 控制 PB13
 *     - IN3_H / IN3_L → 控制 PB14
 *     - IN4_H / IN4_L → 控制 PB15
 *
 * @warning 长时间通电会导致电机发热，建议在空闲时调用 Motor_Ctrl_Off() 关闭输出。
 */
#include "delay.h"
#include "stepper_motor.h"

/**
 * @brief 初始化步进电机控制引脚（PB12～PB15）
 * @details 配置为推挽输出模式，初始状态全部拉低（电机静止）
 */
void motor_configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);//使能GPIOF时钟

    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;//推挽输出
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStruct);
  	// 初始状态：所有绕组关闭（电机静止）
    IN1_L; IN2_L; IN3_L; IN4_L;
}

/**
 * @brief 步进电机正转（四相八拍模式）
 * @param n 每拍延时时间（单位：微秒），值越大转速越慢
 *
 * @details 四相八拍相序（Half-Step）：
 *   拍序 | A相 | B相 | Ā相 | B̄相 | 说明
 *   -----|-----|-----|------|------|--------
 *    1   | H   | L   | L    | L    | A↑
 *    2   | H   | L   | L    | H    | A+B̄↑
 *    3   | L   | H   | L    | H    | B̄↑
 *    4   | L   | H   | H    | H    | B̄+Ā↑
 *    5   | L   | L   | H    | H    | Ā↑
 *    6   | L   | L   | H    | L    | Ā+B↑
 *    7   | L   | L   | L    | L    | B↑ 
 *    8   | H   | L   | L    | L    | B+A↑
 */
void motor_control_F(int n)
{
    IN1_H; delay_us(n);
    IN4_L; delay_us(n);
    IN2_H; delay_us(n);
    IN1_L; delay_us(n);
    IN3_H; delay_us(n);
    IN2_L; delay_us(n);
    IN4_H; delay_us(n);
    IN3_L; delay_us(n);
}

/**
 * @brief 步进电机反转（当前实现有误）
 * @param n 每拍延时（微秒）
 */
void motor_control_Z(int n)
{
    IN3_L; delay_us(n);
    IN4_H; delay_us(n);
    IN2_L; delay_us(n);
    IN3_H; delay_us(n);
    IN1_L; delay_us(n);
    IN2_H; delay_us(n);
    IN4_L; delay_us(n);
    IN1_H; delay_us(n);
}

/**
 * @brief 关闭步进电机所有输出（防止发热）
 * @details 将 IN1～IN4 全部拉低，使电机绕组断电，进入自由停止状态
 *
 * @warning 在电机长时间静止时务必调用此函数，避免过热或功耗过高。
 */
void Motor_Ctrl_Off(void)
{
	GPIO_ResetBits(GPIOB, GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15);
}

