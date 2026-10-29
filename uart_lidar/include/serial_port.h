/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-11-01
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-11-01 16:21:50
 * @FilePath: /OpenHarmony/unionpi_whale_demo/uart_lidar/include/serial_port.h
 * @Description: 串口通信模块头文件 (基于uart_base的C语言封装)
 */

#ifndef SERIAL_PORT_H
#define SERIAL_PORT_H

#include "lidar_information.h"
#include "mtime.h"
#include "uart_base.h"
#include <stdint.h>
#include <sys/ioctl.h> 
#include <sys/select.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* 超时配置结构 */
    typedef struct
    {
        uint32_t inter_byte_timeout;
        uint32_t read_timeout_constant;
        uint32_t read_timeout_multiplier;
        uint32_t write_timeout_constant;
        uint32_t write_timeout_multiplier;
    } Timeout;

    /* 毫秒定时器结构 */
    typedef struct
    {
        struct timespec expiry;
    } MillisecondTimer;

    /* 串口结构 */
    typedef struct
    {
        char port[256];         /* 串口设备路径 */
        unsigned long baudrate; /* 波特率 */
        int fd;                 /* 文件描述符 */
        int is_open;            /* 是否已打开 */
        uint32_t byte_time_ns;  /* 传输一个字节所需的纳秒数 */
        Timeout timeout;        /* 超时配置 */
        int databits;           /* 数据位 */
        int stopbits;           /* 停止位 */
        char parity;            /* 校验位 */
        int flow_control;       /* 流控模式 */
    } Serial_Port;

    /**
     * @brief 创建简单超时配置
     * @param timeout_t 超时时间(毫秒)
     * @return Timeout 超时配置
     */
    static inline Timeout timeout_simple(uint32_t timeout_t)
    {
        Timeout t;
        t.inter_byte_timeout = 0xFFFFFFFF;
        t.read_timeout_constant = timeout_t;
        t.read_timeout_multiplier = 0;
        t.write_timeout_constant = timeout_t;
        t.write_timeout_multiplier = 0;
        return t;
    }

    /**
     * @brief 创建毫秒定时器
     * @param timer 定时器指针
     * @param millis 毫秒数
     */
    void millisecond_timer_create(MillisecondTimer *timer, uint64_t millis);

    /**
     * @brief 获取定时器剩余时间
     * @param timer 定时器指针
     * @return 剩余毫秒数
     */
    int64_t millisecond_timer_remaining(MillisecondTimer *timer);

    /**
     * @brief 创建串口对象
     * @param port 串口设备路径
     * @param baudrate 波特率
     * @param timeout 超时配置
     * @return Serial_Port* 串口对象指针
     */
    Serial_Port *serial_port_create(const char *port, uint32_t baudrate, Timeout timeout);

    /**
     * @brief 销毁串口对象
     * @param serial 串口对象指针
     */
    void serial_port_destroy(Serial_Port *serial);

    /**
     * @brief 打开串口
     * @param serial 串口对象指针
     * @return 1成功, 0失败
     */
    int serial_port_open(Serial_Port *serial);

    /**
     * @brief 关闭串口
     * @param serial 串口对象指针
     */
    void serial_port_close(Serial_Port *serial);

    /**
     * @brief 从串口读取数据
     * @param serial 串口对象指针
     * @param buf 接收缓冲区
     * @param size 要读取的字节数
     * @return 实际读取的字节数
     */
    result_t serial_port_read(Serial_Port *serial, uint8_t *buf, size_t size);

    /**
     * @brief 向串口写入数据
     * @param serial 串口对象指针
     * @param data 要写入的数据
     * @param length 数据长度
     * @return 实际写入的字节数
     */
    size_t serial_port_write(Serial_Port *serial, const uint8_t *data, size_t length);

    /**
     * @brief 等待串口有足够的数据可读
     * @param serial 串口对象指针
     * @param data_count 需要的数据字节数
     * @param timeout 超时时间(毫秒)
     * @param returned_size 实际可读的字节数
     * @return result_t 结果状态
     */
    result_t serial_port_wait_for_data(Serial_Port *serial, size_t data_count, uint64_t timeout, size_t *returned_size);

    /**
     * @brief 检查串口缓冲区可读字节数
     * @param serial 串口对象指针
     * @return 可读字节数
     */
    size_t serial_port_available(Serial_Port *serial);

    /**
     * @brief 设置DTR信号
     * @param serial 串口对象指针
     * @param level DTR电平
     * @return 1成功, 0失败
     */
    int serial_port_set_dtr(Serial_Port *serial, int level);

    /**
     * @brief 获取字节传输时间
     * @param serial 串口对象指针
     * @return 字节传输时间(纳秒)
     */
    uint32_t serial_port_get_byte_time(Serial_Port *serial);

#ifdef __cplusplus
}
#endif

#endif /* SERIAL_PORT_H */
