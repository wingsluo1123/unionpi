/*
 * @Author: UnionPi Whale Development Team
 * @Date: 2025-10-31 19:11:01
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-31 22:10:01
 * @FilePath: /OpenHarmony/unionpi_whale_demo/uart_chassis/include/chassis_driver.h
 * @Description: 底盘控制驱动接口
 */

#ifndef __CHASSIS_DRIVER_H__
#define __CHASSIS_DRIVER_H__

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // 底盘信息结构体
    typedef struct
    {
        // BMS信息
        struct
        {
            uint64_t timestamp;        // 时间戳
            int16_t volume_percentage; // 电池容量%
            float current_a;           // 电池电流A
            float voltage_v;           // 电池电压V
        } bms;

        // 里程计信息
        struct
        {
            int32_t right_back_odometer;  // 右后里程
            int32_t right_front_odometer; // 右前里程
            int32_t left_front_odometer;  // 左前里程
            int32_t left_back_odometer;   // 左后里程
        } odometer;

        // 速度信息
        struct
        {
            int32_t right_back_speed;  // 右后速度
            int32_t right_front_speed; // 右前速度
            int32_t left_front_speed;  // 左前速度
            int32_t left_back_speed;   // 左后速度
        } speed;
    } chassis_info_t;

    // 底盘控制状态结构体
    typedef struct
    {
        int fd;                   // 串口文件描述符
        int16_t target_linear;    // 目标线速度
        int16_t target_angular;   // 目标角速度
        volatile int running;     // 控制线程运行标志
        pthread_t control_thread; // 控制线程ID
        pthread_mutex_t mutex;    // 线程同步互斥锁
    } chassis_control_t;

    /**
     * @brief 初始化底盘驱动
     * @param device 串口设备路径
     * @return 成功返回文件描述符，失败返回-1
     */
    int chassis_init(const char *device);

    /**
     * @brief 关闭底盘驱动
     * @param fd 串口文件描述符
     */
    void chassis_close(int fd);

    /**
     * @brief 使能底盘
     * @param fd 串口文件描述符
     * @return 成功返回0，失败返回-1
     */
    int chassis_power_on(int fd);

    /**
     * @brief 失能底盘
     * @param fd 串口文件描述符
     * @return 成功返回0，失败返回-1
     */
    int chassis_power_off(int fd);

    /**
     * @brief 设置底盘速度
     * @param fd 串口文件描述符
     * @param linear_x 线速度 (mm/s)
     * @param angular_z 角速度 (rad/s)
     * @return 成功返回0，失败返回-1
     */
    int chassis_set_velocity(int fd, int32_t linear_x, float angular_z);

    /**
     * @brief 设置底盘速度（原始值）
     * @param fd 串口文件描述符
     * @param linear 线速度 (-1000 ~ 1000 mm/s)
     * @param angular 角速度 (-1000 ~ 1000 centidegrees/s)
     * @return 成功返回0，失败返回-1
     */
    int chassis_set_velocity_raw(int fd, int16_t linear, int16_t angular);

    /**
     * @brief 停止底盘运动
     * @param fd 串口文件描述符
     * @return 成功返回0，失败返回-1
     */
    int chassis_stop(int fd);

    /**
     * @brief 读取底盘信息
     * @param fd 串口文件描述符
     * @param info 底盘信息结构体指针
     * @return 成功返回0，失败返回-1
     */
    int chassis_read_info(int fd, chassis_info_t *info);

    /**
     * @brief 切换速度角度模式
     * @param fd 串口文件描述符
     * @return 成功返回0，失败返回-1
     */
    int chassis_switch_speed_angle_mode(int fd);

    /**
     * @brief 启动持续运动控制线程
     * @param fd 串口文件描述符
     * @param linear 线速度 (-1000 ~ 1000 mm/s)
     * @param angular 角速度 (-1000 ~ 1000 centidegrees/s)
     * @return 成功返回控制状态结构体指针，失败返回NULL
     */
    chassis_control_t *chassis_start_continuous_control(int fd, int16_t linear, int16_t angular);

    /**
     * @brief 更新持续运动控制参数
     * @param control 控制状态结构体指针
     * @param linear 线速度 (-1000 ~ 1000 mm/s)
     * @param angular 角速度 (-1000 ~ 1000 centidegrees/s)
     * @return 成功返回0，失败返回-1
     */
    int chassis_update_continuous_control(chassis_control_t *control, int16_t linear, int16_t angular);

    /**
     * @brief 停止持续运动控制线程
     * @param control 控制状态结构体指针
     */
    void chassis_stop_continuous_control(chassis_control_t *control);

#ifdef __cplusplus
}
#endif

#endif // __CHASSIS_DRIVER_H__