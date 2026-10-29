/**
 ******************************************************************************
 * @file           : pthread_usr.c
 * @author         : Dylan
 * @brief          : 线程函数实现
 * @attention      : 正在构建
 * @date           : 2023.3.22
 ******************************************************************************
 */

#include "pthread_usr.h"
#include "square_walk.h"
#include "circle_walk.h"        //走圆圈的头文件

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

PC_SendType send_data;
extern integral_acc integral_length;
float fAcc[3], fGyro[3], fAngle[3];
int mode_flag = 0;
int vel_temp = 0, yaw_temp = 0, ang_temp = 0;
float cumulativeZAngle = 0.0f;
int new_socket = -1;

static SquareWalkController g_square_ctrl;
static CircleWalkController g_circle_ctrl;         //走圆圈

// 里程计全局变量
static int32_t g_left_front_odom = 0;
static int32_t g_left_back_odom = 0;
static int32_t g_right_front_odom = 0;
static int32_t g_right_back_odom = 0;
static float g_current_total_dist = 0.0f;
static float g_last_total_dist = 0.0f;

// 辅助函数：解析4字节小端有符号整数
static int32_t parse_int32(unsigned char *data) {
    return (int32_t)(data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
}

// 辅助函数：计算校验和
static unsigned char calculate_xor_checksum(unsigned char *data, int len) {
    unsigned char checksum = 0;
    for (int i = 2; i < len - 1; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

#define ACC_UPDATE 0x01
#define GYRO_UPDATE 0x02
#define ANGLE_UPDATE 0x04
#define MAG_UPDATE 0x08
#define READ_UPDATE 0x80
/**
 * @attention 暂时未启用的串口读取函数
 * @return
 */


void *_serial_input_task(void)
{
    unsigned char state = 0;
    unsigned char packet[14];
    unsigned char ch;
    pthread_detach(pthread_self());
    
    printf("[Chassis] Input task started\n");
    
    while (1)
    {
        if (read(fd_chassis, &ch, 1) <= 0) {
            usleep(1000);
            continue;
        }

        switch (state) {
            case 0: // 等待 0x0A
                if (ch == 0x0A) state = 1;
                break;
            case 1: // 等待 0x0C
                if (ch == 0x0C) {
                    packet[0] = 0x0A;
                    packet[1] = 0x0C;
                    state = 2;
                } else {
                    state = 0;
                }
                break;
            case 2: // 读取剩余 12 字节 (packet[2] 到 packet[13])
                packet[2] = ch;
                int bytes_to_read = 11;
                int bytes_read = 0;
                while (bytes_read < bytes_to_read) {
                    int n = read(fd_chassis, &packet[3 + bytes_read], bytes_to_read - bytes_read);
                    if (n > 0) bytes_read += n;
                    else usleep(500);
                }

                // 校验
                if (packet[13] == calculate_xor_checksum(packet, 14)) {
                    unsigned char cmd = packet[4];
                    if (cmd == 0x61) { // 右侧里程
                        g_right_back_odom = parse_int32(&packet[5]);
                        g_right_front_odom = parse_int32(&packet[9]);
                    } else if (cmd == 0x62) { // 左侧里程
                        g_left_front_odom = parse_int32(&packet[5]);
                        g_left_back_odom = parse_int32(&packet[9]);
                        
                        // 更新总距离 - 使用差速里程计
                        // 注意：左右轮方向可能相反，需要取绝对值
                        float avg_left = ((float)abs(g_left_front_odom) + (float)abs(g_left_back_odom)) / 2.0f;
                        float avg_right = ((float)abs(g_right_front_odom) + (float)abs(g_right_back_odom)) / 2.0f;
                        // 使用两侧平均值作为行驶距离（单位：mm -> m）
                        g_current_total_dist = (avg_left + avg_right) / 2.0f / 1000.0f;
                        
                        // 调试：打印原始里程计数据
                        static int odom_counter = 0;
                        if (++odom_counter % 10 == 0) {
                            printf("[Odom] L:%d/%d R:%d/%d -> avg_L:%.1f avg_R:%.1f -> %.3fm\n",
                                   g_left_front_odom, g_left_back_odom,
                                   g_right_front_odom, g_right_back_odom,
                                   avg_left, avg_right,
                                   g_current_total_dist);
                        }
                    }
                }
                state = 0;
                break;
        }
    }
}

void _serial_output_task(void)
{
    
    pthread_detach(pthread_self());
    send_data = Power(1);
    write(fd_chassis, (unsigned char *)send_data.send_data, 12);
    
    // 发送切换到速度角速度模式指令 (0x0A 0x0C 0x01 0x00 0x05 0x00 0x04)
    unsigned char switch_mode_cmd[] = {0x0A, 0x0C, 0x01, 0x00, 0x05, 0x00, 0x04};
    write(fd_chassis, switch_mode_cmd, sizeof(switch_mode_cmd));
    
    int forward_setAngle = -1000;
    
    while (1)
    {
        // pthread_mutex_lock(&lock);
        
        if (mode_flag == 1) {
            // 方形行走模式 (使用 SquareWalk 控制器)
            // 计算距离增量
            float dist_delta = g_current_total_dist - g_last_total_dist;
            g_last_total_dist = g_current_total_dist;

            // 调试输出：每 20 次循环打印一次（约 1 秒）
            static int debug_counter = 0;
            if (++debug_counter % 20 == 0) {
                printf("[DEBUG] dist=%.3f, delta=%.3f, yaw=%.1f\n", 
                       g_current_total_dist, dist_delta, fAngle[2]);
            }

            // 如果增量异常大（比如刚启动或溢出），则忽略
            if (fabs(dist_delta) > 0.5f) dist_delta = 0.0f;

            // 更新状态并获取控制量 (每 50ms 周期)
            square_walk_update(&g_square_ctrl, fAngle[2], dist_delta);

            int16_t vel, ang;
            square_walk_get_control(&g_square_ctrl, &vel, &ang);

            send_data = ChangeSendType(vel, 0, ang);
            write(fd_chassis, send_data.send_data, 12);

            usleep(50000); 
        } else if (mode_flag == 2) {
            if (forward_setAngle == -1000){
                forward_setAngle = cumulativeZAngle;
            }
            if (cumulativeZAngle >= (forward_setAngle-2) && cumulativeZAngle <= (forward_setAngle+2)){
                vel_temp = 20;
                yaw_temp = 0; 
                ang_temp = 0;
                send_data = ChangeSendType(vel_temp, yaw_temp, ang_temp);
                write(fd_chassis, (unsigned char *)send_data.send_data, 12);
                usleep(300000);
            } else if ((cumulativeZAngle - forward_setAngle) <= 180 && (cumulativeZAngle - forward_setAngle) >0) {
                vel_temp = 0;
                yaw_temp = 0; 
                ang_temp = -10;
                send_data = ChangeSendType(vel_temp, yaw_temp, ang_temp);
                write(fd_chassis, (unsigned char *)send_data.send_data, 12);
                usleep(300000);
            } else {
                vel_temp = 0;
                yaw_temp = 0; 
                ang_temp = 10;
                send_data = ChangeSendType(vel_temp, yaw_temp, ang_temp);
                write(fd_chassis, (unsigned char *)send_data.send_data, 12);
                usleep(300000);
            }
        } else if (mode_flag == 3) {
            float dist_delta = g_current_total_dist - g_last_total_dist;
            g_last_total_dist = g_current_total_dist;

            if (fabs(dist_delta) > 0.5f) dist_delta = 0.0f;

            circle_walk_update(&g_circle_ctrl, fAngle[2], dist_delta);

            int16_t vel, ang;
            circle_walk_get_control(&g_circle_ctrl, &vel, &ang);

            send_data = ChangeSendType(vel, 0, ang);
            write(fd_chassis, send_data.send_data, 12);

            usleep(50000);
        } else {
            forward_setAngle = -1000;
            // printf("vel_temp:%d yaw_temp:%d ang_temp:%d\n",vel_temp, yaw_temp, ang_temp);
            send_data = ChangeSendType(vel_temp, yaw_temp, ang_temp);
            // printf("vel_mid: %d \n", vel_temp);
            write(fd_chassis, (unsigned char *)send_data.send_data, 12);
            // printf("vel_after:%d \n", vel_temp);
            usleep(300000);
        }
        // pthread_mutex_lock(&lock);
        // if (ret > 0)
        // {
        //      printf("send success, data is  \n");
        // }
        // else
        // {
        //     printf("send error!\r\n");
        // }
        
    }
}
/**
 * @brief imu调用
 */

float normalizeAngle(float angle) {
    while (angle > 180.0f) {
        angle -= 360.0f;
    }
    while (angle < -180.0f) {
        angle += 360.0f;
    }
    return angle;
}

// 计算在y轴上的重力加速度分量
float computeGravityY(float pitchAngle) {
    // 计算重力加速度分量
    return sin(pitchAngle * M_PI / 180.0);
}


void _serial_IMU_input_task(void)
{
    int i;
    char cBuff[1];
    
    const float deltaTime = 0.01f;  // 10毫秒
    while (1)
    {
        while (serial_read_data(fd_imu, cBuff, 1))
        {
            WitSerialDataIn(cBuff[0]);
        }
        // printf("\n");
        usleep(10000);
        if (s_cDataUpdate)
        {
            // pthread_mutex_lock(&lock);
            for (i = 0; i < 3; i++)
            {
                fAcc[i] = sReg[AX + i] / 32768.0f * 16.0f;
                fGyro[i] = sReg[GX + i] / 32768.0f * 2000.0f;
                fAngle[i] = sReg[Roll + i] / 32768.0f * 180.0f;
            }
            // 更新累计z轴角度
            cumulativeZAngle += fGyro[2] * deltaTime;
            cumulativeZAngle = normalizeAngle(cumulativeZAngle);
            
            // 更新PID控制器的实际角度 - 关键修复！
            static float last_angle = 0.0f;
            static int angle_update_counter = 0;
            
            // 检查IMU数据是否有效更新
            if (s_cDataUpdate & ANGLE_UPDATE) {
                pid.ActualAngle = fAngle[2];  // 使用IMU的Yaw角度作为实际角度
                last_angle = fAngle[2];
                angle_update_counter++;
                
                // 实时调试信息 - 每次角度更新都打印
                char imu_debug[200];
                int len = snprintf(imu_debug, sizeof(imu_debug), 
                                 "IMU ANGLE UPDATE - Raw: %d, Angle: %.3f, Counter: %d\n", 
                                 sReg[Roll + 2], fAngle[2], angle_update_counter);
                if (new_socket > 0) {
                    send(new_socket, imu_debug, len, 0);
                }
             } else {
                // 如果没有新的角度数据，使用上次的角度值
                pid.ActualAngle = last_angle;
                
                // 警告：没有收到角度更新
                static int no_update_counter = 0;
                no_update_counter++;
                if (no_update_counter % 50 == 0) {  // 每50次循环警告一次
                    char warning[200];
                    int len = snprintf(warning, sizeof(warning), 
                                     "WARNING: No IMU angle update! Using last angle: %.3f\n", 
                                     last_angle);
                    if (new_socket > 0) {
                        send(new_socket, warning, len, 0);
                    }
                  }
            }
            
            s_cDataUpdate = 0;  // 重置标志，等待下次数据更新
            // if (s_cDataUpdate & ACC_UPDATE)
            // {
            //     sprintf(imu_acc_buf, "acc:%.3f,%.3f,%.3f\r\n", fAcc[0], fAcc[1], fAcc[2]);
            //     printf("acc:%.3f %.3f %.3f\r\n", fAcc[0], fAcc[1], fAcc[2]);
            // }
            // if (s_cDataUpdate & GYRO_UPDATE)
            // {
            //     printf("gyro:%.3f %.3f %.3f\r\n", fGyro[0], fGyro[1], fGyro[2]);
            // }
            // if (s_cDataUpdate & ANGLE_UPDATE)
            // {
            //     printf("angle:%.3f %.3f %.3f\r\n", fAngle[0], fAngle[1], fAngle[2]);
            // }
            // if (s_cDataUpdate & MAG_UPDATE)
            // {
            //     printf("mag:%d %d %d\r\n", sReg[HX], sReg[HY], sReg[HZ]);
            // }
            // pthread_mutex_unlock(&lock);
        }
    }

    serial_close(fd_imu);
}

/**
 * @brief tcp_client_socket线程
 */


char buffer[BUFFER_SIZE];

int set_non_blocking(int sockfd) {
    int flags;
#if defined(O_NONBLOCK)
    if (-1 == (flags = fcntl(sockfd, F_GETFL, 0)))
        flags = 0;
    return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
#else
    flags = 1;
    return ioctl(sockfd, FIONBIO, &flags);
#endif
}

void _tcp_server_socket(void) {
    int server_fd, local_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    char *message = "Sever recevied!";

    // 创建TCP套接字
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // 设置套接字选项，允许地址重用
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(server_fd);  // 确保在错误情况下关闭套接字
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 绑定套接字到指定端口
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 监听连接
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    char debug_message[200];
    int length = snprintf(debug_message, sizeof(debug_message), "Waiting for connection...\n");
    
    // 发送调试信息到TCP客户端（如果连接存在）
    if (new_socket > 0) {
        if (send(new_socket, debug_message, length, 0) == -1) {
            perror("send debug message failed");
        }
    }

    while (1) {
        if ((local_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }

        // 将局部套接字赋值给全局变量
        new_socket = local_socket;

        length = snprintf(debug_message, sizeof(debug_message), "Connection established with client\n");
        
        // 发送调试信息到TCP客户端
        if (send(new_socket, debug_message, length, 0) == -1) {
            perror("send debug message failed");
        }

        // 循环等待客户端数据
        while (1) {
            int valread = read(new_socket, buffer, BUFFER_SIZE);
            if (valread <= 0) {
                length = snprintf(debug_message, sizeof(debug_message), "Client disconnected or read error\n");
                
                // 发送调试信息到TCP客户端（如果连接仍然有效）
                if (new_socket > 0) {
                    send(new_socket, debug_message, length, 0);  // 尝试发送，但不检查错误
                }
                close(new_socket);
                break;
            }

            
            int filtered_length = 0;
            for (int i = 0; i < valread; ++i) {
                if (buffer[i] != '\r' && buffer[i] != '\n') {
                    buffer[filtered_length++] = buffer[i];
                }
            }
            buffer[filtered_length] = '\0';
            if (filtered_length > 0) {
                length = snprintf(debug_message, sizeof(debug_message), 
                                "Received from client: %.*s\nFiltered length: %d\n", 
                                filtered_length, buffer, filtered_length);
                
                // 发送调试信息到TCP客户端
                if (send(new_socket, debug_message, length, 0) == -1) {
                    perror("send debug message failed");
                }
                pthread_mutex_lock(&lock);
                if (strcmp(buffer, "1") == 0) {
                    mode_flag = 0;
                    vel_temp = 200; yaw_temp = 0; ang_temp = 0;
                } else if (strcmp(buffer, "2") == 0) {
                    mode_flag = 0;
                    vel_temp = -200; yaw_temp = 0; ang_temp = 0;
                } else if (strcmp(buffer, "3") == 0) {
                    mode_flag = 0;
                    vel_temp = 0; yaw_temp = 0; ang_temp = 200;
                } else if (strcmp(buffer, "4") == 0) {
                    mode_flag = 0;
                    vel_temp = 0; yaw_temp = 0; ang_temp = -200;
                } else if (strcmp(buffer, "5") == 0) {
                    mode_flag = 0;
                    vel_temp = 200; yaw_temp = 200; ang_temp = 0;
                } else if (strcmp(buffer, "6") == 0) {
                    mode_flag = 0;
                    vel_temp = 20; yaw_temp = -20; ang_temp = 0;
                } else if (strcmp(buffer, "7") == 0) {
                    mode_flag = 0;
                    vel_temp = 200; yaw_temp = 0; ang_temp = 0;
                } else if (strncmp(buffer, "square", 6) == 0) {
                    // 初始化并启动方形轨迹模式，支持 square 或 square 0.5 格式
                    mode_flag = 1;
                    g_last_total_dist = g_current_total_dist;
                    float side_length = 0.1f;
                    if (strlen(buffer) > 6) {
                        char *param = buffer + 7;
                        side_length = atof(param);
                        if (side_length <= 0.01f) side_length = 0.1f;
                    }
                    square_walk_init(&g_square_ctrl, side_length, 50.0f);
                    square_walk_start(&g_square_ctrl);
                    printf("[TCP] Square walk started with side length: %.2fm\n", side_length);
                } else if (strncmp(buffer, "circle", 6) == 0) {
                    // 初始化并启动圆形轨迹模式，支持 circle 或 circle 0.5 格式
                    mode_flag = 3;
                    g_last_total_dist = g_current_total_dist;
                    float radius = 0.1f;
                    if (strlen(buffer) > 6) {
                        char *param = buffer + 7;
                        radius = atof(param);
                        if (radius <= 0.01f) radius = 0.1f;
                    }
                    circle_walk_init(&g_circle_ctrl, radius, 50.0f);
                    circle_walk_start(&g_circle_ctrl);
                    printf("[TCP] Circle walk started with radius: %.2fm\n", radius);
                } else if (strcmp(buffer, "turn_left") == 0) {
                    // 汽车式前进左转：前进 + 左转角速度（转弯半径 = vel/ang ≈ 40）
                    mode_flag = 0;
                    vel_temp = 200; yaw_temp = 0; ang_temp = 5;
                } else if (strcmp(buffer, "turn_right") == 0) {
                    // 汽车式前进右转：前进 + 右转角速度
                    mode_flag = 0;
                    vel_temp = 200; yaw_temp = 0; ang_temp = -5;
                } else if (strcmp(buffer, "turn_back_left") == 0) {
                    // 汽车式后退左转：后退 + 右转角速度（车体向左后方移动）
                    mode_flag = 0;
                    vel_temp = -200; yaw_temp = 0; ang_temp = -5;
                } else if (strcmp(buffer, "turn_back_right") == 0) {
                    // 汽车式后退右转：后退 + 左转角速度（车体向右后方移动）
                    mode_flag = 0;
                    vel_temp = -200; yaw_temp = 0; ang_temp = 5;
                } else if (strcmp(buffer, "forward") == 0) {
                    mode_flag = 2;
                } else if (strcmp(buffer, "stop") == 0) {
                    // 添加停止指令
                    mode_flag = 0;
                    vel_temp = 0; yaw_temp = 0; ang_temp = 0;
                } else {
                    mode_flag = 0;
                    vel_temp = 0; yaw_temp = 0; ang_temp = 0;
                }
                
                length = snprintf(debug_message, sizeof(debug_message), 
                                "mode_flag: %d\nUpdated values: vel_temp = %d, yaw_temp = %d, ang_temp = %d\n", 
                                mode_flag, vel_temp, yaw_temp, ang_temp);
                
                // 发送调试信息到TCP客户端
                if (send(new_socket, debug_message, length, 0) == -1) {
                    perror("send debug message failed");
                }

                pthread_mutex_unlock(&lock);
                if (send(new_socket, message, strlen(message), 0) == -1) {
                    length = snprintf(debug_message, sizeof(debug_message), "defeat\n");
                    
                    // 发送调试信息到TCP客户端（如果连接仍然有效）
                    if (new_socket > 0) {
                        send(new_socket, debug_message, length, 0);  // 尝试发送，但不检查错误
                    }
                    perror("send");
                    close(new_socket);
                    break;
                }
            }
            memset(buffer, 0, BUFFER_SIZE);
        }
    }

    close(server_fd);
}

