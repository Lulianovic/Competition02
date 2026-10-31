/**
 * @file    bh1750.c
 * @brief   BH1750 驱动实现（软件 I²C，SCL=PB8，SDA=PB9）
 */
#include "bh1750.h"
#include "app_config.h"
#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"
#include "delay.h"

#define BH1750_ADDR_WRITE   0x46    /* 0x23 << 1 */
#define BH1750_ADDR_READ    0x47

#define BH_CMD_POWER_ON     0x01
#define BH_CMD_RESET        0x07
#define BH_CMD_CONT_HRES    0x10    /* 连续高分辨率模式 1 */

/* ---------- 底层引脚操作 ---------- */
#define SCL_H()  GPIO_SetBits(I2C_PORT, I2C_SCL_PIN)
#define SCL_L()  GPIO_ResetBits(I2C_PORT, I2C_SCL_PIN)
#define SDA_H()  GPIO_SetBits(I2C_PORT, I2C_SDA_PIN)
#define SDA_L()  GPIO_ResetBits(I2C_PORT, I2C_SDA_PIN)
#define SDA_RD() GPIO_ReadInputDataBit(I2C_PORT, I2C_SDA_PIN)

/** SDA 设为推挽输出 */
static void sda_out(void)
{
    I2C_PORT->MODER &= ~(0x03u << (9 * 2));
    I2C_PORT->MODER |=  (0x01u << (9 * 2));
}

/** SDA 设为输入（上拉） */
static void sda_in(void)
{
    I2C_PORT->MODER  &= ~(0x03u << (9 * 2));            /* 输入模式 */
    I2C_PORT->PUPDR  &= ~(0x03u << (9 * 2));
    I2C_PORT->PUPDR  |=  (0x01u << (9 * 2));            /* 上拉 */
}

static void i2c_delay(void)
{
    delay_us(5);   /* 约 100 kHz */
}

static void i2c_start(void)
{
    sda_out();
    SDA_H();
    SCL_H();
    i2c_delay();
    SDA_L();
    i2c_delay();
    SCL_L();
}

static void i2c_stop(void)
{
    sda_out();
    SCL_L();
    SDA_L();
    i2c_delay();
    SCL_H();
    i2c_delay();
    SDA_H();
    i2c_delay();
}

/** 发送 1 字节，返回 0 表示收到 ACK */
static uint8_t i2c_send_byte(uint8_t dat)
{
    uint8_t i;
    uint8_t ack;

    sda_out();
    for (i = 0; i < 8; i++) {
        SCL_L();
        if (dat & 0x80) { SDA_H(); } else { SDA_L(); }
        dat <<= 1;
        i2c_delay();
        SCL_H();
        i2c_delay();
    }
    SCL_L();

    /* 读取 ACK */
    sda_in();
    i2c_delay();
    SCL_H();
    i2c_delay();
    ack = SDA_RD();
    SCL_L();
    sda_out();          /* 立即恢复为输出，避免影响 OLED */
    return ack;         /* 0 = ACK */
}

/** 读取 1 字节，ack=0 表示主机回应 ACK */
static uint8_t i2c_read_byte(uint8_t ack)
{
    uint8_t i;
    uint8_t dat = 0;

    sda_in();
    for (i = 0; i < 8; i++) {
        SCL_L();
        i2c_delay();
        SCL_H();
        dat <<= 1;
        if (SDA_RD()) { dat |= 0x01; }
        i2c_delay();
    }
    SCL_L();

    /* 主机发送 ACK/NACK */
    sda_out();
    if (ack) { SDA_H(); } else { SDA_L(); }
    i2c_delay();
    SCL_H();
    i2c_delay();
    SCL_L();
    SDA_H();
    return dat;
}

void BH1750_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    gpio.GPIO_Pin   = I2C_SCL_PIN | I2C_SDA_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_OUT;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(I2C_PORT, &gpio);

    SCL_H();
    SDA_H();

    i2c_start();
    i2c_send_byte(BH1750_ADDR_WRITE);
    i2c_send_byte(BH_CMD_POWER_ON);
    i2c_stop();

    delay_ms(10);

    i2c_start();
    i2c_send_byte(BH1750_ADDR_WRITE);
    i2c_send_byte(BH_CMD_CONT_HRES);
    i2c_stop();

    delay_ms(180);      /* 等待首次转换完成（典型 120 ms） */
}

uint8_t BH1750_ReadRaw(uint16_t *raw)
{
    uint8_t hi, lo;

    i2c_start();
    if (i2c_send_byte(BH1750_ADDR_READ) != 0) {   /* 无 ACK */
        i2c_stop();
        return 0;
    }
    hi = i2c_read_byte(1);      /* 主机回应 ACK，继续读下一字节 */
    lo = i2c_read_byte(0);      /* 最后一字节回应 NACK */
    i2c_stop();

    *raw = ((uint16_t)hi << 8) | lo;
    return 1;
}

uint8_t BH1750_Read(float *lux)
{
    uint16_t raw = 0;

    if (BH1750_ReadRaw(&raw) == 0) {
        return 0;
    }
    *lux = (float)raw / 1.2f;
    return 1;
}
