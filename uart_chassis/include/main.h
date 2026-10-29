/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-31 22:09:47
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-31 22:11:32
 * @FilePath: /OpenHarmony/unionpi_whale_demo/uart_chassis/include/main.h
 * @Description: 底盘控制示例程序头文件
 */

#ifndef __MAIN_H__
#define __MAIN_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "chassis_driver.h"
#include "uart_base.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

    /**
     * @brief 打印使用帮助信息
     */
    void print_usage(const char *program_name);

    /**
     * @brief 解析命令行参数
     *
     * @param argc 命令行参数个数
     * @param argv 命令行参数数组
     * @return 成功返回串口设备路径，失败返回NULL
     */
    const char *parse_arguments(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif // __MAIN_H__