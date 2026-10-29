/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-30 18:47:19
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-30 22:49:45
 * @FilePath: /OpenHarmony/unionpi_whale_demo/uart_ultra/include/uart_base.h
 * @Description: 通用串口工具接口
 */

#ifndef __UART_BASE_H__
#define __UART_BASE_H__

#include <sys/types.h>
#include <termios.h>

// 通用返回码定义
#define UART_OK 0
#define UART_ERR (-1)

// 串口流控模式枚举
enum SerialFlowControl
{
    SERIAL_FLOW_NONE = 0,
    SERIAL_FLOW_HARDWARE = 1,
};

// 串口配置结构体
struct SerialPortConfig
{
    int baudrate;     /* 目标波特率 */
    int databits;     /* 数据位 */
    int stopbits;     /* 停止位 */
    char parity;      /* 奇偶校验 */
    int vtime;        /* 读取超时 (deciseconds) */
    int vmin;         /* 最小读取字节数 */
    int flow_control; /* 流控模式，参见 SerialFlowControl */
    int raw_mode;     /* 是否使用原始模式 */
};

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief 转换波特率为系统定义的速度值
     * @param baudrate 波特率
     * @return 成功返回对应的speed_t值，失败返回B115200
     */
    speed_t UartConvertBaud(int baudrate);

    /**
     * @brief 配置串口参数
     * @param fd 串口文件描述符
     * @param config 串口配置结构体指针
     * @return 成功返回UART_OK，失败返回UART_ERR
     */
    int UartConfigure(int fd, const struct SerialPortConfig *config);

    /**
     * @brief 初始化串口（配置串口参数的别名函数）
     * @param fd 串口文件描述符
     * @param config 串口配置结构体指针
     * @return 成功返回UART_OK，失败返回UART_ERR
     */
    int UartInit(int fd, const struct SerialPortConfig *config);

    /**
     * @brief 打开串口设备
     * @param device 串口设备路径
     * @param flags 打开标志
     * @return 成功返回文件描述符，失败返回UART_ERR
     */
    int UartOpen(const char *device, int flags);

    /**
     * @brief 关闭串口设备
     * @param fd 串口文件描述符
     */
    void UartClose(int fd);

    /**
     * @brief 从串口读取数据
     * @param fd 串口文件描述符
     * @param buffer 接收缓冲区
     * @param length 要读取的字节数
     * @return 成功返回实际读取的字节数，失败返回-1
     */
    ssize_t UartRead(int fd, void *buffer, size_t length);

    /**
     * @brief 向串口写入数据
     * @param fd 串口文件描述符
     * @param buffer 发送缓冲区
     * @param length 要写入的字节数
     * @return 成功返回实际写入的字节数，失败返回-1
     */
    ssize_t UartWrite(int fd, const void *buffer, size_t length);

#ifdef __cplusplus
}
#endif

#endif