/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-30 17:23:58
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-30 18:37:50
 * @FilePath: /OpenHarmony/unionpi_whale_demo/pwm_motor/include/pwm_base.h
 * @Description: PWM 驱动接口定义文件
 */

#ifndef __PWM_H__
#define __PWM_H__

// 错误码定义
#define PWM_ERR (-1)
#define PWM_WRONOG_CHANNEL (-2)
#define PWM_FILE_NOT_EXIST (-3)

// PWM使能状态定义
#define PWM_NOT_ENABLED 0
#define PWM_IS_ENABLED 1

// PWM极性定义
#define PWM_POLARITY_NORMAL 0
#define PWM_POLARITY_INVERSED 1

// PWM通道定义
#define PWM0 0
#define PWM1 1

// PWM引脚目录定义
#define PWM0_PEX "/sys/class/pwm/pwmchip0"
#define PWM1_PEX "/sys/class/pwm/pwmchip1"

#ifdef __cplusplus
extern "C"
{
#endif
    /**
     * @brief 初始化PWM引脚
     *
     * 初始化指定的PWM通道，生成对应的引脚目录
     *
     * @param pwmChannel PWM通道号
     * @return 成功返回0，失败返回错误码
     */
    int init_pwm(int pwmChannel);

    /**
     * @brief 设置PWM周期
     *
     * 设置指定PWM通道的周期值
     *
     * @param pwmChannel PWM通道号
     * @param period 周期值(单位:纳秒)
     * @return 成功返回0，失败返回错误码
     */
    int set_pwm_period(int pwmChannel, int period);

    /**
     * @brief 设置PWM占空比
     *
     * 设置指定PWM通道在一个周期内的高电平时间
     *
     * @param pwmChannel PWM通道号
     * @param dutyCycle 高电平时间值(单位:纳秒)
     * @return 成功返回0，失败返回错误码
     */
    int set_pwm_dutyCycle(int pwmChannel, int dutyCycle);

    /**
     * @brief 设置PWM极性
     *
     * 设置指定PWM通道的输出极性
     *
     * @param pwmChannel PWM通道号
     * @param polarity 极性值
     * @return 成功返回0，失败返回错误码
     */
    int set_pwm_polarity(int pwmChannel, int polarity);

    /**
     * @brief 使能PWM输出
     *
     * 打开指定PWM通道，使其开始输出PWM信号
     *
     * @param pwmChannel PWM通道号
     * @param isEnable 使能状态
     * @return 成功返回0，失败返回错误码
     */
    int set_pwm_enable(int pwmChannel, int isEnable);

    /**
     * @brief 获取PWM周期值
     *
     * 获取指定PWM通道的周期值
     *
     * @param pwmChannel PWM通道号
     * @return 成功返回周期值，失败返回错误码
     */
    int get_pwm_period(int pwmChannel);

    /**
     * @brief 获取PWM占空比值
     *
     * 获取指定PWM通道在一个周期内的高电平时间
     *
     * @param pwmChannel PWM通道号
     * @return 成功返回占空比值，失败返回错误码
     */
    int get_pwm_dutyCycle(int pwmChannel);

    /**
     * @brief 获取PWM极性
     *
     * 获取指定PWM通道的输出极性设置
     *
     * @param pwmChannel PWM通道号
     * @return 成功返回极性值，失败返回错误码
     */
    int get_pwm_polarity(int pwmChannel);

    /**
     * @brief 检查PWM使能状态
     *
     * 检查指定PWM通道是否已使能
     *
     * @param pwmChannel PWM通道号
     * @return 成功返回使能状态，失败返回错误码
     */
    int is_pwm_enabled(int pwmChannel);

#ifdef __cplusplus
}
#endif

#endif
