/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-30 20:30:00
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-30 22:49:29
 * @FilePath: /OpenHarmony/unionpi_whale_demo/uart_ultra/src/main.c
 * @Description: 超声波串口读取程序实现文件
 */

#include "../include/main.h"

// UART设备文件描述符
static int fd;

/**
 * @brief 打印使用帮助信息
 */
void print_usage(const char *program_name)
{
    printf("[%s] Usage: %s --uart-port <device_path>\n", __func__, program_name);
    printf("[%s] Example: %s --uart-port /dev/ttyUSB0\n", __func__, program_name);
    printf("[%s] Example: %s --uart-port /dev/ttyS0\n", __func__, program_name);
}

/**
 * @brief 解析命令行参数
 *
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 成功返回串口设备路径，失败返回NULL
 */
const char *parse_arguments(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("[%s] Error: Invalid number of arguments\n", __func__);
        print_usage(argv[0]);
        return NULL;
    }

    if (strcmp(argv[1], "--uart-port") != 0)
    {
        printf("[%s] Error: Invalid argument '%s'\n", __func__, argv[1]);
        print_usage(argv[0]);
        return NULL;
    }

    const char *uart_port = argv[2];
    if (uart_port == NULL || strlen(uart_port) == 0)
    {
        printf("[%s] Error: Invalid UART port specified\n", __func__);
        print_usage(argv[0]);
        return NULL;
    }

    return uart_port;
}

/**
 * @brief 解析超声波数据，提取距离数值
 *
 * @param data 接收到的字符串数据，例如 "183mm"
 * @return 成功返回距离数值（毫米），失败返回-1
 */
int parse_ultrasonic_data(const char *data)
{
    if (data == NULL || strlen(data) == 0)
    {
        return -1;
    }

    // 查找数字部分
    int distance = -1;
    if (sscanf(data, "%dmm", &distance) == 1)
    {
        return distance;
    }

    // 如果没有匹配到 "mm"，尝试只解析数字
    if (sscanf(data, "%d", &distance) == 1)
    {
        return distance;
    }

    return -1;
}

/**
 * @brief 处理接收到的数据行
 *
 * @param buffer 数据缓冲区
 * @param length 数据长度
 */
void process_received_data(char *buffer, size_t length)
{
    // 确保字符串结束
    buffer[length] = '\0';

    // 按行处理数据
    char *line = strtok(buffer, "\r\n");
    while (line != NULL)
    {
        // 跳过空行
        if (strlen(line) > 0)
        {
            int distance = parse_ultrasonic_data(line);
            if (distance >= 0)
            {
                printf("[%s] Distance: %d mm (Raw: %s)\n", __func__, distance, line);
                fflush(stdout);
            }
            else
            {
                printf("[%s] Invalid data format: %s\n", __func__, line);
                fflush(stdout);
            }
        }
        line = strtok(NULL, "\r\n");
    }
}

/**
 * @brief 主函数入口
 *
 * 程序执行的起始点，初始化UART设备并持续读取超声波数据
 *
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组，必须包含--uart-port和串口设备路径
 * @return 返回程序执行状态码，正常退出返回0
 */
int main(int argc, char **argv)
{
    // 解析命令行参数
    const char *uart_dev = parse_arguments(argc, argv);
    if (uart_dev == NULL)
    {
        return UART_ERR;
    }

    printf("[%s] Ultrasonic Reader Starting...\n", __func__);
    printf("[%s] Using UART device: %s\n", __func__, uart_dev);

    // 打开串口设备
    fd = UartOpen(uart_dev, O_RDWR | O_NOCTTY);
    if (fd == UART_ERR)
    {
        printf("[%s] Failed to open UART device: %s\n", __func__, strerror(errno));
        return UART_ERR;
    }

    printf("[%s] UART device opened successfully, fd: %d\n", __func__, fd);

    struct SerialPortConfig config = {
        .baudrate = 115200,
        .databits = 8,
        .stopbits = 1,
        .parity = 'N',
        .vtime = 5, // 0.5秒超时
        .vmin = 0,  // 非阻塞读取
        .flow_control = SERIAL_FLOW_NONE,
        .raw_mode = 1 // 使用原始模式
    };

    int ret = UartInit(fd, &config);
    if (ret == UART_ERR)
    {
        printf("[%s] Failed to initialize UART: %s\n", __func__, strerror(errno));
        UartClose(fd);
        return UART_ERR;
    }

    printf("[%s] UART initialized successfully (9600 8N1)\n", __func__);
    printf("[%s] Starting ultrasonic data reading...\n", __func__);
    printf("[%s] Press Ctrl+C to exit\n", __func__);
    fflush(stdout);

    // 数据接收缓冲区
    char recv_buffer[256];
    char line_buffer[512] = {0}; // 用于拼接不完整的行

    // 主循环：持续读取超声波数据
    while (1)
    {
        memset(recv_buffer, 0, sizeof(recv_buffer));
        ssize_t bytes_read = UartRead(fd, recv_buffer, sizeof(recv_buffer) - 1);

        if (bytes_read > 0)
        {
            // 将新数据追加到行缓冲区
            strncat(line_buffer, recv_buffer, sizeof(line_buffer) - strlen(line_buffer) - 1);

            // 检查是否有完整的行（包含换行符）
            char *newline_pos = strchr(line_buffer, '\n');
            if (newline_pos != NULL)
            {
                // 处理完整的行
                *newline_pos = '\0'; // 临时终止字符串
                process_received_data(line_buffer, strlen(line_buffer));

                // 将剩余数据移到缓冲区开头
                memmove(line_buffer, newline_pos + 1, strlen(newline_pos + 1) + 1);
            }

            // 防止缓冲区溢出
            if (strlen(line_buffer) > sizeof(line_buffer) - 50)
            {
                printf("[%s] Warning: Line buffer nearly full, clearing\n", __func__);
                memset(line_buffer, 0, sizeof(line_buffer));
            }
        }
        else if (bytes_read == 0)
        {
            // 超时，继续读取
            usleep(50000); // 50ms延时
        }
        else
        {
            printf("[%s] Read error: %s\n", __func__, strerror(errno));
            break;
        }
    }

    // 清理资源
    UartClose(fd);
    printf("[%s] Ultrasonic reader stopped\n", __func__);
    return 0;
}