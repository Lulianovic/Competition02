/**
 * @file    sys.h
 * @brief   STM32F40x 系统核心头文件：中断分组配置与位带操作宏
 *
 * 本文件提供两类核心功能：
 * 1. **NVIC 中断优先级分组配置**（通过 NVIC_Configuration()）
 * 2. **GPIO 位带操作宏**（实现类似 51 单片机的 PAout(5)=1 风格）
 *
 * @note
 * - 位带操作基于 Cortex-M3 的 **Bit-Band 特性**（参考《CM3 权威指南》第5章）；
 * - 所有 GPIO 宏仅适用于 STM32F10x 系列（外设基地址固定）；
 * - 本文件包含 stm32f10x.h，无需重复包含。
 *
 */
#ifndef __SYS_H
#define __SYS_H	 
#include "stm32f4xx.h" 
																	    
	 
/* ==============================================================================
 *                          位带操作（Bit-Banding）宏
 * ============================================================================== */

/**
 * @defgroup BitBand_Macros Bit-Band Operation Macros
 * @brief 基于 Cortex-M3 位带区的单比特原子操作
 * @{
 */

/** 
 * @brief 计算位带别名地址
 * @param addr  字节地址（必须位于 SRAM 或外设位带区）
 * @param bitnum 比特位编号（0～7）
 * @return 位带别名地址（可直接读写该地址操作单个比特）
 * @note 仅适用于 STM32F10x 的 SRAM (0x20000000～0x200FFFFF) 和外设 (0x40000000～0x400FFFFF)
 */
#define BITBAND(addr, bitnum) ((addr & 0xF0000000)+0x2000000+((addr &0xFFFFF)<<5)+(bitnum<<2)) 

/** 
 * @brief 通过地址访问 volatile 32 位内存
 */
#define MEM_ADDR(addr)  *((volatile unsigned long  *)(addr)) 

/** 
 * @brief 通过位带宏访问单个比特
 */
#define BIT_ADDR(addr, bitnum)   MEM_ADDR(BITBAND(addr, bitnum)) 
/** @} */ // end of BitBand_Macros

/* ==============================================================================
 *                        GPIO 位带地址映射（STM32F10x）
 * ============================================================================== */

// ODR (Output Data Register) 地址
#define GPIOA_ODR_Addr    (GPIOA_BASE+20) //0x40020014
#define GPIOB_ODR_Addr    (GPIOB_BASE+20) //0x40020414 
#define GPIOC_ODR_Addr    (GPIOC_BASE+20) //0x40020814 
#define GPIOD_ODR_Addr    (GPIOD_BASE+20) //0x40020C14 
#define GPIOE_ODR_Addr    (GPIOE_BASE+20) //0x40021014 
#define GPIOF_ODR_Addr    (GPIOF_BASE+20) //0x40021414    
#define GPIOG_ODR_Addr    (GPIOG_BASE+20) //0x40021814   
#define GPIOH_ODR_Addr    (GPIOH_BASE+20) //0x40021C14    
#define GPIOI_ODR_Addr    (GPIOI_BASE+20) //0x40022014     

#define GPIOA_IDR_Addr    (GPIOA_BASE+16) //0x40020010 
#define GPIOB_IDR_Addr    (GPIOB_BASE+16) //0x40020410 
#define GPIOC_IDR_Addr    (GPIOC_BASE+16) //0x40020810 
#define GPIOD_IDR_Addr    (GPIOD_BASE+16) //0x40020C10 
#define GPIOE_IDR_Addr    (GPIOE_BASE+16) //0x40021010 
#define GPIOF_IDR_Addr    (GPIOF_BASE+16) //0x40021410 
#define GPIOG_IDR_Addr    (GPIOG_BASE+16) //0x40021810 
#define GPIOH_IDR_Addr    (GPIOH_BASE+16) //0x40021C10 
#define GPIOI_IDR_Addr    (GPIOI_BASE+16) //0x40022010 
 
/* ==============================================================================
 *                      GPIO 位操作宏（单引脚读写）
 * ============================================================================== */

/**
 * @defgroup GPIO_Bit_Ops GPIO Single-Pin Bit Operations
 * @brief 类似 51 单片机的 PAout(5) = 1 风格操作
 * @note 参数 n 必须为 0～15 的常量表达式（编译期确定），否则行为未定义。
 * @{
 */
#define PAout(n)   BIT_ADDR(GPIOA_ODR_Addr,n)  //输出 
#define PAin(n)    BIT_ADDR(GPIOA_IDR_Addr,n)  //输入 

#define PBout(n)   BIT_ADDR(GPIOB_ODR_Addr,n)  //输出 
#define PBin(n)    BIT_ADDR(GPIOB_IDR_Addr,n)  //输入 

#define PCout(n)   BIT_ADDR(GPIOC_ODR_Addr,n)  //输出 
#define PCin(n)    BIT_ADDR(GPIOC_IDR_Addr,n)  //输入 

#define PDout(n)   BIT_ADDR(GPIOD_ODR_Addr,n)  //输出 
#define PDin(n)    BIT_ADDR(GPIOD_IDR_Addr,n)  //输入 

#define PEout(n)   BIT_ADDR(GPIOE_ODR_Addr,n)  //输出 
#define PEin(n)    BIT_ADDR(GPIOE_IDR_Addr,n)  //输入

#define PFout(n)   BIT_ADDR(GPIOF_ODR_Addr,n)  //输出 
#define PFin(n)    BIT_ADDR(GPIOF_IDR_Addr,n)  //输入

#define PGout(n)   BIT_ADDR(GPIOG_ODR_Addr,n)  //输出 
#define PGin(n)    BIT_ADDR(GPIOG_IDR_Addr,n)  //输入

#define PHout(n)   BIT_ADDR(GPIOH_ODR_Addr,n)  //输出 
#define PHin(n)    BIT_ADDR(GPIOH_IDR_Addr,n)  //输入

#define PIout(n)   BIT_ADDR(GPIOI_ODR_Addr,n)  //输出 
#define PIin(n)    BIT_ADDR(GPIOI_IDR_Addr,n)  //输入

/* ==============================================================================
 *                            函数声明
 * ============================================================================== */
//以下为汇编函数
void WFI_SET(void);		//执行WFI指令
void INTX_DISABLE(void);//关闭所有中断
void INTX_ENABLE(void);	//开启所有中断
void MSR_MSP(u32 addr);	//设置堆栈地址 


#endif

