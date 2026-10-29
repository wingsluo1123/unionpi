/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-31 14:15:00
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-11-11 19:38:08
 * @FilePath: /OpenHarmony/unionpi_whale_demo/rs485_trans/include/rs485_base.h
 * @Description: RS485基础驱动层接口定义
 */

#ifndef RS485_BASE_H
#define RS485_BASE_H
 
#include <stddef.h>
#include <termios.h>

// RS485 DE/RE控制GPIO引脚
#define RS485_CTRL_GPIO 33

// RS485工作模式定义
#define RS485_MODE_RECV 0 // 接收模式
#define RS485_MODE_SEND 1 // 发送模式

// RS485驱动配置结构体
typedef struct
{
    const char *device_path; // 设备路径
    speed_t baud_rate;       // 波特率
    int data_bits;           // 数据位
    int stop_bits;           // 停止位
    int parity;              // 奇偶校验
} Rs485Config;

// RS485设备句柄
typedef struct
{
    int fd;             // 文件描述符
    Rs485Config config; // 配置信息
    int is_open;        // 是否已打开
    int gpio_initialized; // GPIO是否已初始化
} Rs485Handle;

// 函数声明
/**
 * @brief 解析波特率字符串到速度常量
 * @param baud_str 波特率字符串
 * @return speed_t 波特率常量
 */
speed_t Rs485ParseBaudRate(const char *baud_str);

/**
 * @brief 创建RS485句柄
 * @param config RS485配置
 * @return Rs485Handle* 成功返回句柄指针，失败返回NULL
 */
Rs485Handle *Rs485Create(const Rs485Config *config);

/**
 * @brief 打开RS485设备
 * @param handle RS485句柄
 * @return int 成功返回0，失败返回-1
 */
int Rs485Open(Rs485Handle *handle);

/**
 * @brief 配置RS485设备参数
 * @param handle RS485句柄
 * @return int 成功返回0，失败返回-1
 */
int Rs485Configure(Rs485Handle *handle);

/**
 * @brief 初始化RS485 DE/RE控制GPIO
 * @param handle RS485句柄
 * @return int 成功返回0，失败返回-1
 */
int Rs485InitGpio(Rs485Handle *handle);

/**
 * @brief 设置RS485工作模式（收发切换）
 * @param handle RS485句柄
 * @param mode 工作模式，RS485_MODE_RECV为接收，RS485_MODE_SEND为发送
 * @return int 成功返回0，失败返回-1
 */
int Rs485SetMode(Rs485Handle *handle, int mode);

/**
 * @brief 发送数据
 * @param handle RS485句柄
 * @param data 要发送的数据
 * @param length 数据长度
 * @return int 成功返回实际发送字节数，失败返回-1
 */
int Rs485Send(Rs485Handle *handle, const char *data, size_t length);

/**
 * @brief 接收数据
 * @param handle RS485句柄
 * @param buffer 接收缓冲区
 * @param buffer_size 缓冲区大小
 * @param timeout_ms 超时时间（毫秒）
 * @return int 成功返回实际接收字节数，失败返回-1
 */
int Rs485Receive(Rs485Handle *handle, char *buffer, size_t buffer_size, int timeout_ms);

/**
 * @brief 关闭RS485设备
 * @param handle RS485句柄
 * @return int 成功返回0，失败返回-1
 */
int Rs485Close(Rs485Handle *handle);

/**
 * @brief 销毁RS485句柄
 * @param handle RS485句柄
 */
void Rs485Destroy(Rs485Handle *handle);

/**
 * @brief 获取设备状态
 * @param handle RS485句柄
 * @return int 设备打开返回1，关闭返回0，错误返回-1
 */
int Rs485GetStatus(Rs485Handle *handle);

/**
 * @brief 刷新输出缓冲区
 * @param handle RS485句柄
 * @return int 成功返回0，失败返回-1
 */
int Rs485Flush(Rs485Handle *handle);

#endif // RS485_BASE_H