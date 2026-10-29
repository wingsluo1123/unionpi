/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-11-01
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-11-01
 * @FilePath: /OpenHarmony/unionpi_whale_demo/uart_lidar/src/locker.c
 * @Description: 线程同步工具实现 (C语言版本)
 */

#include "../include/locker.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief 初始化锁
 * @param locker 锁对象指针
 * @return 成功返回0，失败返回-1
 */
int locker_init(Locker *locker)
{
    if (locker == NULL)
    {
        return -1;
    }
    return pthread_mutex_init(&locker->mutex, NULL);
}

/**
 * @brief 销毁锁
 * @param locker 锁对象指针
 */
void locker_destroy(Locker *locker)
{
    if (locker != NULL)
    {
        pthread_mutex_unlock(&locker->mutex);
        pthread_mutex_destroy(&locker->mutex);
    }
}

/**
 * @brief 加锁
 * @param locker 锁对象指针
 * @param timeout 超时时间(毫秒)
 * @return LOCK_STATUS 锁状态
 */
LOCK_STATUS locker_lock(Locker *locker, unsigned long timeout)
{
    if (locker == NULL)
    {
        return LOCK_FAILED;
    }

    if (timeout == 0xFFFFFFFF)
    {
        if (pthread_mutex_lock(&locker->mutex) == 0)
        {
            return LOCK_OK;
        }
    }
    else if (timeout == 0)
    {
        if (pthread_mutex_trylock(&locker->mutex) == 0)
        {
            return LOCK_OK;
        }
    }
    else
    {
        struct timespec wait_time;
        struct timeval now;
        gettimeofday(&now, NULL);

        wait_time.tv_sec = timeout / 1000 + now.tv_sec;
        wait_time.tv_nsec = (timeout % 1000) * 1000000 + now.tv_usec * 1000;

        if (wait_time.tv_nsec >= 1000000000)
        {
            ++wait_time.tv_sec;
            wait_time.tv_nsec -= 1000000000;
        }

#if !defined(__ANDROID__)
        switch (pthread_mutex_timedlock(&locker->mutex, &wait_time))
        {
        case 0:
            return LOCK_OK;
        case ETIMEDOUT:
            return LOCK_TIMEOUT;
        }
#else
        /* Android fallback */
        struct timeval timenow;
        struct timespec sleepytime;
        sleepytime.tv_sec = 0;
        sleepytime.tv_nsec = 10000000; /* 10ms */

        while (pthread_mutex_trylock(&locker->mutex) == EBUSY)
        {
            gettimeofday(&timenow, NULL);
            if (timenow.tv_sec >= wait_time.tv_sec && (timenow.tv_usec * 1000) >= wait_time.tv_nsec)
            {
                return LOCK_TIMEOUT;
            }
            nanosleep(&sleepytime, NULL);
        }
        return LOCK_OK;
#endif
    }

    return LOCK_FAILED;
}

/**
 * @brief 解锁
 * @param locker 锁对象指针
 */
void locker_unlock(Locker *locker)
{
    if (locker != NULL)
    {
        pthread_mutex_unlock(&locker->mutex);
    }
}

/**
 * @brief 初始化事件对象
 * @param event 事件对象指针
 * @param is_auto_reset 是否自动重置
 * @param is_signal 初始信号状态
 * @return 成功返回0，失败返回-1
 */
int event_init(Event *event, int is_auto_reset, int is_signal)
{
    if (event == NULL)
    {
        return -1;
    }

    event->is_signalled = is_signal;
    event->is_auto_reset = is_auto_reset;

    int ret = pthread_condattr_init(&event->cond_cattr);
    if (ret != 0)
    {
        fprintf(stderr, "[%s] Failed to init condattr\n", __func__);
        return -1;
    }

    ret = pthread_condattr_setclock(&event->cond_cattr, CLOCK_MONOTONIC);
    if (ret != 0)
    {
        pthread_condattr_destroy(&event->cond_cattr);
        return -1;
    }

    ret = pthread_mutex_init(&event->cond_locker, NULL);
    if (ret != 0)
    {
        pthread_condattr_destroy(&event->cond_cattr);
        return -1;
    }

    ret = pthread_cond_init(&event->cond_var, &event->cond_cattr);
    if (ret != 0)
    {
        pthread_mutex_destroy(&event->cond_locker);
        pthread_condattr_destroy(&event->cond_cattr);
        return -1;
    }

    return 0;
}

/**
 * @brief 销毁事件对象
 * @param event 事件对象指针
 */
void event_destroy(Event *event)
{
    if (event != NULL)
    {
        pthread_condattr_destroy(&event->cond_cattr);
        pthread_mutex_destroy(&event->cond_locker);
        pthread_cond_destroy(&event->cond_var);
    }
}

/**
 * @brief 设置事件信号
 * @param event 事件对象指针
 * @param is_signal 信号状态
 */
void event_set(Event *event, int is_signal)
{
    if (event == NULL)
    {
        return;
    }

    if (is_signal)
    {
        pthread_mutex_lock(&event->cond_locker);
        if (event->is_signalled == 0)
        {
            event->is_signalled = 1;
            pthread_cond_signal(&event->cond_var);
        }
        pthread_mutex_unlock(&event->cond_locker);
    }
    else
    {
        pthread_mutex_lock(&event->cond_locker);
        event->is_signalled = 0;
        pthread_mutex_unlock(&event->cond_locker);
    }
}

/**
 * @brief 等待事件
 * @param event 事件对象指针
 * @param timeout 超时时间(毫秒)
 * @return EVENT_STATUS 事件状态
 */
EVENT_STATUS event_wait(Event *event, unsigned long timeout)
{
    if (event == NULL)
    {
        return EVENT_FAILED;
    }

    EVENT_STATUS ans = EVENT_OK;
    pthread_mutex_lock(&event->cond_locker);

    if (!event->is_signalled)
    {
        if (timeout == 0xFFFFFFFF)
        {
            pthread_cond_wait(&event->cond_var, &event->cond_locker);
        }
        else
        {
            struct timespec wait_time;
            clock_gettime(CLOCK_MONOTONIC, &wait_time);

            wait_time.tv_sec += timeout / 1000;
            wait_time.tv_nsec += (timeout % 1000) * 1000000ULL;

            if (wait_time.tv_nsec >= 1000000000)
            {
                ++wait_time.tv_sec;
                wait_time.tv_nsec -= 1000000000;
            }

            switch (pthread_cond_timedwait(&event->cond_var, &event->cond_locker, &wait_time))
            {
            case 0:
                /* signalled */
                break;
            case ETIMEDOUT:
                /* time up */
                ans = EVENT_TIMEOUT;
                goto _final;
            default:
                ans = EVENT_FAILED;
                goto _final;
            }
        }
    }

    if (event->is_auto_reset)
    {
        event->is_signalled = 0;
    }

_final:
    pthread_mutex_unlock(&event->cond_locker);
    return ans;
}