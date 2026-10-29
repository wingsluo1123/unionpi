/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-11-01
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-11-01 16:21:42
 * @FilePath: /OpenHarmony/unionpi_whale_demo/uart_lidar/include/locker.h
 * @Description: 线程同步工具头文件 (C语言版本)
 */

#ifndef __LOCKER_H__
#define __LOCKER_H__

#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
 
#ifdef __cplusplus
extern "C"
{
#endif

    /* 锁状态枚举 */
    typedef enum
    {
        LOCK_OK = 0,
        LOCK_TIMEOUT = -1,
        LOCK_FAILED = -2
    } LOCK_STATUS;

    /* 事件状态枚举 */
    typedef enum
    {
        EVENT_FAILED = 0,
        EVENT_OK = 1,
        EVENT_TIMEOUT = 2
    } EVENT_STATUS;

    /* 互斥锁结构 */
    typedef struct
    {
        pthread_mutex_t mutex;
    } Locker;

    /* 条件变量事件结构 */
    typedef struct
    {
        pthread_cond_t cond_var;
        pthread_condattr_t cond_cattr;
        pthread_mutex_t cond_locker;
        int is_signalled;
        int is_auto_reset;
    } Event;

    /**
     * @brief 初始化互斥锁
     * @param locker 锁指针
     * @return 0成功, -1失败
     */
    int locker_init(Locker *locker);

    /**
     * @brief 销毁互斥锁
     * @param locker 锁指针
     */
    void locker_destroy(Locker *locker);

    /**
     * @brief 加锁
     * @param locker 锁指针
     * @param timeout 超时时间(毫秒), 0xFFFFFFFF表示永久等待
     * @return LOCK_STATUS 锁状态
     */
    LOCK_STATUS locker_lock(Locker *locker, unsigned long timeout);

    /**
     * @brief 解锁
     * @param locker 锁指针
     */
    void locker_unlock(Locker *locker);

    /**
     * @brief 初始化事件
     * @param event 事件指针
     * @param is_auto_reset 是否自动重置
     * @param is_signal 初始信号状态
     * @return 0成功, -1失败
     */
    int event_init(Event *event, int is_auto_reset, int is_signal);

    /**
     * @brief 销毁事件
     * @param event 事件指针
     */
    void event_destroy(Event *event);

    /**
     * @brief 设置事件信号
     * @param event 事件指针
     * @param is_signal 信号状态
     */
    void event_set(Event *event, int is_signal);

    /**
     * @brief 等待事件信号
     * @param event 事件指针
     * @param timeout 超时时间(毫秒), 0xFFFFFFFF表示永久等待
     * @return EVENT_STATUS 事件状态
     */
    EVENT_STATUS event_wait(Event *event, unsigned long timeout);

#ifdef __cplusplus
}
#endif

#endif /* __LOCKER_H__ */
