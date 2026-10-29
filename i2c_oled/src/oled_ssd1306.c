/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-31 12:46:27
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-31 12:50:27
 * @FilePath: /OpenHarmony/unionpi_whale_demo/i2c_oled/src/oled_ssd1306.c
 * @Description: oled ssd1306 驱动源码
 */

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "../include/main.h"
#include "../include/oled_fonts.h"
#include "../include/oled_ssd1306.h"
#include "securec.h"

#define OLED_I2C_ADDR 0x3C
// 写命令
#define OLED_I2C_CMD 0x00
// 写数据
#define OLED_I2C_DATA 0x40

/**
 * @brief 通过I2C向OLED写一个字节数据
 *
 * 通过I2C接口向指定地址的OLED设备写入一个字节数据
 *
 * @param devName I2C设备路径
 * @param addr OLED设备I2C地址
 * @param regAddr 寄存器地址
 * @param byte 要写入的字节数据
 * @return 成功返回0，失败返回错误码
 */
static uint32_t I2cWriteByte(const char *devName, uint8_t addr, uint8_t regAddr, uint8_t byte)
{
    int fd = -1;
    int ret = -1;
    uint8_t buffer[2] = {regAddr, byte};

    struct i2c_rdwr_ioctl_data i2c_data;

    fd = open(devName, O_RDWR);
    if (fd < 0)
    {
        printf("[%s] Error: Can not open dev %s\n", __func__, devName);
        return -1;
    }

    i2c_data.nmsgs = 1;
    i2c_data.msgs = (struct i2c_msg *)malloc(i2c_data.nmsgs * sizeof(struct i2c_msg));
    if (i2c_data.msgs == NULL)
    {
        printf("[%s] Error: Memory allocation failed\n", __func__);
        close(fd);
        return -2;
    }

    i2c_data.msgs[0].addr = addr;
    i2c_data.msgs[0].flags = 0; // write
    i2c_data.msgs[0].len = sizeof(buffer);
    i2c_data.msgs[0].buf = buffer;

    ret = ioctl(fd, I2C_RDWR, &i2c_data);
    if (ret < 0)
    {
        printf("[%s] Error: I2C write failed\n", __func__);
        free(i2c_data.msgs);
        close(fd);
        return -3;
    }

    free(i2c_data.msgs);
    close(fd);
    return 0;
}

/**
 * @brief 向OLED写命令
 *
 * 向OLED设备发送命令字节
 *
 * @param devName I2C设备路径
 * @param addr OLED设备I2C地址
 * @param cmd 要发送的命令
 * @return 成功返回0，失败返回错误码
 */
static uint32_t WriteCmd(const char *devName, uint8_t addr, uint8_t cmd) { return I2cWriteByte(devName, addr, OLED_I2C_CMD, cmd); }

/**
 * @brief 向OLED写数据
 *
 * 向OLED设备发送数据字节
 *
 * @param devName I2C设备路径
 * @param addr OLED设备I2C地址
 * @param data 要发送的数据
 * @return 成功返回0，失败返回错误码
 */
static uint32_t WriteData(const char *devName, uint8_t addr, uint8_t data) { return I2cWriteByte(devName, addr, OLED_I2C_DATA, data); }

/**
 * @brief 初始化OLED显示屏
 *
 * 初始化SSD1306 OLED显示屏，设置显示参数
 *
 * @param devName I2C设备路径
 * @param addr OLED设备I2C地址
 * @return 成功返回0，失败返回错误码
 */
uint32_t OledInit(const char *devName, uint8_t addr)
{
    static const uint8_t initCmds[] = {
        0xAE, // --display off
        0x00, // ---set low column address
        0x10, // ---set high column address
        0x40, // --set start line address
        0xB0, // --set page address
        0x81, // contract control
        0xFF, // --128
        0xA1, // set segment remap
        0xA6, // --normal / reverse
        0xA8, // --set multiplex ratio(1 to 64)
        0x3F, // --1/32 duty
        0xC8, // Com scan direction
        0xD3, // -set display offset
        0x00,
        0xD5, // set osc division
        0x80,
        0xD8, // set area color mode off
        0x05,
        0xD9, // Set Pre-Charge Period
        0xF1,
        0xDA, // set com pin configuration
        0x12,
        0xDB, // set Vcomh
        0x30,
        0x8D, // set charge pump enable
        0x14,
        0xAF, // --turn on oled panel
    };

    // 发送初始化命令序列
    for (size_t i = 0; i < sizeof(initCmds); i++)
    {
        if (WriteCmd(devName, addr, initCmds[i]) != 0)
        {
            printf("[%s] Error: Failed to send init command %zu\n", __func__, i);
            return -1;
        }
        usleep(1000); // 1ms延时
    }

    printf("[%s] OLED SSD1306 initialized successfully\n", __func__);
    return 0;
}

/**
 * @brief 设置显示位置
 *
 * 设置OLED显示屏的光标位置
 *
 * @param devName I2C设备路径
 * @param addr OLED设备I2C地址
 * @param x X坐标
 * @param y Y坐标
 */
void OledSetPosition(const char *devName, uint8_t addr, uint8_t x, uint8_t y)
{
    WriteCmd(devName, addr, 0xb0 + y);
    WriteCmd(devName, addr, ((x & 0xf0) >> 4) | 0x10);
    WriteCmd(devName, addr, (x & 0x0f) | 0x01);
}

/**
 * @brief 填充屏幕
 *
 * 用指定数据填充整个OLED屏幕
 *
 * @param devName I2C设备路径
 * @param addr OLED设备I2C地址
 * @param fillData 填充数据
 */
void OledFillScreen(const char *devName, uint8_t addr, uint8_t fillData)
{
    uint8_t m, n;
    for (m = 0; m < 8; m++)
    {
        WriteCmd(devName, addr, 0xb0 + m); // page0-page1
        WriteCmd(devName, addr, 0x00);     // low column start address
        WriteCmd(devName, addr, 0x10);     // high column start address
        for (n = 0; n < 128; n++)
        {
            WriteData(devName, addr, fillData);
        }
    }
}

/**
 * @brief 显示字符
 *
 * 在指定位置显示单个字符
 *
 * @param devName I2C设备路径
 * @param addr OLED设备I2C地址
 * @param x X坐标
 * @param y Y坐标
 * @param ch 要显示的字符
 * @param font 字体类型
 */
void OledShowChar(const char *devName, uint8_t addr, uint8_t x, uint8_t y, uint8_t ch, Font font)
{
    uint8_t c = 0, i = 0;

    c = ch - ' ';
    if (x > 128 - 1)
    {
        x = 0;
        y = y + 2;
    }

    if (font == FONT8x16)
    {
        OledSetPosition(devName, addr, x, y);
        for (i = 0; i < 8; i++)
        {
            WriteData(devName, addr, F8X16[c * 16 + i]);
        }
        OledSetPosition(devName, addr, x, y + 1);
        for (i = 0; i < 8; i++)
        {
            WriteData(devName, addr, F8X16[c * 16 + i + 8]);
        }
    }
    else
    {
        OledSetPosition(devName, addr, x, y);
        for (i = 0; i < 6; i++)
        {
            WriteData(devName, addr, F6x8[c][i]);
        }
    }
}

/**
 * @brief 显示字符串
 *
 * 在指定位置显示字符串
 *
 * @param devName I2C设备路径
 * @param addr OLED设备I2C地址
 * @param x X坐标
 * @param y Y坐标
 * @param str 要显示的字符串
 * @param font 字体类型
 */
void OledShowString(const char *devName, uint8_t addr, uint8_t x, uint8_t y, const char *str, Font font)
{
    uint8_t j = 0;
    if (str == NULL)
    {
        printf("[%s] Error: String is NULL\n", __func__);
        return;
    }

    while (str[j])
    {
        OledShowChar(devName, addr, x, y, str[j], font);
        if (font == FONT8x16)
        {
            x += 8;
        }
        else
        {
            x += 6;
        }
        if (x > 120)
        {
            x = 0;
            y += 2;
        }
        j++;
    }
}