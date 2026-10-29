/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-31 12:46:24
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-31 12:50:13
 * @FilePath: /OpenHarmony/unionpi_whale_demo/i2c_oled/src/main.c
 * @Description: I2C 驱动 ssd1306像素屏幕源文件
 */

#include "../include/main.h"
#include "../include/oled_ssd1306.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// SSD1306 I2C地址
#define OLED_I2C_ADDR 0x3C

/**
 * @brief 打印使用帮助信息
 */
void print_usage(const char *program_name)
{
    printf("[%s] Usage: %s --i2c-port <device_path>\n", __func__, program_name);
    printf("[%s] Example: %s --i2c-port /dev/i2c-5\n", __func__, program_name);
    printf("[%s] Example: %s --i2c-port /dev/i2c-0\n", __func__, program_name);
}

/**
 * @brief 解析命令行参数
 *
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 成功返回I2C设备路径，失败返回NULL
 */
const char *parse_arguments(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("[%s] Error: Invalid number of arguments\n", __func__);
        print_usage(argv[0]);
        return NULL;
    }

    if (strcmp(argv[1], "--i2c-port") != 0)
    {
        printf("[%s] Error: Invalid argument '%s'\n", __func__, argv[1]);
        print_usage(argv[0]);
        return NULL;
    }

    const char *i2c_port = argv[2];
    if (i2c_port == NULL || strlen(i2c_port) == 0)
    {
        printf("[%s] Error: Invalid I2C port specified\n", __func__);
        print_usage(argv[0]);
        return NULL;
    }

    return i2c_port;
}

/**
 * @brief 主函数入口
 *
 * 程序执行的起始点，用于初始化OLED显示屏并显示文本信息
 *
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组，必须包含--i2c-port和I2C设备路径
 * @return 返回程序执行状态码，正常退出返回0
 */
int main(int argc, char **argv)
{
    printf("[%s] OLED Demo Start\n", __func__);

    // 解析命令行参数
    const char *i2c_dev = parse_arguments(argc, argv);
    if (i2c_dev == NULL)
    {
        return -1;
    }

    printf("[%s] Using I2C device: %s\n", __func__, i2c_dev);

    // 初始化OLED显示屏
    if (OledInit(i2c_dev, OLED_I2C_ADDR) != 0)
    {
        printf("[%s] Error: Failed to initialize OLED\n", __func__);
        return -1;
    }

    OledFillScreen(i2c_dev, OLED_I2C_ADDR, 0x00);

    OledShowString(i2c_dev, OLED_I2C_ADDR, 0, 0, "Hello,World!", FONT8x16);
    OledShowString(i2c_dev, OLED_I2C_ADDR, 0, 2, "OpenHarmony", FONT8x16);
    OledShowString(i2c_dev, OLED_I2C_ADDR, 0, 4, "Standard System", FONT6x8);
    OledShowString(i2c_dev, OLED_I2C_ADDR, 0, 6, "OLED Demo", FONT6x8);

    printf("[%s] OLED Demo Display Complete\n", __func__);

    while (1)
    {
        sleep(1);
    }

    return 0;
}