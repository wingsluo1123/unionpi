/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-31 15:00:00
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-31 15:00:00
 * @FilePath: /OpenHarmony/unionpi_whale_demo/spi_ws2812/include/ws2812_driver.h
 * @Description: WS2812驱动接口定义文件
 */

#ifndef __WS2812_DRIVER_H__
#define __WS2812_DRIVER_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // RGB颜色结构体
    typedef struct
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    } CRGB;

    // HSV颜色结构体
    typedef struct
    {
        float h;
        float s;
        float v;
    } CHSV;

    // 预定义颜色枚举
    enum Color
    {
        RED = 1,
        GREEN,
        BLUE,
        YELLOW,
        CYAN,
        MAGENTA,
        WHITE,
        BLACK
    };

// 错误码定义
#define WS2812_OK 0
#define WS2812_ERR (-1)

    /**
     * @brief 初始化WS2812控制器
     * @param spi_device SPI设备路径
     * @param led_count LED数量
     * @return 成功返回WS2812_OK，失败返回WS2812_ERR
     */
    int Ws2812Init(const char *spi_device, int led_count);

    /**
     * @brief 清理WS2812资源
     */
    void Ws2812Cleanup(void);

    /**
     * @brief 设置单个LED颜色
     * @param led_index LED索引
     * @param color RGB颜色
     */
    void Ws2812SetColor(int led_index, CRGB color);

    /**
     * @brief 设置所有LED相同颜色
     * @param color RGB颜色
     */
    void Ws2812SetAllColor(CRGB color);

    /**
     * @brief 显示LED（刷新输出）
     */
    void Ws2812Show(void);

    /**
     * @brief 清除所有LED
     */
    void Ws2812Clear(void);

    /**
     * @brief HSV转RGB颜色转换
     * @param h 色调 (0-360)
     * @param s 饱和度 (0-1)
     * @param v 亮度 (0-1)
     * @param r 输出红色分量
     * @param g 输出绿色分量
     * @param b 输出蓝色分量
     */
    void Hsv2Rgb(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b);

    /**
     * @brief 从枚举获取颜色
     * @param color 颜色枚举
     * @return RGB颜色结构体
     */
    CRGB ColorFromEnum(enum Color color);

    /**
     * @brief 彩虹效果
     * @param offset 彩虹偏移量
     */
    void RainbowEffect(int offset);

    /**
     * @brief 呼吸效果
     * @param color 基础颜色
     * @param brightness 亮度 (0-1)
     */
    void BreathingEffect(CRGB color, float brightness);

    /**
     * @brief 追逐效果
     * @param color 追逐颜色
     * @param position 当前位置
     * @param tail_length 尾巴长度
     */
    void ChaseEffect(CRGB color, int position, int tail_length);

#ifdef __cplusplus
}
#endif

#endif /* __WS2812_DRIVER_H__ */