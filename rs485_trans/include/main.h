/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-31 14:08:05
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-11-11 19:38:04
 * @FilePath: /OpenHarmony/unionpi_whale_demo/rs485_trans/include/main.h
 * @Description: RS485通信程序主程序头文件
 */

#ifndef MAIN_H
#define MAIN_H
 
#include "rs485_base.h"
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// 程序配置
#define PROGRAM_NAME "RS485 Communication Tool"
#define MAX_MESSAGE_LENGTH 1024
#define DEFAULT_BAUD_RATE B9600
#define RECEIVE_TIMEOUT_MS 1000

// 全局变量
extern volatile int g_running;
extern Rs485Handle *g_handle;

// 函数声明
void Rs485SignalHandler(int sig);
void Rs485PrintUsage(const char *program_name);
int Rs485ParseArgs(int argc, char *argv[], char **device, char **baud_str);
void *Rs485ReceiveThread(void *arg);

#endif // MAIN_H