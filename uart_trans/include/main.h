/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-30 18:49:32
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-30 18:50:07
 * @FilePath: /OpenHarmony/unionpi_whale_demo/uart_trans/include/main.h
 * @Description: UART收发示例程序头文件
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

// 默认串口设备（仅用于向后兼容）
#define UART_TTL_NAME "/dev/ttyUSB0"

#ifdef __cplusplus
}
#endif

#endif
