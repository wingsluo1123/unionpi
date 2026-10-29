/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-30 20:30:00
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-30 22:49:44
 * @FilePath: /OpenHarmony/unionpi_whale_demo/uart_ultra/include/main.h
 * @Description: 超声波串口读取程序头文件
 */

#ifndef __MAIN_H__
#define __MAIN_H__

#include "uart_base.h"
#include <errno.h>
#include <fcntl.h>
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
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#endif