/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-31 14:45:00
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-31 16:00:06
 * @FilePath: /OpenHarmony/unionpi_whale_demo/udp_server/src/main.c
 * @Description: UDP服务器程序实现
 */

#include "../include/main.h"
#include <time.h>

// 全局变量
volatile int g_running = 1;
static int g_server_fd = -1;
static ClientInfo g_current_client = {0};
static int g_has_client = 0;

/**
 * @brief 信号处理函数
 * @param sig 信号值
 */
void UdpSignalHandler(int sig)
{
    printf("\n[%s] Received signal %d, shutting down server...\n", __func__, sig);
    g_running = 0;
}

/**
 * @brief 打印程序使用说明
 * @param program_name 程序名
 */
void UdpPrintUsage(const char *program_name)
{
    printf("[%s] Usage: %s --udp-port <port>\n", __func__, program_name);
    printf("[%s] Example: %s --udp-port 8888\n", __func__, program_name);
    printf("[%s] Default port range: 1024-65535\n", __func__);
}

/**
 * @brief 解析命令行参数
 * @param argc 参数数量
 * @param argv 参数数组
 * @param port 端口号指针
 * @return int 成功返回0，失败返回-1
 */
int UdpParseArgs(int argc, char *argv[], int *port)
{
    printf("[%s] Parsing command line arguments\n", __func__);

    *port = 0;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--udp-port") == 0)
        {
            if (i + 1 < argc)
            {
                *port = atoi(argv[++i]);
                if (*port <= 0 || *port > 65535)
                {
                    printf("[%s] Error: Invalid port number %d. Port must be 1-65535\n", __func__, *port);
                    return -1;
                }
                printf("[%s] UDP port set to: %d\n", __func__, *port);
            }
            else
            {
                printf("[%s] Error: --udp-port requires a port number\n", __func__);
                return -1;
            }
        }
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            UdpPrintUsage(argv[0]);
            return 1;
        }
        else
        {
            printf("[%s] Error: Unknown argument: %s\n", __func__, argv[i]);
            return -1;
        }
    }

    if (*port == 0)
    {
        printf("[%s] Error: --udp-port is required\n", __func__);
        return -1;
    }

    return 0;
}

/**
 * @brief 打印设备IP地址
 *
 * 获取并打印设备所有网络接口的IPv4地址信息
 */
void UdpPrintDeviceIP(void)
{
    printf("[%s] Getting device IP addresses\n", __func__);

    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    // 获取网络接口地址列表
    if (getifaddrs(&ifaddr) == -1)
    {
        printf("[%s] Failed to get network interfaces: %s\n", __func__, strerror(errno));
        return;
    }

    printf("[%s] Available network interfaces:\n", __func__);

    // 遍历所有网络接口
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;

        // 检查是否为IPv4地址
        if (family == AF_INET)
        {
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0)
            {
                printf("[%s] getnameinfo() failed: %s\n", __func__, gai_strerror(s));
                continue;
            }
            // 打印接口名称和IP地址
            printf("[%s] Interface: %s, IP Address: %s\n", __func__, ifa->ifa_name, host);
        }
    }
    freeifaddrs(ifaddr);
}

/**
 * @brief 发送消息线程函数
 *
 * 负责从标准输入读取用户输入并发送到客户端
 *
 * @param arg 套接字描述符指针
 * @return 线程退出状态
 */
void *UdpSendMessage(void *arg)
{
    int sock = *(int *)arg;
    char send_buffer[BUFFER_SIZE] = {0};

    printf("[%s] Send message thread started\n", __func__);
    printf("[%s] Enter message to send to client (type 'exit' to quit):\n", __func__);
    fflush(stdout);

    while (g_running)
    {
        if (fgets(send_buffer, sizeof(send_buffer), stdin) == NULL)
        {
            printf("[%s] Input stream closed\n", __func__);
            break;
        }

        if (!g_running)
        {
            break;
        }

        // 移除换行符
        send_buffer[strlen(send_buffer) - 1] = '\0';

        // 添加空输入检查
        if (strlen(send_buffer) == 0)
        {
            continue; // 跳过空输入，不发送
        }

        // 如果用户输入了 "exit"，则设置退出标志
        if (!strcmp(send_buffer, "exit") || !strcmp(send_buffer, "quit"))
        {
            g_running = 0;
            printf("[%s] Shutting down server...\n", __func__);
            break;
        }

        // 检查是否有客户端连接
        if (!g_has_client)
        {
            printf("[%s] No client connected. Message not sent: {%s}\n", __func__, send_buffer);
            memset(send_buffer, 0, sizeof(send_buffer));
            continue;
        }

        // 发送UDP数据包到最近的客户端
        ssize_t send_len = sendto(sock, send_buffer, strlen(send_buffer), 0, (struct sockaddr *)&g_current_client.addr, g_current_client.addr_len);
        if (send_len < 0)
        {
            printf("[%s] Send failed: %s\n", __func__, strerror(errno));
        }
        else
        {
            printf("[%s] Send to %s:%d: {%s}\n", __func__, inet_ntoa(g_current_client.addr.sin_addr), ntohs(g_current_client.addr.sin_port), send_buffer);
        }

        memset(send_buffer, 0, sizeof(send_buffer));
    }

    printf("[%s] Send message thread exiting\n", __func__);
    pthread_exit(NULL);
}

/**
 * @brief 接收消息线程函数
 *
 * 负责接收来自客户端的UDP消息并显示
 *
 * @param arg 套接字描述符指针
 * @return 线程退出状态
 */
void *UdpReceiveMessage(void *arg)
{
    int sock = *(int *)arg;
    char recv_buffer[BUFFER_SIZE] = {0};
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    printf("[%s] Receive message thread started\n", __func__);

    while (g_running)
    {
        // 接收UDP数据包
        ssize_t recv_len = recvfrom(sock, recv_buffer, sizeof(recv_buffer) - 1, 0, (struct sockaddr *)&client_addr, &client_addr_len);
        if (recv_len < 0)
        {
            if (g_running)
            { // 只有在运行状态下才报告错误
                printf("[%s] Receive failed: %s\n", __func__, strerror(errno));
            }
            break;
        }

        if (recv_len == 0)
        {
            printf("[%s] Received empty packet\n", __func__);
            continue;
        }

        recv_buffer[recv_len] = '\0'; // 确保字符串结束

        // 更新当前客户端信息
        g_current_client.addr = client_addr;
        g_current_client.addr_len = client_addr_len;
        g_current_client.last_seen = time(NULL);
        g_current_client.active = 1;
        g_has_client = 1;

        printf("[%s] Receive from %s:%d: {%s}\n", __func__, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), recv_buffer);

        memset(recv_buffer, 0, sizeof(recv_buffer));
    }

    printf("[%s] Receive message thread exiting\n", __func__);
    pthread_exit(NULL);
}

/**
 * @brief 主函数
 * @param argc 参数数量
 * @param argv 参数数组
 * @return int 程序退出码
 */
int main(int argc, char *argv[])
{
    printf("[%s] Starting %s\n", __func__, PROGRAM_NAME);

    // 设置信号处理
    signal(SIGINT, UdpSignalHandler);
    signal(SIGTERM, UdpSignalHandler);

    int port;

    // 解析命令行参数
    int parse_result = UdpParseArgs(argc, argv, &port);
    if (parse_result != 0)
    {
        if (parse_result < 0)
        {
            UdpPrintUsage(argv[0]);
        }
        return (parse_result < 0) ? 1 : 0;
    }

    // 打印设备IP地址
    UdpPrintDeviceIP();

    struct sockaddr_in server_addr;

    // 创建UDP套接字
    g_server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_server_fd < 0)
    {
        printf("[%s] Socket creation failed: %s\n", __func__, strerror(errno));
        return 1;
    }

    printf("[%s] UDP socket created successfully\n", __func__);

    // 设置套接字选项
    int opt = 1;
    if (setsockopt(g_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        printf("[%s] Setsockopt failed: %s\n", __func__, strerror(errno));
        close(g_server_fd);
        return 1;
    }

    printf("[%s] Socket options set successfully\n", __func__);

    // 配置服务器地址结构
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // 绑定套接字到地址
    if (bind(g_server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("[%s] Bind failed: %s\n", __func__, strerror(errno));
        close(g_server_fd);
        return 1;
    }

    printf("[%s] Socket bound to port %d\n", __func__, port);
    printf("[%s] UDP server is listening on port %d...\n", __func__, port);

    pthread_t send_thread, receive_thread;

    // 创建发送线程
    if (pthread_create(&send_thread, NULL, UdpSendMessage, &g_server_fd) != 0)
    {
        printf("[%s] Failed to create send thread: %s\n", __func__, strerror(errno));
        close(g_server_fd);
        return 1;
    }

    // 创建接收线程
    if (pthread_create(&receive_thread, NULL, UdpReceiveMessage, &g_server_fd) != 0)
    {
        printf("[%s] Failed to create receive thread: %s\n", __func__, strerror(errno));
        close(g_server_fd);
        return 1;
    }

    printf("[%s] Server threads created successfully\n", __func__);

    // 等待接收线程结束
    pthread_join(receive_thread, NULL);

    // 关闭套接字
    printf("[%s] Closing server socket\n", __func__);
    close(g_server_fd);

    // 等待发送线程结束
    pthread_join(send_thread, NULL);

    printf("[%s] %s terminated successfully\n", __func__, PROGRAM_NAME);
    return 0;
}