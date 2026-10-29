/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-11-01
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-11-01
 * @FilePath: /OpenHarmony/unionpi_whale_demo/uart_lidar/src/main.c
 * @Description: M1CT-D2激光雷达驱动主程序 (C语言版本)
 */

#include "../include/lidar_data_processing.h"
#include "../include/lidar_information.h"
#include "../include/mtime.h"
#include "../include/serial_port.h"
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 全局变量 */
static volatile int g_running = 1;
static Serial_Port *g_serial = NULL;
static Lidar_Data_Processing g_processor = {0};

/**
 * @brief 信号处理函数
 * @param sig 信号编号
 */
void signal_handler(int sig)
{
    printf("\n[%s] Caught signal %d, exiting...\n", __func__, sig);
    g_running = 0;
}

/**
 * @brief 解析COIN-D6点云数据包
 * @param data 数据缓冲区
 * @param len 数据长度
 * @param packet_num 包编号
 */
static void parse_coin_d6_packet(const uint8_t *data, size_t len, int packet_num)
{
    if (len < 10)
    {
        return;
    }

    // 检查包头
    if (data[0] != 0xAA || data[1] != 0x55)
    {
        return;
    }

    uint8_t lsn = data[3];                   // 采样点数量
    uint16_t fsa = (data[5] << 8) | data[4]; // 起始角度（小端）
    uint16_t lsa = (data[7] << 8) | data[6]; // 结束角度（小端）

    // 计算角度
    float fsa_angle = (fsa >> 1) / 64.0f;
    float lsa_angle = (lsa >> 1) / 64.0f;

    // 检查数据长度
    size_t expected_len = 10 + lsn * 3;
    if (len < expected_len)
    {
        return;
    }

    // 打印所有采样点
    for (int i = 0; i < lsn; i++)
    {
        size_t offset = 10 + i * 3;

        // 根据COIN-D6协议手册解析每个采样点 Si (3字节):
        // Si_L   = 低字节 (data[offset])
        // Si_2nd = 第二字节 (data[offset+1])
        // Si_H   = 高字节 (data[offset+2])
        uint8_t Si_L = data[offset];
        uint8_t Si_2nd = data[offset + 1];
        uint8_t Si_H = data[offset + 2];

        // 距离（Distance）: Distance(mm) = Si_H * 64 + (Si_2nd >> 2)
        uint16_t distance = Si_H * 64 + (Si_2nd >> 2);

        // 强度（Intensity）: Intensity = (Si_2nd & 0x03) * 64 + (Si_L >> 2)
        uint8_t intensity = (Si_2nd & 0x03) * 64 + (Si_L >> 2);

        // 高反标志（High Reflection）: High_Reflection_Flag = Si_L & 0x01
        uint8_t high_reflection = Si_L & 0x01;

        float angle = fsa_angle + (lsa_angle - fsa_angle) * i / (lsn - 1);

        // 打印所有点
        printf("[%s] Angle: %6.2f°  Distance: %4d mm  Intensity: %3d  HighRef: %d\n", __func__, angle, distance, intensity, high_reflection);
    }
}

/**
 * @brief 数据处理线程
 * @param arg 线程参数
 * @return 线程返回值
 */
void *data_processing_thread(void *arg)
{
    uint8_t recv_buffer[4096];
    size_t buffer_pos = 0;
    int packet_count = 0;

    printf("[%s] Data processing thread started\n", __func__);

    /* 先等待一下让雷达启动 */
    sleep_ms(500);

    while (g_running)
    {
        // 读取数据到缓冲区
        size_t available = serial_port_available(g_serial);
        if (available > 0)
        {
            size_t space = sizeof(recv_buffer) - buffer_pos;
            size_t to_read = (available > space) ? space : available;

            result_t bytes_read = serial_port_read(g_serial, recv_buffer + buffer_pos, to_read);
            if (bytes_read > 0)
            {
                buffer_pos += bytes_read;
            }
        }

        // 在缓冲区中查找并解析数据包
        size_t i = 0;
        while (i + 10 <= buffer_pos)
        {
            // 查找包头 0xAA 0x55
            if (recv_buffer[i] == 0xAA && recv_buffer[i + 1] == 0x55)
            {
                uint8_t lsn = recv_buffer[i + 3];
                size_t packet_len = 10 + lsn * 3;

                if (i + packet_len <= buffer_pos)
                {
                    // 完整的数据包
                    packet_count++;
                    parse_coin_d6_packet(recv_buffer + i, packet_len, packet_count);
                    i += packet_len;
                }
                else
                {
                    // 数据包不完整，等待更多数据
                    break;
                }
            }
            else if (recv_buffer[i] == 0xA5 && i + 1 < buffer_pos && recv_buffer[i + 1] == 0x5A)
            {
                // 控制指令响应，跳过
                if (i + 4 <= buffer_pos)
                {
                    uint16_t cmd_len = recv_buffer[i + 2] | (recv_buffer[i + 3] << 8);
                    size_t full_len = 7 + cmd_len; // A5 5A + len(2) + checksum(2) + type(1) + data
                    if (i + full_len <= buffer_pos)
                    {
                        i += full_len;
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
            else
            {
                // 跳过无效字节
                i++;
            }
        }

        // 移动剩余数据到缓冲区开头
        if (i > 0 && i < buffer_pos)
        {
            memmove(recv_buffer, recv_buffer + i, buffer_pos - i);
            buffer_pos -= i;
        }
        else if (i == buffer_pos)
        {
            buffer_pos = 0;
        }

        // 缓冲区满了，清空
        if (buffer_pos >= sizeof(recv_buffer) - 100)
        {
            buffer_pos = 0;
        }

        usleep(1000); // 1ms延时
    }

    printf("\n[%s] Data processing thread stopped\n", __func__);
    return NULL;
}

/**
 * @brief 打印使用帮助信息
 * @param program_name 程序名称
 */
void print_usage(const char *program_name)
{
    printf("[%s] Usage: %s --uart-port <device_path> [options]\n", __func__, program_name);
    printf("[%s] Options:\n", __func__);
    printf("[%s]   --uart-port <path>    Serial port device path (required)\n", __func__);
    printf("[%s]   --baudrate <rate>     Baudrate (default: 230400)\n", __func__);
    printf("[%s]   --help                Show this help message\n", __func__);
    printf("[%s] Example:\n", __func__);
    printf("[%s]   %s --uart-port /dev/ttyUSB0 --baudrate 230400\n", __func__, program_name);
    printf("[%s] Note: This driver supports M1CT_Coin_D2/COIN-D6 lidar only\n", __func__);
}

/**
 * @brief 解析命令行参数
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @param port 串口设备路径输出
 * @param baudrate 波特率输出
 * @return 成功返回0，失败返回-1
 */
int parse_arguments(int argc, char **argv, char **port, int *baudrate)
{
    static struct option long_options[] = {{"uart-port", required_argument, 0, 'p'}, {"baudrate", required_argument, 0, 'b'}, {"help", no_argument, 0, 'h'}, {0, 0, 0, 0}};

    *port = NULL;
    *baudrate = 230400;

    int opt;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "p:b:h", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
        case 'p':
            *port = optarg;
            break;
        case 'b':
            *baudrate = atoi(optarg);
            break;
        case 'h':
            print_usage(argv[0]);
            return -1;
        default:
            print_usage(argv[0]);
            return -1;
        }
    }

    if (*port == NULL)
    {
        printf("[%s] Error: --uart-port is required\n", __func__);
        print_usage(argv[0]);
        return -1;
    }

    return 0;
}

/**
 * @brief 主函数
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 程序退出码
 */
int main(int argc, char **argv)
{
    char *uart_port = NULL;
    int baudrate = 230400;

    printf("[%s] M1CT-D2 Lidar Driver Starting...\n", __func__);

    /* 解析命令行参数 */
    if (parse_arguments(argc, argv, &uart_port, &baudrate) != 0)
    {
        return -1;
    }

    printf("[%s] Configuration:\n", __func__);
    printf("[%s]   Port: %s\n", __func__, uart_port);
    printf("[%s]   Baudrate: %d\n", __func__, baudrate);
    printf("[%s]   Lidar Model: M1CT_Coin_D2 / COIN-D6\n", __func__);

    /* 注册信号处理 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* 创建串口对象 */
    Timeout timeout = timeout_simple(DEFAULT_TIMEOUT);
    Serial_Port *serial = serial_port_create(uart_port, baudrate, timeout);
    if (serial == NULL)
    {
        printf("[%s] Failed to create serial port\n", __func__);
        return -1;
    }

    /* 打开串口 */
    if (!serial_port_open(serial))
    {
        printf("[%s] Failed to open serial port\n", __func__);
        serial_port_destroy(serial);
        return -1;
    }

    /* 设置DTR */
    serial_port_set_dtr(serial, 0);
    sleep_ms(100);

    printf("[%s] Lidar initialized successfully\n", __func__);
    printf("[%s] Starting lidar... (Press Ctrl+C to stop)\n", __func__);

    /* 发送启动命令 */
    serial_port_write(serial, start_lidar, 4);
    printf("[%s] Start command sent\n", __func__);

    /* 保存全局串口 */
    g_serial = serial;

    /* 初始化数据处理器 */
    printf("[%s] Initializing lidar data processor...\n", __func__);
    lidar_data_processing_init(&g_processor);

    /* 创建数据处理线程 */
    pthread_t data_thread;
    int ret = pthread_create(&data_thread, NULL, data_processing_thread, NULL);
    if (ret != 0)
    {
        printf("[%s] Failed to create data processing thread: %d\n", __func__, ret);
        serial_port_close(serial);
        serial_port_destroy(serial);
        return -1;
    }

    printf("[%s] Lidar is running...\n", __func__);

    /* 等待线程结束 */
    pthread_join(data_thread, NULL);

    /* 停止雷达 */
    printf("\n[%s] Stopping lidar...\n", __func__);
    serial_port_write(serial, end_lidar, 4);
    sleep_ms(100);

    /* 清理资源 */
    serial_port_close(serial);
    serial_port_destroy(serial);

    printf("[%s] Lidar driver stopped\n", __func__);
    return 0;
}