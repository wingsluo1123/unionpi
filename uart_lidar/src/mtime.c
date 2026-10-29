/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-11-01
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-11-01
 * @FilePath: /OpenHarmony/unionpi_whale_demo/uart_lidar/src/mtime.c
 * @Description: 时间工具函数实现 (C语言版本)
 */

#include "../include/mtime.h"
#include <string.h>
#include <sys/time.h>

/**
 * @brief 获取当前时间 (单调时钟)
 * @param precision 时间精度
 * @return 当前时间值
 */
int64_t current_times(int precision)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    int64_t nanoseconds = (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec;

    switch (precision)
    {
    case TIME_NANOSECOND:
        return nanoseconds;
    case TIME_MICROSECOND:
        return nanoseconds / 1000;
    case TIME_MILLISECOND:
        return nanoseconds / 1000000;
    case TIME_SECOND:
        return ts.tv_sec;
    case TIME_MINUTE:
        return ts.tv_sec / 60;
    case TIME_HOUR:
        return ts.tv_sec / 3600;
    }
    return 0;
}

/**
 * @brief 获取毫秒时间戳
 * @return 毫秒时间戳
 */
uint32_t getms(void)
{
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000L + t.tv_nsec / 1000000L;
}

/**
 * @brief 获取当前时间 (实时时钟)
 * @return 当前时间(纳秒)
 */
uint64_t getTime(void)
{
    struct timespec tim;
    clock_gettime(CLOCK_REALTIME, &tim);
    return (uint64_t)tim.tv_sec * 1000000000ULL + tim.tv_nsec;
}

/**
 * @brief 延时(毫秒)
 * @param ms 延时时间(毫秒)
 */
void delay(uint32_t ms)
{
    while (ms >= 1000)
    {
        usleep(1000 * 1000);
        ms -= 1000;
    }

    if (ms != 0)
    {
        usleep(ms * 1000);
    }
}

/**
 * @brief 休眠多少毫秒
 * @param ms 休眠时间(毫秒)
 */
void sleep_ms(int ms)
{
    int ts = ms / 1000;
    int ns = ms % 1000;
    struct timespec r = {ts, ns * 1000 * 1000};
    nanosleep(&r, NULL);
}

/**
 * @brief 获取本地日期时间
 * @param time_zone 时区 (默认8为北京时间)
 * @return struct tm 结构
 */
struct tm localdate(int time_zone)
{
    time_t unix_sec = time(NULL);
    struct tm tms;
    memset(&tms, 0, sizeof(tms));

    static const int kHoursInDay = 24;
    static const int kMinutesInHour = 60;
    static const int kDaysFromUnixTime = 2472632;
    static const int kDaysFromYear = 153;
    static const int kMagicUnkonwnFirst = 146097;
    static const int kMagicUnkonwnSec = 1461;

    tms.tm_sec = unix_sec % kMinutesInHour;
    int i = (unix_sec / kMinutesInHour);
    tms.tm_min = i % kMinutesInHour;
    i /= kMinutesInHour;
    tms.tm_hour = (i + time_zone) % kHoursInDay;
    tms.tm_mday = (i + time_zone) / kHoursInDay;

    int a = tms.tm_mday + kDaysFromUnixTime;
    int b = (a * 4 + 3) / kMagicUnkonwnFirst;
    int c = (-b * kMagicUnkonwnFirst) / 4 + a;
    int d = ((c * 4 + 3) / kMagicUnkonwnSec);
    int e = -d * kMagicUnkonwnSec;
    e = e / 4 + c;
    int m = (5 * e + 2) / kDaysFromYear;

    tms.tm_mday = -(kDaysFromYear * m + 2) / 5 + e + 1;
    tms.tm_mon = (-m / 10) * 12 + m + 2;
    tms.tm_year = b * 100 + d - 6700 + (m / 10);

    return tms;
}

/**
 * @brief 获取时间字符串
 * @param fmt 格式化字符串 (默认"%F %T"即"YYYY-MM-DD HH:MM:SS")
 * @return 时间字符串指针(静态缓冲区)
 */
char *time_str(const char *fmt)
{
    static char str[256];
    struct tm tms = localdate(8); /* 默认北京时间 */

    if (fmt == NULL)
    {
        fmt = "%F %T";
    }

    strftime(str, 255, fmt, &tms);
    return str;
}