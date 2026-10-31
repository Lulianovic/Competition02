/**
 ******************************************************************************
 * @file    usart3.c
 * @brief   STM32 USART3 驱动实现（用于 ESP8266 通信）
 * @details
 *          - 初始化 USART3（PB10-TX, PB11-RX），波特率可配置
 *          - 使用 **接收中断 + 定时器超时** 判定一帧数据结束
 *          - 支持将接收到的数据 **透传至 USART1（用于调试）**
 *          - 提供 \c u3_printf() 函数，实现 printf 重定向到 USART3
 *
 * @note
 *          - 帧结束判定逻辑：若连续两个字节接收间隔 >10ms，则认为一帧结束
 *          - 使用 TIM4 作为超时计时器（10ms 中断周期）
 *          - 接收状态标志 \c USART3_RX_STA：
 *                bit15: 1 = 已接收完整一帧，0 = 正在接收或空闲
 *                bit14~0: 当前已接收字节数
 *
 * @warning
 *          - 缓冲区大小由 \c USART3_MAX_RECV_LEN 限制，超长数据将被截断
 *          - 未处理接收溢出（ORE）错误，高波特率下可能丢数据
 ******************************************************************************
 */

#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "sys.h"
#include "usart3.h"
#include "usart.h"
#include "stdarg.h"     // 用于 va_list, vsprintf
#include "string.h"     // 用于 strlen

/* 全局变量声明 */
u8 flag_200ms = 0;              ///< 200ms 软件定时标志（当前未使用）
u8 timex = 0;                   ///< 通用时间计数器（当前未使用）

/** @brief USART3 接收缓冲区，最大存储 USART3_MAX_RECV_LEN 字节 */
u8 USART3_RX_BUF[USART3_MAX_RECV_LEN];

/** @brief USART3 发送缓冲区（用于 u3_printf） */
u8 USART3_TX_BUF[USART3_MAX_SEND_LEN];

/* 忽略 GCC 警告（因 AT 指令接口使用 char* 与 u8* 混合） */
#pragma GCC diagnostic ignored "-Wpointer-sign"
#pragma GCC diagnostic ignored "-Wint-conversion"
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wreturn-type"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"

/* 禁用 semihosting（用于 printf 重定向到串口） */
#pragma import(__use_no_semihosting)

void TIM4_Int_Init(u16 arr,u16 psc);

/**
 * @brief USART3 接收状态标志
 * @details
 *          - bit15 (MSB): 帧完成标志
 *                0 → 未完成（正在接收或空闲）
 *                1 → 已接收完整一帧，等待主程序处理
 *          - bit14~0: 当前已接收的有效字节数（最大 32767）
 *
 * @note 使用 volatile 保证中断与主循环间可见性
 */
vu16 USART3_RX_STA = 0;


/**
 * @brief USART3 中断服务函数（接收中断）
 * @details
 *          1. 读取接收到的字节 \c res
 *          2. **透传至 USART1**（用于调试：PC 可通过 USART1 查看 ESP8266 返回）
 *          3. 若尚未完成一帧（bit15=0）：
 *               - 将字节存入 \c USART3_RX_BUF
 *               - 清零 TIM4 计数器（重置超时）
 *               - 若是第一个字节，启动 TIM4（开始计时）
 *          4. 若缓冲区满，强制标记帧完成（bit15=1）
 *
 * @note
 *          - 此设计实现 **“空闲超时”帧检测**，适用于不定长 AT 应答
 *          - 透传至 USART1 是调试技巧，方便监控 ESP8266 通信
 */
void USART3_IRQHandler(void)
{
    u8 res = 0;

    /* 检查是否为接收中断 */
    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
        USART_ClearITPendingBit(USART3, USART_IT_RXNE); // 清除中断标志

        res = USART_ReceiveData(USART3); // 读取接收到的数据

        /* ========== 调试透传：将 ESP8266 数据转发到 USART1 ========== */
        while (!(USART1->SR & USART_FLAG_TXE)); // 等待 USART1 发送寄存器空
        USART_SendData(USART1, res);            // 通过 USART1 输出（如连接 PC）

        /* ========== 接收缓冲区管理 ========== */
        if ((USART3_RX_STA & (1 << 15)) == 0) // 若尚未完成一帧
        {
            if (USART3_RX_STA < USART3_MAX_RECV_LEN) // 缓冲区未满
            {
                TIM_SetCounter(TIM4, 0); // 重置 TIM4 计数器（延长超时）

                if (USART3_RX_STA == 0) // 第一个字节，启动定时器
                {
                    TIM_Cmd(TIM4, ENABLE); // 启动 TIM4（开始 10ms 超时计时）
                }

                USART3_RX_BUF[USART3_RX_STA++] = res; // 存入缓冲区
            }
            else
            {
                USART3_RX_STA |= (1 << 15); // 缓冲区满，强制标记完成
            }
        }
    }
}


/**
 * @brief 初始化 USART3（用于连接 ESP8266）
 * @param bound 波特率（如 115200）
 * @details
 *          - 引脚：PB10 (TX), PB11 (RX)
 *          - 配置：8N1，无流控
 *          - 使能接收中断
 *          - 初始化 TIM4 为 10ms 超时定时器（用于帧结束检测）
 *          - 初始关闭 TIM4，待接收到首字节再开启
 */
void usart3_init(u32 bound)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    /* 使能时钟 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);  //使能GPIOA时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

    /* 复位 USART3 */
    USART_DeInit(USART3);

    //串口1对应引脚复用映射
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_USART3);  // GPIOA9复用为USART1
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_USART3); // GPIOA10复用为USART1

    // USART1端口配置
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_10; // GPIOA9与GPIOA10
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;            //复用功能
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;       //速度50MHz
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;          //推挽复用输出
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;            //上拉
    GPIO_Init(GPIOB, &GPIO_InitStructure);                  //初始化PA9，PA10

    /* USART3 参数配置 */
    USART_InitStructure.USART_BaudRate = bound;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART3, &USART_InitStructure);

    /* 使能 USART3 */
    USART_Cmd(USART3, ENABLE);

    /* 使能接收中断 */
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

    /* 配置 NVIC 中断优先级 */
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* 初始化 TIM4：10ms 中断（用于超时检测） */
    TIM4_Int_Init(1000 - 1, 7200 - 1); 

    USART3_RX_STA = 0;      // 清空接收状态
    TIM_Cmd(TIM4, DISABLE); // 初始关闭定时器，待接收首字节再开启
}


/**
 * @brief USART3 的 printf 重定向函数
 * @param fmt 格式化字符串（同标准 printf）
 * @details
 *          - 使用 \c vsprintf 将格式化内容写入 \c USART3_TX_BUF
 *          - 循环发送每个字节，等待发送完成（TC 标志）
 *          - **注意**：此函数为阻塞式，且不支持中断发送
 *
 * @warning
 *          - 发送内容长度不能超过 \c USART3_MAX_SEND_LEN
 *          - 高频调用可能阻塞系统，建议仅用于调试
 */
void u3_printf(char* fmt, ...)
{
    u16 i, j;
    va_list ap;

    va_start(ap, fmt);
    vsprintf((char*)USART3_TX_BUF, fmt, ap); // 格式化到发送缓冲区
    va_end(ap);

    i = strlen((const char*)USART3_TX_BUF); // 获取实际长度

    for (j = 0; j < i; j++)
    {
        while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET); // 等待发送完成
        USART_SendData(USART3, USART3_TX_BUF[j]);
    }
}

void TIM4_Int_Init(u16 arr,u16 psc)
{	
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);//TIM3时钟使能    
	
	//定时器TIM7初始化
	TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure); //根据指定的参数初始化TIMx的时间基数单位
 
	TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE ); //使能指定的TIM7中断,允许更新中断
	
	TIM_Cmd(TIM4,ENABLE);//开启定时器7
	
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1 ;//抢占优先级0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;		//子优先级2
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
	
}	

// //定时器2中断服务程序		    
// void TIM2_IRQHandler(void)
// { 	
// 	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)//是更新中断
// 	{	 
	
// 		USART1_RX_STA|=1<<15;	//标记接收完成
// 		TIM_ClearITPendingBit(TIM2, TIM_IT_Update  );  //清除TIM2更新中断标志    
// 		TIM_Cmd(TIM2, DISABLE);  //关闭TIM2 	
// 	}	    
// }

// void TIM4_IRQHandler(void)
// { 	
// 	if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)//是更新中断
// 	{	 
	
// 		USART3_RX_STA|=1<<15;	//标记接收完成
// 		TIM_ClearITPendingBit(TIM4, TIM_IT_Update  );  //清除TIM4更新中断标志    
// 		TIM_Cmd(TIM4, DISABLE);  //关闭TIM4 	
// 	}	    
// }