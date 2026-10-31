/**
 * @file    usart.c
 * @brief   STM32F10x 串口驱动：USART1（主调试口）与 USART2（辅助通信口）
 *
 * 本文件实现：
 * - **USART1**：用于调试输出（重定向 printf）和中断接收；
 * - **USART2**：通用串口通信（仅初始化，中断服务需用户扩展）；
 * - 支持标准库 `printf` 通过 USART1 输出（禁用半主机模式）。
 *
 * @note
 * - USART1 引脚：TX=PA9, RX=PA10；
 * - USART2 引脚：TX=PA2, RX=PA3；
 * - 接收使用全局缓冲区 `USART_RX_BUF` 和状态标记 `USART_RX_STA`（仅 USART1）；
 * - 当前 USART1 中断仅清除标志，**未处理数据接收逻辑**（需用户补充）。
 *
 * @warning
 * - 若使用 `printf`，确保主频 ≥ 8 MHz，否则高速波特率可能失真；
 * - 接收缓冲区无溢出保护，请确保上位机发送长度 ≤ `USART_REC_LEN - 1`；
 * - USART2 初始化未配置 NVIC，若需中断，需额外调用 NVIC 配置。
 */
#include "sys.h"
#include "usart.h"

/*------------------ 调试输出支持（重定向 printf） ------------------*/
#if 1   /****** 支持 printf() 函数 ******/
#include <stdio.h>

#pragma GCC diagnostic ignored "-Wunknown-pragmas"
//确保程序中不包含任何半主机的函数
#pragma import(__use_no_semihosting)
/**
 * @brief 标准库所需的 FILE 结构（用于 printf 重定向）
 */              
struct __FILE 
{ 
	int handle; 

}; 

/** 标准输出流 */
FILE __stdout;       

/**
 * @brief 系统退出函数（避免半主机调用）
 */
void _sys_exit(int x) 
{ 
	x = x; 
} 

/**
 * @brief 重定向 fputc 到 USART1，支持 printf
 * @param ch 要发送的字符
 * @param f  文件指针（忽略）
 * @return 发送的字符
 * @note 此函数被 printf 自动调用，阻塞直到发送完成。
 */
int fputc(int ch, FILE *f)
{      
	while((USART1->SR&0X40)==0);//循环发送,直到发送完毕   
    USART1->DR = (u8) ch;      
	return ch;
}

/**
 * @brief GCC 环境下重写_write()函数实现printf()函数的底层操作代码
 * @param ch 要发送的字符
 * @param f  文件指针（忽略）
 * @return 发送的字符
 * @note 此函数被 printf 自动调用，阻塞直到发送完成。
 */
int _write (int fd, char *pBuffer, int size)  
{  
    int i = 0;
	for (i = 0; i < size; i++)  
    {	// 等待上一个字符发送完成
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
		// 发送字符
        USART_SendData(USART1, (uint8_t) *pBuffer++);
    }  
    return size;  
}
#endif

/*------------------ USART1 初始化 ------------------*/

/**
 * @brief 初始化 USART1（PA9-TX, PA10-RX）
 * @param bound 波特率（如 115200）
 * @details
 * - 配置 GPIO 为复用推挽（TX）和浮空输入（RX）；
 * - 使能接收中断（RXNE）；
 * - NVIC 配置：抢占优先级 3，子优先级 3；
 * - 支持 printf 重定向（通过 fputc）。
 * @note
 * - 接收中断服务函数需用户完善数据处理逻辑；
 * - 当前 ISR 仅清除中断标志，防止死循环。
 */
void uart_init(u32 bound)
{
    // GPIO端口设置
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);  //使能GPIOA时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE); //使能USART1时钟

    //串口1对应引脚复用映射
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);  // GPIOA9复用为USART1
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1); // GPIOA10复用为USART1

    // USART1端口配置
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10; // GPIOA9与GPIOA10
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;            //复用功能
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;       //速度50MHz
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;          //推挽复用输出
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;            //上拉
    GPIO_Init(GPIOA, &GPIO_InitStructure);                  //初始化PA9，PA10

    // USART1 初始化设置
    USART_InitStructure.USART_BaudRate = bound;                                     //波特率设置
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;                     //字长为8位数据格式
    USART_InitStructure.USART_StopBits = USART_StopBits_1;                          //一个停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;                             //无奇偶校验位
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; //无硬件数据流控制
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;                 //收发模式
    USART_Init(USART1, &USART_InitStructure);                                       //初始化串口1

    USART_Cmd(USART1, ENABLE); //使能串口1

    // USART_ClearFlag(USART1, USART_FLAG_TC);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); //开启相关中断

    // Usart1 NVIC 配置
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;         //串口1中断通道
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3; //抢占优先级3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;        //子优先级3
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;           // IRQ通道使能
    NVIC_Init(&NVIC_InitStructure);                           //根据指定的参数初始化VIC寄存器、
}

/*------------------ USART1 中断服务函数 ------------------*/

/**
 * @brief USART1 全局中断服务函数
 * @details
 * - 仅处理 RXNE（接收非空中断）；
 * - **当前仅清除中断标志，未读取 DR 或更新缓冲区**；
 * - 用户应在此处添加接收逻辑（如存入 USART_RX_BUF）。
 * @warning
 * - 必须读取 DR 或清除标志，否则会重复进入中断；
 * - 建议在 ISR 中读取 `USART_ReceiveData(USART1)` 避免数据丢失。
 */
/*
 * 【本项目改造】删除此处的空 USART1_IRQHandler。
 * 原因：该空实现只清标志、不读 DR，会丢数据；且真实接收中断已在
 *       APPLICATION/serial_protocol.c 中实现（中断内只做字节搬运）。
 *       若两处同时定义会产生重复符号错误。
 */