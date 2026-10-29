/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-31 22:09:47
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-31 22:11:50
 * @FilePath: /OpenHarmony/unionpi_whale_demo/uart_chassis/src/chassis_driver.c
 * @Description: 底盘控制驱动实现
 */

#include "chassis_driver.h"
#include "uart_base.h"
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 定义常量
#define CHASSIS_MAX_LINEAR_SPEED 1000 // 最大线速度 mm/s
#define CHASSIS_MAX_ANGULAR_SPEED 20  // 最大角速度 度/s

// 底盘尺寸 (毫米)
#define CHASSIS_LENGTH 210
#define CHASSIS_WIDTH 151

// 协议帧头
#define FRAME_HEADER_1 0x0A
#define FRAME_HEADER_2 0x0C

// 命令类型
#define CMD_POWER_CONTROL 0x01  // 电源控制命令
#define CMD_MOTION_CONTROL 0x02 // 运动控制命令
#define CMD_SWITCH_MODE 0x05    // 切换模式命令
#define CMD_ODOMETER_RIGHT 0x61 // 右轮里程
#define CMD_ODOMETER_LEFT 0x62  // 左轮里程
#define CMD_SPEED_RIGHT 0x65    // 右轮速度
#define CMD_SPEED_LEFT 0x66     // 左轮速度
#define CMD_BMS_INFO 0x69       // BMS信息

// 控制线程发送间隔（微秒）
#define CONTROL_THREAD_INTERVAL 200000 // 0.2秒

// 计算校验和
static uint8_t calculate_checksum(uint8_t *data)
{
    uint8_t checksum = 0;
    for (int i = 2; i < data[2] + 3; i++)
    {
        checksum ^= data[i];
    }
    return checksum & 0xFF;
}

// 有符号转换
static int32_t type_conversion(uint32_t data, int len)
{
    if (data & (1 << (len * 8 - 1)))
    {
        data = data - (1 << (len * 8));
    }
    return (int32_t)data;
}

/**
 * @brief 初始化底盘驱动
 * @param device 串口设备路径
 * @return 成功返回文件描述符，失败返回-1
 */
int chassis_init(const char *device)
{
    // 打开串口
    int fd = UartOpen(device, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        printf("[%s] Failed to open UART device: %s\n", __func__, strerror(errno));
        return -1;
    }

    // 配置串口
    struct SerialPortConfig config = {.baudrate = 115200, .databits = 8, .stopbits = 1, .parity = 'N', .vtime = 0, .vmin = 0, .flow_control = SERIAL_FLOW_NONE, .raw_mode = 1};

    if (UartInit(fd, &config) != UART_OK)
    {
        printf("[%s] Failed to initialize UART\n", __func__);
        UartClose(fd);
        return -1;
    }

    return fd;
}

/**
 * @brief 关闭底盘驱动
 * @param fd 串口文件描述符
 */
void chassis_close(int fd)
{
    if (fd >= 0)
    {
        UartClose(fd);
    }
}

/**
 * @brief 使能底盘
 * @param fd 串口文件描述符
 * @return 成功返回0，失败返回-1
 */
int chassis_power_on(int fd)
{
    if (fd < 0)
    {
        printf("[%s] Invalid file descriptor\n", __func__);
        return -1;
    }

    uint8_t cmd[] = {FRAME_HEADER_1, FRAME_HEADER_2, 0x01, 0x00, CMD_POWER_CONTROL, 0x01, 0x01};
    cmd[6] = calculate_checksum(cmd);

    ssize_t result = UartWrite(fd, cmd, sizeof(cmd));

    // 等待底盘完成初始化（5秒）
    if (result == sizeof(cmd))
    {
        printf("[%s] Power on command sent, waiting for initialization...\n", __func__);
        sleep(5);
        return 0;
    }

    printf("[%s] Failed to send power on command\n", __func__);
    return -1;
}

/**
 * @brief 失能底盘
 * @param fd 串口文件描述符
 * @return 成功返回0，失败返回-1
 */
int chassis_power_off(int fd)
{
    if (fd < 0)
    {
        printf("[%s] Invalid file descriptor\n", __func__);
        return -1;
    }

    uint8_t cmd[] = {FRAME_HEADER_1, FRAME_HEADER_2, 0x01, 0x00, CMD_POWER_CONTROL, 0x00, 0x00};
    cmd[6] = calculate_checksum(cmd);

    ssize_t result = UartWrite(fd, cmd, sizeof(cmd));
    if (result != sizeof(cmd))
    {
        printf("[%s] Failed to send power off command\n", __func__);
        return -1;
    }

    return 0;
}

/**
 * @brief 内部函数：设置底盘速度（原始值）
 * @param fd 串口文件描述符
 * @param linear 线速度 (-1000 ~ 1000 mm/s)
 * @param angular 角速度 (-1000 ~ 1000 centidegrees/s)
 * @return 成功返回0，失败返回-1
 */
static int internal_chassis_set_velocity_raw(int fd, int16_t linear, int16_t angular)
{
    if (fd < 0)
    {
        printf("[%s] Invalid file descriptor\n", __func__);
        return -1;
    }

    uint8_t cmd[12];
    cmd[0] = FRAME_HEADER_1;
    cmd[1] = FRAME_HEADER_2;
    cmd[2] = 0x06;
    cmd[3] = 0x00;
    cmd[4] = CMD_MOTION_CONTROL;

    // 限制速度范围
    if (linear > CHASSIS_MAX_LINEAR_SPEED)
        linear = CHASSIS_MAX_LINEAR_SPEED;
    if (linear < -CHASSIS_MAX_LINEAR_SPEED)
        linear = -CHASSIS_MAX_LINEAR_SPEED;

    // 角速度以百分度为单位（度*100）
    if (angular > CHASSIS_MAX_ANGULAR_SPEED * 100)
        angular = CHASSIS_MAX_ANGULAR_SPEED * 100;
    if (angular < -CHASSIS_MAX_ANGULAR_SPEED * 100)
        angular = -CHASSIS_MAX_ANGULAR_SPEED * 100;

    // 原地旋转情况
    if (linear == 0 && angular != 0)
    {
        // 原地旋转速度计算
        float round_vel_float = sqrt(CHASSIS_LENGTH * CHASSIS_LENGTH + CHASSIS_WIDTH * CHASSIS_WIDTH) * (angular / 100.0);
        int16_t round_vel = (int16_t)round_vel_float;

        if (round_vel > CHASSIS_MAX_LINEAR_SPEED)
            round_vel = CHASSIS_MAX_LINEAR_SPEED;
        if (round_vel < -CHASSIS_MAX_LINEAR_SPEED)
            round_vel = -CHASSIS_MAX_LINEAR_SPEED;

        // 线速度为0，角速度转换为旋转速度
        cmd[5] = 0x00;                     // linear low byte
        cmd[6] = 0x00;                     // linear high byte
        cmd[7] = 0x00;                     // angular low byte
        cmd[8] = 0x00;                     // angular high byte
        cmd[9] = round_vel & 0xFF;         // round vel low byte
        cmd[10] = (round_vel >> 8) & 0xFF; // round vel high byte
        cmd[11] = 0x00;                    // checksum placeholder
    }
    else
    {
        // 正常运动情况
        cmd[5] = linear & 0xFF;         // linear low byte
        cmd[6] = (linear >> 8) & 0xFF;  // linear high byte
        cmd[7] = angular & 0xFF;        // angular low byte
        cmd[8] = (angular >> 8) & 0xFF; // angular high byte
        cmd[9] = 0x00;                  // unused
        cmd[10] = 0x00;                 // unused
        cmd[11] = 0x00;                 // checksum placeholder
    }

    cmd[11] = calculate_checksum(cmd);

    ssize_t result = UartWrite(fd, cmd, sizeof(cmd));
    return (result == sizeof(cmd)) ? 0 : -1;
}

/**
 * @brief 设置底盘速度（原始值）
 * @param fd 串口文件描述符
 * @param linear 线速度 (-1000 ~ 1000 mm/s)
 * @param angular 角速度 (-1000 ~ 1000 centidegrees/s)
 * @return 成功返回0，失败返回-1
 */
int chassis_set_velocity_raw(int fd, int16_t linear, int16_t angular) { return internal_chassis_set_velocity_raw(fd, linear, angular); }

/**
 * @brief 设置底盘速度
 * @param fd 串口文件描述符
 * @param linear_x 线速度 (mm/s)
 * @param angular_z 角速度 (rad/s)
 * @return 成功返回0，失败返回-1
 */
int chassis_set_velocity(int fd, int32_t linear_x, float angular_z)
{
    if (fd < 0)
    {
        printf("[%s] Invalid file descriptor\n", __func__);
        return -1;
    }

    // 将线速度从mm/s转换为整数
    int16_t linear = (int16_t)linear_x;

    // 将角速度从rad/s转换为centidegrees/s
    int16_t angular = (int16_t)(angular_z * 180.0 * 100 / M_PI);

    return chassis_set_velocity_raw(fd, linear, angular);
}

/**
 * @brief 停止底盘运动
 * @param fd 串口文件描述符
 * @return 成功返回0，失败返回-1
 */
int chassis_stop(int fd) { return chassis_set_velocity_raw(fd, 0, 0); }

/**
 * @brief 切换速度角度模式
 * @param fd 串口文件描述符
 * @return 成功返回0，失败返回-1
 */
int chassis_switch_speed_angle_mode(int fd)
{
    if (fd < 0)
    {
        printf("[%s] Invalid file descriptor\n", __func__);
        return -1;
    }

    uint8_t cmd[] = {FRAME_HEADER_1, FRAME_HEADER_2, 0x01, 0x00, CMD_SWITCH_MODE, 0x00, 0x04};
    cmd[6] = calculate_checksum(cmd);

    ssize_t result = UartWrite(fd, cmd, sizeof(cmd));
    if (result != sizeof(cmd))
    {
        printf("[%s] Failed to send switch mode command\n", __func__);
        return -1;
    }

    return 0;
}

/**
 * @brief 读取底盘信息
 * @param fd 串口文件描述符
 * @param info 底盘信息结构体指针
 * @return 成功返回0，失败返回-1
 */
int chassis_read_info(int fd, chassis_info_t *info)
{
    if (fd < 0 || info == NULL)
    {
        printf("[%s] Invalid parameters\n", __func__);
        return -1;
    }

    uint8_t buffer[32];
    ssize_t bytes_read = UartRead(fd, buffer, sizeof(buffer));

    if (bytes_read <= 0)
    {
        // 没有数据可读
        return -1;
    }

    // 检查帧头
    if (bytes_read >= 2 && buffer[0] == FRAME_HEADER_1 && buffer[1] == FRAME_HEADER_2)
    {
        // 验证校验和
        uint8_t checksum = calculate_checksum(buffer);
        if (checksum != buffer[bytes_read - 1])
        {
            printf("[%s] Checksum verification failed\n", __func__);
            return -1; // 校验和错误
        }

        uint8_t cmd = buffer[4];

        // 解析不同类型的数据包
        if (cmd == CMD_BMS_INFO && bytes_read == 14)
        {
            // BMS信息
            info->bms.timestamp = 0; // 在实际应用中可以设置为当前时间戳
            info->bms.volume_percentage = type_conversion(buffer[5] | (buffer[6] << 8), 2);
            info->bms.current_a = type_conversion(buffer[7] | (buffer[8] << 8), 2) / 100.0;
            info->bms.voltage_v = type_conversion(buffer[9] | (buffer[10] << 8), 2) / 100.0;
            return 0;
        }
        else if ((cmd == CMD_ODOMETER_RIGHT || cmd == CMD_ODOMETER_LEFT) && bytes_read == 14)
        {
            // 里程计信息
            if (cmd == CMD_ODOMETER_RIGHT)
            {
                info->odometer.right_back_odometer = type_conversion(buffer[5] | (buffer[6] << 8) | (buffer[7] << 16) | (buffer[8] << 24), 4);
                info->odometer.right_front_odometer = type_conversion(buffer[9] | (buffer[10] << 8) | (buffer[11] << 16) | (buffer[12] << 24), 4);
            }
            else
            {
                info->odometer.left_front_odometer = type_conversion(buffer[5] | (buffer[6] << 8) | (buffer[7] << 16) | (buffer[8] << 24), 4);
                info->odometer.left_back_odometer = type_conversion(buffer[9] | (buffer[10] << 8) | (buffer[11] << 16) | (buffer[12] << 24), 4);
            }
            return 0;
        }
        else if ((cmd == CMD_SPEED_RIGHT || cmd == CMD_SPEED_LEFT) && bytes_read == 14)
        {
            // 速度信息
            if (cmd == CMD_SPEED_RIGHT)
            {
                info->speed.right_back_speed = type_conversion(buffer[5] | (buffer[6] << 8) | (buffer[7] << 16) | (buffer[8] << 24), 4);
                info->speed.right_front_speed = type_conversion(buffer[9] | (buffer[10] << 8) | (buffer[11] << 16) | (buffer[12] << 24), 4);
            }
            else
            {
                info->speed.left_front_speed = type_conversion(buffer[5] | (buffer[6] << 8) | (buffer[7] << 16) | (buffer[8] << 24), 4);
                info->speed.left_back_speed = type_conversion(buffer[9] | (buffer[10] << 8) | (buffer[11] << 16) | (buffer[12] << 24), 4);
            }
            return 0;
        }
    }

    // 未知数据包或者解析失败
    return -1;
}

// 控制线程函数
static void *control_thread_func(void *arg)
{
    chassis_control_t *control = (chassis_control_t *)arg;

    while (control->running)
    {
        // 获取当前目标速度
        pthread_mutex_lock(&control->mutex);
        int16_t linear = control->target_linear;
        int16_t angular = control->target_angular;
        int fd = control->fd;
        pthread_mutex_unlock(&control->mutex);

        // 发送控制指令
        internal_chassis_set_velocity_raw(fd, linear, angular);

        // 等待下一周期
        usleep(CONTROL_THREAD_INTERVAL);
    }

    return NULL;
}

/**
 * @brief 启动持续运动控制线程
 * @param fd 串口文件描述符
 * @param linear 线速度 (-1000 ~ 1000 mm/s)
 * @param angular 角速度 (-1000 ~ 1000 centidegrees/s)
 * @return 成功返回控制状态结构体指针，失败返回NULL
 */
chassis_control_t *chassis_start_continuous_control(int fd, int16_t linear, int16_t angular)
{
    if (fd < 0)
    {
        printf("[%s] Invalid file descriptor\n", __func__);
        return NULL;
    }

    chassis_control_t *control = (chassis_control_t *)malloc(sizeof(chassis_control_t));
    if (!control)
    {
        printf("[%s] Failed to allocate memory for control structure\n", __func__);
        return NULL;
    }

    control->fd = fd;
    control->target_linear = linear;
    control->target_angular = angular;
    control->running = 1;

    // 初始化互斥锁
    if (pthread_mutex_init(&control->mutex, NULL) != 0)
    {
        printf("[%s] Failed to initialize mutex\n", __func__);
        free(control);
        return NULL;
    }

    // 创建控制线程
    if (pthread_create(&control->control_thread, NULL, control_thread_func, control) != 0)
    {
        printf("[%s] Failed to create control thread\n", __func__);
        pthread_mutex_destroy(&control->mutex);
        free(control);
        return NULL;
    }

    return control;
}

/**
 * @brief 更新持续运动控制参数
 * @param control 控制状态结构体指针
 * @param linear 线速度 (-1000 ~ 1000 mm/s)
 * @param angular 角速度 (-1000 ~ 1000 centidegrees/s)
 * @return 成功返回0，失败返回-1
 */
int chassis_update_continuous_control(chassis_control_t *control, int16_t linear, int16_t angular)
{
    if (!control)
    {
        printf("[%s] Invalid control structure\n", __func__);
        return -1;
    }

    pthread_mutex_lock(&control->mutex);
    control->target_linear = linear;
    control->target_angular = angular;
    pthread_mutex_unlock(&control->mutex);

    return 0;
}

/**
 * @brief 停止持续运动控制线程
 * @param control 控制状态结构体指针
 */
void chassis_stop_continuous_control(chassis_control_t *control)
{
    if (!control)
    {
        printf("[%s] Invalid control structure\n", __func__);
        return;
    }

    // 停止线程
    control->running = 0;
    pthread_join(control->control_thread, NULL);

    // 销毁互斥锁
    pthread_mutex_destroy(&control->mutex);

    // 释放内存
    free(control);
}