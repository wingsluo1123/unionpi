/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-30 16:36:36
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-11-27 16:50:19
 * @FilePath: /OpenHarmony/unionpi_whale_demo/gpio_led/include/gpio_base.h
 * @Description: GPIO 驱动接口定义文件
 */

#ifndef GPIO_BASE_H
#define GPIO_BASE_H

// GPIO导出和取消导出文件路径
#define GPIO_EXPORT "/sys/class/gpio/export"
#define GPIO_UNEXPORT "/sys/class/gpio/unexport"
#define GPIO_PEX "/sys/class/gpio/gpio"

// GPIO引脚定义
#define GPIO_00 71   // GPIO1,RK_PD7
#define GPIO_01 62   // GPIO1,RK_PD6
#define GPIO_02 6    // GPIO0,RK_PA6
#define GPIO_03 15   // GPIO0,RK_PB7
#define GPIO_04 129  // GPIO4,RK_PA1
#define GPIO_05 135  // GPIO4,RK_PA7
#define GPIO_06 134  // GPIO4,RK_PA6
#define GPIO_07 45   // GPIO1,RK_PB5
#define GPIO_08 123  // GPIO1,RK_PB4
#define GPIO_09 41   // GPIO1,RK_PB1
#define GPIO_10 42   // GPIO1,RK_PB2
#define GPIO_11 43   // GPIO1,RK_PB3
#define GPIO_12 34   // GPIO1,RK_PA2
#define GPIO_13 35   // GPIO1,RK_PA3
#define GPIO_14 112  // GPIO3,RK_PC0
#define GPIO_15 113  // GPIO3,RK_PC1
#define GPIO_16 137  // GPIO4,RK_PB1
#define GPIO_18 128  // GPIO4,RK_PA0
#define GPIO_19 130  // GPIO4,RK_PA2
#define GPIO_20 136  // GPIO4,RK_PB0
#define GPIO_21 140  // GPIO4,RK_PB4
#define GPIO_22 146  // GPIO4,RK_PC2
#define GPIO_23 36   // GPIO1,RK_PA4
#define GPIO_24 106  // GPIO3,RK_PB2
#define GPIO_25 221  // GPIO3,RK_PD5
#define GPIO_26 147  // GPIO4,RK_PC3
#define GPIO_27 150  // GPIO4,RK_PC6

// 方向定义
#define GPIO_DIRECTION_IN 0
#define GPIO_DIRECTION_OUT 1

// 导出状态定义
#define GPIO_NOT_EXPORT 0
#define GPIO_EXPORTED 1

// 错误码定义
#define GPIO_ERR (-1)
#define GPIO_NOT_EXPORT_ERR (-2)

// 电平值定义
#define GPIO_LOW_LEVE 0
#define GPIO_HIGH_LEVE 1

/**
 * @brief 导出或取消导出GPIO引脚
 *
 * 通过向/sys/class/gpio/export或/sys/class/gpio/unexport写入GPIO编号来导出或取消导出GPIO引脚
 *
 * @param gpioNum GPIO引脚编号
 * @param bExport 导出标志，0表示取消导出，1表示导出
 * @return 成功返回0，失败返回错误码
 */
int GPIO_Export(int gpioNum, int bExport);

/**
 * @brief 设置GPIO引脚方向
 *
 * 设置指定GPIO引脚为输入或输出模式
 *
 * @param gpioNum GPIO引脚编号
 * @param direction 方向，0表示输入，1表示输出
 * @return 成功返回0，失败返回错误码
 */
int GPIO_SetDirection(int gpioNum, int direction);

/**
 * @brief 设置GPIO引脚电平值
 *
 * 设置指定GPIO引脚的电平值（高电平或低电平）
 *
 * @param gpioNum GPIO引脚编号
 * @param value 电平值，0表示低电平，1表示高电平
 * @return 成功返回0，失败返回错误码
 */
int GPIO_SetValue(int gpioNum, int value);

/**
 * @brief 检查GPIO引脚是否已导出
 *
 * 检查指定GPIO引脚是否已经被导出
 *
 * @param gpioNum GPIO引脚编号
 * @param *value 导出状态指针，0表示未导出，1表示已导出
 * @return 成功返回0，失败返回错误码
 */
int GPIO_IsExport(int gpioNum, int *value);

/**
 * @brief 获取GPIO引脚方向
 *
 * 获取指定GPIO引脚的方向设置（输入或输出）
 *
 * @param gpioNum GPIO引脚编号
 * @param *value 方向值指针，0表示输入，1表示输出
 * @return 成功返回0，失败返回错误码
 */
int GPIO_GetDirection(int gpioNum, int *value);

/**
 * @brief 获取GPIO引脚电平值
 *
 * 获取指定GPIO引脚的当前电平值
 *
 * @param gpioNum GPIO引脚编号
 * @param *value 电平值指针，0表示低电平，1表示高电平
 * @return 成功返回0，失败返回错误码
 */
int GPIO_GetValue(int gpioNum, int *value);

#endif
