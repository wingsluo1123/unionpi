/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-31 17:00:00
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-31 17:00:00
 * @FilePath: /OpenHarmony/unionpi_whale_demo/uart_imu/include/main.h
 * @Description: UART IMU 传感器示例程序主头文件
 */
#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "REG.h"
#include "uart_base.h"
#include "wit_c_sdk.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

// 其他错误码定义
#define ERR (-1)

// 默认IMU串口设备（仅用于向后兼容）
#define UART_IMU_DEFAULT "/dev/ttyUSB0"

/**
 * @brief 加速度数据更新标志
 */
#define ACC_UPDATE 0x01

/**
 * @brief 陀螺仪数据更新标志
 */
#define GYRO_UPDATE 0x02

/**
 * @brief 角度数据更新标志
 */
#define ANGLE_UPDATE 0x04

/**
 * @brief 磁力计数据更新标志
 */
#define MAG_UPDATE 0x08

/**
 * @brief 读取数据更新标志
 */
#define READ_UPDATE 0x80

    /**
     * @brief 支持的波特率列表
     */
    extern const int c_uiBaud[];

    /**
     * @brief 自动扫描传感器设备
     *
     * @param fd 串口文件描述符
     * @param dev 设备路径
     * @return 成功返回0，失败返回-1
     */
    int AutoScanSensor(int fd, const char *dev);

    /**
     * @brief 传感器数据更新回调函数
     *
     * @param uiReg 寄存器地址
     * @param uiRegNum 寄存器数量
     */
    void SensorDataUpdata(uint32_t uiReg, uint32_t uiRegNum);

    /**
     * @brief 延时函数
     *
     * @param ucMs 延时毫秒数
     */
    void Delayms(uint16_t ucMs);

#ifdef __cplusplus
}
#endif

#endif