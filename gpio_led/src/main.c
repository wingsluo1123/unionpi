/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-30 16:50:34
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-11-27 17:01:29
 * @FilePath: /OpenHarmony/unionpi_whale_demo/gpio_led/src/main.c
 * @Description: GPIO LED灯控制示例代码实现文件
 */

#include "../include/main.h"
#include "../include/gpio_base.h"
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * @brief 打印使用帮助信息
 */
void print_usage(const char *program_name)
{
    printf("[%s] Usage: %s --gpio-port <port_number>\n", __func__, program_name);
    printf("[%s] Supported GPIO ports: 22, 26, 27\n", __func__);
    printf("[%s] Example: %s --gpio-port 26\n", __func__, program_name);
}

/**
 * @brief 验证GPIO端口号并返回对应的内部编号
 *
 * @param port 用户输入的GPIO端口号
 * @return 成功返回内部GPIO编号，失败返回-1
 */
int validate_gpio_port(int port)
{
    switch (port)
    {
    case 0:
        return GPIO_00;
    case 1:
        return GPIO_01;
    case 2:
        return GPIO_02;
    case 3:
        return GPIO_03;
    case 4:
        return GPIO_04;
    case 5:
        return GPIO_05;
    case 6:
        return GPIO_06;
    case 7:
        return GPIO_07;
    case 8:
        return GPIO_08;
    case 9:
        return GPIO_09;
    case 10:
        return GPIO_10;
    case 11:
        return GPIO_11;
    case 12:
        return GPIO_12;
    case 13:
        return GPIO_13;
    case 14:
        return GPIO_14;
    case 15:
        return GPIO_15;
    case 16:
        return GPIO_16;
    case 18:
        return GPIO_18;
    case 19:
        return GPIO_19;
    case 20:
        return GPIO_20;
    case 21:
        return GPIO_21;
    case 22:
        return GPIO_22;
    case 23:
        return GPIO_23;
    case 24:
        return GPIO_24;
    case 25:
        return GPIO_25;
    case 26:
        return GPIO_26;
    case 27:
        return GPIO_27;
    default:
        return -1;
    }
}

/**
 * @brief 解析命令行参数
 *
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 成功返回GPIO内部编号，失败返回-1
 */
int parse_arguments(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("[%s] Error: Invalid number of arguments\n", __func__);
        print_usage(argv[0]);
        return -1;
    }

    if (strcmp(argv[1], "--gpio-port") != 0)
    {
        printf("[%s] Error: Invalid argument '%s'\n", __func__, argv[1]);
        print_usage(argv[0]);
        return -1;
    }

    int port = atoi(argv[2]);
    int gpio_num = validate_gpio_port(port);

    if (gpio_num == -1)
    {
        printf("[%s] Error: Invalid GPIO port '%d'. Only ports 22, 26, 27 are supported\n", __func__, port);
        print_usage(argv[0]);
        return -1;
    }

    return gpio_num;
}

/**
 * @brief 主函数入口
 *
 * 程序执行的起始点，用于控制GPIO引脚连接的LED灯亮灭。
 *
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组，必须包含--gpio-port和GPIO端口号
 * @return 返回程序执行状态码，正常退出返回0
 */
int main(int argc, char **argv)
{
    // 解析命令行参数
    int gpioNum = parse_arguments(argc, argv);
    if (gpioNum == -1)
    {
        return GPIO_ERR;
    }

    printf("[%s] Using GPIO port: %d (internal number: %d)\n", __func__, (gpioNum == GPIO_22) ? 22 : (gpioNum == GPIO_26) ? 26 : 27, gpioNum);

    // 将getchar设置为非阻塞
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL, 0) | O_NONBLOCK);

    // 定义其他参数
    int bExport = GPIO_EXPORTED;
    int direction = GPIO_DIRECTION_OUT;
    int getValue = -1;

    // 判断gpio口是否已经导出，如未导出则执行对应函数
    GPIO_IsExport(gpioNum, &getValue);
    if (getValue == GPIO_NOT_EXPORT)
    {
        printf("[%s] GPIO port not exported, exporting now...\n", __func__);
        GPIO_Export(gpioNum, bExport);
    }
    else
    {
        printf("[%s] GPIO port already exported\n", __func__);
    }

    // 设置gpio口为输出模式
    printf("[%s] Setting GPIO direction to output\n", __func__);
    GPIO_SetDirection(gpioNum, direction);

    // 控制LED灯的亮灭
    printf("[%s] Starting LED control...\n", __func__);
    printf("[%s] Press Enter key to stop LED control.\n", __func__);
    while (1)
    {
        // 检测是否按下Enter键，如果按下则退出循环
        if (getchar() == '\n')
        {
            break;
        }

        // 设置gpio口为高电平，点亮LED灯
        GPIO_SetValue(gpioNum, GPIO_HIGH_LEVE);
        usleep(500000); // 延时500毫秒

        // 获取对应gpio口的模式并打印
        GPIO_GetDirection(gpioNum, &getValue);
        printf("[%s] gpioNum:[%d], direction:[%d]\n", __func__, gpioNum, getValue);

        // 获取对应gpio口的电平值并打印
        GPIO_GetValue(gpioNum, &getValue);
        printf("[%s] gpioNum:[%d], value:[%d] (LED ON)\n", __func__, gpioNum, getValue);

        // 设置gpio口为低电平，熄灭LED灯
        GPIO_SetValue(gpioNum, GPIO_LOW_LEVE);
        usleep(500000); // 延时500毫秒

        // 获取对应gpio口的模式并打印
        GPIO_GetDirection(gpioNum, &getValue);
        printf("[%s] gpioNum:[%d], direction:[%d]\n", __func__, gpioNum, getValue);

        // 获取对应gpio口的电平值并打印
        GPIO_GetValue(gpioNum, &getValue);
        printf("[%s] gpioNum:[%d], value:[%d] (LED OFF)\n", __func__, gpioNum, getValue);
    }

    printf("[%s] LED control stopped.\n", __func__);

    return 0;
}