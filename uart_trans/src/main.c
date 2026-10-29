/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-30 18:50:17
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-30 19:46:02
 * @FilePath: /OpenHarmony/unionpi_whale_demo/uart_trans/src/main.c
 * @Description: UART 收发例子实现文件
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
 * @brief 串口接收数据线程函数
 *
 * 该线程持续监听串口数据，一旦接收到数据就打印到终端
 *
 * @return 返回线程执行状态
 */
void *_serial_input_task(void)
{
    unsigned char recv_buff[256];

    printf("[%s] UART Receiver Ready!\n", __func__);
    fflush(stdout);

    while (1)
    {
        memset(recv_buff, 0, sizeof(recv_buff));
        ssize_t ret = UartRead(fd, recv_buff, sizeof(recv_buff) - 1);
        if (ret > 0)
        {
            recv_buff[ret] = '\0'; // 确保字符串结束
            printf("[%s] Received: %s\n", __func__, recv_buff);
            fflush(stdout);
        }
        else if (ret == 0)
        {
            // 超时或没有数据，继续循环
            usleep(10000); // 10ms延时
        }
        else
        {
            printf("[%s] Read error: %s\n", __func__, strerror(errno));
            fflush(stdout);
            break;
        }
    }
    return NULL;
}

/**
 * @brief 主函数入口
 *
 * 程序执行的起始点，初始化UART设备并创建收发线程
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

    printf("[%s] Using UART device: %s\n", __func__, uart_dev);

    // 打开串口设备
    fd = UartOpen(uart_dev, O_RDWR | O_NOCTTY);
    if (fd == UART_ERR)
    {
        printf("[%s] Failed to open UART device: %s\n", __func__, strerror(errno));
        return UART_ERR;
    }

    printf("[%s] UART device opened successfully, fd: %d\n", __func__, fd);

    // 配置串口参数
    struct SerialPortConfig config = {
        .baudrate = 9600,
        .databits = 8,
        .stopbits = 1,
        .parity = 'N',
        .vtime = 1, // 0.1秒超时
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

    // 创建线程，执行读串口的操作
    pthread_t pid_t;
    ret = pthread_create(&pid_t, NULL, (void *)_serial_input_task, NULL);
    if (ret != 0)
    {
        printf("[%s] Failed to create receiver thread: %s\n", __func__, strerror(ret));
        UartClose(fd);
        return UART_ERR;
    }

    printf("[%s] Receiver thread created successfully\n", __func__);
    printf("[%s] UART Demo Start! Type messages and press Enter to send:\n", __func__);
    fflush(stdout);

    // 主线程处理发送
    char send_buf[256];
    while (1)
    {
        if (fgets(send_buf, sizeof(send_buf), stdin) != NULL)
        {
            size_t len = strlen(send_buf);
            // 发送到串口
            ssize_t bytes_written = UartWrite(fd, send_buf, len);
            if (bytes_written > 0)
            {
                printf("[%s] Sent %zd bytes: %s", __func__, bytes_written, send_buf);
                fflush(stdout);
            }
            else
            {
                printf("[%s] Failed to send data: %s\n", __func__, strerror(errno));
                fflush(stdout);
            }
        }
    }

    // 清理资源
    UartClose(fd);
    return 0;
}