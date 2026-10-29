/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-31 14:45:00
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-31 16:00:02
 * @FilePath: /OpenHarmony/unionpi_whale_demo/udp_server/include/main.h
 * @Description: UDP服务器程序头文件
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
#define PROGRAM_NAME "UDP Server"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 5

// 全局变量
extern volatile int g_running;

// 客户端信息结构体
typedef struct
{
    struct sockaddr_in addr;
    socklen_t addr_len;
    time_t last_seen;
    int active;
} ClientInfo;

// 函数声明
void UdpPrintUsage(const char *program_name);
int UdpParseArgs(int argc, char *argv[], int *port);
void UdpPrintDeviceIP(void);
void UdpSignalHandler(int sig);
void *UdpSendMessage(void *arg);
void *UdpReceiveMessage(void *arg);

#endif // MAIN_H