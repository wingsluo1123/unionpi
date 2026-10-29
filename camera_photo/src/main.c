/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-11-01 00:00:00
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-11-01 00:00:00
 * @FilePath: /OpenHarmony/unionpi_whale_demo/camera/src/main.c
 * @Description: 简易HTTP服务 + 按钮触发拍照并显示最新图片
 */

#include "main.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>

/**
 * @brief HTTP服务端口
 */
#define HTTP_PORT 8080

/**
 * @brief 最新图片保存路径
 */
#define LATEST_JPG "./picture/latest.jpg"

/**
 * @brief 确保图片目录存在
 */
static void ensure_picture_dir(void)
{
    struct stat st = {0};
    if (stat("./picture", &st) == -1)
    {
        mkdir("./picture", 0755);
    }
}

/**
 * @brief 发送HTTP响应
 *
 * @param client 客户端套接字
 * @param status HTTP状态码
 * @param ctype 内容类型
 * @param extra_headers 额外的HTTP头
 * @param body 响应体内容
 * @param body_len 响应体长度
 */
static void send_http(int client, const char *status, const char *ctype, const char *extra_headers, const void *body, size_t body_len)
{
    char header[512];
    int n = snprintf(header, sizeof(header),
                     "HTTP/1.1 %s\r\n"
                     "Content-Type: %s\r\n"
                     "Content-Length: %zu\r\n"
                     "%s"
                     "Connection: close\r\n\r\n",
                     status, ctype, body_len, extra_headers ? extra_headers : "");
    send(client, header, n, 0);
    if (body && body_len > 0)
    {
        send(client, body, body_len, 0);
    }
}

/**
 * @brief 处理根路径请求
 *
 * @param client 客户端套接字
 */
static void handle_index(int client)
{
    const char *html = "<!doctype html><html><head><meta charset=\"utf-8\"><title>Camera</title>"
                       "<style>body{font-family:sans-serif;margin:2rem}img{max-width:100%%;height:auto;border:1px solid #ccc}button{padding:.6rem 1rem}</style>"
                       "</head><body>"
                       "<h1>Camera Capture</h1>"
                       "<form method=\"POST\" action=\"/capture\">"
                       "<button type=\"submit\">获取图片并展示</button>"
                       "</form>"
                       "<p>最新图片：</p>"
                       "<img id=\"img\" src=\"/latest.jpg?t=";
    char tbuf[64];
    snprintf(tbuf, sizeof(tbuf), "%jd", (intmax_t)time(NULL));
    const char *tail = "\" alt=\"latest\">"
                       "<script>setInterval(()=>{const img=document.getElementById('img');img.src='/latest.jpg?t='+Date.now()},60000);</script>"
                       "</body></html>";
    char page[2048];
    snprintf(page, sizeof(page), "%s%s%s", html, tbuf, tail);
    send_http(client, "200 OK", "text/html; charset=utf-8", NULL, page, strlen(page));
}

/**
 * @brief 处理最新图片请求
 *
 * @param client 客户端套接字
 */
static void handle_latest(int client)
{
    FILE *fp = fopen(LATEST_JPG, "rb");
    if (!fp)
    {
        const char *msg = "No image yet. Click the button to capture.";
        send_http(client, "404 Not Found", "text/plain; charset=utf-8", NULL, msg, strlen(msg));
        return;
    }
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *buf = (char *)malloc(sz);
    if (!buf)
    {
        fclose(fp);
        const char *msg = "OOM";
        send_http(client, "500 Internal Server Error", "text/plain; charset=utf-8", NULL, msg, strlen(msg));
        return;
    }
    fread(buf, 1, sz, fp);
    fclose(fp);
    send_http(client, "200 OK", "image/jpeg", NULL, buf, sz);
    free(buf);
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
    printf("[%s] Usage: %s --device <device_path> --width <width> --height <height> --port <port>\n", __func__, program_name);
    printf("[%s] Example: %s --device /dev/video0 --width 640 --height 480 --port 8080\n", __func__, program_name);
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
 * @return 成功返回 0，失败返回 -1
 */
int parse_arguments(int argc, char **argv, const char **device_ptr, int *width_ptr, int *height_ptr, int *port_ptr)
{
    // 检查参数数量
    if (argc != 9)
    { // 程序名 + 4个参数对
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
        else
        {
            printf("[%s] Error: Invalid argument '%s'\n", "parse_arguments", argv[i]);
            print_usage(argv[0]);
            return -1;
        }
    }

    // 检查所有必需参数是否都已设置
    if (*device_ptr == NULL || *width_ptr <= 0 || *height_ptr <= 0 || *port_ptr <= 0)
    {
        printf("[%s] Error: All parameters must be specified with valid values\n", "parse_arguments");
        print_usage(argv[0]);
        return -1;
    }

    return 0;
}

/**
 * @brief 主函数入口
 *
 * 程序执行的起始点，用于启动HTTP服务器并处理摄像头拍照请求
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
    const char *device = NULL;

    if (parse_arguments(argc, argv, &device, &width, &height, &port) != 0)
    {
        return -1;
    }

    ensure_picture_dir();

    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0)
    {
        printf("[%s] socket failed\n", __func__);
        return -1;
    }
    int yes = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(srv, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        printf("[%s] bind failed\n", __func__);
        close(srv);
        return -1;
    }
    if (listen(srv, 8) < 0)
    {
        printf("[%s] listen failed\n", __func__);
        close(srv);
        return -1;
    }
    printf("[%s] HTTP server started on port %d. Access http://<IP>:%d/\n", __func__, port, port);

    for (;;)
    {
        struct sockaddr_in cli;
        socklen_t clen = sizeof(cli);
        int c = accept(srv, (struct sockaddr *)&cli, &clen);
        if (c < 0)
        {
            printf("[%s] accept failed\n", __func__);
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
        else if (strcmp(method, "GET") == 0 && strncmp(path, "/latest.jpg", 11) == 0)
        {
            handle_latest(c);
        }
        else if (strcmp(method, "POST") == 0 && strncmp(path, "/capture", 8) == 0)
        {
            char err[256] = {0};
            int rc = CameraCaptureSingleJpeg(device, width, height, LATEST_JPG, err, sizeof(err));
            if (rc == 0)
            {
                // redirect back to /
                const char *hdr = "Location: /\r\n";
                send_http(c, "302 Found", "text/plain", hdr, "OK", 2);
            }
            else if (rc == -2)
            {
                const char *msg = "当前格式非MJPEG，暂不支持自动转码为JPG。请将摄像头设置为MJPEG。";
                send_http(c, "415 Unsupported Media Type", "text/plain; charset=utf-8", NULL, msg, strlen(msg));
            }
            else
            {
                char msg[512];
                snprintf(msg, sizeof(msg), "Capture failed: %s", err);
                send_http(c, "500 Internal Server Error", "text/plain; charset=utf-8", NULL, msg, strlen(msg));
            }
        }
        else
        {
            const char *msg = "Not Found";
            send_http(c, "404 Not Found", "text/plain; charset=utf-8", NULL, msg, strlen(msg));
        }
        close(c);
    }
    close(srv);
    return 0;
}
