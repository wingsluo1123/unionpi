/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-31 14:35:00
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-31 15:58:44
 * @FilePath: /OpenHarmony/unionpi_whale_demo/tcp_client/src/main.c
 * @Description: TCP客户端程序实现
 */

#include "../include/main.h"

// 全局变量
volatile int g_exit_flag = 0;

/**
 * @brief 信号处理函数
 * @param sig 信号值
 */
void TcpSignalHandler(int sig)
{
    printf("\n[%s] Received signal %d, shutting down client...\n", __func__, sig);
    g_exit_flag = 1;
}

/**
 * @brief 打印程序使用说明
 * @param program_name 程序名
 */
void TcpPrintUsage(const char *program_name)
{
    printf("[%s] Usage: %s --tcp-port <port> --server-ip <ip>\n", __func__, program_name);
    printf("[%s] Example: %s --tcp-port 8888 --server-ip 127.0.0.1\n", __func__, program_name);
    printf("[%s] Port range: 1024-65535\n", __func__);
    printf("[%s] Server IP: IPv4 address format\n", __func__);
}

/**
 * @brief 解析命令行参数
 * @param argc 参数数量
 * @param argv 参数数组
 * @param port 端口号指针
 * @param server_ip 服务器IP地址缓冲区
 * @return int 成功返回0，失败返回-1
 */
int TcpParseArgs(int argc, char *argv[], int *port, char *server_ip)
{
    printf("[%s] Parsing command line arguments\n", __func__);

    *port = 0;
    server_ip[0] = '\0';

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--tcp-port") == 0)
        {
            if (i + 1 < argc)
            {
                *port = atoi(argv[++i]);
                if (*port <= 0 || *port > 65535)
                {
                    printf("[%s] Error: Invalid port number %d. Port must be 1-65535\n", __func__, *port);
                    return -1;
                }
                printf("[%s] TCP port set to: %d\n", __func__, *port);
            }
            else
            {
                printf("[%s] Error: --tcp-port requires a port number\n", __func__);
                return -1;
            }
        }
        else if (strcmp(argv[i], "--server-ip") == 0)
        {
            if (i + 1 < argc)
            {
                strncpy(server_ip, argv[++i], MAX_IP_LEN - 1);
                server_ip[MAX_IP_LEN - 1] = '\0';
                printf("[%s] Server IP set to: %s\n", __func__, server_ip);
            }
            else
            {
                printf("[%s] Error: --server-ip requires an IP address\n", __func__);
                return -1;
            }
        }
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            TcpPrintUsage(argv[0]);
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
        printf("[%s] Error: --tcp-port is required\n", __func__);
        return -1;
    }

    if (server_ip[0] == '\0')
    {
        printf("[%s] Error: --server-ip is required\n", __func__);
        return -1;
    }

    return 0;
}

/**
 * @brief 打印设备IP地址
 *
 * 获取并打印设备所有网络接口的IPv4地址信息
 */
void TcpPrintDeviceIP(void)
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
 * 负责从标准输入读取用户输入并发送到服务器
 *
 * @param arg 套接字描述符指针
 * @return 线程退出状态
 */
void *TcpSendMessage(void *arg)
{
    int sock = *(int *)arg;
    char send_buffer[BUFFER_SIZE] = {0};

    printf("[%s] Send message thread started\n", __func__);
    printf("[%s] Enter message to send to server (type 'exit' to quit):\n", __func__);
    fflush(stdout);

    while (!g_exit_flag)
    {
        if (fgets(send_buffer, sizeof(send_buffer), stdin) == NULL)
        {
            printf("[%s] Input stream closed\n", __func__);
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
            g_exit_flag = 1; // 设置全局退出标志
            printf("[%s] Exiting client...\n", __func__);
            // 关闭套接字的读端，让接收线程退出
            shutdown(sock, SHUT_RD);
            break;
        }

        if (send(sock, send_buffer, strlen(send_buffer), 0) <= 0)
        {
            printf("[%s] Send failed: %s\n", __func__, strerror(errno));
            g_exit_flag = 1;
            break;
        }
        printf("[%s] Send: {%s}\n", __func__, send_buffer);
        memset(send_buffer, 0, sizeof(send_buffer));
    }

    printf("[%s] Send message thread exiting\n", __func__);
    pthread_exit(NULL);
}

/**
 * @brief 接收消息线程函数
 *
 * 负责接收来自服务器的消息并显示
 *
 * @param arg 套接字描述符指针
 * @return 线程退出状态
 */
void *TcpReceiveMessage(void *arg)
{
    int sock = *(int *)arg;
    char recv_buffer[BUFFER_SIZE] = {0};

    printf("[%s] Receive message thread started\n", __func__);

    while (!g_exit_flag)
    {
        // 接收服务器的响应
        ssize_t recv_len = recv(sock, recv_buffer, sizeof(recv_buffer) - 1, 0);
        if (recv_len <= 0)
        {
            if (!g_exit_flag)
            { // 只有在非正常退出时才显示错误
                if (recv_len == 0)
                {
                    printf("[%s] Server disconnected\n", __func__);
                }
                else
                {
                    printf("[%s] Receive failed: %s\n", __func__, strerror(errno));
                }
                g_exit_flag = 1;
            }
            break;
        }

        recv_buffer[recv_len] = '\0'; // 确保字符串结束
        printf("[%s] Receive: {%s}\n", __func__, recv_buffer);
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
    signal(SIGINT, TcpSignalHandler);
    signal(SIGTERM, TcpSignalHandler);

    char server_ip[MAX_IP_LEN];
    int server_port;

    // 解析命令行参数
    int parse_result = TcpParseArgs(argc, argv, &server_port, server_ip);
    if (parse_result != 0)
    {
        if (parse_result < 0)
        {
            TcpPrintUsage(argv[0]);
        }
        return (parse_result < 0) ? 1 : 0;
    }

    // 打印设备的IP地址
    TcpPrintDeviceIP();

    int sock = 0;
    struct sockaddr_in serv_addr;

    // 创建套接字
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        printf("[%s] Socket creation error: %s\n", __func__, strerror(errno));
        return 1;
    }

    printf("[%s] Socket created successfully\n", __func__);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);

    // 将IP地址转换为二进制形式
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0)
    {
        printf("[%s] Invalid address/ Address not supported: %s\n", __func__, server_ip);
        close(sock);
        return 1;
    }

    printf("[%s] Attempting to connect to server %s:%d\n", __func__, server_ip, server_port);

    // 连接到服务器
    while (!g_exit_flag && connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("[%s] Connection failed: %s. Retrying in 1 second...\n", __func__, strerror(errno));
        sleep(1);
    }

    if (g_exit_flag)
    {
        printf("[%s] Connection attempt cancelled\n", __func__);
        close(sock);
        return 0;
    }

    printf("[%s] Connected to server %s:%d\n", __func__, server_ip, server_port);

    pthread_t send_thread, receive_thread;

    // 创建发送线程
    if (pthread_create(&send_thread, NULL, TcpSendMessage, &sock) != 0)
    {
        printf("[%s] Failed to create send thread: %s\n", __func__, strerror(errno));
        close(sock);
        return 1;
    }

    // 创建接收线程
    if (pthread_create(&receive_thread, NULL, TcpReceiveMessage, &sock) != 0)
    {
        printf("[%s] Failed to create receive thread: %s\n", __func__, strerror(errno));
        close(sock);
        g_exit_flag = 1; // 设置退出标志，让发送线程自然退出
        pthread_join(send_thread, NULL);
        return 1;
    }

    printf("[%s] Client threads created successfully\n", __func__);

    // 等待发送线程结束
    pthread_join(send_thread, NULL);

    // 关闭套接字，让接收线程从recv()中返回
    close(sock);

    // 等待接收线程结束
    pthread_join(receive_thread, NULL);

    printf("[%s] %s exited successfully\n", __func__, PROGRAM_NAME);
    return 0;
}