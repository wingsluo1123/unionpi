/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-30 17:33:27
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-30 17:35:25
 * @FilePath: /OpenHarmony/unionpi_whale_demo/pwm_motor/src/main.c
 * @Description: PWM 驱动电机案例源文件
 */

#include "../include/main.h"

// 初始占空比，对应舵机0度位置(0.5ms)
#define INIT_DUTY 500000

// 最大占空比，对应舵机180度位置(2.5ms)
#define MAX_DUTY 2500000

/**
 * @brief 打印使用帮助信息
 */
void print_usage(const char *program_name)
{
    printf("[%s] Usage: %s --pwm-channel <channel_number>\n", __func__, program_name);
    printf("[%s] Supported PWM channels: 0, 1\n", __func__);
    printf("[%s] Example: %s --pwm-channel 0\n", __func__, program_name);
}

/**
 * @brief 验证PWM通道号并返回对应的通道值
 *
 * @param channel 用户输入的PWM通道号
 * @return 成功返回PWM通道值，失败返回-1
 */
int validate_pwm_channel(int channel)
{
    switch (channel)
    {
    case 0:
        return PWM0;
    case 1:
        return PWM1;
    default:
        return -1;
    }
}

/**
 * @brief 解析命令行参数
 *
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 成功返回PWM通道值，失败返回-1
 */
int parse_arguments(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("[%s] Error: Invalid number of arguments\n", __func__);
        print_usage(argv[0]);
        return -1;
    }

    if (strcmp(argv[1], "--pwm-channel") != 0)
    {
        printf("[%s] Error: Invalid argument '%s'\n", __func__, argv[1]);
        print_usage(argv[0]);
        return -1;
    }

    int channel = atoi(argv[2]);
    int pwm_channel = validate_pwm_channel(channel);

    if (pwm_channel == -1)
    {
        printf("[%s] Error: Invalid PWM channel '%d'. Only channels 0, 1 are supported\n", __func__, channel);
        print_usage(argv[0]);
        return -1;
    }

    return pwm_channel;
}

/**
 * @brief 计算并验证最终占空比值
 *
 * 确保计算出的占空比值在有效范围内，避免超出舵机可接受范围
 *
 * @param pwm_duty 输入的PWM占空比值
 * @param final_duty 计算后的最终占空比值
 * @return 返回验证后的实际占空比值
 */
int get_final_duty(int pwm_duty, int final_duty)
{
    int real_duty = 0;
    // 判断最终转到的角度是否超过溢出值，输出实际转到的角度
    if (final_duty < INIT_DUTY)
    {
        real_duty = INIT_DUTY;
        printf("[%s] Beyond the angle of rotation\n", __func__);
    }
    else if (final_duty > MAX_DUTY)
    {
        real_duty = MAX_DUTY;
        printf("[%s] Beyond the angle of rotation\n", __func__);
    }
    else
    {
        real_duty = final_duty;
    }
    return real_duty;
}

/**
 * @brief 主函数入口
 *
 * 程序执行的起始点，用于控制舵机角度循环变化
 *
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组，必须包含--pwm-channel和PWM通道号
 * @return 返回程序执行状态码，正常退出返回0
 */
int main(int argc, char **argv)
{
    // 解析命令行参数
    int pwmChannel = parse_arguments(argc, argv);
    if (pwmChannel == -1)
    {
        return PWM_ERR;
    }

    printf("[%s] Using PWM channel: %d\n", __func__, pwmChannel);

    int isEnable = PWM_IS_ENABLED;
    int angle = 0;
    int duty_cycle = 0;

    // 初始化 PWM
    printf("[%s] Initializing PWM channel %d...\n", __func__, pwmChannel);
    init_pwm(pwmChannel);

    printf("[%s] Setting PWM period to %d ns (20ms)...\n", __func__, PERIOD);
    set_pwm_period(pwmChannel, PERIOD); // 20ms 周期

    printf("[%s] Enabling PWM output...\n", __func__);
    set_pwm_enable(pwmChannel, isEnable);

    int direction = 1; // 1: 增加角度, -1: 减少角度

    printf("[%s] PWM Demo Start!\n", __func__);

    while (1)
    {
        // 计算占空比：INIT_DUTY + (angle / 180.0) * (MAX_DUTY - INIT_DUTY)
        duty_cycle = INIT_DUTY + (int)(angle / 180.0 * (MAX_DUTY - INIT_DUTY));
        set_pwm_dutyCycle(pwmChannel, duty_cycle);

        printf("[%s] Current angle: %d degrees, duty_cycle: %d ns\n", __func__, angle, duty_cycle);

        // 根据方向设置步长和延迟：正向阶段性（10度/秒），反向平滑（1度/0.1秒）
        int step = (direction == 1) ? 10 : 1;
        int delay_us = (direction == 1) ? 1000000 : 100000; // 1秒 vs 0.1秒

        // 更新角度
        angle += direction * step;
        if (angle >= 180)
        {
            printf("[%s] Reached maximum angle, starting to decrease\n", __func__);
            direction = -1; // 开始减少
        }
        else if (angle <= 0)
        {
            printf("[%s] Reached minimum angle, starting to increase\n", __func__);
            direction = 1; // 开始增加
        }

        // 延迟
        usleep(delay_us);
    }

    return 0;
}