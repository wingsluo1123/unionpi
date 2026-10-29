/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-30 20:03:51
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-11-11 19:39:11
 * @FilePath: /OpenHarmony/unionpi_whale_demo/rs485_trans/src/main.c
 * @Description: RS485交互式收发程序
 */

#include "../include/main.h"

// 全局变量
volatile int g_running = 1;
Rs485Handle *g_handle = NULL;

/**
 * @brief 信号处理函数
 * @param sig 信号值
 */
void Rs485SignalHandler(int sig)
{
    printf("\n[%s] Received signal %d, shutting down...\n", __func__, sig);
    g_running = 0;
}

/**
 * @brief 打印使用说明
 * @param program_name 程序名
 */
void Rs485PrintUsage(const char *program_name)
{
    printf("[%s] Usage: %s --rs485-port <device> [--baud-rate <rate>]\n", __func__, program_name);
    printf("[%s] \n", __func__);
    printf("[%s] Example:\n", __func__);
    printf("[%s]   %s --rs485-port /dev/ttyS6 --baud-rate 115200\n", __func__, program_name);
    printf("[%s] \n", __func__);
    printf("[%s] Supported baud rates:\n", __func__);
    printf("[%s]   9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 1500000\n", __func__);
    printf("[%s]   Default: 9600\n", __func__);
}

/**
 * @brief 解析命令行参数
 * @param argc 参数数量
 * @param argv 参数数组
 * @param device 设备路径指针
 * @param baud_str 波特率字符串指针
 * @return int 成功返回0，失败返回-1
 */
int Rs485ParseArgs(int argc, char *argv[], char **device, char **baud_str)
{
    *device = NULL;
    *baud_str = "9600";

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--rs485-port") == 0)
        {
            if (i + 1 < argc)
            {
                *device = argv[++i];
            }
            else
            {
                printf("[%s] Error: --rs485-port requires a device path\n", __func__);
                return -1;
            }
        }
        else if (strcmp(argv[i], "--baud-rate") == 0)
        {
            if (i + 1 < argc)
            {
                *baud_str = argv[++i];
            }
            else
            {
                printf("[%s] Error: --baud-rate requires a rate value\n", __func__);
                return -1;
            }
        }
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            Rs485PrintUsage(argv[0]);
            return 1;
        }
        else
        {
            printf("[%s] Error: Unknown argument: %s\n", __func__, argv[i]);
            return -1;
        }
    }

    if (*device == NULL)
    {
        printf("[%s] Error: --rs485-port is required\n", __func__);
        return -1;
    }

    return 0;
}

/**
 * @brief RS485接收线程函数
 * @param arg 未使用
 * @return void* 线程返回值
 */
void *Rs485ReceiveThread(void *arg)
{
    char recv_buffer[MAX_MESSAGE_LENGTH];
    
    (void)arg;
    
    while (g_running)
    {
        memset(recv_buffer, 0, sizeof(recv_buffer));
        int ret = Rs485Receive(g_handle, recv_buffer, sizeof(recv_buffer), RECEIVE_TIMEOUT_MS);
        
        if (ret > 0)
        {
            size_t len = strlen(recv_buffer);
            if (len > 0 && recv_buffer[len - 1] == '\n')
            {
                recv_buffer[len - 1] = '\0';
            }
            // 清除当前行，打印接收数据，然后重新打印提示符
            printf("\r\033[K");  // \r回到行首，\033[K清除到行尾
            printf("RS485< %s\n", recv_buffer);
            printf("RS485> ");
            fflush(stdout);
        }
        else if (ret < 0)
        {
            printf("[%s] Receive error, retrying...\n", __func__);
            fflush(stdout);
            sleep(1);
        }
    } 
    
    return NULL;
}

/**
 * @brief 主函数
 * @param argc 参数数量
 * @param argv 参数数组
 * @return int 程序退出码
 */
int main(int argc, char *argv[])
{
    char *device;
    char *baud_str;
    pthread_t recv_thread;
    
    signal(SIGINT, Rs485SignalHandler);
    signal(SIGTERM, Rs485SignalHandler);

    int parse_result = Rs485ParseArgs(argc, argv, &device, &baud_str);
    if (parse_result != 0)
    {
        if (parse_result < 0)
        {
            Rs485PrintUsage(argv[0]);
        }
        return (parse_result < 0) ? 1 : 0;
    }

    printf("[%s] Starting %s\n", __func__, PROGRAM_NAME);
    printf("[%s] Device: %s, Baud rate: %s\n", __func__, device, baud_str);

    Rs485Config config = {
        .device_path = device,
        .baud_rate = Rs485ParseBaudRate(baud_str),
        .data_bits = 8,
        .stop_bits = 1,
        .parity = 0
    };

    g_handle = Rs485Create(&config);
    if (g_handle == NULL)
    {
        printf("[%s] Failed to create RS485 handle\n", __func__);
        return 1;
    }

    if (Rs485Open(g_handle) != 0)
    {
        printf("[%s] Failed to open RS485 device\n", __func__);
        Rs485Destroy(g_handle);
        return 1;
    }

    if (Rs485Configure(g_handle) != 0)
    {
        printf("[%s] Failed to configure RS485 device\n", __func__);
        Rs485Destroy(g_handle);
        return 1;
    }

    if (Rs485InitGpio(g_handle) != 0)
    {
        printf("[%s] Failed to initialize GPIO control\n", __func__);
        Rs485Destroy(g_handle);
        return 1;
    }

    printf("[%s] RS485 initialized successfully\n", __func__);

    int ret = pthread_create(&recv_thread, NULL, Rs485ReceiveThread, NULL);
    if (ret != 0)
    {
        printf("[%s] Failed to create receiver thread: %s\n", __func__, strerror(ret));
        Rs485Destroy(g_handle);
        return 1;
    }

    printf("[%s] Type messages and press Enter to send (Ctrl+C to exit):\n", __func__);
    fflush(stdout);

    char send_buffer[MAX_MESSAGE_LENGTH];
    while (g_running)
    {
        printf("RS485> ");
        fflush(stdout);
        
        if (fgets(send_buffer, sizeof(send_buffer), stdin) == NULL)
        {
            if (g_running)
            {
                printf("\n[%s] EOF reached, exiting...\n", __func__);
            }
            break;
        }

        if (!g_running)
        {
            break;
        }

        size_t len = strlen(send_buffer);
        if (len > 0 && send_buffer[len - 1] == '\n')
        {
            send_buffer[len - 1] = '\0';
            len--;
        }

        if (len == 0)
        {
            continue;
        }

        strcat(send_buffer, "\n");
        len++;

        int send_ret = Rs485Send(g_handle, send_buffer, len);
        if (send_ret < 0)
        {
            printf("[%s] Send failed, continuing...\n", __func__);
            fflush(stdout);
        }
    }

    pthread_join(recv_thread, NULL);

    printf("[%s] Cleaning up resources...\n", __func__);
    Rs485Destroy(g_handle);

    printf("[%s] %s terminated successfully\n", __func__, PROGRAM_NAME);
    return 0;
}
