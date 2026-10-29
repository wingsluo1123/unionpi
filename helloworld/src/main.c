/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-09-22 21:31:54
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-30 16:03:35
 * @Description: helloWorld 示例代码实现文件
 */

#include "../include/main.h"
#include <stdio.h>

/**
 * @brief 主函数入口
 *
 * 程序执行的起始点，调用HelloPrint函数。
 *
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 返回程序执行状态码，正常退出返回0
 */
int main(int argc, char **argv)
{
    printf("========================================\n");
    printf("[%s] This is the helloworld demo\n", __func__);
    printf("========================================\n");

    HelloPrint();
    return 0;
}

/**
 * @brief 打印HelloWorld信息到控制台的具体实现
 *
 * 在控制台输出格式化的"Hello World!"信息，包含装饰性空行。
 */
void HelloPrint()
{
    printf("\n\n");
    printf("\n\t[%s] Hello World!\n", __func__);
    printf("\n\n");
}