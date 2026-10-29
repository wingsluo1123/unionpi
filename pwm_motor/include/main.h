/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-30 17:32:12
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-30 18:37:55
 * @FilePath: /OpenHarmony/unionpi_whale_demo/pwm_motor/include/main.h
 * @Description: PWM驱动电机示例程序头文件
 */

#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pwm_base.h"

// 舵机参数
#define INIT_DUTY 500000 // 0.5ms, 对应 0 度
#define MAX_DUTY 2500000 // 2.5ms, 对应 180 度
#define PERIOD 20000000  // 20ms 周期

#endif // __MAIN_H__
