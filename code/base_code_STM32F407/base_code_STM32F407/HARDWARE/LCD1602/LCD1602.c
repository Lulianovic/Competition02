/**
 * @file LCD1602.c
 * @brief LCD1602 字符显示屏驱动实现（GPI0 控制、命令与数据写入、用户自定义字符）
 *
 * 该文件实现基于 HD44780 类型指令集的 LCD1602 驱动：
 * - GPIO 初始化
 * - LCD 初始化序列
 * - 命令/数据写入函数（阻塞直至忙标志释放）
 * - 字符串显示及 CGRAM 用户自定义字符写入
 */

#include "LCD1602.h"

/*********************************** GPIO 初始化 ******************************************/
/**
 * @brief 初始化用于 LCD1602 的 GPIO 引脚
 *
 * 配置说明：
 * - PB: 控制信号 EN、RW、RS，推挽输出
 * - PA0..PA7: 并行数据总线，推挽输出
 */
void GPIO_INIT(void)
{
    GPIO_InitTypeDef PB_InitStruct;
    GPIO_InitTypeDef PA_InitStruct;
    
    // 使能GPIO时钟 (STM32F4使用AHB1总线)
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    
    // 配置GPIOB的EN、RW、RS引脚为推挽输出，速度50MHz
    PB_InitStruct.GPIO_Pin = EN | RW | RS;
    PB_InitStruct.GPIO_Mode = GPIO_Mode_OUT;          // 输出模式
    PB_InitStruct.GPIO_OType = GPIO_OType_PP;         // 推挽输出
    PB_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;      // 输出速度50MHz
    PB_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;       // 无上下拉
    GPIO_Init(GPIOB, &PB_InitStruct);
    
    // 配置GPIOA的Pin0-Pin7为推挽输出，速度50MHz
    PA_InitStruct.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 |
                             GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 |
                             GPIO_Pin_6 | GPIO_Pin_7;
    PA_InitStruct.GPIO_Mode = GPIO_Mode_OUT;          // 输出模式
    PA_InitStruct.GPIO_OType = GPIO_OType_PP;         // 推挽输出
    PA_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;      // 输出速度50MHz
    PA_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;       // 无上下拉
    GPIO_Init(GPIOA, &PA_InitStruct);
}
/*********************************** GPIO 初始化 ******************************************/

/*********************************** LCD 初始化 ******************************************/
/**
 * @brief 初始化 LCD1602
 *
 * 完成 GPIO 初始化并按顺序发送 LCD 指令以进入工作模式。
 */
void LCD_INIT(void){
	GPIO_INIT();    // 初始化 GPIO
    
	GPIO_Write( GPIOA, 0x0000 );
	GPIO_Write( GPIOB, 0x0000 );
    
	/* 使用宏组合设置数据长度、显示开、光标等 */
	LCD_WRITE_CMD( LCD1602A_CMD6 | LCD1602A_CMD6_DL_SET| LCD1602A_CMD6_N_SET );
	LCD_WRITE_CMD( LCD1602A_CMD4 | LCD1602A_CMD4_D_SET |LCD1602A_CMD4_C_SET|LCD1602A_CMD4_B_SET);
	LCD_WRITE_CMD( LCD1602A_CMD3 | LCD1602A_CMD3_ID_SET);
	LCD_WRITE_CMD( LCD1602A_CMD1 );
}
/*********************************** LCD 初始化 ******************************************/

/*********************************** 写入命令函数 ******************************************/
/**
 * @brief 向 LCD 写入命令（8-bit）
 * @param CMD 要写入的命令字节
 *
 * @note 此函数会先等待 LCD 不忙，然后在控制线上发送命令并触发 EN 脉冲。
 */
void LCD_WRITE_CMD( unsigned char CMD ){
	ReadBusy();
	GPIO_ResetBits( GPIOB, RS );
	GPIO_ResetBits( GPIOB, RW );
	GPIO_ResetBits( GPIOB, EN );
	GPIO_Write( GPIOA, CMD );
	GPIO_SetBits( GPIOB, EN );
	GPIO_ResetBits( GPIOB, EN );
}
/*********************************** 写入命令函数 ******************************************/

/*********************************** 写入单个 Byte 函数 **************************************/
/**
 * @brief 向 LCD 写入一个数据字节（用于显示）
 * @param ByteData 要写入的数据字节
 */
void LCD_WRITE_ByteDATA( unsigned char ByteData ){
	ReadBusy();
	GPIO_SetBits( GPIOB, RS );
	GPIO_ResetBits( GPIOB, RW );
	GPIO_ResetBits( GPIOB, EN );
	GPIO_Write( GPIOA, ByteData );
	GPIO_SetBits( GPIOB, EN );
	GPIO_ResetBits( GPIOB, EN );
}

/*********************************** 写入字符串函数 ******************************************/
/**
 * @brief 在指定行列写入字符串（以 null 结尾）
 * @param StrData 指向以 null 结尾的字符串
 * @param row 行号（0=第一行, 1=第二行）
 * @param col 列偏移（从 0 开始）
 */
void LCD_WRITE_StrDATA( char *StrData, unsigned char row, unsigned char col ){
	unsigned char baseAddr = 0x00;
	if ( row ){
		baseAddr = 0xc0;
	}else{
		baseAddr = 0x80;
	}
	baseAddr += col;

	LCD_WRITE_CMD( baseAddr );
	while ( *StrData != '\0' ){
		LCD_WRITE_ByteDATA( *StrData );
		baseAddr++;
		StrData++;
	}
}

/*********************************** 读忙函数 ******************************************/
/**
 * @brief 读取 LCD 的忙标志（BUSY），在返回前保证 LCD 可接受新命令
 *
 * @note 调用此函数前需确保数据总线方向可被设置为输入，本函数会临时将 PA7 配为浮空输入读取 BUSY 信号。
 */
void ReadBusy(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    
    // 先将GPIOA的低8位引脚设置为输出，输出0xFF
    GPIO_Write(GPIOA, 0x00FF);
    
    // 临时将GPIOA的Pin7配置为浮空输入模式
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;           // 输入模式
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;       // 浮空输入（无上下拉）
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // 设置RS为低电平（命令模式），RW为高电平（读操作）
    GPIO_ResetBits(GPIOB, RS);
    GPIO_SetBits(GPIOB, RW);
    
    // 拉高EN使能信号开始读操作
    GPIO_SetBits(GPIOB, EN);
    
    // 等待忙标志位清零（PA7为低电平表示空闲）
    while(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_7) == Bit_SET);
    
    // 拉低EN结束读操作
    GPIO_ResetBits(GPIOB, EN);
    
    // 将GPIOA的低8位引脚重新配置为推挽输出模式
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 |
                               GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 |
                               GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;          // 输出模式
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;         // 推挽输出
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;      // 输出速度50MHz
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;       // 无上下拉
    GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/*********************************** 写入用户自定义图像 ******************************************/
/**
 * @brief 将用户自定义字符图样写入 CGRAM
 * @param pos 自定义字符槽位 (0..7)
 * @param ImgInfo 指向字符位模数据（以 '\0' 结尾）
 *
 * @note 每个自定义字符占 8 字节（8 行），CGRAM 地址按规格分配，本函数将根据 pos 计算起始地址。
 */
void WUserImg(unsigned char pos,unsigned char *ImgInfo){
	unsigned char cgramAddr;
    
	if( pos <= 1 ) cgramAddr = 0x40;
	if( pos > 1 && pos <= 3 ) cgramAddr = 0x50;
	if( pos > 3 && pos <= 5 ) cgramAddr = 0x60;
	if( pos > 5 && pos <= 7 ) cgramAddr = 0x70;

	LCD_WRITE_CMD( (cgramAddr + (pos%2) * 8) );
    
	while( *ImgInfo != '\0' ){
		LCD_WRITE_ByteDATA( *ImgInfo );
		ImgInfo++;
	}
}
/*********************************** 写入用户自定义图像 ******************************************/
