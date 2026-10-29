/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-31 15:00:00
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-31 15:00:00
 * @FilePath: /OpenHarmony/unionpi_whale_demo/spi_ws2812/include/main.h
 * @Description: 主函数头文件
 */

#ifndef __MAIN_H__
#define __MAIN_H__

#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief 打印使用帮助信息
     */
    void print_usage(const char *program_name);

    /**
     * @brief 解析命令行参数
     * @param argc 命令行参数个数
     * @param argv 命令行参数数组
     * @param spi_device 输出SPI设备路径
     * @param led_count 输出LED数量
     * @return 成功返回0，失败返回-1
     */
    int parse_arguments(int argc, char **argv, const char **spi_device, int *led_count);

    /**
     * @brief 信号处理函数
     */
    void signal_handler(int sig);

    // 演示函数声明
    void demo_basic_colors(void);
    void demo_rainbow_cycle(void);
    void demo_breathing_effect(void);
    void demo_chase_effect(void);
    void demo_color_wipe(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H__ */