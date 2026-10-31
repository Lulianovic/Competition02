/**
 * @file led.c
 * @brief LED 控制模块实现（PA0）
 *
 * 本模块提供对板载 LED 的初始化接口，使用 PA0 控制输出。
 */


#include "led.h"
#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
  

// 初始化 GPIO 用于控制 LED
void LED_Init(void)
{    	 
  GPIO_InitTypeDef  GPIO_InitStructure;

  /* 【本项目改造】状态灯：PA0 -> PC2（PA0 已分配给土壤湿度 ADC 与 DHT11 无冲突风险隔离） */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);//使能GPIOC时钟

  //GPIO 初始化设置
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;//PC2 状态灯
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;//下拉（默认熄灭）
  GPIO_Init(GPIOC, &GPIO_InitStructure);//初始化GPIO

  GPIO_ResetBits(GPIOC, GPIO_Pin_2);
}

 
