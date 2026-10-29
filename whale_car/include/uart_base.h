/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-30 18:47:19
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-30 18:47:58
 * @FilePath: /OpenHarmony/unionpi_whale_demo/uart_trans/include/uart_base.h
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

    speed_t UartConvertBaud(int baudrate);

    int UartConfigure(int fd, const struct SerialPortConfig *config);

    int UartInit(int fd, const struct SerialPortConfig *config);

    int UartOpen(const char *device, int flags);

    void UartClose(int fd);

    ssize_t UartRead(int fd, void *buffer, size_t length);

    ssize_t UartWrite(int fd, const void *buffer, size_t length);

#ifdef __cplusplus
}
#endif

#endif