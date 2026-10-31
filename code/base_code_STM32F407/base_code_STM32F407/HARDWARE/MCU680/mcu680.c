/**
 * @file mcu680.c
 * @brief MCU680 气体传感器驱动实现（串口通信与帧封装）
 *
 * 本模块通过 USART2 与 MCU680 模块通讯，支持发送初始化指令、接收数据帧、帧校验、
 * 以及把数据打包后通过串口发送给上位机。
 */

#include "mcu680.h"
#include "delay.h"

/* 硬件/软件说明见 mcu680.h */

uint8_t str_temperature[10] = {0};
uint8_t str_humidity[10] = {0};
uint8_t str_airQuality[10] = {0};
uint8_t str_airPressure[10] = {0};
uint8_t str_altitude[10] = {0};
uint8_t RX_BUF[50]={0},stata=0;

/**
 * @brief 配置 MCU680 使用的中断（USART2）
 *
 * 配置 NVIC，使能 USART2 中断以便接收异步数据帧。
 */
void mcu680_irq_Configuration(void)
{
	NVIC_InitTypeDef NVIC_X;
	NVIC_X.NVIC_IRQChannel = USART2_IRQn;
	NVIC_X.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_X.NVIC_IRQChannelSubPriority = 0;
	NVIC_X.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_X);
}

/**
 * @brief 发送初始化/控制指令给 MCU680 模块
 *
 * 发送两帧：一帧用于模块握手/查询，随后发送自动输出启动指令。
 */
void send_Instruction(void)
{
	uint8_t send_data[4] = {0};
	send_data[0] = 0xa5;
	send_data[1] = 0x55;
	send_data[2] = 0x3F;
	send_data[3] = 0x39;
	USART_Send_bytes(send_data, 4);

	delay_ms(1);

	send_data[0] = 0xa5;
	send_data[1] = 0x56;
	send_data[2] = 0x02;
	send_data[3] = 0xfd;
	USART_Send_bytes(send_data, 4);
	delay_ms(1);
}

/**
 * @brief USART2 中断处理，用于接收 MCU680 上报的数据帧
 *
 * 接收缓冲以 0x5A 0x5A 为帧头，并根据长度字决定帧尾，接收完成后将数据复制到 RX_BUF 并置位 stata。
 */
void USART2_IRQHandler(void)
{
	static uint8_t rebuf[20]={0},i=0;
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		rebuf[i++]=USART_ReceiveData(USART2);
		if(rebuf[0]!=0x5a) i=0; // 帧头错误
		if((i==2)&&(rebuf[1]!=0x5a)) i=0;
		if(i>4)
		{
			if(i==rebuf[3]+5)
			{
			   memcpy(RX_BUF,rebuf,i);
				stata=1;
				i=0;
			}
		}
		USART_ClearFlag(USART2,USART_FLAG_RXNE);
	}
}

/**
 * @brief 校验接收到的帧并复制到输出缓冲
 * @param data 输出缓冲指针（由调用者提供），函数在成功时将帧数据复制到此处
 * @return 1 验证通过并复制成功，0 验证失败
 */
uint8_t CHeck(uint8_t *data)
{
	uint8_t sum=0,number=0,i=0;
	number=RX_BUF[3]+5;
	if(number>20) return 0;
	for(i=0;i<number-1;i++) sum+=RX_BUF[i];
	if(sum==RX_BUF[number-1])
	{
		memcpy(data,RX_BUF,number);
		return 1;
	}
	else
		return 0;
}

/**
 * @brief 发送单字节数据（阻塞直到发送寄存器空）
 * @param byte 要发送的字节
 */
void USART_send_byte(uint8_t byte)
{
	while(USART_GetFlagStatus(USART2,USART_FLAG_TC)==RESET);
	USART2->DR=byte;
}

/**
 * @brief 发送多个字节（按顺序调用单字节发送）
 * @param Buffer 指向数据缓冲区
 * @param Length 要发送的字节数
 */
void USART_Send_bytes(uint8_t *Buffer, uint8_t Length)
{
	uint8_t i=0;
	while(i<Length)
	{
		USART_send_byte(Buffer[i++]);
	}
}

/**
 * @brief 发送带校验和的多字节数据（最后一字节作为校验位）
 * @param Buffer 缓冲区，最后一字节将被累加为校验和
 * @param Length 缓冲区长度
 */
void USART_Send(uint8_t *Buffer, uint8_t Length)
{
	uint8_t i=0;
	while(i<Length)
	{
		if(i<(Length-1)) Buffer[Length-1]+=Buffer[i];
		USART_send_byte(Buffer[i++]);
	}
}

/**
 * @brief 将 16-bit 数据打包并发送为一帧
 * @param data 指向 16-bit 数据数组
 * @param length 数据长度（元素个数）
 * @param send 功能字节
 */
void send_out(int16_t *data,uint8_t length,uint8_t send)
{
	uint8_t TX_DATA[30],i=0,k=0;
	memset(TX_DATA,0,(2*length+5));
	TX_DATA[i++]=0X5A;
	TX_DATA[i++]=0X5A;
	TX_DATA[i++]=send;
	TX_DATA[i++]=2*length;
	for(k=0;k<length;k++)
	{
		TX_DATA[i++]=(uint16_t)data[k]>>8;
		TX_DATA[i++]=(uint16_t)data[k];
	}
	USART_Send(TX_DATA,i);
}

/**
 * @brief 将 8-bit 数据打包并发送为一帧
 * @param data 指向数据缓冲
 * @param length 数据长度
 * @param send 功能字节
 */
void send_8bit_out(uint8_t *data,uint8_t length,uint8_t send)
{
	uint8_t TX_DATA[50],i=0,k=0;
	memset(TX_DATA,0,(2*length+5));
	TX_DATA[i++]=0X5A;
	TX_DATA[i++]=0X5A;
	TX_DATA[i++]=send;
	TX_DATA[i++]=length;
	for(k=0;k<length;k++)
	{
		TX_DATA[i++]=(uint16_t)data[k];
	}
	USART_Send(TX_DATA,i);
}

/**
 * @brief USART2初始化函数 (STM32F4标准库版本)
 * 
 * 配置USART2外设，使用PA2(TX)和PA3(RX)作为通信引脚，
 * 支持中断接收功能。
 * 
 * @param BaudRatePrescaler 波特率设置，例如9600, 115200等
 */
void Usart_Int2(uint32_t BaudRatePrescaler)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    /**
     * @brief 使能时钟
     * USART2属于APB1总线，GPIOA属于AHB1总线
     */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    
    /**
     * @brief 配置USART2引脚复用功能
     * PA2 -> USART2_TX, PA3 -> USART2_RX
     * 在STM32F4中，USART2的TX/RX功能对应AF7
     */
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);  // PA2 复用为USART2_TX
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);  // PA3 复用为USART2_RX

    /**
     * @brief 配置USART2_TX引脚(PA2)
     * 配置为复用推挽输出模式，速度50MHz
     */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;               // 复用功能
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;          // 输出速度50MHz
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;             // 推挽输出
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;              // 上拉
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    /**
     * @brief 配置USART2_RX引脚(PA3)
     * 配置为复用功能，带弱上拉
     */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;               // 复用功能
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;          // 输出速度50MHz
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;             // 开漏输出
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;              // 上拉
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /**
     * @brief 配置USART2参数
     */
    USART_InitStructure.USART_BaudRate = BaudRatePrescaler;                    // 波特率设置
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;                // 8位数据格式
    USART_InitStructure.USART_StopBits = USART_StopBits_1;                     // 1位停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;                        // 无校验位
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无硬件流控
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;            // 接收和发送模式
    
    /**
     * @brief 初始化USART2
     */
    USART_Init(USART2, &USART_InitStructure);
    
    /**
     * @brief 配置NVIC中断
     * 使能USART2中断
     */
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    /**
     * @brief 使能USART2接收中断
     */
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE); //开启接收中断
    
    /**
     * @brief 使能USART2外设
     */
    USART_Cmd(USART2, ENABLE);
}