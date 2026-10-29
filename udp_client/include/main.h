/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-31 14:50:00
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-31 15:59:53
 * @FilePath: /OpenHarmony/unionpi_whale_demo/udp_client/include/main.h
 * @Description: UDP客户端程序头文件
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
#define PROGRAM_NAME "UDP Client"
#define BUFFER_SIZE 1024
#define MAX_IP_LEN 16

// 全局变量
extern volatile int g_exit_flag;

// 函数声明
void UdpPrintUsage(const char *program_name);
int UdpParseArgs(int argc, char *argv[], int *port, char *server_ip);
void UdpPrintDeviceIP(void);
void UdpSignalHandler(int sig);
void *UdpSendMessage(void *arg);
void *UdpReceiveMessage(void *arg);

// 传递给线程的参数结构
typedef struct
{
    int sock;
    struct sockaddr_in server_addr;
    socklen_t server_addr_len;
} ThreadArgs;

#endif // MAIN_H