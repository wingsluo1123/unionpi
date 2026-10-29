/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-30 16:36:48
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-11-11 19:39:09
 * @FilePath: /OpenHarmony/unionpi_whale_demo/rs485_trans/src/gpio_base.c
 * @Description: GPIO 驱动接口实现文件
 */

#include "../include/gpio_base.h"
#include "securec.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * @brief 导出或取消导出GPIO引脚的具体实现
 *
 * 通过向/sys/class/gpio/export或/sys/class/gpio/unexport写入GPIO编号来导出或取消导出GPIO引脚
 *
 * @param gpioNum GPIO引脚编号
 * @param bExport 导出标志，0表示取消导出，1表示导出
 * @return 成功返回0，失败返回错误码
 */
int GPIO_Export(int gpioNum, int bExport)
{
    int fd, len;
    char buf[128];
    const char *path = bExport ? GPIO_EXPORT : GPIO_UNEXPORT;
    
    fd = open(path, O_WRONLY);
    if (fd < 0)
    {
        printf("[%s] Failed to open %s: %s\n", __func__, path, strerror(errno));
        return GPIO_ERR;
    }
    
    len = snprintf(buf, sizeof(buf), "%d", gpioNum);
    if (write(fd, buf, len) < 0)
    {
        printf("[%s] Failed to write to %s: %s\n", __func__, path, strerror(errno));
        close(fd);
        return GPIO_ERR;
    }
    
    close(fd);
    return 0;
}

/**
 * @brief 设置GPIO引脚方向的具体实现
 *
 * 设置指定GPIO引脚为输入或输出模式
 *
 * @param gpioNum GPIO引脚编号
 * @param direction 方向，0表示输入，1表示输出
 * @return 成功返回0，失败返回错误码
 */
int GPIO_SetDirection(int gpioNum, int direction)
{
    int ret_sprintf_s = -1;

    // 检查GPIO是否已导出
    char gpio_file_name[128];
    (void)memset_s(gpio_file_name, sizeof(gpio_file_name), 0, sizeof(gpio_file_name));
    ret_sprintf_s = snprintf_s(gpio_file_name, sizeof(gpio_file_name), sizeof(gpio_file_name), "%s%d/direction", GPIO_PEX, gpioNum);
    if (ret_sprintf_s != 0)
    {
    }

    if (access(gpio_file_name, F_OK) != 0)
    {
        printf("[%s] gpio%d not export", __func__, gpioNum);
        return GPIO_NOT_EXPORT_ERR;
    }
    // 设置GPIO方向

    FILE *fp = NULL;
    fp = fopen(gpio_file_name, "r+");
    if (fp == NULL)
    {
        printf("[%s] open %s%d/direction failed", __func__, GPIO_PEX, gpioNum);
        return GPIO_ERR;
    }

    if (direction == GPIO_DIRECTION_IN)
    {
        fprintf(fp, "%s", "in");
    }
    else if (direction == GPIO_DIRECTION_OUT)
    {
        fprintf(fp, "%s", "out");
    }

    (void)fclose(fp);
    fp = NULL;

    return 0;
}

/**
 * @brief 设置GPIO引脚电平值的具体实现
 *
 * 设置指定GPIO引脚的电平值（高电平或低电平）
 *
 * @param gpioNum GPIO引脚编号
 * @param value 电平值，0表示低电平，1表示高电平
 * @return 成功返回0，失败返回错误码
 */
int GPIO_SetValue(int gpioNum, int value)
{
    int ret_sprintf_s = -1;

    // 检查GPIO是否已导出
    char gpio_file_name[128];
    (void)memset_s(gpio_file_name, sizeof(gpio_file_name), 0, sizeof(gpio_file_name));
    ret_sprintf_s = snprintf_s(gpio_file_name, sizeof(gpio_file_name), sizeof(gpio_file_name), "%s%d/value", GPIO_PEX, gpioNum);
    if (ret_sprintf_s != 0)
    {
    }

    if (access(gpio_file_name, F_OK) != 0)
    {
        printf("[%s] gpio%d not export", __func__, gpioNum);
        return GPIO_NOT_EXPORT_ERR;
    }
    // 设置GPIO值

    FILE *fp = NULL;
    fp = fopen(gpio_file_name, "r+");
    if (fp == NULL)
    {
        printf("[%s] open %s%d/value failed", __func__, GPIO_PEX, gpioNum);
        return GPIO_ERR;
    }

    if (value == GPIO_LOW_LEVE)
    {
        fprintf(fp, "%s", "0");
    }
    else if (value == GPIO_HIGH_LEVE)
    {
        fprintf(fp, "%s", "1");
    }

    (void)fclose(fp);
    fp = NULL;

    return 0;
}

/**
 * @brief 检查GPIO引脚是否已导出的具体实现
 *
 * 检查指定GPIO引脚是否已经被导出
 *
 * @param gpioNum GPIO引脚编号
 * @param *value 导出状态指针，0表示未导出，1表示已导出
 * @return 成功返回0，失败返回错误码
 */
int GPIO_IsExport(int gpioNum, int *value)
{
    int ret_sprintf_s = -1;

    if (value == NULL)
    {
        return GPIO_ERR;
    }
    // 检查GPIO是否已导出
    char gpio_file_name[128];
    (void)memset_s(gpio_file_name, sizeof(gpio_file_name), 0, sizeof(gpio_file_name));
    ret_sprintf_s = snprintf_s(gpio_file_name, sizeof(gpio_file_name), sizeof(gpio_file_name), "%s%d/value", GPIO_PEX, gpioNum);
    if (ret_sprintf_s != 0)
    {
    }

    if (access(gpio_file_name, F_OK) != 0)
    {
        printf("[%s] gpio%d not export", __func__, gpioNum);
        *value = GPIO_NOT_EXPORT;
    }
    else
    {
        *value = GPIO_EXPORTED;
    }
    return 0;
}

/**
 * @brief 获取GPIO引脚方向的具体实现
 *
 * 获取指定GPIO引脚的方向设置（输入或输出）
 *
 * @param gpioNum GPIO引脚编号
 * @param *value 方向值指针，0表示输入，1表示输出
 * @return 成功返回0，失败返回错误码
 */
int GPIO_GetDirection(int gpioNum, int *value)
{
    int ret = 0;
    int ret_sprintf_s = -1;

    if (value == NULL)
    {
        return GPIO_ERR;
    }
    // 检查GPIO是否已导出
    char gpio_file_name[128];
    (void)memset_s(gpio_file_name, sizeof(gpio_file_name), 0, sizeof(gpio_file_name));
    ret_sprintf_s = snprintf_s(gpio_file_name, sizeof(gpio_file_name), sizeof(gpio_file_name), "%s%d/direction", GPIO_PEX, gpioNum);
    if (ret_sprintf_s != 0)
    {
    }

    if (access(gpio_file_name, F_OK) != 0)
    {
        printf("[%s] gpio%d not export", __func__, gpioNum);
        return GPIO_NOT_EXPORT_ERR;
    }
    // 获取GPIO方向

    FILE *fp = NULL;
    char buffer[20] = {0};
    fp = fopen(gpio_file_name, "r");
    if (fp == NULL)
    {
        printf("[%s] read %s%d/direction failed", __func__, GPIO_PEX, gpioNum);
        return GPIO_ERR;
    }
    (void)fread(buffer, sizeof(buffer), 1, fp);
    (void)fclose(fp);
    fp = NULL;
    if (strstr(buffer, "out") != NULL)
    {
        *value = GPIO_DIRECTION_OUT;
    }
    else if (strstr(buffer, "in") != NULL)
    {
        *value = GPIO_DIRECTION_IN;
    }
    else
    {
        ret = GPIO_ERR;
    }
    return ret;
}

/**
 * @brief 获取GPIO引脚电平值的具体实现
 *
 * 获取指定GPIO引脚的当前电平值
 *
 * @param gpioNum GPIO引脚编号
 * @param *value 电平值指针，0表示低电平，1表示高电平
 * @return 成功返回0，失败返回错误码
 */
int GPIO_GetValue(int gpioNum, int *value)
{
    int ret = 0;
    int ret_sprintf_s = -1;

    if (value == NULL)
    {
        return GPIO_ERR;
    }
    // 检查GPIO是否已导出
    char gpio_file_name[128];
    (void)memset_s(gpio_file_name, sizeof(gpio_file_name), 0, sizeof(gpio_file_name));
    ret_sprintf_s = snprintf_s(gpio_file_name, sizeof(gpio_file_name), sizeof(gpio_file_name), "%s%d/value", GPIO_PEX, gpioNum);
    if (ret_sprintf_s != 0)
    {
    }

    if (access(gpio_file_name, F_OK) != 0)
    {
        printf("[%s] gpio%d not export", __func__, gpioNum);
        return GPIO_NOT_EXPORT_ERR;
    }
    // 获取GPIO值

    FILE *fp = NULL;
    char buffer[20] = {0};
    fp = fopen(gpio_file_name, "r");
    if (fp == NULL)
    {
        printf("[%s] read %s%d/value failed", __func__, GPIO_PEX, gpioNum);
        return GPIO_ERR;
    }
    (void)fread(buffer, sizeof(buffer), 1, fp);
    (void)fclose(fp);
    fp = NULL;
    if (strstr(buffer, "0") != NULL)
    {
        *value = GPIO_LOW_LEVE;
    }
    else if (strstr(buffer, "1") != NULL)
    {
        *value = GPIO_HIGH_LEVE;
    }
    else
    {
        ret = GPIO_ERR;
    }
    return ret;
}
