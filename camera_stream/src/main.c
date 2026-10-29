/*
 * @Author: OpenHarmony Camera Team
 * @Date: 2025-11-01 00:00:00
 * @LastEditors: OpenHarmony Camera Team
 * @LastEditTime: 2025-11-01 00:00:00
 * @FilePath: /OpenHarmony/unionpi_whale_demo/camera_stream/src/main.c
 * @Description: 简易HTTP服务 + 实时视频流展示
 */

#include "main.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <time.h>

/**
 * @brief 视频流边界标识
 */
#define BOUNDARY "frameboundary123"

/**
 * @brief 全局视频设备信息
 */
static struct _v4l2_video g_video = {0};

/**
 * @brief 线程控制变量
 */
static volatile int g_streaming = 0;
static volatile int g_client_connected = 0;
static int g_server_socket = -1;

/**
 * @brief 发送HTTP响应头
 *
 * @param client 客户端套接字
 * @param status HTTP状态码
 * @param ctype 内容类型
 * @param extra_headers 额外的HTTP头
 */
static void send_http_header(int client, const char *status, const char *ctype, const char *extra_headers)
{
    char header[512];
    int n = snprintf(header, sizeof(header),
                     "HTTP/1.1 %s\r\n"
                     "Content-Type: %s\r\n"
                     "%s"
                     "Connection: close\r\n\r\n",
                     status, ctype, extra_headers ? extra_headers : "");
    send(client, header, n, 0);
}

/**
 * @brief 处理根路径请求
 *
 * @param client 客户端套接字
 */
static void handle_index(int client)
{
    const char *html = "<!doctype html>"
                       "<html>"
                       "<head>"
                       "<meta charset=\"utf-8\">"
                       "<title>Camera Video Stream</title>"
                       "<style>"
                       "body { font-family: sans-serif; margin: 2rem; }"
                       "img { max-width: 100%; height: auto; border: 1px solid #ccc; }"
                       "h1 { color: #333; }"
                       "</style>"
                       "</head>"
                       "<body>"
                       "<h1>Camera Video Stream</h1>"
                       "<p>实时视频流：</p>"
                       "<img src=\"/stream\" alt=\"video stream\" onerror=\"this.src='/stream?' + new Date().getTime();\">"
                       "<p><button onclick=\"location.reload()\">刷新页面</button></p>"
                       "<p>如果视频未显示，请确认：<br>"
                       "1. 摄像头设备路径正确<br>"
                       "2. 摄像头支持MJPEG格式<br>"
                       "3. 分辨率和帧率设置正确</p>"
                       "</body>"
                       "</html>";

    send_http_header(client, "200 OK", "text/html; charset=utf-8", NULL);
    send(client, html, strlen(html), 0);
}

/**
 * @brief URL解码
 *
 * @param s 需要解码的字符串
 */
static void url_decode(char *s)
{
    // minimal decode: convert + to space, %xx to char
    char *o = s;
    for (; *s; s++, o++)
    {
        if (*s == '+')
        {
            *o = ' ';
        }
        else if (*s == '%' && isxdigit((unsigned char)s[1]) && isxdigit((unsigned char)s[2]))
        {
            char hex[3] = {s[1], s[2], 0};
            *o = (char)strtol(hex, NULL, 16);
            s += 2;
        }
        else
        {
            *o = *s;
        }
    }
    *o = '\0';
}

/**
 * @brief 打印使用帮助信息
 */
void print_usage(const char *program_name)
{
    printf("[%s] Usage: %s --device <device_path> --width <width> --height <height> --port <port> --fps <frames_per_second>\n", __func__, program_name);
    printf("[%s] Example: %s --device /dev/video0 --width 640 --height 480 --port 8080 --fps 30\n", __func__, program_name);
}

/**
 * @brief 解析命令行参数
 *
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @param device_ptr 指向设备路径指针的指针
 * @param width_ptr 指向宽度的指针
 * @param height_ptr 指向高度的指针
 * @param port_ptr 指向端口的指针
 * @param fps_ptr 指向帧率的指针
 * @return 成功返回 0，失败返回 -1
 */
int parse_arguments(int argc, char **argv, const char **device_ptr, int *width_ptr, int *height_ptr, int *port_ptr, int *fps_ptr)
{
    // 检查参数数量
    if (argc != 11)
    { // 程序名 + 5个参数对
        printf("[%s] Error: Invalid number of arguments\n", "parse_arguments");
        print_usage(argv[0]);
        return -1;
    }

    for (int i = 1; i < argc;)
    {
        if (strcmp(argv[i], "--device") == 0 && i + 1 < argc)
        {
            *device_ptr = argv[i + 1];
            i += 2;
        }
        else if (strcmp(argv[i], "--width") == 0 && i + 1 < argc)
        {
            *width_ptr = atoi(argv[i + 1]);
            i += 2;
        }
        else if (strcmp(argv[i], "--height") == 0 && i + 1 < argc)
        {
            *height_ptr = atoi(argv[i + 1]);
            i += 2;
        }
        else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc)
        {
            *port_ptr = atoi(argv[i + 1]);
            i += 2;
        }
        else if (strcmp(argv[i], "--fps") == 0 && i + 1 < argc)
        {
            *fps_ptr = atoi(argv[i + 1]);
            i += 2;
        }
        else
        {
            printf("[%s] Error: Invalid argument '%s'\n", "parse_arguments", argv[i]);
            print_usage(argv[0]);
            return -1;
        }
    }

    // 检查所有必需参数是否都已设置
    if (*device_ptr == NULL || *width_ptr <= 0 || *height_ptr <= 0 || *port_ptr <= 0 || *fps_ptr <= 0)
    {
        printf("[%s] Error: All parameters must be specified with valid values\n", "parse_arguments");
        print_usage(argv[0]);
        return -1;
    }

    return 0;
}

/**
 * @brief 视频流发送线程函数
 *
 * @param arg 客户端套接字指针
 * @return NULL
 */
void *stream_thread(void *arg)
{
    int client = *(int *)arg;
    int index = 0, bytesused = 0;
    char header[256];
    int ret = 0;

    printf("[%s] Stream thread started\n", __func__);

    // 发送多部分响应头
    char extra_headers[256];
    snprintf(extra_headers, sizeof(extra_headers),
             "Cache-Control: no-cache\r\n"
             "Pragma: no-cache\r\n"
             "Expires: 0\r\n"
             "Content-Type: multipart/x-mixed-replace; boundary=" BOUNDARY "\r\n"
             "X-Accel-Buffering: no\r\n"
             "Transfer-Encoding: chunked\r\n"
             "\r\n");

    char response_header[512];
    int header_len = snprintf(response_header, sizeof(response_header),
                              "HTTP/1.1 200 OK\r\n"
                              "Content-Type: multipart/x-mixed-replace; boundary=" BOUNDARY "\r\n"
                              "Cache-Control: no-cache\r\n"
                              "Pragma: no-cache\r\n"
                              "Expires: 0\r\n"
                              "X-Accel-Buffering: no\r\n"
                              "Connection: close\r\n"
                              "\r\n");

    send(client, response_header, header_len, 0);

    while (g_streaming && g_client_connected)
    {
        // 从队列取出数据
        ret = CameraReadFrame(&g_video, &index, &bytesused);
        if (ret < 0)
        {
            printf("[%s] Failed to read frame\n", __func__);
            break;
        }

        // 检查是否为JPEG/MJPEG数据
        unsigned char *framePtr = (unsigned char *)g_video.mmap_buf[index].addr;
        int is_jpeg = (bytesused >= 2 && framePtr[0] == 0xFF && framePtr[1] == 0xD8);
        if (!is_jpeg)
        {
            printf("[%s] Frame is not JPEG/MJPEG format\n", __func__);
            CameraQueueBuffer(&g_video, index);
            continue;
        }

        // 发送边界和头部
        int header_len = snprintf(header, sizeof(header),
                                  "\r\n--" BOUNDARY "\r\n"
                                  "Content-Type: image/jpeg\r\n"
                                  "Content-Length: %d\r\n"
                                  "\r\n",
                                  bytesused);
        send(client, header, header_len, 0);

        // 发送图像数据
        send(client, g_video.mmap_buf[index].addr, bytesused, 0);

        // 将缓冲区重新放入队列
        CameraQueueBuffer(&g_video, index);

        // 短暂休眠以控制帧率
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 33333333; // 约30fps的延迟
        nanosleep(&ts, NULL);
    }

    printf("[%s] Stream thread stopped\n", __func__);
    return NULL;
}

/**
 * @brief 处理视频流请求
 *
 * @param client 客户端套接字
 */
static void handle_stream(int client)
{
    pthread_t tid;
    int ret = 0;

    // 设置客户端连接标志
    g_client_connected = 1;

    // 创建视频流线程
    ret = pthread_create(&tid, NULL, stream_thread, &client);
    if (ret != 0)
    {
        printf("[%s] Failed to create stream thread: %s\n", __func__, strerror(errno));
        const char *msg = "Internal Server Error";
        send_http_header(client, "500 Internal Server Error", "text/plain; charset=utf-8", NULL);
        send(client, msg, strlen(msg), 0);
        g_client_connected = 0;
        return;
    }

    // 等待客户端断开连接
    char buffer[256];
    while (g_client_connected)
    {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(client, &read_fds);

        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int activity = select(client + 1, &read_fds, NULL, NULL, &timeout);
        if (activity < 0)
        {
            printf("[%s] select error: %s\n", __func__, strerror(errno));
            break;
        }
        else if (activity == 0)
        {
            // 超时，继续检查连接状态
            continue;
        }
        else
        {
            // 客户端发送了数据（可能是关闭连接）
            int valread = recv(client, buffer, sizeof(buffer), MSG_DONTWAIT);
            if (valread <= 0)
            {
                printf("[%s] Client disconnected\n", __func__);
                break;
            }
        }
    }

    // 停止视频流
    g_client_connected = 0;

    // 等待线程结束
    pthread_join(tid, NULL);
}

/**
 * @brief 信号处理函数
 *
 * @param sig 信号编号
 */
void signal_handler(int sig)
{
    printf("[%s] Received signal %d\n", __func__, sig);
    g_streaming = 0;
    g_client_connected = 0;

    if (g_server_socket != -1)
    {
        close(g_server_socket);
        g_server_socket = -1;
    }
}

/**
 * @brief 初始化摄像头设备
 *
 * @param device 设备路径
 * @param width 图像宽度
 * @param height 图像高度
 * @return 成功返回 0，失败返回 -1
 */
int init_camera(const char *device, int width, int height)
{
    int ret = 0;

    // 打开设备
    g_video.fd = CameraOpen(device);
    if (g_video.fd < 0)
    {
        printf("[%s] Failed to open camera device: %s\n", __func__, device);
        return -1;
    }

    // 查询设备能力
    if (CameraQueryCapability(&g_video) < 0)
    {
        printf("[%s] Failed to query camera capability\n", __func__);
        ret = -1;
        goto error;
    }

    // 设置参数
    if (CameraSetParameters(&g_video, width, height) < 0)
    {
        printf("[%s] Failed to set camera parameters\n", __func__);
        ret = -1;
        goto error;
    }

    // 设置内存映射(使用4个缓冲区)
    if (CameraSetMemoryMap(&g_video, 4) < 0)
    {
        printf("[%s] Failed to set memory map\n", __func__);
        ret = -1;
        goto error;
    }

    // 启动数据流
    if (CameraStreamOn(&g_video) < 0)
    {
        printf("[%s] Failed to start camera stream\n", __func__);
        ret = -1;
        goto error;
    }

    return 0;

error:
    CameraClose(&g_video);
    return ret;
}

/**
 * @brief 主函数入口
 *
 * 程序执行的起始点，用于启动HTTP服务器并处理摄像头视频流请求
 *
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 返回程序执行状态码，正常退出返回0
 */
int main(int argc, char **argv)
{
    int width = 0;
    int height = 0;
    int port = 0;
    int fps = 0;
    const char *device = NULL;

    if (parse_arguments(argc, argv, &device, &width, &height, &port, &fps) != 0)
    {
        return -1;
    }

    // 初始化摄像头
    if (init_camera(device, width, height) != 0)
    {
        printf("[%s] Failed to initialize camera\n", __func__);
        return -1;
    }

    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 启动流传输标志
    g_streaming = 1;

    // 启动HTTP服务器
    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0)
    {
        printf("[%s] socket failed: %s\n", __func__, strerror(errno));
        CameraClose(&g_video);
        return -1;
    }

    g_server_socket = srv;

    int yes = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(srv, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        printf("[%s] bind failed: %s\n", __func__, strerror(errno));
        close(srv);
        CameraClose(&g_video);
        return -1;
    }

    if (listen(srv, 8) < 0)
    {
        printf("[%s] listen failed: %s\n", __func__, strerror(errno));
        close(srv);
        CameraClose(&g_video);
        return -1;
    }

    printf("[%s] HTTP server started on port %d. Access http://<IP>:%d/\n", __func__, port, port);
    printf("[%s] Video stream settings: %dx%d @ %d fps\n", __func__, width, height, fps);

    while (g_streaming)
    {
        struct sockaddr_in cli;
        socklen_t clen = sizeof(cli);
        int c = accept(srv, (struct sockaddr *)&cli, &clen);
        if (c < 0)
        {
            if (g_streaming)
            {
                printf("[%s] accept failed: %s\n", __func__, strerror(errno));
            }
            continue;
        }

        char req[2048];
        int n = recv(c, req, sizeof(req) - 1, 0);
        if (n <= 0)
        {
            close(c);
            continue;
        }
        req[n] = 0;

        // parse first line: METHOD PATH HTTP/1.1
        char method[8] = {0}, path[512] = {0};
        sscanf(req, "%7s %511s", method, path);
        url_decode(path);

        if (strcmp(method, "GET") == 0 && (strcmp(path, "/") == 0 || strncmp(path, "/index", 6) == 0))
        {
            handle_index(c);
        }
        else if (strcmp(method, "GET") == 0 && strcmp(path, "/stream") == 0)
        {
            handle_stream(c);
        }
        else
        {
            const char *msg = "Not Found";
            send_http_header(c, "404 Not Found", "text/plain; charset=utf-8", NULL);
            send(c, msg, strlen(msg), 0);
        }
        close(c);
    }

    // 关闭摄像头
    CameraStreamOff(&g_video);
    CameraClose(&g_video);

    // 关闭服务器套接字
    if (srv != -1)
    {
        close(srv);
    }

    printf("[%s] Server stopped\n", __func__);
    return 0;
}