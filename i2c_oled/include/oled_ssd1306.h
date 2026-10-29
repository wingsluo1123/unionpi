/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-31 12:46:11
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-31 12:48:00
 * @FilePath: /OpenHarmony/unionpi_whale_demo/i2c_oled/include/oled_ssd1306.h
 * @Description: OLED SSD1306驱动接口定义文件
 */

#ifndef OLED_SSD1306_H
#define OLED_SSD1306_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // 字库类型
    typedef enum Font
    {
        FONT6x8 = 1,
        FONT8x16
    } Font;

    /**
     * @brief 初始化OLED显示屏
     *
     * 初始化SSD1306 OLED显示屏，设置显示参数
     *
     * @param devName I2C设备路径
     * @param addr OLED设备I2C地址
     * @return 成功返回0，失败返回错误码
     */
    uint32_t OledInit(const char *devName, uint8_t addr);

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
    void OledSetPosition(const char *devName, uint8_t addr, uint8_t x, uint8_t y);

    /**
     * @brief 填充屏幕
     *
     * 用指定数据填充整个OLED屏幕
     *
     * @param devName I2C设备路径
     * @param addr OLED设备I2C地址
     * @param fillData 填充数据
     */
    void OledFillScreen(const char *devName, uint8_t addr, uint8_t fillData);

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
    void OledShowChar(const char *devName, uint8_t addr, uint8_t x, uint8_t y, uint8_t ch, Font font);

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
    void OledShowString(const char *devName, uint8_t addr, uint8_t x, uint8_t y, const char *str, Font font);

#ifdef __cplusplus
}
#endif

#endif // OLED_SSD1306_H