/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-31 12:00:00
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-31 12:00:00
 * @FilePath: /OpenHarmony/unionpi_whale_demo/uart_ble/include/main.h
 * @Description: UART BLE 透传示例程序主头文件
 */
#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uart_base.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

// 其他错误码定义
#define ERR (-1)

// 默认BLE串口设备（仅用于向后兼容）
#define UART_BLE_DEFAULT "/dev/ttyUSB0"

#ifdef __cplusplus
}
#endif

#endif