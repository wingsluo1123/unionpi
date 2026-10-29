#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include "../include/uart_base.h"
#include "../include/camera_driver.h"

#define BUFFER_SIZE 4096
#define BASE_PORT 8000

// 设备结构体
typedef struct {
    char *device_path;
    int baudrate;
    int port;
    char *name;
} DeviceConfig;

// 全局设备配置
DeviceConfig devices[5] = {
    {NULL, 115200, BASE_PORT + 0, "IMU"},
    {NULL, 115200, BASE_PORT + 1, "AMC"},
    {NULL, 115200, BASE_PORT + 2, "Radar"},
    {NULL, 230400, BASE_PORT + 3, "Lidar"},
    {NULL, 0, BASE_PORT + 4, "Camera"}  // Camera不需要baudrate
};

// 摄像头全局变量
static struct _v4l2_video g_video = {0};
static volatile int g_streaming = 0;

// 摄像头参数（带默认值）
static int g_camera_width = 640;
static int g_camera_height = 480;
static int g_camera_fps = 30;

// 线程参数结构体
typedef struct {
    DeviceConfig *config;
} ThreadArgs;

// 前向声明
void signal_handler(int sig);
void *device_thread(void *arg);
int open_serial_port(const char *port, int baudrate);
int create_tcp_server(int port);
void handle_serial_client(int client_fd, int serial_fd, const char *name);
void handle_camera_client(int client_fd);

// 信号处理函数
void signal_handler(int sig)
{
    printf("\n正在关闭程序...\n");
    g_streaming = 0;
    // 关闭所有串口和socket在各自线程中处理
    exit(0);
}

// 打印使用帮助信息
void print_usage(const char *program_name)
{
    printf("使用方法: %s --IMU <device> --AMC <device> --Radar <device> --Lidar <device> --Camera <device> [选项]\n", program_name);
    printf("示例: %s --IMU /dev/ttyUSB0 --AMC /dev/ttyUSB1 --Radar /dev/ttyUSB2 --Lidar /dev/ttyUSB3 --Camera /dev/video21\n", program_name);
    printf("选项:\n");
    printf("  --width <width>     摄像头宽度 (默认: 640)\n");
    printf("  --height <height>   摄像头高度 (默认: 480)\n");
    printf("  --fps <fps>         摄像头帧率 (默认: 30)\n");
    printf("注意: IMU、AMC、Radar、Lidar、Camera 参数是必需的，选项参数可选\n");
}

// 解析命令行参数
int parse_arguments(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--IMU") == 0) {
            if (i + 1 >= argc) {
                printf("[%s] 错误: --IMU 参数缺少值\n", __func__);
                return -1;
            }
            devices[0].device_path = argv[++i];
        } else if (strcmp(argv[i], "--AMC") == 0) {
            if (i + 1 >= argc) {
                printf("[%s] 错误: --AMC 参数缺少值\n", __func__);
                return -1;
            }
            devices[1].device_path = argv[++i];
        } else if (strcmp(argv[i], "--Radar") == 0) {
            if (i + 1 >= argc) {
                printf("[%s] 错误: --Radar 参数缺少值\n", __func__);
                return -1;
            }
            devices[2].device_path = argv[++i];
        } else if (strcmp(argv[i], "--Lidar") == 0) {
            if (i + 1 >= argc) {
                printf("[%s] 错误: --Lidar 参数缺少值\n", __func__);
                return -1;
            }
            devices[3].device_path = argv[++i];
        } else if (strcmp(argv[i], "--Camera") == 0) {
            if (i + 1 >= argc) {
                printf("[%s] 错误: --Camera 参数缺少值\n", __func__);
                return -1;
            }
            devices[4].device_path = argv[++i];
        } else if (strcmp(argv[i], "--width") == 0) {
            if (i + 1 >= argc) {
                printf("[%s] 错误: --width 参数缺少值\n", __func__);
                return -1;
            }
            g_camera_width = atoi(argv[++i]);
            if (g_camera_width <= 0) {
                printf("[%s] 错误: 无效的宽度值 %d\n", __func__, g_camera_width);
                return -1;
            }
        } else if (strcmp(argv[i], "--height") == 0) {
            if (i + 1 >= argc) {
                printf("[%s] 错误: --height 参数缺少值\n", __func__);
                return -1;
            }
            g_camera_height = atoi(argv[++i]);
            if (g_camera_height <= 0) {
                printf("[%s] 错误: 无效的高度值 %d\n", __func__, g_camera_height);
                return -1;
            }
        } else if (strcmp(argv[i], "--fps") == 0) {
            if (i + 1 >= argc) {
                printf("[%s] 错误: --fps 参数缺少值\n", __func__);
                return -1;
            }
            g_camera_fps = atoi(argv[++i]);
            if (g_camera_fps <= 0) {
                printf("[%s] 错误: 无效的帧率值 %d\n", __func__, g_camera_fps);
                return -1;
            }
        } else {
            printf("[%s] 错误: 未知参数 '%s'\n", __func__, argv[i]);
            return -1;
        }
    }

    // 检查必需参数是否都已设置
    for (int i = 0; i < 5; i++) {
        if (devices[i].device_path == NULL) {
            printf("[%s] 错误: 缺少 %s 设备参数\n", __func__, devices[i].name);
            print_usage(argv[0]);
            return -1;
        }
    }

    return 0;
}

// 获取并打印网络信息
void print_network_info()
{
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        printf("[%s] getifaddrs: %s\n", __func__, strerror(errno));
        return;
    }

    printf("[%s] 网络信息:\n", __func__);

    // 获取IP地址
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;

        if (family == AF_INET) {
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                           host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("[%s] getnameinfo() failed: %s\n", __func__, gai_strerror(s));
                continue;
            }

            // 只显示非本地回环的IP
            if (strcmp(ifa->ifa_name, "lo") != 0 && strncmp(host, "127.", 4) != 0) {
                printf("[%s] IP地址 (%s): %s\n", __func__, ifa->ifa_name, host);
            }
        }
    }

    freeifaddrs(ifaddr);
    printf("[%s] \n", __func__);
}

// 打开并配置串口
int open_serial_port(const char *port, int baudrate)
{
    // 打开串口
    int fd = UartOpen(port, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd == UART_ERR)
    {
        perror("打开串口失败");
        return -1;
    }

    // 配置串口参数
    struct SerialPortConfig config = {
        .baudrate = baudrate,
        .databits = 8,
        .stopbits = 1,
        .parity = 'n',                    // 无校验
        .vtime = 1,                       // 读取超时 0.1秒
        .vmin = 0,                        // 最小读取字节数 0（非阻塞）
        .flow_control = SERIAL_FLOW_NONE, // 无流控
        .raw_mode = 1                     // 原始模式
    };

    if (UartInit(fd, &config) != UART_OK)
    {
        perror("配置串口失败");
        UartClose(fd);
        return -1;
    }

    printf("[%s] 串口 %s 打开成功，波特率：%d\n", __func__, port, baudrate);
    return fd;
}

// 创建TCP服务器
int create_tcp_server(int port)
{
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    // 创建socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket创建失败");
        return -1;
    }

    // 设置socket选项
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt失败");
        close(server_fd);
        return -1;
    }

    // 设置地址结构
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // 绑定地址
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind失败");
        close(server_fd);
        return -1;
    }

    // 开始监听
    if (listen(server_fd, 1) < 0)
    {
        perror("listen失败");
        close(server_fd);
        return -1;
    }

    printf("[%s] TCP服务器启动成功，端口：%d\n", __func__, port);
    return server_fd;
}

// 处理串口客户端连接
void handle_serial_client(int client_fd, int serial_fd, const char *name)
{
    fd_set readfds;
    char buffer[BUFFER_SIZE];
    int max_fd, n;
    struct timeval timeout;

    printf("[%s] %s 客户端已连接，开始数据透传...\n", __func__, name);

    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(serial_fd, &readfds);
        FD_SET(client_fd, &readfds);

        max_fd = (serial_fd > client_fd) ? serial_fd : client_fd;

        // 设置超时（1秒）
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int activity = select(max_fd + 1, &readfds, NULL, NULL, &timeout);

        if (activity < 0)
        {
            if (errno == EINTR)
                continue; // 被信号中断，继续
            perror("select错误");
            break;
        }

        if (activity == 0)
        {
            // 超时，检查连接是否仍然有效
            char test_buf[1];
            if (recv(client_fd, test_buf, sizeof(test_buf), MSG_PEEK | MSG_DONTWAIT) == 0)
            {
                printf("[%s] %s 客户端连接已断开\n", __func__, name);
                break;
            }
            continue;
        }

        // 从串口读取数据并发送到TCP客户端
        if (FD_ISSET(serial_fd, &readfds))
        {
            n = UartRead(serial_fd, buffer, BUFFER_SIZE);
            if (n > 0)
            {
                if (send(client_fd, buffer, n, 0) < 0)
                {
                    perror("发送到TCP客户端失败");
                    break;
                }
                printf("[%s] %s 串口→网络: %d 字节\n", __func__, name, n);
            }
            else if (n < 0)
            {
                if (errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    perror("读取串口失败");
                    break;
                }
            }
        }

        // 从TCP客户端读取数据并写入串口
        if (FD_ISSET(client_fd, &readfds))
        {
            n = recv(client_fd, buffer, BUFFER_SIZE, 0);
            if (n > 0)
            {
                if (UartWrite(serial_fd, buffer, n) < 0)
                {
                    perror("写入串口失败");
                    break;
                }
                printf("[%s] %s 网络→串口: %d 字节\n", __func__, name, n);
            }
            else if (n == 0)
            {
                printf("[%s] %s 客户端断开连接\n", __func__, name);
                break;
            }
            else
            {
                perror("从TCP客户端接收失败");
                break;
            }
        }
    }

    close(client_fd);
    printf("[%s] %s 客户端连接处理结束\n", __func__, name);
}

// 处理摄像头客户端连接
void handle_camera_client(int client_fd)
{
    int index = 0, bytesused = 0;
    int ret = 0;
    long frame_interval_ns = 1000000000L / g_camera_fps; // 根据设置的fps计算帧间隔

    printf("[%s] 摄像头客户端已连接，开始发送MJPEG流...\n", __func__);

    while (g_streaming)
    {
        struct timespec start_time;
        clock_gettime(CLOCK_MONOTONIC, &start_time);

        // 从队列取出数据
        ret = CameraReadFrame(&g_video, &index, &bytesused);
        if (ret < 0)
        {
            printf("[%s] 读取摄像头帧失败\n", __func__);
            break;
        }

        // 检查是否为JPEG/MJPEG数据
        unsigned char *framePtr = (unsigned char *)g_video.mmap_buf[index].addr;
        int is_jpeg = (bytesused >= 2 && framePtr[0] == 0xFF && framePtr[1] == 0xD8);
        if (!is_jpeg)
        {
            printf("[%s] 帧不是JPEG/MJPEG格式\n", __func__);
            CameraQueueBuffer(&g_video, index);
            continue;
        }

        // 发送帧头：4字节帧大小（网络字节序）+ JPEG数据
        uint32_t frame_size = htonl(bytesused);
        int sent = send(client_fd, &frame_size, sizeof(frame_size), 0);
        if (sent <= 0)
        {
            printf("[%s] 摄像头客户端断开或发送错误\n", __func__);
            CameraQueueBuffer(&g_video, index);
            break;
        }

        sent = send(client_fd, framePtr, bytesused, 0);
        if (sent <= 0)
        {
            printf("[%s] 摄像头客户端断开或发送错误\n", __func__);
            CameraQueueBuffer(&g_video, index);
            break;
        }

        printf("[%s] 摄像头发送帧: %d 字节\n", __func__, bytesused);

        // 重新放入队列
        CameraQueueBuffer(&g_video, index);

        // 控制帧率
        struct timespec end_time;
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        long elapsed_ns = (end_time.tv_sec - start_time.tv_sec) * 1000000000L + (end_time.tv_nsec - start_time.tv_nsec);
        if (elapsed_ns < frame_interval_ns)
        {
            usleep((frame_interval_ns - elapsed_ns) / 1000);
        }
    }

    close(client_fd);
    printf("[%s] 摄像头客户端连接处理结束\n", __func__);
}

// 设备线程函数
void *device_thread(void *arg)
{
    ThreadArgs *thread_args = (ThreadArgs *)arg;
    DeviceConfig *config = thread_args->config;
    int server_fd, client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    printf("[%s] 启动 %s 线程，端口 %d，设备 %s\n", __func__, config->name, config->port, config->device_path);

    // 创建TCP服务器
    server_fd = create_tcp_server(config->port);
    if (server_fd < 0)
    {
        printf("[%s] %s TCP服务器创建失败\n", __func__, config->name);
        free(thread_args);
        return NULL;
    }

    // 对于串口设备，打开串口
    int device_fd = -1;
    if (strcmp(config->name, "Camera") != 0)
    {
        device_fd = open_serial_port(config->device_path, config->baudrate);
        if (device_fd < 0)
        {
            printf("[%s] %s 串口打开失败\n", __func__, config->name);
            close(server_fd);
            free(thread_args);
            return NULL;
        }
    }
    else
    {
        // 摄像头已在主线程初始化
        device_fd = g_video.fd;
    }

    // 主循环：等待客户端连接
    while (1)
    {
        printf("[%s] %s 等待客户端连接...\n", __func__, config->name);
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0)
        {
            perror("accept失败");
            continue;
        }

        printf("[%s] %s 新的客户端连接: %s:%d\n", __func__, config->name, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        if (strcmp(config->name, "Camera") == 0)
        {
            handle_camera_client(client_fd);
        }
        else
        {
            handle_serial_client(client_fd, device_fd, config->name);
        }
    }

    // 清理资源
    if (strcmp(config->name, "Camera") != 0)
    {
        UartClose(device_fd);
    }
    close(server_fd);
    free(thread_args);
    return NULL;
}

int main(int argc, char *argv[])
{
    pthread_t threads[5];
    ThreadArgs *thread_args[5];

    printf("[%s] Whale Car 多设备透传服务器\n", __func__);

    // 解析命令行参数
    if (parse_arguments(argc, argv) != 0)
    {
        return 1;
    }

    // 注册信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 打印网络信息
    print_network_info();

    // 初始化摄像头
    printf("[%s] 初始化摄像头: %s\n", __func__, devices[4].device_path);
    g_video.fd = CameraOpen(devices[4].device_path);
    if (g_video.fd < 0)
    {
        printf("[%s] 摄像头打开失败\n", __func__);
        return 1;
    }

    if (CameraQueryCapability(&g_video) < 0 ||
        CameraSetParameters(&g_video, g_camera_width, g_camera_height) < 0 ||
        CameraSetMemoryMap(&g_video, 4) < 0 ||
        CameraStreamOn(&g_video) < 0)
    {
        printf("[%s] 摄像头初始化失败\n", __func__);
        CameraClose(&g_video);
        return 1;
    }

    g_streaming = 1;
    printf("[%s] 摄像头参数: %dx%d @ %d fps\n", __func__, g_camera_width, g_camera_height, g_camera_fps);

    // 创建设备线程
    for (int i = 0; i < 5; i++)
    {
        thread_args[i] = malloc(sizeof(ThreadArgs));
        if (thread_args[i] == NULL)
        {
            printf("[%s] 内存分配失败: %s\n", __func__, strerror(errno));
            return 1;
        }
        thread_args[i]->config = &devices[i];

        if (pthread_create(&threads[i], NULL, device_thread, thread_args[i]) != 0)
        {
            printf("[%s] 线程创建失败: %s\n", __func__, strerror(errno));
            free(thread_args[i]);
            return 1;
        }
    }

    printf("[%s] 所有设备线程已启动，服务器运行中...\n", __func__);

    // 等待所有线程结束（实际上不会结束，除非信号）
    for (int i = 0; i < 5; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // 清理摄像头
    CameraStreamOff(&g_video);
    CameraClose(&g_video);

    return 0;
}