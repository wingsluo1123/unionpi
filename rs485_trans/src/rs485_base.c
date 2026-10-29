/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-31 14:15:00
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-11-11 19:39:17
 * @FilePath: /OpenHarmony/unionpi_whale_demo/rs485_trans/src/rs485_base.c
 * @Description: RS485基础驱动层实现
 */

#include "../include/rs485_base.h"
#include "../include/gpio_base.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

/**
 * @brief 解析波特率字符串到速度常量
 * @param baud_str 波特率字符串
 * @return speed_t 波特率常量
 */
speed_t Rs485ParseBaudRate(const char *baud_str)
{
    int baud_int = atoi(baud_str);
    switch (baud_int)
    {
    case 9600:
        return B9600;
    case 19200:
        return B19200;
    case 38400:
        return B38400;
    case 57600:
        return B57600;
    case 115200:
        return B115200;
    case 230400:
        return B230400;
    case 460800:
        return B460800;
    case 921600:
        return B921600;
    case 1500000:
        return B1500000;
    default:
        printf("[%s] Unsupported baud rate: %s, using 9600\n", __func__, baud_str);
        return B9600;
    }
}

/**
 * @brief 创建RS485句柄
 * @param config RS485配置
 * @return Rs485Handle* 成功返回句柄指针，失败返回NULL
 */
Rs485Handle *Rs485Create(const Rs485Config *config)
{
    if (config == NULL || config->device_path == NULL)
    {
        printf("[%s] Invalid configuration\n", __func__);
        return NULL;
    }

    Rs485Handle *handle = (Rs485Handle *)malloc(sizeof(Rs485Handle));
    if (handle == NULL)
    {
        printf("[%s] Failed to allocate memory for handle\n", __func__);
        return NULL;
    }

    // 复制配置
    handle->config = *config;
    handle->fd = -1;
    handle->is_open = 0;
    handle->gpio_initialized = 0;

    return handle;
}

/**
 * @brief 打开RS485设备
 * @param handle RS485句柄
 * @return int 成功返回0，失败返回-1
 */
int Rs485Open(Rs485Handle *handle)
{
    if (handle == NULL)
    {
        printf("[%s] Invalid handle\n", __func__);
        return -1;
    }

    if (handle->is_open)
    {
        return 0;
    }

    handle->fd = open(handle->config.device_path, O_RDWR | O_NOCTTY | O_NDELAY);
    if (handle->fd < 0)
    {
        printf("[%s] Failed to open device %s: %s\n", __func__, handle->config.device_path, strerror(errno));
        return -1;
    }

    handle->is_open = 1;
    return 0;
}

/**
 * @brief 配置RS485设备参数
 * @param handle RS485句柄
 * @return int 成功返回0，失败返回-1
 */
int Rs485Configure(Rs485Handle *handle)
{
    if (handle == NULL || !handle->is_open)
    {
        printf("[%s] Invalid handle or device not open\n", __func__);
        return -1;
    }

    struct termios options;
    if (tcgetattr(handle->fd, &options) != 0)
    {
        printf("[%s] Failed to get terminal attributes: %s\n", __func__, strerror(errno));
        return -1;
    }

    // 设置波特率
    cfsetispeed(&options, handle->config.baud_rate);
    cfsetospeed(&options, handle->config.baud_rate);

    // 设置串口参数
    options.c_cflag |= (CLOCAL | CREAD); // 忽略调制解调器线状态，启用接收
    options.c_cflag &= ~PARENB;          // 无奇偶校验
    options.c_cflag &= ~CSTOPB;          // 1个停止位
    options.c_cflag &= ~CSIZE;           // 清除数据位设置
    options.c_cflag |= CS8;              // 8个数据位

    // 设置输入模式
    options.c_iflag &= ~(IXON | IXOFF | IXANY);         // 禁用软件流控制
    options.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG); // 原始输入模式

    // 设置输出模式
    options.c_oflag &= ~OPOST; // 原始输出模式

    // 设置控制字符
    options.c_cc[VMIN] = 0;  // 非阻塞读取
    options.c_cc[VTIME] = 1; // 超时时间0.1秒

    if (tcsetattr(handle->fd, TCSANOW, &options) != 0)
    {
        printf("[%s] Failed to set terminal attributes: %s\n", __func__, strerror(errno));
        return -1;
    }

    return 0;
}

/**
 * @brief 初始化RS485 DE/RE控制GPIO
 * @param handle RS485句柄
 * @return int 成功返回0，失败返回-1
 */
int Rs485InitGpio(Rs485Handle *handle)
{
    if (handle == NULL)
    {
        printf("[%s] Invalid handle\n", __func__);
        return -1;
    }

    if (handle->gpio_initialized)
    {
        return 0;
    }

    // 检查GPIO是否已导出
    int export_status = 0;
    int ret = GPIO_IsExport(RS485_CTRL_GPIO, &export_status);
    
    if (ret == 0 && export_status == GPIO_NOT_EXPORT)
    {
        // GPIO未导出，需要导出
        ret = GPIO_Export(RS485_CTRL_GPIO, GPIO_EXPORTED);
        if (ret != 0)
        {
            printf("[%s] Failed to export GPIO %d\n", __func__, RS485_CTRL_GPIO);
            printf("[%s] Please check if you have permission to access /sys/class/gpio/\n", __func__);
            return -1;
        }
        
        // 等待系统准备好GPIO文件
        usleep(200000); // 等待200ms
    }

    // 设置GPIO为输出模式
    if (GPIO_SetDirection(RS485_CTRL_GPIO, GPIO_DIRECTION_OUT) != 0)
    {
        printf("[%s] Failed to set GPIO %d direction to output\n", __func__, RS485_CTRL_GPIO);
        // 尝试取消导出后重试
        GPIO_Export(RS485_CTRL_GPIO, GPIO_NOT_EXPORT);
        usleep(100000);
        if (GPIO_Export(RS485_CTRL_GPIO, GPIO_EXPORTED) == 0)
        {
            usleep(200000);
            if (GPIO_SetDirection(RS485_CTRL_GPIO, GPIO_DIRECTION_OUT) != 0)
            {
                printf("[%s] Retry failed\n", __func__);
                return -1;
            }
        }
        else
        {
            return -1;
        } 
    }

    // 默认设置为接收模式（低电平）
    if (GPIO_SetValue(RS485_CTRL_GPIO, GPIO_LOW_LEVE) != 0)
    {
        printf("[%s] Failed to set GPIO %d to low level\n", __func__, RS485_CTRL_GPIO);
        return -1;
    }

    handle->gpio_initialized = 1;
    return 0;
}

/**
 * @brief 设置RS485工作模式（收发切换）
 * @param handle RS485句柄
 * @param mode 工作模式，RS485_MODE_RECV为接收，RS485_MODE_SEND为发送
 * @return int 成功返回0，失败返回-1
 */
int Rs485SetMode(Rs485Handle *handle, int mode)
{
    if (handle == NULL)
    {
        printf("[%s] Invalid handle\n", __func__);
        return -1;
    }

    if (!handle->gpio_initialized)
    {
        printf("[%s] GPIO not initialized\n", __func__);
        return -1;
    }

    if (mode == RS485_MODE_SEND)
    {
        // 发送模式：设置GPIO为高电平
        if (GPIO_SetValue(RS485_CTRL_GPIO, GPIO_HIGH_LEVE) != 0)
        {
            printf("[%s] Failed to switch to send mode\n", __func__);
            return -1;
        }
    }
    else if (mode == RS485_MODE_RECV)
    {
        // 接收模式：设置GPIO为低电平
        if (GPIO_SetValue(RS485_CTRL_GPIO, GPIO_LOW_LEVE) != 0)
        {
            printf("[%s] Failed to switch to receive mode\n", __func__);
            return -1;
        }
    }
    else
    {
        printf("[%s] Invalid mode: %d\n", __func__, mode);
        return -1;
    }

    return 0;
}

/**
 * @brief 发送数据
 * @param handle RS485句柄
 * @param data 要发送的数据
 * @param length 数据长度
 * @return int 成功返回实际发送字节数，失败返回-1
 */
int Rs485Send(Rs485Handle *handle, const char *data, size_t length)
{
    if (handle == NULL || !handle->is_open || data == NULL)
    {
        printf("[%s] Invalid parameters\n", __func__);
        return -1;
    }

    // 切换到发送模式
    if (Rs485SetMode(handle, RS485_MODE_SEND) != 0)
    {
        printf("[%s] Failed to switch to send mode\n", __func__);
        return -1;
    }

    // 短暂延迟，确保模式切换完成
    usleep(1000); // 1ms

    ssize_t bytes_written = write(handle->fd, data, length);
    if (bytes_written < 0)
    {
        printf("[%s] Failed to write to RS485 port: %s\n", __func__, strerror(errno));
        // 切换回接收模式
        Rs485SetMode(handle, RS485_MODE_RECV);
        return -1;
    }

    // 等待数据发送完成
    tcdrain(handle->fd);
    
    // 短暂延迟，确保数据发送完成
    usleep(1000); // 1ms

    // 切换回接收模式
    Rs485SetMode(handle, RS485_MODE_RECV);

    return (int)bytes_written;
}

/**
 * @brief 接收数据
 * @param handle RS485句柄
 * @param buffer 接收缓冲区
 * @param buffer_size 缓冲区大小
 * @param timeout_ms 超时时间（毫秒）
 * @return int 成功返回实际接收字节数，失败返回-1
 */
int Rs485Receive(Rs485Handle *handle, char *buffer, size_t buffer_size, int timeout_ms)
{
    if (handle == NULL || !handle->is_open || buffer == NULL)
    {
        printf("[%s] Invalid parameters\n", __func__);
        return -1;
    }

    // 确保处于接收模式
    if (Rs485SetMode(handle, RS485_MODE_RECV) != 0)
    {
        printf("[%s] Failed to switch to receive mode\n", __func__);
        return -1;
    }

    fd_set read_fds;
    struct timeval timeout;

    FD_ZERO(&read_fds);
    FD_SET(handle->fd, &read_fds);

    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    int select_result = select(handle->fd + 1, &read_fds, NULL, NULL, &timeout);
    if (select_result < 0)
    {
        printf("[%s] Select failed: %s\n", __func__, strerror(errno));
        return -1;
    }
    else if (select_result == 0)
    {
        // 超时不打印日志，避免刷屏
        return 0; // 超时
    }

    ssize_t bytes_read = read(handle->fd, buffer, buffer_size - 1);
    if (bytes_read < 0)
    {
        printf("[%s] Failed to read from RS485 port: %s\n", __func__, strerror(errno));
        return -1;
    }

    buffer[bytes_read] = '\0'; // 添加字符串结束符
    return (int)bytes_read;
}

/**
 * @brief 关闭RS485设备
 * @param handle RS485句柄
 * @return int 成功返回0，失败返回-1
 */
int Rs485Close(Rs485Handle *handle)
{
    if (handle == NULL)
    {
        printf("[%s] Invalid handle\n", __func__);
        return -1;
    }

    if (!handle->is_open)
    {
        printf("[%s] Device already closed\n", __func__);
        return 0;
    }

    printf("[%s] Closing RS485 device\n", __func__);

    // 取消导出GPIO
    if (handle->gpio_initialized)
    {
        printf("[%s] Unexporting GPIO %d\n", __func__, RS485_CTRL_GPIO);
        GPIO_Export(RS485_CTRL_GPIO, GPIO_NOT_EXPORT);
        handle->gpio_initialized = 0;
    }

    if (close(handle->fd) != 0)
    {
        printf("[%s] Failed to close device: %s\n", __func__, strerror(errno));
        return -1;
    }

    handle->fd = -1;
    handle->is_open = 0;

    printf("[%s] Device closed successfully\n", __func__);
    return 0;
}

/**
 * @brief 销毁RS485句柄
 * @param handle RS485句柄
 */
void Rs485Destroy(Rs485Handle *handle)
{
    if (handle == NULL)
    {
        printf("[%s] Invalid handle\n", __func__);
        return;
    }

    printf("[%s] Destroying RS485 handle\n", __func__);

    if (handle->is_open)
    {
        Rs485Close(handle);
    }

    free(handle);
    printf("[%s] Handle destroyed\n", __func__);
}

/**
 * @brief 获取设备状态
 * @param handle RS485句柄
 * @return int 设备打开返回1，关闭返回0，错误返回-1
 */
int Rs485GetStatus(Rs485Handle *handle)
{
    if (handle == NULL)
    {
        printf("[%s] Invalid handle\n", __func__);
        return -1;
    }

    return handle->is_open ? 1 : 0;
}

/**
 * @brief 刷新输出缓冲区
 * @param handle RS485句柄
 * @return int 成功返回0，失败返回-1
 */
int Rs485Flush(Rs485Handle *handle)
{
    if (handle == NULL || !handle->is_open)
    {
        printf("[%s] Invalid handle or device not open\n", __func__);
        return -1;
    }

    printf("[%s] Flushing output buffer\n", __func__);

    if (tcdrain(handle->fd) != 0)
    {
        printf("[%s] Failed to flush output: %s\n", __func__, strerror(errno));
        return -1;
    }

    printf("[%s] Output buffer flushed\n", __func__);
    return 0;
}