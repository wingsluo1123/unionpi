/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @FilePath: /OpenHarmony/unionpi_whale_demo/uart_chassis/src/main.c
 * @Description: 底盘控制 — TCP 手机控制 + 雷达避障回传
 *
 * TCP 协议（手机 ↔ 开发板，端口 8001，纯文本，每行一条）：
 *   手机→车: v:300:0  stop  quit
 *   车→手机: r:距离mm (雷达避障数据)
 */

#include "../include/main.h"
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define TCP_PORT    8001
#define BACKLOG     3
#define BUF_SIZE    256

static volatile int g_running = 1;
static int g_server_fd = -1;
static int g_client_fd = -1;
static int g_radar_running = 0;
static pthread_t g_radar_thread;
static chassis_control_t *g_control = NULL;

static void sig_handler(int sig)
{
    (void)sig;
    printf("\n[%s] 收到退出信号，正在关闭...\n", __func__);
    g_running = 0;
    if (g_client_fd >= 0) { shutdown(g_client_fd, SHUT_RDWR); close(g_client_fd); g_client_fd = -1; }
    if (g_server_fd >= 0) { shutdown(g_server_fd, SHUT_RDWR); close(g_server_fd); g_server_fd = -1; }
}

void print_usage(const char *program_name)
{
    printf("用法: %s --uart-port <device_path>\n", program_name);
    printf("示例: %s --uart-port /dev/ttyUSB0\n", program_name);
}

/* 雷达回传线程：复用 uart_base 读取 /dev/ttyUSB2，发给手机 */
static void *radar_thread_func(void *arg)
{
    int client = *(int*)arg;
    int rfd = UartOpen("/dev/ttyUSB2", O_RDWR | O_NOCTTY);
    if (rfd < 0) {
        printf("[radar] 无法打开雷达串口\n");
        return NULL;
    }
    struct SerialPortConfig cfg = {
        .baudrate = 115200, .databits = 8, .stopbits = 1, .parity = 'N',
        .vtime = 5, .vmin = 0, .flow_control = SERIAL_FLOW_NONE, .raw_mode = 1
    };
    UartInit(rfd, &cfg);
    printf("[radar] 雷达线程已启动\n");

    char rbuf[256];
    char line[512];
    memset(line, 0, sizeof(line));
    while (g_radar_running && g_running) {
        memset(rbuf, 0, sizeof(rbuf));
        ssize_t n = UartRead(rfd, rbuf, sizeof(rbuf) - 1);
        if (n <= 0) { usleep(50000); continue; }
        strncat(line, rbuf, sizeof(line) - strlen(line) - 1);
        char *nl = strchr(line, '\n');
        if (nl) {
            *nl = '\0';
            int dist = 0;
            if (sscanf(line, "%dmm", &dist) == 1 && dist > 0) {
                char obuf[32];
                snprintf(obuf, sizeof(obuf), "r:%d\n", dist);
                send(client, obuf, strlen(obuf), 0);
            } else if (sscanf(line, "%d", &dist) == 1 && dist > 0) {
                char obuf[32];
                snprintf(obuf, sizeof(obuf), "r:%d\n", dist);
                send(client, obuf, strlen(obuf), 0);
            }
            memmove(line, nl + 1, strlen(nl + 1) + 1);
        }
        if (strlen(line) > sizeof(line) - 50) {
            memset(line, 0, sizeof(line));
        }
    }
    UartClose(rfd);
    printf("[radar] 雷达线程已停止\n");
    return NULL;
}

static void handle_command(chassis_control_t *control, const char *input)
{
    int16_t linear = 0, angular = 0;

    if (strcmp(input, "stop") == 0) {
        chassis_update_continuous_control(control, 0, 0);
        printf("[%s] → 已停止\n", __func__);
        return;
    }
    if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0) {
        printf("[%s] → 收到退出命令\n", __func__);
        g_running = 0;
        return;
    }

    if (sscanf(input, "v:%hd:%hd", &linear, &angular) == 2) goto apply;
    if (sscanf(input, "%hd=%hd", &linear, &angular) == 2) goto apply;

    printf("[%s] → 未知命令: {%s}\n", __func__, input);
    return;

apply:
    if (linear > 1000) linear = 1000;
    if (linear < -1000) linear = -1000;
    if (angular > 2000) angular = 2000;
    if (angular < -2000) angular = -2000;

    if (chassis_update_continuous_control(control, linear, angular) == 0) {
        printf("[%s] → 速度: linear=%d, angular=%d\n", __func__, linear, angular);
    } else {
        printf("[%s] → 设置速度失败\n", __func__);
    }
}

int main(int argc, char *argv[])
{
    const char *uart_dev = NULL;
    int port = TCP_PORT;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--uart-port") == 0 && i + 1 < argc) {
            uart_dev = argv[++i];
        } else if (strcmp(argv[i], "--tcp-port") == 0 && i + 1 < argc) {
            port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    if (!uart_dev) {
        printf("错误: 必须指定 --uart-port\n");
        print_usage(argv[0]);
        return -1;
    }

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    printf("[%s] 串口设备: %s\n", __func__, uart_dev);

    int fd = chassis_init(uart_dev);
    if (fd < 0) { printf("[%s] 底盘初始化失败\n", __func__); return -1; }

    printf("[%s] 底盘上电（5秒初始化）...\n", __func__);
    if (chassis_power_on(fd) != 0) { printf("[%s] 上电失败\n", __func__); chassis_close(fd); return -1; }

    printf("[%s] 切换速度角度模式...\n", __func__);
    chassis_switch_speed_angle_mode(fd);

    g_control = chassis_start_continuous_control(fd, 0, 0);
    if (!g_control) { printf("[%s] 启动控制线程失败\n", __func__); chassis_power_off(fd); chassis_close(fd); return -1; }

    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) { printf("[%s] 创建 socket 失败\n", __func__); goto cleanup; }

    int yes = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(srv, (struct sockaddr *)&addr, sizeof(addr)) < 0) { printf("[%s] bind 失败\n", __func__); close(srv); goto cleanup; }
    if (listen(srv, BACKLOG) < 0) { printf("[%s] listen 失败\n", __func__); close(srv); goto cleanup; }

    g_server_fd = srv;
    printf("[%s] ✅ TCP Server 端口 %d，等待手机连接...\n", __func__, port);

    while (g_running) {
        struct sockaddr_in cli_addr;
        socklen_t clen = sizeof(cli_addr);
        int client = accept(srv, (struct sockaddr *)&cli_addr, &clen);
        if (client < 0) { if (g_running) printf("[%s] accept 失败\n", __func__); continue; }

        g_client_fd = client;
        printf("[%s] 📱 客户端已连接: %s\n", __func__, inet_ntoa(cli_addr.sin_addr));

        /* 启动雷达线程 */
        g_radar_running = 1;
        pthread_create(&g_radar_thread, NULL, radar_thread_func, &client);

        char buf[BUF_SIZE];
        while (g_running) {
            memset(buf, 0, sizeof(buf));
            ssize_t n = recv(client, buf, sizeof(buf) - 1, 0);
            if (n <= 0) {
                printf("[%s] 📱 客户端断开\n", __func__);
                chassis_update_continuous_control(g_control, 0, 0);
                g_radar_running = 0;
                pthread_join(g_radar_thread, NULL);
                break;
            }

            char *saveptr;
            char *line = strtok_r(buf, "\n", &saveptr);
            while (line != NULL) {
                char *e = line + strlen(line) - 1;
                while (e >= line && *e == '\r') { *e = '\0'; e--; }
                if (strlen(line) > 0) { printf("[%s] 收到: {%s}\n", __func__, line); handle_command(g_control, line); }
                line = strtok_r(NULL, "\n", &saveptr);
            }
        }
        close(client); g_client_fd = -1;
    }

    close(srv); g_server_fd = -1;

cleanup:
    printf("[%s] 关闭...\n", __func__);
    if (g_control) { chassis_stop_continuous_control(g_control); g_control = NULL; }
    chassis_power_off(fd);
    chassis_close(fd);
    printf("[%s] 程序退出\n", __func__);
    return 0;
}
