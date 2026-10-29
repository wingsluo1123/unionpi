/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-31 15:00:00
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-31 15:00:00
 * @FilePath: /OpenHarmony/unionpi_whale_demo/spi_ws2812/src/main.c
 * @Description: WS2812 LED控制演示程序主函数
 */

#include "../include/main.h"
#include "../include/ws2812_driver.h"

// 全局变量
static volatile int g_running = 1;
static int g_led_count = 8;

/**
 * @brief 打印使用帮助信息
 */
void print_usage(const char *program_name)
{
    printf("[%s] Usage: %s --spi-port <device_path> --led-num <count>\n", __func__, program_name);
    printf("[%s] Example: %s --spi-port /dev/spidev0.0 --led-num 8\n", __func__, program_name);
    printf("[%s] Example: %s --spi-port /dev/spidev1.0 --led-num 60\n", __func__, program_name);
    printf("[%s] LED count range: 1-300\n", __func__);
}

/**
 * @brief 解析命令行参数
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @param spi_device 输出SPI设备路径
 * @param led_count 输出LED数量
 * @return 成功返回0，失败返回-1
 */
int parse_arguments(int argc, char **argv, const char **spi_device, int *led_count)
{
    if (argc != 5)
    {
        printf("[%s] Error: Invalid number of arguments\n", __func__);
        print_usage(argv[0]);
        return -1;
    }

    const char *spi_port = NULL;
    int led_num = 0;

    // 解析参数
    for (int i = 1; i < argc; i += 2)
    {
        if (i + 1 >= argc)
        {
            printf("[%s] Error: Missing value for argument %s\n", __func__, argv[i]);
            print_usage(argv[0]);
            return -1;
        }

        if (strcmp(argv[i], "--spi-port") == 0)
        {
            spi_port = argv[i + 1];
        }
        else if (strcmp(argv[i], "--led-num") == 0)
        {
            led_num = atoi(argv[i + 1]);
        }
        else
        {
            printf("[%s] Error: Unknown argument '%s'\n", __func__, argv[i]);
            print_usage(argv[0]);
            return -1;
        }
    }

    // 验证参数
    if (spi_port == NULL || strlen(spi_port) == 0)
    {
        printf("[%s] Error: Invalid SPI port specified\n", __func__);
        print_usage(argv[0]);
        return -1;
    }

    if (led_num <= 0 || led_num > 300)
    {
        printf("[%s] Error: Invalid LED count (1-300): %d\n", __func__, led_num);
        print_usage(argv[0]);
        return -1;
    }

    *spi_device = spi_port;
    *led_count = led_num;
    return 0;
}

/**
 * @brief 信号处理函数
 */
void signal_handler(int sig)
{
    printf("\n[%s] Received signal %d, exiting...\n", __func__, sig);
    g_running = 0;
}

/**
 * @brief 演示1: 基本颜色测试
 */
void demo_basic_colors(void)
{
    enum Color colors[] = {RED, GREEN, BLUE, YELLOW, CYAN, MAGENTA, WHITE};
    const char *color_names[] = {"红色", "绿色", "蓝色", "黄色", "青色", "品红", "白色"};
    int num_colors = sizeof(colors) / sizeof(colors[0]);

    for (int i = 0; i < num_colors && g_running; i++)
    {
        printf("[%s] Displaying %s\n", __func__, color_names[i]);
        CRGB color = ColorFromEnum(colors[i]);
        Ws2812SetAllColor(color);
        Ws2812Show();
        sleep(1);
    }

    Ws2812Clear();
    sleep(1);
}

/**
 * @brief 演示2: 彩虹循环效果
 */
void demo_rainbow_cycle(void)
{
    printf("[%s] Rainbow cycling...\n", __func__);
    for (int cycle = 0; cycle < 3 && g_running; cycle++)
    {
        for (int offset = 0; offset < 360 && g_running; offset += 5)
        {
            RainbowEffect(offset);
            Ws2812Show();
            usleep(50000); // 50ms延迟
        }
    }

    Ws2812Clear();
    sleep(1);
}

/**
 * @brief 演示3: 呼吸效果
 */
void demo_breathing_effect(void)
{
    CRGB colors[] = {ColorFromEnum(BLUE), ColorFromEnum(GREEN), ColorFromEnum(RED)};
    const char *color_names[] = {"蓝色", "绿色", "红色"};
    int num_colors = sizeof(colors) / sizeof(colors[0]);

    for (int c = 0; c < num_colors && g_running; c++)
    {
        printf("[%s] %s breathing effect\n", __func__, color_names[c]);

        // 呼吸周期：渐亮 -> 渐暗
        for (int cycle = 0; cycle < 2 && g_running; cycle++)
        {
            // 渐亮
            for (int i = 0; i <= 100 && g_running; i += 2)
            {
                float brightness = i / 100.0;
                BreathingEffect(colors[c], brightness);
                Ws2812Show();
                usleep(30000); // 30ms延迟
            }

            // 渐暗
            for (int i = 100; i >= 0 && g_running; i -= 2)
            {
                float brightness = i / 100.0;
                BreathingEffect(colors[c], brightness);
                Ws2812Show();
                usleep(30000); // 30ms延迟
            }
        }
    }

    Ws2812Clear();
    sleep(1);
}

/**
 * @brief 演示4: 追逐效果
 */
void demo_chase_effect(void)
{
    CRGB colors[] = {ColorFromEnum(RED), ColorFromEnum(GREEN), ColorFromEnum(BLUE)};
    const char *color_names[] = {"红色", "绿色", "蓝色"};
    int num_colors = sizeof(colors) / sizeof(colors[0]);
    int tail_length = 5;

    for (int c = 0; c < num_colors && g_running; c++)
    {
        printf("[%s] %s chase effect\n", __func__, color_names[c]);

        // 两轮追逐
        for (int round = 0; round < 2 && g_running; round++)
        {
            for (int pos = 0; pos < g_led_count && g_running; pos++)
            {
                ChaseEffect(colors[c], pos, tail_length);
                Ws2812Show();
                usleep(80000); // 80ms延迟
            }
        }
    }

    Ws2812Clear();
    sleep(1);
}

/**
 * @brief 演示5: 颜色扫描效果
 */
void demo_color_wipe(void)
{
    CRGB colors[] = {ColorFromEnum(CYAN), ColorFromEnum(MAGENTA), ColorFromEnum(YELLOW)};
    const char *color_names[] = {"青色", "品红", "黄色"};
    int num_colors = sizeof(colors) / sizeof(colors[0]);

    for (int c = 0; c < num_colors && g_running; c++)
    {
        printf("[%s] %s wipe effect\n", __func__, color_names[c]);

        // 正向扫描
        Ws2812Clear();
        for (int i = 0; i < g_led_count && g_running; i++)
        {
            Ws2812SetColor(i, colors[c]);
            Ws2812Show();
            usleep(50000); // 50ms延迟
        }

        sleep(1);

        // 反向清除
        for (int i = g_led_count - 1; i >= 0 && g_running; i--)
        {
            Ws2812SetColor(i, (CRGB){0, 0, 0});
            Ws2812Show();
            usleep(50000); // 50ms延迟
        }
    }

    sleep(1);
}

/**
 * @brief 主函数入口
 *
 * 程序执行的起始点，初始化WS2812并演示各种灯光效果
 *
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组，必须包含--spi-port和--led-num参数
 * @return 返回程序执行状态码，正常退出返回0
 */
int main(int argc, char *argv[])
{
    const char *spi_device = NULL;
    int led_count = 0;

    printf("[%s] WS2812 LED Demo Start\n", __func__);

    // 解析命令行参数
    if (parse_arguments(argc, argv, &spi_device, &led_count) != 0)
    {
        return -1;
    }

    printf("[%s] Using SPI device: %s\n", __func__, spi_device);
    printf("[%s] LED count: %d\n", __func__, led_count);

    // 注册信号处理函数
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("[%s] =======================================\n", __func__);
    printf("[%s]     WS2812 LED Strip Demo\n", __func__);
    printf("[%s] =======================================\n", __func__);
    printf("[%s] Press Ctrl+C to exit\n", __func__);
    printf("[%s] =======================================\n", __func__);

    // 更新全局LED数量
    g_led_count = led_count;

    // 初始化WS2812
    if (Ws2812Init(spi_device, led_count) != WS2812_OK)
    {
        printf("[%s] Error: WS2812 initialization failed\n", __func__);
        return -1;
    }

    // 清除所有LED
    Ws2812Clear();
    sleep(1);

    // 演示循环
    while (g_running)
    {
        printf("[%s] Demo 1: Basic color test\n", __func__);
        demo_basic_colors();
        if (!g_running)
            break;

        printf("[%s] Demo 2: Rainbow cycle effect\n", __func__);
        demo_rainbow_cycle();
        if (!g_running)
            break;

        printf("[%s] Demo 3: Breathing effect\n", __func__);
        demo_breathing_effect();
        if (!g_running)
            break;

        printf("[%s] Demo 4: Chase effect\n", __func__);
        demo_chase_effect();
        if (!g_running)
            break;

        printf("[%s] Demo 5: Color wipe effect\n", __func__);
        demo_color_wipe();
        if (!g_running)
            break;

        printf("[%s] All demos complete, restarting...\n\n", __func__);
    }

    // 清理并退出
    printf("\n[%s] Clearing all LEDs...\n", __func__);
    Ws2812Clear();
    Ws2812Cleanup();

    printf("[%s] Program exited\n", __func__);
    return 0;
}