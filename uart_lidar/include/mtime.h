/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-11-01
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-11-01 16:21:44
 * @FilePath: /OpenHarmony/unionpi_whale_demo/uart_lidar/include/mtime.h
 * @Description: 时间工具函数头文件 (C语言版本)
 */

#ifndef __H_MTIME_H__
#define __H_MTIME_H__

#include <stdint.h>
#include <time.h>
#include <unistd.h>
 
#ifdef __cplusplus
extern "C"
{
#endif

    /* 时间精度枚举 */
    typedef enum
    {
        TIME_NANOSECOND = 0, /* 纳秒 */
        TIME_MICROSECOND,    /* 微秒 */
        TIME_MILLISECOND,    /* 毫秒 */
        TIME_SECOND,         /* 秒 */
        TIME_MINUTE,         /* 分钟 */
        TIME_HOUR            /* 小时 */
    } TIME_PRECISION;

    /**
     * @brief 获取当前时间 (单调时钟)
     * @param precision 时间精度
     * @return 当前时间值
     */
    int64_t current_times(int precision);

    /**
     * @brief 获取当前时间 (实时时钟)
     * @return 当前时间(纳秒)
     */
    uint64_t getTime(void);

    /**
     * @brief 获取毫秒时间戳
     * @return 毫秒时间戳
     */
    uint32_t getms(void);

    /**
     * @brief 延时(毫秒)
     * @param ms 延时时间(毫秒)
     */
    void delay(uint32_t ms);

    /**
     * @brief 休眠多少毫秒
     * @param ms 休眠时间(毫秒)
     */
    void sleep_ms(int ms);

    /**
     * @brief 获取本地日期时间
     * @param time_zone 时区 (默认8为北京时间)
     * @return struct tm 结构
     */
    struct tm localdate(int time_zone);

    /**
     * @brief 获取时间字符串
     * @param fmt 格式化字符串 (默认"%F %T"即"YYYY-MM-DD HH:MM:SS")
     * @return 时间字符串指针(静态缓冲区)
     */
    char *time_str(const char *fmt);

#ifdef __cplusplus
}
#endif

#endif /* __H_MTIME_H__ */
