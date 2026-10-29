/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-31 15:00:00
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-31 15:00:00
 * @FilePath: /OpenHarmony/unionpi_whale_demo/spi_ws2812/src/ws2812_driver.c
 * @Description: WS2812驱动实现文件
 */

#include "../include/ws2812_driver.h"
#include "../include/spi_base.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// SPI配置参数
static const uint32_t WS2812_SPI_SPEED = 2857142; // 2.857MHz for 350ns timing

// LED配置
static int g_num_leds = 0;
static CRGB *g_led_data = NULL;
static uint8_t *g_tx_buffer = NULL;
static size_t g_tx_length = 0;
static int g_spi_fd = -1;

// WS2812位模式映射表
// WS2812 '0' -> SPI 1000, WS2812 '1' -> SPI 1100
static const uint8_t bitpair_to_byte[] = {
    0b10001000, // 00
    0b10001100, // 01
    0b11001000, // 10
    0b11001100, // 11
};

/**
 * @brief 制作SPI发送缓冲区
 */
static void make_spi_buffer(void)
{
    memset(g_tx_buffer, 0, g_tx_length);

    for (int led_num = 0; led_num < g_num_leds; led_num++)
    {
        unsigned spi_ptr = led_num * 3 * 4; // 每个LED 3色 * 4字节/色
        int dimness = 8;                    // 亮度除数，降低亮度

        // GRB顺序发送 (WS2812要求)
        uint8_t colors[3] = {g_led_data[led_num].g / dimness, g_led_data[led_num].r / dimness, g_led_data[led_num].b / dimness};

        for (int color_idx = 0; color_idx < 3; color_idx++)
        {
            uint8_t color_val = colors[color_idx];
            g_tx_buffer[spi_ptr++] = bitpair_to_byte[(color_val & 0b11000000) >> 6];
            g_tx_buffer[spi_ptr++] = bitpair_to_byte[(color_val & 0b00110000) >> 4];
            g_tx_buffer[spi_ptr++] = bitpair_to_byte[(color_val & 0b00001100) >> 2];
            g_tx_buffer[spi_ptr++] = bitpair_to_byte[(color_val & 0b00000011) >> 0];
        }
    }

    // 添加复位脉冲 (12字节的0，确保>50us的复位时间)
    memset(g_tx_buffer + g_num_leds * 3 * 4, 0, 12);
}

/**
 * @brief 初始化WS2812控制器
 * @param spi_device SPI设备路径
 * @param led_count LED数量
 * @return 成功返回WS2812_OK，失败返回WS2812_ERR
 */
int Ws2812Init(const char *spi_device, int led_count)
{
    if (spi_device == NULL)
    {
        printf("[%s] Error: SPI device path is NULL\n", __func__);
        return WS2812_ERR;
    }

    if (led_count <= 0 || led_count > 300)
    {
        printf("[%s] Error: Invalid LED count (1-300): %d\n", __func__, led_count);
        return WS2812_ERR;
    }

    g_num_leds = led_count;

    // 打开SPI设备
    g_spi_fd = SpiOpen(spi_device);
    if (g_spi_fd < 0)
    {
        printf("[%s] Error: Failed to open SPI device\n", __func__);
        return WS2812_ERR;
    }

    // 配置SPI参数
    SpiConfig spi_config = {.mode = 0, .bits_per_word = 8, .speed_hz = WS2812_SPI_SPEED};

    if (SpiConfigure(g_spi_fd, &spi_config) != SPI_OK)
    {
        printf("[%s] Error: Failed to configure SPI\n", __func__);
        goto error_exit;
    }

    // 分配LED数据缓冲区
    g_led_data = (CRGB *)calloc(g_num_leds, sizeof(CRGB));
    if (!g_led_data)
    {
        printf("[%s] Error: Failed to allocate LED data buffer\n", __func__);
        goto error_exit;
    }

    // 分配SPI传输缓冲区 (每LED 12字节 + 12字节复位)
    g_tx_length = g_num_leds * 3 * 4 + 12;
    g_tx_buffer = (uint8_t *)calloc(g_tx_length, sizeof(uint8_t));
    if (!g_tx_buffer)
    {
        printf("[%s] Error: Failed to allocate SPI transfer buffer\n", __func__);
        goto error_exit;
    }

    printf("[%s] WS2812 initialized successfully:\n", __func__);
    printf("[%s]   LED count: %d\n", __func__, g_num_leds);
    printf("[%s]   SPI speed: %d Hz\n", __func__, WS2812_SPI_SPEED);
    printf("[%s]   Buffer size: %zu bytes\n", __func__, g_tx_length);

    return WS2812_OK;

error_exit:
    Ws2812Cleanup();
    return WS2812_ERR;
}

/**
 * @brief 清理WS2812资源
 */
void Ws2812Cleanup(void)
{
    if (g_led_data)
    {
        free(g_led_data);
        g_led_data = NULL;
    }

    if (g_tx_buffer)
    {
        free(g_tx_buffer);
        g_tx_buffer = NULL;
    }

    if (g_spi_fd >= 0)
    {
        SpiClose(g_spi_fd);
        g_spi_fd = -1;
    }

    printf("[%s] WS2812 resources cleaned up\n", __func__);
}

/**
 * @brief 设置单个LED颜色
 * @param led_index LED索引
 * @param color RGB颜色
 */
void Ws2812SetColor(int led_index, CRGB color)
{
    if (led_index >= 0 && led_index < g_num_leds && g_led_data)
    {
        g_led_data[led_index] = color;
    }
}

/**
 * @brief 设置所有LED相同颜色
 * @param color RGB颜色
 */
void Ws2812SetAllColor(CRGB color)
{
    if (g_led_data)
    {
        for (int i = 0; i < g_num_leds; i++)
        {
            g_led_data[i] = color;
        }
    }
}

/**
 * @brief 显示LED（刷新输出）
 */
void Ws2812Show(void)
{
    if (g_led_data && g_tx_buffer && g_spi_fd >= 0)
    {
        make_spi_buffer();
        SpiTransfer(g_spi_fd, g_tx_buffer, NULL, g_tx_length, 0);
    }
}

/**
 * @brief 清除所有LED
 */
void Ws2812Clear(void)
{
    Ws2812SetAllColor((CRGB){0, 0, 0});
    Ws2812Show();
}

/**
 * @brief HSV转RGB颜色转换
 * @param h 色调 (0-360)
 * @param s 饱和度 (0-1)
 * @param v 亮度 (0-1)
 * @param r 输出红色分量
 * @param g 输出绿色分量
 * @param b 输出蓝色分量
 */
void Hsv2Rgb(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b)
{
    int hi = ((int)h / 60) % 6;
    float f = h * 1.0 / 60 - hi;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    switch (hi)
    {
    case 0:
        *r = 255 * v;
        *g = 255 * t;
        *b = 255 * p;
        break;
    case 1:
        *r = 255 * q;
        *g = 255 * v;
        *b = 255 * p;
        break;
    case 2:
        *r = 255 * p;
        *g = 255 * v;
        *b = 255 * t;
        break;
    case 3:
        *r = 255 * p;
        *g = 255 * q;
        *b = 255 * v;
        break;
    case 4:
        *r = 255 * t;
        *g = 255 * p;
        *b = 255 * v;
        break;
    case 5:
        *r = 255 * v;
        *g = 255 * p;
        *b = 255 * q;
        break;
    }
}

/**
 * @brief 从枚举获取颜色
 * @param color 颜色枚举
 * @return RGB颜色结构体
 */
CRGB ColorFromEnum(enum Color color)
{
    CRGB rgb = {0, 0, 0};
    switch (color)
    {
    case RED:
        rgb = (CRGB){255, 0, 0};
        break;
    case GREEN:
        rgb = (CRGB){0, 255, 0};
        break;
    case BLUE:
        rgb = (CRGB){0, 0, 255};
        break;
    case YELLOW:
        rgb = (CRGB){255, 255, 0};
        break;
    case CYAN:
        rgb = (CRGB){0, 255, 255};
        break;
    case MAGENTA:
        rgb = (CRGB){255, 0, 255};
        break;
    case WHITE:
        rgb = (CRGB){255, 255, 255};
        break;
    case BLACK:
        rgb = (CRGB){0, 0, 0};
        break;
    }
    return rgb;
}

/**
 * @brief 彩虹效果
 * @param offset 彩虹偏移量
 */
void RainbowEffect(int offset)
{
    if (!g_led_data)
        return;

    for (int i = 0; i < g_num_leds; i++)
    {
        float hue = (float)((i * 360 / g_num_leds + offset) % 360);
        uint8_t r, g, b;
        Hsv2Rgb(hue, 1.0, 1.0, &r, &g, &b);
        g_led_data[i] = (CRGB){r, g, b};
    }
}

/**
 * @brief 呼吸效果
 * @param color 基础颜色
 * @param brightness 亮度 (0-1)
 */
void BreathingEffect(CRGB color, float brightness)
{
    if (!g_led_data)
        return;

    for (int i = 0; i < g_num_leds; i++)
    {
        g_led_data[i] = (CRGB){(uint8_t)(color.r * brightness), (uint8_t)(color.g * brightness), (uint8_t)(color.b * brightness)};
    }
}

/**
 * @brief 追逐效果
 * @param color 追逐颜色
 * @param position 当前位置
 * @param tail_length 尾巴长度
 */
void ChaseEffect(CRGB color, int position, int tail_length)
{
    if (!g_led_data)
        return;

    // 清除所有LED
    Ws2812SetAllColor((CRGB){0, 0, 0});

    // 设置主LED
    Ws2812SetColor(position % g_num_leds, color);

    // 设置尾巴，逐渐变暗
    for (int i = 1; i <= tail_length; i++)
    {
        int tail_pos = (position - i + g_num_leds) % g_num_leds;
        float brightness = (float)(tail_length - i + 1) / (tail_length + 1);
        CRGB tail_color = {(uint8_t)(color.r * brightness), (uint8_t)(color.g * brightness), (uint8_t)(color.b * brightness)};
        Ws2812SetColor(tail_pos, tail_color);
    }
}