/**
 ******************************************************************************
 * @file    esp8266.c
 * @brief   ESP8266 Wi-Fi 模块 AT 指令驱动与通信实验集合
 * @details
 *          - 提供多种工作模式（STA / AP / STA+AP）配置示例
 *          - 支持 TCP/UDP 单连接与多连接通信
 *          - 实现透传模式（Serial-to-Network）的进入与退出
 *          - 封装 AT 指令发送、应答检测、字符串查找等基础功能
 *
 * @note
 *          - ESP8266 通过 USART3 与 STM32 通信（波特率通常为 115200）
 *          - 所有 AT 指令以 "\r\n" 结尾（由 \c u3_printf 自动添加）
 *          - 接收使用全局缓冲区 \c USART3_RX_BUF 和状态标志 \c USART3_RX_STA
 *          - 透传模式退出需发送 "+++" 且前后有 >10ms 空闲（关键！）
 *
 * @warning
 *          - 代码中部分函数存在逻辑缺陷（如未正确处理应答超时、缓冲区复用）
 *          - 建议在实际项目中增加重试机制、错误恢复和更健壮的解析逻辑
 ******************************************************************************
 */

#include "esp8266.h"
#include "string.h"
#include "usart.h"
#include "usart3.h"
#include "stm32f4xx.h"
#include "sys.h" 
#include "delay.h"

/* 忽略 GCC 警告（因 AT 指令接口使用 char* 与 uint8_t* 混合） */
#pragma GCC diagnostic ignored "-Wpointer-sign"
#pragma GCC diagnostic ignored "-Wint-conversion"
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wreturn-type"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"


/**
 * @brief 测试 ESP8266 基础 AT 指令
 * @details
 *          - AT：模块响应 "OK" 表示通信正常
 *          - AT+GMR：查询固件版本
 *          - ATE1：开启命令回显（便于调试）
 *          - AT+CWMODE=1：设置为 Station 模式
 *          - AT+UART=...：重新配置串口参数（注意：执行后需同步更改 STM32 波特率！）
 *
 * @warning
 *          执行 \c AT+UART 后若不立即切换 STM32 串口波特率，后续通信将失败！
 */
void esp8266_AT_test(void)
{
    esp8266_send_cmd("AT", "OK", 500);              // 基础通信测试
    esp8266_send_cmd("AT+GMR", "OK", 500);          // 固件版本
    esp8266_send_cmd("ATE1", "OK", 500);            // 开启回显
    esp8266_send_cmd("AT+CWMODE=1", "OK", 500);     // Station 模式
    esp8266_send_cmd("AT+UART=115200,8,1,0,3", "OK", 500); // 重设 UART（慎用！）
}


/**
 * @brief 配置 ESP8266 为 Station (STA) 模式并连接路由器
 * @details
 *          - 设置 MAC 地址（可选）
 *          - 扫描附近 AP（AT+CWLAP）
 *          - 连接指定 Wi-Fi（SSID="HTPL", 密码="1234567890"）
 *          - 设置静态 IP（192.168.4.2）
 *          - 断开连接（AT+CWQAP）
 *
 * @note 实际使用时应替换 SSID 和密码为真实值
 */
void esp8266_STA_test(void)
{
    esp8266_send_cmd("AT+CWMODE=1", "OK", 10);
    esp8266_send_cmd("AT+CIPSTAMAC=\"18:fe:35:98:d3:7b\"", "OK", 10); // 设置 STA MAC
    esp8266_send_cmd("AT+CWLAP", "OK", 10);                           // 扫描 AP
    esp8266_send_cmd("AT+CWJAP=\"HTPL\",\"1234567890\"", "OK", 10);   // 连接 Wi-Fi
    esp8266_send_cmd("AT+CIPSTA=\"192.168.4.2\"", "OK", 10);          // 设置静态 IP
    esp8266_send_cmd("AT+CWQAP", "OK", 10);                           // 断开 Wi-Fi
}


/**
 * @brief 配置 ESP8266 为 SoftAP 模式（创建热点）
 * @details
 *          - 创建名为 "esp8266"、密码 "123456789" 的 AP
 *          - 信道 6，加密方式 WPA_WPA2_PSK（4）
 *          - 设置 AP MAC 和 IP 地址
 */
void esp8266_AP_test(void)
{
    esp8266_send_cmd("AT+CWMODE=2", "OK", 10);
    esp8266_send_cmd("AT+CWSAP=\"esp8266\",\"123456789\",6,4", "OK", 10); // 创建 AP
    esp8266_send_cmd("AT+CIPAPMAC=\"18:fe:35:98:d3:7b\"", "OK", 10);       // 设置 AP MAC
    esp8266_send_cmd("AT+CIPAP=\"192.168.4.2\"", "OK", 10);                // 设置 AP IP
}


/**
 * @brief TCP 单连接客户端（STA 模式）
 * @details
 *          - 连接到远程 TCP 服务器（192.168.4.1:4000）
 *          - 发送 两次 字节数据 "HELLO" "WORLD"
 *          - 关闭连接
 *
 * @note 适用于简单点对点通信
 */
void esp8266_TCP_Single_STA(void)
{
    esp8266_send_cmd("AT+CWMODE=3","OK",500);
    
    esp8266_send_cmd("AT+CIPMUX=0","OK",500);
    
    esp8266_send_cmd("AT+CWJAP=\"HTPL\",\"1234567890\"","OK",500);
    
    esp8266_send_cmd("AT+CIPSTART=\"TCP\",\"192.168.4.1\",4000","OK",3000);
    
    esp8266_send_cmd("AT+CIPSEND=5","OK",500);
    
    esp8266_send_cmd("Hello","OK",500);
    
    esp8266_send_cmd("AT+CIPSEND=5","OK",500);
    
    esp8266_send_cmd("World","OK",4000);
    
    esp8266_send_cmd("AT+CIPCLOSE","OK",500);
}



/**
 * @brief TCP 多连接服务器（STA 模式）
 * @details
 *          - 开启多连接模式（CIPMUX=1）
 *          - 启动 TCP 服务器（默认端口 333）
 *          - 向连接 0 发送 "ABCD"
 *          - 关闭该连接
 *
 * @warning
 *          此函数缺少等待客户端连接的逻辑，实际需监听 "+IPD" 事件
 */
void esp8266_TCP_Multilink_STA(void)
{
    esp8266_send_cmd("AT+CWMODE=3", "OK", 10);
    esp8266_send_cmd("AT+CIPMUX=1", "OK", 10);
    esp8266_send_cmd("AT+CWJAP=\"HTPL\",\"1234567890\"", "OK", 10);
    esp8266_send_cmd("AT+CIPSERVER=1", "OK", 50);                       // 启动服务器
    esp8266_send_cmd("AT+CIPSEND=0,4", "OK", 10);                       // 向 ID=0 发送
    esp8266_send_cmd("ABCD", "OK", 100);
    esp8266_send_cmd("AT+CIPCLOSE=0", "OK", 10);
}


/**
 * @brief ESP8266 UDP 单连接测试函数
 * @details
 *          执行标准 UDP 通信初始化流程：
 *          1. 设置 Wi-Fi 模式为 STA+AP（3）
 *          2. 设置单连接模式（CIPMUX=0）
 *          3. 连接指定 AP（SSID="HTPL", 密码="1234567890"）
 *          4. 建立 UDP 通道：
 *               - 目标 IP: 192.168.4.1
 *               - 目标端口: 4000
 *               - 本地端口: 1112（固定，便于服务器回连）
 *          5. 发送 11 字节数据："Hello World"
 *          6. （可选）关闭连接
 *
 * @note
 *          - `esp8266_send_cmd(cmd, ack, timeout)`:
 *                • 发送 cmd + "\r\n"
 *                • 等待 ack 字符串出现（如 "OK"）
 *                • 超时时间单位：毫秒
 *          - UDP 无需真正“连接”，但 AT+CIPSTART 用于绑定目标地址
 *          - 本地端口 1112 可省略（默认随机），但固定端口便于调试
 */
void esp8266_UDP_Single(void)
{
    // 1. 设置 Wi-Fi 模式：3 = STA + AP（混合模式）
    //    - STA 用于连接路由器上网
    //    - AP 可用于本地调试（本实验主要用 STA）
    esp8266_send_cmd("AT+CWMODE=3", "OK", 500);

    // 2. 设置为单连接模式（UDP 通常使用单连接）
    esp8266_send_cmd("AT+CIPMUX=0", "OK", 500);

    // 3. 连接 Wi-Fi 路由器（请根据实际环境修改 SSID 和密码）
    esp8266_send_cmd("AT+CWJAP=\"HTPL\",\"1234567890\"", "OK", 3000);

    // 4. 建立 UDP 通信通道
    //    格式：AT+CIPSTART="UDP","remote_ip",remote_port[,local_port]
    //    - remote_ip: 192.168.4.1（Wi-Fi 监视器或 PC 服务器 IP）
    //    - remote_port: 4000（服务器监听端口）
    //    - local_port: 1112（ESP8266 本地绑定端口，可选）
    esp8266_send_cmd("AT+CIPSTART=\"UDP\",\"192.168.4.1\",4000,1112", "OK", 500);

    // 5. 发送 11 字节数据（"Hello World" 共 11 字符）
    //    注意：AT+CIPSEND=11 后需等待 ">" 提示符再发送数据
    //    （假设 esp8266_send_cmd 内部已处理此流程）
    esp8266_send_cmd("AT+CIPSEND=11", ">", 500);
    esp8266_send_cmd("Hello World", "SEND OK", 5000);

    // 6. 【可选】关闭 UDP 连接（实际 UDP 无连接，此操作释放资源）
    //    关闭后无法再发送，除非重新 CIPSTART
    esp8266_send_cmd("AT+CIPCLOSE", "OK", 500);
}


/**
 * @brief ESP8266 UDP 多连接测试函数（示例：仅使用 Link ID=0）
 * @details
 *          执行标准多连接 UDP 初始化流程：
 *          1. 设置 Wi-Fi 模式为 STA+AP（3）
 *          2. 启用多连接模式（CIPMUX=1）
 *          3. 连接指定 AP（SSID="HTPL", 密码="1234567890"）
 *          4. 建立 UDP 通道（Link ID=0）：
 *               - 目标 IP: 192.168.4.1
 *               - 目标端口: 4000
 *               - 本地端口: 1112（固定，便于服务器回连）
 *               - 模式: 2（UDP 模式，通常可省略）
 *          5. 向 Link 0 发送 "Hello World"
 *          6. 关闭 Link 0（可选）
 *
 * @note
 *          - 多连接模式下，每条连接需指定 Link ID（0~4）
 *          - AT+CIPSTART 语法：
 *                AT+CIPSTART=<link_id>,"UDP","remote_ip",remote_port[,local_port][,mode]
 *          - mode=2 表示 UDP（默认即为 UDP，通常可省略）
 *          - 本示例仅演示单路（ID=0），实际可扩展至多路
 */
void esp8266_UDP_Multilink(void)
{
    // 1. 设置 Wi-Fi 模式：3 = STA + AP（混合模式）
    esp8266_send_cmd("AT+CWMODE=3", "OK", 500);

    // 2. 启用多连接模式（关键步骤！）
    //    - CIPMUX=0：单连接
    //    - CIPMUX=1：多连接（最多 5 路）
    esp8266_send_cmd("AT+CIPMUX=1", "OK", 500);

    // 3. 连接 Wi-Fi 路由器（请根据实际环境修改 SSID 和密码）
    esp8266_send_cmd("AT+CWJAP=\"HTPL\",\"1234567890\"", "OK", 2000);

    // 4. 建立 UDP 连接（Link ID = 0）
    //    参数说明：
    //      - 0           → Link ID
    //      - "UDP"       → 协议类型
    //      - "192.168.4.1" → 目标服务器 IP
    //      - 4000        → 目标端口
    //      - 1112        → 本地绑定端口（可选，便于回连）
    //      - 2           → 模式（2=UDP，通常可省略）
    esp8266_send_cmd("AT+CIPSTART=0,\"UDP\",\"192.168.4.1\",4000,1112,2", "OK", 3000);

    // 5. 发送 11 字节数据到 Link 0
    //    注意：需先发送 AT+CIPSEND=0,11，等待 ">" 后再发纯数据
    //    （假设 esp8266_send_cmd 能正确处理此流程）
    esp8266_send_cmd("AT+CIPSEND=0,11", ">", 500);
    esp8266_send_cmd("Hello World", "SEND OK", 5000);

    // 6. 【可选】关闭 Link 0
    //    关闭后该 Link ID 可重新用于新连接
    esp8266_send_cmd("AT+CIPCLOSE=0", "OK", 500);
}

/**
 * @brief ESP8266 TCP 透传模式初始化函数
 * @details
 *          执行标准透传建立流程：
 *          1. 设置 Wi-Fi 模式为 STA+AP（3）
 *          2. 设置单连接模式（CIPMUX=0）
 *          3. 连接指定 AP（SSID="HTPL", 密码="1234567890"）
 *          4. 建立 TCP 连接（目标 IP:192.168.4.1, 端口:4000）
 *          5. 启用透传模式（CIPMODE=1）
 *          6. 发送初始数据（可选）
 *          7. 【等待用户手动退出透传】
 *          8. 退出透传并关闭连接
 *
 * @note
 *          - 透传一旦启动，**STM32 与 ESP8266 之间不再解析 AT 指令**
 *          - 所有从 USART1 收到的数据 → 自动经 USART3 发送给服务器
 *          - 所有从服务器收到的数据 → 自动经 USART1 输出到 PC
 *          - **退出透传的唯一方式**：在串口助手中发送 "+++"（前后需 >1 秒无数据）
 */
void esp8266_TCP_SeriaNet(void)
{
    // 1. 设置 Wi-Fi 模式：3 = STA + AP
    esp8266_send_cmd("AT+CWMODE=3", "OK", 500);

    // 2. 设置为单连接模式（透传仅支持单连接）
    esp8266_send_cmd("AT+CIPMUX=0", "OK", 500);

    // 3. 连接 Wi-Fi 路由器
    esp8266_send_cmd("AT+CWJAP=\"HTPL\",\"1234567890\"", "OK", 2000);

    // 4. 建立 TCP 连接（目标服务器必须已启动！）
    esp8266_send_cmd("AT+CIPSTART=\"TCP\",\"192.168.4.1\",4000", "OK", 500);

    // 5. 启用透传模式
    //    - CIPMODE=1：进入透传
    //    - 此后所有串口数据直发服务器，不再识别 AT 指令
    esp8266_send_cmd("AT+CIPMODE=1", "OK", 500);

    // 6. 开始透传（发送 CIPSEND 后，ESP8266 返回 ">" 即进入透传）
    esp8266_send_cmd("AT+CIPSEND", "OK", 500); // 实际应等待 ">"，此处简化

    // 7. 【透传阶段开始】
    //    - 此时 STM32 的 USART1 接收中断会将数据直接写入 USART3_TX
    //    - USART3_RX 中断会将服务器数据写入 USART1_TX
    //    - 用户可在串口助手中自由输入/接收数据
    //
    //      关键：要退出透传，必须在串口助手中手动发送：
    //          +++（三个加号，无空格，前后 1 秒无其他数据）
    //
    // 8. 发送测试数据（可选，验证连接）
    //    注意：此行在真实透传中可能被忽略或作为用户数据发送
    esp8266_send_cmd("Hello World", "OK", 8000);

    // 9. 【程序将在此处阻塞，直到用户发送 "+++"】
    //    发送 "+++" 后，ESP8266 退出透传，返回 "OK"
    //    此时才能继续执行后续 AT 指令
    esp8266_send_cmd("AT+CIPMODE=0", "OK", 500);

    // 10. 关闭 TCP 连接
    esp8266_send_cmd("AT+CIPCLOSE", "OK", 500);
}


/**
 * @brief ESP8266 UDP 透传模式初始化函数
 * @details
 *          执行标准 UDP 透传建立流程：
 *          1. 设置 Wi-Fi 模式为 STA+AP（3）
 *          2. 设置单连接模式（CIPMUX=0）
 *          3. 连接指定 AP（SSID="HTPL", 密码="1234567890"）
 *          4. 建立 UDP 通道（目标 IP:192.168.4.1, 端口:4000）
 *             （本地端口默认随机，也可指定如 ,2233）
 *          5. 启用透传模式（CIPMODE=1）
 *          6. 启动透传（CIPSEND）
 *          7. 【等待用户手动退出透传】
 *          8. 退出透传并关闭连接
 *
 * @note
 *          - UDP 透传与 TCP 透传行为类似，但：
 *                • 无连接建立过程（CIPSTART 仅绑定目标地址）
 *                • 数据以 UDP 包形式发送，不保证可靠到达
 *          - 透传启动后，**所有串口输入 = UDP 数据负载**
 *          - 服务器回发的 UDP 数据会自动显示在串口助手中
 */
void esp8266_UDP_SeriaNet(void)
{
    // 1. 设置 Wi-Fi 模式：3 = STA + AP（混合模式）
    esp8266_send_cmd("AT+CWMODE=3", "OK", 500);

    // 2. 设置为单连接模式（透传仅支持单连接）
    esp8266_send_cmd("AT+CIPMUX=0", "OK", 500);

    // 3. 连接 Wi-Fi 路由器（请根据实际环境修改 SSID 和密码）
    esp8266_send_cmd("AT+CWJAP=\"HTPL\",\"1234567890\"", "OK", 2000);

    // 4. 建立 UDP 通道
    //    格式：AT+CIPSTART="UDP","remote_ip",remote_port[,local_port][,mode]
    //    本例未指定 local_port（默认随机），若需固定可改为：
    //        "AT+CIPSTART=\"UDP\",\"192.168.4.1\",4000,2233"
    esp8266_send_cmd("AT+CIPSTART=\"UDP\",\"192.168.4.1\",4000", "OK", 500);

    // 5. 启用透传模式
    //    - CIPMODE=1：进入透传
    //    - 此后所有串口数据直发 UDP，不再识别 AT 指令
    esp8266_send_cmd("AT+CIPMODE=1", "OK", 500);

    // 6. 开始透传（发送 CIPSEND 后，ESP8266 返回 ">" 即进入透传）
    esp8266_send_cmd("AT+CIPSEND", "OK", 500); // 实际应等待 ">" 提示符

    // 7. 【透传阶段开始】
    //    - 用户在串口助手输入 "LED ON" → 自动封装为 UDP 包发往 192.168.4.1:4000
    //    - 服务器回发 "ACK" → 自动显示在串口助手
    //
    //     关键退出方式：
    //        停止输入 ≥1 秒 → 输入 "+++" → 再停 ≥1 秒
    //        ESP8266 将退出透传，返回 "OK"

    // 8. 【注意】以下指令在真实透传中不会被执行！
    //    因为程序会阻塞在透传状态，直到用户发送 "+++"
    //    此处仅为示意后续操作流程
    esp8266_send_cmd("Hello Wrold", "", 1000); //实际会被当作用户数据发送！

    // 9. 退出透传（需先手动发 "+++"）
    esp8266_send_cmd("AT+CIPMODE=0", "OK", 500);

    // 10. 关闭 UDP 连接（释放资源）
    esp8266_send_cmd("AT+CIPCLOSE", "OK", 500);
}


/**
 * @brief TCP 多连接服务器（AP 模式）
 * @details
 *          - 假设已处于 AP+STA 模式且客户端已连接
 *          - 直接向连接 0 发送数据
 *
 * @warning 缺少连接建立和数据接收处理，仅作发送演示
 */
void esp8266_TCP_Multilink_AP(void)
{
    esp8266_send_cmd("AT+CWMODE=3", "OK", 10);
    esp8266_send_cmd("AT+CIPMUX=1", "OK", 10);
    esp8266_send_cmd("AT+CIPSEND=0,4", "OK", 10);
    esp8266_send_cmd("ABCD", "OK", 1);
    esp8266_send_cmd("AT+CIPCLOSE=0", "OK", 10);
}


/**
 * @brief 配置 ESP8266 进入透传模式（完整流程）
 * @details
 *          - 重启模块确保干净状态
 *          - 连接指定 Wi-Fi
 *          - 建立 TCP 连接
 *          - 开启透传并进入发送状态
 *          - 此后 STM32 串口数据将直接透传到服务器
 */
void esp8266_start_trans(void)
{
    esp8266_send_cmd("AT+CWMODE=1", "OK", 5);
    esp8266_send_cmd("AT+RST", "ready", 2);           // 重启
    delay_ms(1000);                                   // 等待重启完成（原代码延时不足！）

    // 循环连接 Wi-Fi 直到成功
    while (esp8266_send_cmd("AT+CWJAP=\"xxxx\",\"xxxx\"", "WIFI GOT IP", 6));

    esp8266_send_cmd("AT+CIPMUX=0", "OK", 2);
    // 循环连接服务器直到成功
    while (esp8266_send_cmd("AT+CIPSTART=\"TCP\",\"xxx.xxx.xxx.xxx\",xxxx", "CONNECT", 20));

    esp8266_send_cmd("AT+CIPMODE=1", "OK", 2);        // 开启透传
    esp8266_send_cmd("AT+CIPSEND", "OK", 50);         // 进入透传发送状态
}


/**
 * @brief 退出透传模式
 * @retval 0: 成功  1: 失败
 * @details
 *          - 发送 "+++"（必须前后有 >10ms 空闲）
 *          - 发送 "AT" 测试是否退出成功
 *
 * @note 原代码 delay_ms(10) 不足，应 ≥100ms 确保退出
 */
uint8_t esp8266_quit_trans(void)
{
    uint8_t result = 1;
    u3_printf("+++");               // 发送退出序列
    delay_ms(100);                  // ⚠️ 原 delay_ms(10) 太短！建议 100ms+
    result = esp8266_send_cmd("AT", "OK", 20);
    if (result)
        printf("quit_trans failed!");
    else
        printf("quit_trans success!");
    return result;
}


/**
 * @brief 在接收缓冲区中查找指定字符串（当前实现有缺陷）
 * @param str 要查找的字符串
 * @retval 1: 找到  0: 未找到
 *
 * @warning
 *          - 函数未使用全局接收状态标志（注释掉的部分）
 *          - 每次调用都会清空 USART3_RX_STA，可能导致数据丢失
 *          - 仅比较整个缓冲区是否完全等于 \c str，实用性低
 */
uint8_t Find(char *str)
{ 
    char *strx = 0;
    printf("test\n");
    
    // ⚠️ 注释掉的逻辑才是正确的：应检查是否完成一帧接收
    // if(USART3_RX_STA & 0X8000)
    {
        USART3_RX_BUF[USART3_RX_STA & 0X7FFF] = 0; // 添加字符串结束符
        if (strcmp(USART3_RX_BUF, str) == 0)
            return 1;
        else
            return 0;
        
        printf("BACK:%s\n", strx);
        USART3_RX_STA = 0; // ⚠️ 过早清零，可能影响其他函数
    }
}


/**
 * @brief 检查 ESP8266 返回数据中是否包含指定应答
 * @param str 期待的子字符串（如 "OK"）
 * @retval 非 NULL: 指向匹配位置  NULL: 未找到
 *
 * @note 使用 \c strstr 进行子串匹配，比全字匹配更实用
 */
uint8_t* esp8266_check_cmd(uint8_t *str)
{
    char *strx = 0;
    if (USART3_RX_STA & 0X8000)        // 已接收完整一帧
    { 
        USART3_RX_BUF[USART3_RX_STA & 0X7FFF] = 0; // 结束符
        strx = strstr((const char*)USART3_RX_BUF, (const char*)str);
        printf("BACK:%s\n", strx ? strx : "NULL");
        USART3_RX_STA = 0;             // 清除接收标志
    } 
    return (uint8_t*)strx;
}


/**
 * @brief 发送 AT 指令并等待指定应答
 * @param cmd 指令字符串（不含 \r\n）
 * @param ack 期待的应答子串（如 "OK"），可为 NULL
 * @param waittime 超时时间（单位：10ms，即总等待时间为 waittime*10ms？但实际按 ms 计）
 * @retval 0: 成功收到应答  1: 超时
 */
uint8_t esp8266_send_cmd(uint8_t *cmd, uint8_t *ack, u16 waittime)
{
    uint8_t res = 0; 
    USART3_RX_STA = 0;
    
    esp8266_send_data(cmd, waittime); // 发送指令

    if (ack && waittime)
    {
        while (--waittime)
        {
            delay_ms(2);
            if (USART3_RX_STA & 0X8000)
            {
                esp8266_check_cmd(ack); // 检查应答（但未用于判断结果！）
                printf("ack:%s\r\n", (uint8_t*)ack);
                USART3_RX_STA = 0;
                // ⚠️ 应在此处 break 并返回 0，但未实现！
            } 
        }
        if (waittime == 0) res = 1; // 超时
    }
    return res;
}


/**
 * @brief 向 ESP8266 发送数据（AT 指令或透传数据）
 * @param cmd 数据内容
 * @param waittime 等待接收的时间（单位：ms）
 * @retval 指向接收缓冲区的指针（可能为无效数据）
 *
 * @note
 *          - 自动添加 "\r\n"（通过 \c u3_printf）
 *          - 若在透传模式下，此函数不应被调用（应直接写 USART3）
 */
uint8_t* esp8266_send_data(uint8_t *cmd, u16 waittime)
{
    char temp[5];
    char *ack = temp;
    USART3_RX_STA = 0;
    u3_printf("%s\r\n", cmd);   // 发送命令 + \r\n

    if (waittime)
    {
        while (--waittime)
        {
            delay_ms(1);
            if (USART3_RX_STA & 0X8000)
            {
                USART3_RX_BUF[USART3_RX_STA & 0X7FFF] = 0;
                ack = (char*)USART3_RX_BUF;
                printf("ack:%s\r\n", (uint8_t*)ack);
                break;
            } 
        }
    }
    return (uint8_t*)ack;
} 