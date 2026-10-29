/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-31 14:30:00
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-31 16:11:44
 * @FilePath: /OpenHarmony/unionpi_whale_demo/tcp_server/src/main.c
 * @Description: TCP服务器程序实现
 */

#include "../include/main.h"

// 全局变量
volatile int g_client_exit_flag = 0;
volatile int g_running = 1;

/**
 * @brief 信号处理函数
 * @param sig 信号值
 */
void TcpSignalHandler(int sig)
{
    printf("\n[%s] Received signal %d, shutting down server...\n", __func__, sig);
    g_running = 0;
    g_client_exit_flag = 1;
}

/**
 * @brief 打印程序使用说明
 * @param program_name 程序名
 */
void TcpPrintUsage(const char *program_name)
{
    printf("[%s] Usage: %s --tcp-port <port>\n", __func__, program_name);
    printf("[%s] Example: %s --tcp-port 8888\n", __func__, program_name);
    printf("[%s] Default port range: 1024-65535\n", __func__);
}

/**
 * @brief 解析命令行参数
 * @param argc 参数数量
 * @param argv 参数数组
 * @param port 端口号指针
 * @return int 成功返回0，失败返回-1
 */
int TcpParseArgs(int argc, char *argv[], int *port)
{
    printf("[%s] Parsing command line arguments\n", __func__);

    *port = 0;

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
 * 负责从标准输入读取用户输入并发送到客户端
 *
 * @param arg 套接字描述符指针
 * @return 线程退出状态
 */
void *TcpSendMessage(void *arg)
{
    int sock = *(int *)arg;
    char send_buffer[BUFFER_SIZE] = {0};

    printf("[%s] Send message thread started\n", __func__);
    printf("[%s] Enter message to send to client (type 'exit' to quit):\n", __func__);
    fflush(stdout);

    while (!g_client_exit_flag && g_running)
    {
        if (fgets(send_buffer, sizeof(send_buffer), stdin) == NULL)
        {
            printf("[%s] Input stream closed\n", __func__);
            break;
        }

        // 检查是否在等待输入期间客户端断开了
        if (g_client_exit_flag || !g_running)
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
            g_client_exit_flag = 1; // 设置退出标志
            printf("[%s] Closing client connection...\n", __func__);
            break;
        }

        if (send(sock, send_buffer, strlen(send_buffer), 0) <= 0)
        {
            printf("[%s] Send failed: %s\n", __func__, strerror(errno));
            g_client_exit_flag = 1; // 发送失败也设置退出标志
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
 * 负责接收来自客户端的消息并显示
 *
 * @param arg 套接字描述符指针
 * @return 线程退出状态
 */
void *TcpReceiveMessage(void *arg)
{
    int sock = *(int *)arg;
    char recv_buffer[BUFFER_SIZE] = {0};

    printf("[%s] Receive message thread started\n", __func__);

    while (!g_client_exit_flag && g_running)
    {
        // 接收客户端的消息
        ssize_t recv_len = recv(sock, recv_buffer, sizeof(recv_buffer) - 1, 0);
        if (recv_len <= 0)
        {
            if (recv_len == 0)
            {
                printf("[%s] Client disconnected normally\n", __func__);
            }
            else
            {
                printf("[%s] Receive failed: %s\n", __func__, strerror(errno));
            }
            g_client_exit_flag = 1; // 设置退出标志，通知发送线程退出
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

    int port;

    // 解析命令行参数
    int parse_result = TcpParseArgs(argc, argv, &port);
    if (parse_result != 0)
    {
        if (parse_result < 0)
        {
            TcpPrintUsage(argv[0]);
        }
        return (parse_result < 0) ? 1 : 0;
    }

    // 打印设备IP地址
    TcpPrintDeviceIP();

    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // 创建套接字
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        printf("[%s] Socket creation failed: %s\n", __func__, strerror(errno));
        return 1;
    }

    printf("[%s] Socket created successfully\n", __func__);

    // 设置套接字选项
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0)
    {
        printf("[%s] Setsockopt failed: %s\n", __func__, strerror(errno));
        close(server_fd);
        return 1;
    }

    printf("[%s] Socket options set successfully\n", __func__);

    // 配置服务器地址结构
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // 绑定套接字到地址
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("[%s] Bind failed: %s\n", __func__, strerror(errno));
        close(server_fd);
        return 1;
    }

    printf("[%s] Socket bound to port %d\n", __func__, port);

    // 监听连接请求
    if (listen(server_fd, MAX_CLIENTS) < 0)
    {
        printf("[%s] Listen failed: %s\n", __func__, strerror(errno));
        close(server_fd);
        return 1;
    }

    printf("[%s] Server is listening on port %d...\n", __func__, port);

    // 主服务循环
    while (g_running)
    {
        printf("[%s] Waiting for client connections...\n", __func__);

        // 接受客户端连接
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd < 0)
        {
            if (g_running)
            { // 只有在运行状态下才报告错误
                printf("[%s] Accept failed: %s\n", __func__, strerror(errno));
            }
            break;
        }

        printf("[%s] New client connected: %s, port: %d\n", __func__, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // 重置客户端退出标志
        g_client_exit_flag = 0;

        pthread_t send_thread, receive_thread;

        // 创建发送线程
        if (pthread_create(&send_thread, NULL, TcpSendMessage, &client_fd) != 0)
        {
            printf("[%s] Failed to create send thread: %s\n", __func__, strerror(errno));
            close(client_fd);
            continue;
        }

        // 创建接收线程
        if (pthread_create(&receive_thread, NULL, TcpReceiveMessage, &client_fd) != 0)
        {
            printf("[%s] Failed to create receive thread: %s\n", __func__, strerror(errno));
            close(client_fd);
            g_client_exit_flag = 1;          // 设置退出标志
            pthread_join(send_thread, NULL); // 等待发送线程退出
            continue;
        }

        printf("[%s] Client threads created successfully\n", __func__);

        // 等待接收线程结束（它会检测客户端断开）
        pthread_join(receive_thread, NULL);

        // 接收线程结束说明客户端断开，设置退出标志和关闭套接字
        g_client_exit_flag = 1;
        close(client_fd);

        // 等待发送线程结束
        pthread_join(send_thread, NULL);

        printf("[%s] Client session ended. Waiting for new connections...\n", __func__);
    }

    printf("[%s] Closing server socket\n", __func__);
    close(server_fd);

    printf("[%s] %s terminated successfully\n", __func__, PROGRAM_NAME);
    return 0;
}