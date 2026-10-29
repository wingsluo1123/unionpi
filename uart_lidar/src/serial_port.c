/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-11-01
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-11-01
 * @FilePath: /OpenHarmony/unionpi_whale_demo/uart_lidar/src/serial_port.c
 * @Description: 串口通信模块实现 (基于uart_base的C语言封装)
 */

#include "../include/serial_port.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * @brief 获取当前时间戳
 * @return struct timespec 当前时间
 */
static struct timespec timespec_now(void)
{
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return time;
}

/**
 * @brief 毫秒转timespec
 * @param millis 毫秒数
 * @return struct timespec 时间结构体
 */
static struct timespec timespec_from_ms(uint32_t millis)
{
    struct timespec time;
    time.tv_sec = millis / 1000;
    time.tv_nsec = (millis - (time.tv_sec * 1000)) * 1000000;
    return time;
}

/**
 * @brief 创建毫秒定时器
 * @param timer 定时器指针
 * @param millis 毫秒数
 */
void millisecond_timer_create(MillisecondTimer *timer, uint64_t millis)
{
    timer->expiry = timespec_now();
    int64_t tv_nsec = timer->expiry.tv_nsec + (millis * 1000000);

    if (tv_nsec >= 1000000000)
    {
        int64_t sec_diff = tv_nsec / 1000000000;
        timer->expiry.tv_nsec = tv_nsec % 1000000000;
        timer->expiry.tv_sec += sec_diff;
    }
    else
    {
        timer->expiry.tv_nsec = tv_nsec;
    }
}

/**
 * @brief 获取定时器剩余时间
 * @param timer 定时器指针
 * @return int64_t 剩余毫秒数
 */
int64_t millisecond_timer_remaining(MillisecondTimer *timer)
{
    struct timespec now = timespec_now();
    int64_t millis = (timer->expiry.tv_sec - now.tv_sec) * 1000;
    millis += (timer->expiry.tv_nsec - now.tv_nsec) / 1000000;
    return millis;
}

/**
 * @brief 创建串口对象
 * @param port 串口设备路径
 * @param baudrate 波特率
 * @param timeout 超时配置
 * @return Serial_Port* 串口对象指针
 */
Serial_Port *serial_port_create(const char *port, uint32_t baudrate, Timeout timeout)
{
    Serial_Port *serial = (Serial_Port *)malloc(sizeof(Serial_Port));
    if (serial == NULL)
    {
        printf("[%s] Failed to allocate serial port\n", __func__);
        return NULL;
    }

    memset(serial, 0, sizeof(Serial_Port));
    strncpy(serial->port, port, sizeof(serial->port) - 1);
    serial->baudrate = baudrate;
    serial->fd = -1;
    serial->is_open = 0;
    serial->timeout = timeout;
    serial->databits = 8;
    serial->stopbits = 1;
    serial->parity = 'N';
    serial->flow_control = SERIAL_FLOW_NONE;
    serial->byte_time_ns = 0;

    return serial;
}

/**
 * @brief 销毁串口对象
 * @param serial 串口对象指针
 */
void serial_port_destroy(Serial_Port *serial)
{
    if (serial != NULL)
    {
        if (serial->is_open)
        {
            serial_port_close(serial);
        }
        free(serial);
    }
}

/**
 * @brief 打开串口
 * @param serial 串口对象指针
 * @return 1成功, 0失败
 */
int serial_port_open(Serial_Port *serial)
{
    if (serial == NULL)
    {
        return 0;
    }

    if (serial->is_open)
    {
        return 1;
    }

    /* 使用uart_base打开串口 */
    serial->fd = UartOpen(serial->port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (serial->fd == UART_ERR)
    {
        printf("[%s] Failed to open %s: %s\n", __func__, serial->port, strerror(errno));
        return 0;
    }

    /* 配置串口参数 */
    struct SerialPortConfig config = {.baudrate = serial->baudrate, .databits = serial->databits, .stopbits = serial->stopbits, .parity = serial->parity, .vtime = 0, .vmin = 0, .flow_control = serial->flow_control, .raw_mode = 1};

    if (UartInit(serial->fd, &config) != UART_OK)
    {
        printf("[%s] Failed to initialize UART\n", __func__);
        close(serial->fd);
        serial->fd = -1;
        return 0;
    }

    /* 计算字节传输时间 */
    uint32_t bit_time_ns = 1000000000 / serial->baudrate;
    serial->byte_time_ns = bit_time_ns * (1 + serial->databits + (serial->parity != 'N' ? 1 : 0) + serial->stopbits);

    serial->is_open = 1;
    printf("[%s] Serial port %s opened successfully (fd=%d)\n", __func__, serial->port, serial->fd);
    return 1;
}

/**
 * @brief 关闭串口
 * @param serial 串口对象指针
 */
void serial_port_close(Serial_Port *serial)
{
    if (serial != NULL && serial->is_open)
    {
        UartClose(serial->fd);
        serial->fd = -1;
        serial->is_open = 0;
        printf("[%s] Serial port closed\n", __func__);
    }
}

/**
 * @brief 等待可读状态
 * @param serial 串口对象指针
 * @param timeout_t 超时时间(毫秒)
 * @return 1可读, 0不可读
 */
static int wait_readable(Serial_Port *serial, uint32_t timeout_t)
{
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(serial->fd, &readfds);

    struct timespec timeout_ts = timespec_from_ms(timeout_t);
    int r = pselect(serial->fd + 1, &readfds, NULL, NULL, &timeout_ts, NULL);

    if (r < 0)
    {
        if (errno == EINTR)
        {
            return 0;
        }
        return 0;
    }

    if (r == 0)
    {
        return 0;
    }

    if (!FD_ISSET(serial->fd, &readfds))
    {
        return 0;
    }

    return 1;
}

/**
 * @brief 等待串口有足够的数据可读
 * @param serial 串口对象指针
 * @param data_count 需要的数据字节数
 * @param timeout 超时时间(毫秒)
 * @param returned_size 实际可读的字节数
 * @return result_t 结果状态
 */
result_t serial_port_wait_for_data(Serial_Port *serial, size_t data_count, uint64_t timeout, size_t *returned_size)
{
    if (!serial || !serial->is_open)
    {
        return RESULT_FAIL;
    }

    size_t length = 0;
    if (returned_size == NULL)
    {
        returned_size = &length;
    }
    *returned_size = 0;

    /* 检查缓冲区是否已有数据 */
    int bytes_available = 0;
    int ioctl_ret = ioctl(serial->fd, FIONREAD, &bytes_available);

    if (ioctl_ret == 0 && bytes_available > 0)
    {
        *returned_size = (size_t)bytes_available;
        return RESULT_OK;
    }

    fd_set readfds;
    MillisecondTimer total_timeout;
    millisecond_timer_create(&total_timeout, timeout);

    while (serial->is_open)
    {
        int64_t timeout_remaining_ms = millisecond_timer_remaining(&total_timeout);

        if (timeout_remaining_ms <= 0)
        {
            return RESULT_TIMEOUT;
        }

        FD_ZERO(&readfds);
        FD_SET(serial->fd, &readfds);

        struct timespec timeout_val = timespec_from_ms(timeout_remaining_ms);
        int n = pselect(serial->fd + 1, &readfds, NULL, NULL, &timeout_val, NULL);

        if (n < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return RESULT_FAIL;
        }

        if (n == 0)
        {
            return RESULT_TIMEOUT;
        }

        /* 有数据可读，检查数据量 */
        bytes_available = 0;
        ioctl_ret = ioctl(serial->fd, FIONREAD, &bytes_available);

        if (ioctl_ret == 0 && bytes_available > 0)
        {
            *returned_size = (size_t)bytes_available;
            return RESULT_OK;
        }
        else
        {
            /* ioctl失败，但select表明有数据 */
            *returned_size = 1;
            return RESULT_OK;
        }
    }

    return RESULT_TIMEOUT;
}

/**
 * @brief 从串口读取数据
 * @param serial 串口对象指针
 * @param buf 接收缓冲区
 * @param size 要读取的字节数
 * @return 实际读取的字节数
 */
result_t serial_port_read(Serial_Port *serial, uint8_t *buf, size_t size)
{
    if (!serial || !serial->is_open || buf == NULL || size == 0)
    {
        return 0;
    }

    static int read_count = 0;
    read_count++;

    size_t bytes_read = 0;
    long total_timeout_ms = serial->timeout.read_timeout_constant;
    total_timeout_ms += serial->timeout.read_timeout_multiplier * (long)size;

    MillisecondTimer total_timeout;
    millisecond_timer_create(&total_timeout, total_timeout_ms);

    /* 预填充缓冲区 */
    ssize_t bytes_read_now = read(serial->fd, buf, size);
    if (bytes_read_now > 0)
    {
        bytes_read = bytes_read_now;

        if (read_count <= 5)
        {
            printf("[%s] Call #%d: Read %zd bytes: ", __func__, read_count, bytes_read_now);
            for (ssize_t i = 0; i < (bytes_read_now < 16 ? bytes_read_now : 16); i++)
            {
                printf("0x%02X ", buf[i]);
            }
            printf("\n");
        }
    }

    while (bytes_read < size)
    {
        int64_t timeout_remaining_ms = millisecond_timer_remaining(&total_timeout);
        if (timeout_remaining_ms <= 0)
        {
            break;
        }

        uint32_t timeout_t = (timeout_remaining_ms < serial->timeout.inter_byte_timeout) ? timeout_remaining_ms : serial->timeout.inter_byte_timeout;

        if (wait_readable(serial, timeout_t))
        {
            bytes_read_now = read(serial->fd, buf + bytes_read, size - bytes_read);
            if (bytes_read_now < 1)
            {
                continue;
            }

            bytes_read += (size_t)bytes_read_now;
            if (bytes_read == size)
            {
                break;
            }
        }
    }

    return bytes_read;
}

/**
 * @brief 向串口写入数据
 * @param serial 串口对象指针
 * @param data 要写入的数据
 * @param length 数据长度
 * @return 实际写入的字节数
 */
size_t serial_port_write(Serial_Port *serial, const uint8_t *data, size_t length)
{
    if (!serial || !serial->is_open || data == NULL || length == 0)
    {
        return 0;
    }

    fd_set writefds;
    size_t bytes_written = 0;
    long total_timeout_ms = serial->timeout.write_timeout_constant;
    total_timeout_ms += serial->timeout.write_timeout_multiplier * (long)length;

    MillisecondTimer total_timeout;
    millisecond_timer_create(&total_timeout, total_timeout_ms);

    int first_iteration = 1;

    while (bytes_written < length)
    {
        int64_t timeout_remaining_ms = millisecond_timer_remaining(&total_timeout);

        if (!first_iteration && (timeout_remaining_ms <= 0))
        {
            break;
        }
        first_iteration = 0;

        struct timespec timeout = timespec_from_ms(timeout_remaining_ms);
        FD_ZERO(&writefds);
        FD_SET(serial->fd, &writefds);

        int r = pselect(serial->fd + 1, NULL, &writefds, NULL, &timeout, NULL);

        if (r < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            continue;
        }

        if (r == 0)
        {
            break;
        }

        if (r > 0 && FD_ISSET(serial->fd, &writefds))
        {
            ssize_t bytes_written_now = write(serial->fd, data + bytes_written, length - bytes_written);
            if (bytes_written_now < 1)
            {
                continue;
            }

            bytes_written += (size_t)bytes_written_now;
            if (bytes_written == length)
            {
                break;
            }
        }
    }

    return bytes_written;
}

/**
 * @brief 检查串口缓冲区可读字节数
 * @param serial 串口对象指针
 * @return 可读字节数
 */
size_t serial_port_available(Serial_Port *serial)
{
    if (!serial || !serial->is_open)
    {
        return 0;
    }

    int count = 0;
    if (ioctl(serial->fd, TIOCINQ, &count) == -1)
    {
        return 0;
    }

    return (size_t)count;
}

/**
 * @brief 设置DTR信号
 * @param serial 串口对象指针
 * @param level DTR电平
 * @return 1成功, 0失败
 */
int serial_port_set_dtr(Serial_Port *serial, int level)
{
    if (!serial || !serial->is_open)
    {
        return 0;
    }

    int command = TIOCM_DTR;
    if (level)
    {
        if (ioctl(serial->fd, TIOCMBIS, &command) == -1)
        {
            return 0;
        }
    }
    else
    {
        if (ioctl(serial->fd, TIOCMBIC, &command) == -1)
        {
            return 0;
        }
    }

    return 1;
}

/**
 * @brief 获取字节传输时间
 * @param serial 串口对象指针
 * @return 字节传输时间(纳秒)
 */
uint32_t serial_port_get_byte_time(Serial_Port *serial)
{
    if (serial)
    {
        return serial->byte_time_ns;
    }
    return 0;
}