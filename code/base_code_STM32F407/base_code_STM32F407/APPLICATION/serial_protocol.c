/**
 * @file    serial_protocol.c
 * @brief   串口命令协议实现
 *
 * 命令一览：
 *   SOIL,<pct>             注入土壤湿度（需测试模式）
 *   AIR,<temp>,<humi>      注入空气温湿度（需测试模式）
 *   LIGHT,<lux>            注入光照（需测试模式）
 *   MODE,AUTO              回到自动模式（默认，只用真实传感器）
 *   MODE,TEST              进入测试模式（允许注入）
 *   MODE,THRESHOLD         固定阈值策略
 *   MODE,SCORE             多参数评分策略
 *   FAULT,SOIL_TIMEOUT     模拟土壤数据超时
 *   FAULT,CLEAR            清除故障
 *   HELP                   打印帮助
 */
#include "serial_protocol.h"
#include "app_config.h"
#include "sensor_port.h"
#include "control.h"
#include "logger.h"

#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

/* ---------------- 接收缓冲（中断写入，主循环读取） ---------------- */
static volatile char     s_line[SERIAL_LINE_MAX];
static volatile uint16_t s_len   = 0;
static volatile uint8_t  s_ready = 0;

static RunMode s_mode = MODE_AUTO;

/* ---------------- 工具 ---------------- */

static uint8_t ieq(const char *a, const char *b)
{
    while (*a && *b) {
        char ca = *a, cb = *b;
        if (ca >= 'a' && ca <= 'z') { ca -= 32; }
        if (cb >= 'a' && cb <= 'z') { cb -= 32; }
        if (ca != cb) { return 0; }
        a++; b++;
    }
    return (*a == 0 && *b == 0);
}

/**
 * @brief 去掉字符串首尾的空白与回车换行（原地修改）
 * @return 指向第一个有效字符的指针
 */
static char *trim(char *s)
{
    char *end;

    if (s == 0) { return 0; }

    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') { s++; }

    end = s + strlen(s);
    while (end > s) {
        end--;
        if (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n') {
            *end = 0;
        } else {
            break;
        }
    }
    return s;
}

static uint8_t parse_float(const char *s, float *out)
{
    char *end = 0;
    float v;

    if (s == 0 || *s == 0) { return 0; }
    v = strtof(s, &end);
    if (end == s) { return 0; }
    *out = v;
    return 1;
}

/* ---------------- 命令处理 ---------------- */

static void handle(const char *line)
{
    char  buf[SERIAL_LINE_MAX];
    char *cmd;
    char *arg;
    float v1, v2;

    strncpy(buf, line, SERIAL_LINE_MAX - 1);
    buf[SERIAL_LINE_MAX - 1] = 0;

    /* 先整体去首尾空白（串口助手常带尾随空格/回车） */
    cmd = trim(buf);

    arg = strchr(cmd, ',');
    if (arg != 0) {
        *arg = 0;
        arg = trim(arg + 1);      /* 参数同样去空白 */
    }
    cmd = trim(cmd);

    if (ieq(cmd, "HELP")) {
        printf("CMD: SOIL,<pct> | AIR,<t>,<h> | LIGHT,<lux>\r\n");
        printf("CMD: MODE,AUTO | MODE,TEST | MODE,THRESHOLD | MODE,SCORE\r\n");
        printf("CMD: FAULT,SOIL_TIMEOUT | FAULT,CLEAR | HELP\r\n");
        return;
    }

    if (ieq(cmd, "MODE")) {
        if (arg == 0) { printf("ERR,NO_ARG\r\n"); return; }
        if (ieq(arg, "AUTO")) {
            s_mode = MODE_AUTO;
            SensorPort_SetMode(MODE_AUTO);
            printf("OK,MODE=AUTO\r\n");
        } else if (ieq(arg, "TEST")) {
            s_mode = MODE_TEST;
            SensorPort_SetMode(MODE_TEST);
            printf("OK,MODE=TEST\r\n");
        } else if (ieq(arg, "THRESHOLD")) {
            Control_SetStrategy(STRATEGY_THRESHOLD);
            printf("OK,STRATEGY=THRESHOLD\r\n");
        } else if (ieq(arg, "SCORE")) {
            Control_SetStrategy(STRATEGY_SCORE);
            printf("OK,STRATEGY=SCORE\r\n");
        } else {
            printf("ERR,UNKNOWN_MODE\r\n");
        }
        return;
    }

    if (ieq(cmd, "FAULT")) {
        if (arg == 0) { printf("ERR,NO_ARG\r\n"); return; }
        if (ieq(arg, "SOIL_TIMEOUT")) {
            SensorPort_SimulateSoilTimeout();
            printf("OK,SIMULATE_SOIL_TIMEOUT\r\n");
        } else if (ieq(arg, "CLEAR")) {
            Control_ClearFault();
            SensorPort_ClearInjection();
            printf("OK,FAULT_CLEARED\r\n");
        } else {
            printf("ERR,UNKNOWN_FAULT\r\n");
        }
        return;
    }

    /* ---- 以下命令仅在测试模式生效 ---- */
    if (s_mode != MODE_TEST) {
        printf("ERR,TEST_MODE_REQUIRED\r\n");
        return;
    }

    if (ieq(cmd, "SOIL")) {
        if (parse_float(arg, &v1) == 0) { printf("ERR,BAD_FLOAT\r\n"); return; }
        if (v1 < 0.0f || v1 > 100.0f)   { printf("ERR,OUT_OF_RANGE\r\n"); return; }
        SensorPort_InjectSoil(v1);
        printf("OK,SOIL=%.1f\r\n", v1);
        return;
    }

    if (ieq(cmd, "AIR")) {
        char *p2 = (arg != 0) ? strchr(arg, ',') : 0;
        if (arg == 0 || p2 == 0) { printf("ERR,NEED_2_ARGS\r\n"); return; }
        *p2 = 0;
        p2 = trim(p2 + 1);          /* 第二个参数同样去空白 */
        arg = trim(arg);
        if (parse_float(arg, &v1) == 0 || parse_float(p2, &v2) == 0) {
            printf("ERR,BAD_FLOAT\r\n"); return;
        }
        SensorPort_InjectAir(v1, v2);
        printf("OK,AIR=%.1f,%.1f\r\n", v1, v2);
        return;
    }

    if (ieq(cmd, "LIGHT")) {
        if (parse_float(arg, &v1) == 0) { printf("ERR,BAD_FLOAT\r\n"); return; }
        if (v1 < 0.0f) { printf("ERR,OUT_OF_RANGE\r\n"); return; }
        SensorPort_InjectLight(v1);
        printf("OK,LIGHT=%.1f\r\n", v1);
        return;
    }

    printf("ERR,UNKNOWN_CMD\r\n");
}

/* ---------------- 对外接口 ---------------- */

void SerialProtocol_Init(void)
{
    s_len   = 0;
    s_ready = 0;
    s_mode  = MODE_AUTO;
}

void SerialProtocol_Process(uint32_t now_ms)
{
    char buf[SERIAL_LINE_MAX];

    (void)now_ms;

    if (!s_ready) {
        return;
    }

    /* 拷贝到本地后再解析，解析期间允许中断继续接收下一行 */
    strncpy(buf, (const char *)s_line, SERIAL_LINE_MAX - 1);
    buf[SERIAL_LINE_MAX - 1] = 0;
    s_len   = 0;
    s_ready = 0;

    handle(buf);
}

RunMode SerialProtocol_GetMode(void)
{
    return s_mode;
}

/**
 * @brief USART1 接收中断：仅做字节搬运与完成标记
 * @note  本函数取代 SYSTEM/usart/usart.c 中原有的空实现
 */
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        char c = (char)USART_ReceiveData(USART1);
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);

        if (s_ready) {
            return;                     /* 上一行尚未处理，丢弃新字节 */
        }
        if (c == '\r' || c == '\n') {
            if (s_len > 0) {
                s_line[s_len] = 0;
                s_ready = 1;
                s_len   = 0;
            }
        } else if (s_len < SERIAL_LINE_MAX - 1) {
            s_line[s_len++] = c;
        } else {
            s_len = 0;                  /* 超长则整行丢弃，防止越界 */
        }
    }
}
