/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-31 12:30:00
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-31 12:30:00
 * @FilePath: /OpenHarmony/unionpi_whale_demo/uart_imu/src/main.c
 * @Description: UART IMU 传感器案例源文件
 */

#include "../include/main.h"

/**
 * @brief 串口文件描述符
 */
static int fd, s_iCurBaud = 9600;

/**
 * @brief 数据更新标志
 */
volatile char s_cDataUpdate = 0;

/**
 * @brief 支持的波特率列表
 */
const int c_uiBaud[] = {2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600};

/**
 * @brief 打印使用帮助信息
 */
void print_usage(const char *program_name)
{
    printf("[%s] Usage: %s --uart-port <device_path>\n", __func__, program_name);
    printf("[%s] Example: %s --uart-port /dev/ttyUSB0\n", __func__, program_name);
    printf("[%s] Example: %s --uart-port /dev/ttyS0\n", __func__, program_name);
}

/**
 * @brief 解析命令行参数
 *
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 成功返回串口设备路径，失败返回NULL
 */
const char *parse_arguments(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("[%s] Error: Invalid number of arguments\n", __func__);
        print_usage(argv[0]);
        return NULL;
    }

    if (strcmp(argv[1], "--uart-port") != 0)
    {
        printf("[%s] Error: Invalid argument '%s'\n", __func__, argv[1]);
        print_usage(argv[0]);
        return NULL;
    }

    const char *uart_port = argv[2];
    if (uart_port == NULL || strlen(uart_port) == 0)
    {
        printf("[%s] Error: Invalid UART port specified\n", __func__);
        print_usage(argv[0]);
        return NULL;
    }

    return uart_port;
}

/**
 * @brief 主函数入口
 *
 * 程序执行的起始点，用于初始化IMU传感器并通过UART读取传感器数据
 *
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组，必须包含--uart-port和串口设备路径
 * @return 返回程序执行状态码，正常退出返回0
 */
int main(int argc, char *argv[])
{
    // 解析命令行参数
    const char *uart_dev = parse_arguments(argc, argv);
    if (uart_dev == NULL)
    {
        return UART_ERR;
    }

    printf("[%s] Using UART IMU device: %s\n", __func__, uart_dev);

    // 打开串口设备
    fd = UartOpen(uart_dev, O_RDWR | O_NOCTTY);
    if (fd == UART_ERR)
    {
        printf("[%s] Failed to open UART device: %s\n", __func__, strerror(errno));
        return UART_ERR;
    }

    printf("[%s] UART IMU device opened successfully, fd: %d\n", __func__, fd);

    // 配置串口参数
    struct SerialPortConfig config = {
        .baudrate = 9600,
        .databits = 8,
        .stopbits = 1,
        .parity = 'N',
        .vtime = 0, // 非阻塞
        .vmin = 0,  // 非阻塞读取
        .flow_control = SERIAL_FLOW_NONE,
        .raw_mode = 1 // 使用原始模式
    };

    if (UartInit(fd, &config) != UART_OK)
    {
        printf("[%s] Failed to initialize UART IMU: %s\n", __func__, strerror(errno));
        UartClose(fd);
        return UART_ERR;
    }

    printf("[%s] UART IMU initialized successfully (9600 8N1)\n", __func__);

    float fAcc[3], fGyro[3], fAngle[3];
    int i;
    char cBuff[1];

    WitInit(WIT_PROTOCOL_NORMAL, 0x50);
    WitRegisterCallBack(SensorDataUpdata);

    printf("[%s] ********************** WIT-Motion IMU Sensor Example ************************\n", __func__);
    printf("[%s] Scanning for IMU sensor...\n", __func__);

    // 自动扫描传感器
    if (AutoScanSensor(fd, uart_dev) != 0)
    {
        printf("[%s] Cannot find IMU sensor, please check your connection\n", __func__);
        UartClose(fd);
        return UART_ERR;
    }

    printf("[%s] IMU sensor found! Starting data reading...\n", __func__);
    printf("[%s] Press Ctrl+C to exit\n", __func__);

    while (1)
    {
        while (UartRead(fd, (unsigned char *)cBuff, 1) > 0)
        {
            WitSerialDataIn(cBuff[0]);
        }

        printf("*************************************************************************\n");
        Delayms(500);

        if (s_cDataUpdate)
        {
            for (i = 0; i < 3; i++)
            {
                fAcc[i] = sReg[AX + i] / 32768.0f * 16.0f;
                fGyro[i] = sReg[GX + i] / 32768.0f * 2000.0f;
                fAngle[i] = sReg[Roll + i] / 32768.0f * 180.0f;
            }

            if (s_cDataUpdate & ACC_UPDATE)
            {
                printf("Acceleration (g):   X=%.3f, Y=%.3f, Z=%.3f\n", fAcc[0], fAcc[1], fAcc[2]);
                s_cDataUpdate &= ~ACC_UPDATE;
            }
            if (s_cDataUpdate & GYRO_UPDATE)
            {
                printf("Gyroscope (°/s):    X=%.3f, Y=%.3f, Z=%.3f\n", fGyro[0], fGyro[1], fGyro[2]);
                s_cDataUpdate &= ~GYRO_UPDATE;
            }
            if (s_cDataUpdate & ANGLE_UPDATE)
            {
                printf("Angle (°):          Roll=%.3f, Pitch=%.3f, Yaw=%.3f\n", fAngle[0], fAngle[1], fAngle[2]);
                s_cDataUpdate &= ~ANGLE_UPDATE;
            }
            if (s_cDataUpdate & MAG_UPDATE)
            {
                printf("Magnetometer:       X=%d, Y=%d, Z=%d\n", sReg[HX], sReg[HY], sReg[HZ]);
                s_cDataUpdate &= ~MAG_UPDATE;
            }
        }
        else
        {
            printf("Waiting for IMU data...\n");
        }
    }

    UartClose(fd);
    return 0;
}

/**
 * @brief 传感器数据更新回调函数
 *
 * 当传感器数据更新时被调用，设置相应的更新标志位
 *
 * @param uiReg 寄存器地址
 * @param uiRegNum 寄存器数量
 */
void SensorDataUpdata(uint32_t uiReg, uint32_t uiRegNum)
{
    int i;
    for (i = 0; i < uiRegNum; i++)
    {
        switch (uiReg)
        {
            //            case AX:
            //            case AY:
        case AZ:
            s_cDataUpdate |= ACC_UPDATE;
            break;
            //            case GX:
            //            case GY:
        case GZ:
            s_cDataUpdate |= GYRO_UPDATE;
            break;
            //            case HX:
            //            case HY:
        case HZ:
            s_cDataUpdate |= MAG_UPDATE;
            break;
            //            case Roll:
            //            case Pitch:
        case Yaw:
            s_cDataUpdate |= ANGLE_UPDATE;
            break;
        default:
            s_cDataUpdate |= READ_UPDATE;
            break;
        }
        uiReg++;
    }
}

/**
 * @brief 延时函数实现
 *
 * @param ucMs 延时毫秒数
 */
void Delayms(uint16_t ucMs) { usleep(ucMs * 1000); }

/**
 * @brief 自动扫描传感器设备
 *
 * 自动检测并连接到IMU传感器设备
 *
 * @param fd 串口文件描述符
 * @param dev 设备路径
 * @return 成功返回0，失败返回-1
 */
int AutoScanSensor(int fd, const char *dev)
{
    int i, iRetry;
    char cBuff[1];
    struct SerialPortConfig config;

    for (i = 1; i < 10; i++)
    {
        s_iCurBaud = c_uiBaud[i];

        // 重新配置串口波特率
        config.baudrate = s_iCurBaud;
        config.databits = 8;
        config.stopbits = 1;
        config.parity = 'N';
        config.vtime = 0;
        config.vmin = 0;
        config.flow_control = SERIAL_FLOW_NONE;
        config.raw_mode = 1;

        if (UartConfigure(fd, &config) != UART_OK)
        {
            continue;
        }

        iRetry = 2;
        do
        {
            s_cDataUpdate = 0;
            WitReadReg(AX, 3);
            Delayms(200);
            while (UartRead(fd, (unsigned char *)cBuff, 1) > 0)
            {
                WitSerialDataIn(cBuff[0]);
            }
            if (s_cDataUpdate != 0)
            {
                printf("%d baud find sensor\r\n\r\n", c_uiBaud[i]);
                return 0;
            }
            iRetry--;
        } while (iRetry);
    }
    printf("can not find sensor\r\n");
    printf("please check your connection\r\n");
    return -1;
}