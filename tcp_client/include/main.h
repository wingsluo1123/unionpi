/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-31 14:35:00
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-31 15:58:40
 * @FilePath: /OpenHarmony/unionpi_whale_demo/tcp_client/include/main.h
 * @Description: TCP客户端程序头文件
 */

#ifndef MAIN_H
#define MAIN_H

#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// 程序配置
#define PROGRAM_NAME "TCP Client"
#define BUFFER_SIZE 1024
#define MAX_IP_LEN 16

// 全局变量
extern volatile int g_exit_flag;

// 函数声明
void TcpPrintUsage(const char *program_name);
int TcpParseArgs(int argc, char *argv[], int *port, char *server_ip);
void TcpPrintDeviceIP(void);
void TcpSignalHandler(int sig);
void *TcpSendMessage(void *arg);
void *TcpReceiveMessage(void *arg);

#endif // MAIN_H