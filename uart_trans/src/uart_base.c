/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-30 18:47:19
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-30 18:48:58
 * @FilePath: /OpenHarmony/unionpi_whale_demo/uart_trans/src/uart_base.c
 * @Description: 通用串口工具实现
 */

#include "../include/uart_base.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int UartSetupDataBits(int databits, struct termios *options)
{
    if (options == NULL)
    {
        printf("[%s] Error: options is NULL\n", __func__);
        return UART_ERR;
    }

    options->c_cflag &= ~CSIZE;
    switch (databits)
    {
    case 5:
        options->c_cflag |= CS5;
        break;
    case 6:
        options->c_cflag |= CS6;
        break;
    case 7:
        options->c_cflag |= CS7;
        break;
    case 8:
        options->c_cflag |= CS8;
        break;
    default:
        printf("[%s] Error: Invalid data bits %d\n", __func__, databits);
        return UART_ERR;
    }
    return UART_OK;
}

static int UartSetupParity(int parity, struct termios *options)
{
    if (options == NULL)
    {
        printf("[%s] Error: options is NULL\n", __func__);
        return UART_ERR;
    }

    switch (parity)
    {
    case 'n':
    case 'N':
        options->c_cflag &= ~PARENB;
        options->c_cflag &= ~PARODD;
        options->c_iflag &= ~INPCK;
        break;
    case 'o':
    case 'O':
        options->c_cflag |= PARENB;
        options->c_cflag |= PARODD;
        options->c_iflag |= INPCK;
        break;
    case 'e':
    case 'E':
        options->c_cflag |= PARENB;
        options->c_cflag &= ~PARODD;
        options->c_iflag |= INPCK;
        break;
    case 'm':
    case 'M':
#ifdef CMSPAR
        options->c_cflag |= PARENB | PARODD | CMSPAR;
        options->c_iflag |= INPCK;
        break;
#else
        printf("[%s] Error: CMSPAR not supported for mark parity\n", __func__);
        return UART_ERR;
#endif
    case 's':
    case 'S':
#ifdef CMSPAR
        options->c_cflag |= PARENB | CMSPAR;
        options->c_cflag &= ~PARODD;
        options->c_iflag |= INPCK;
        break;
#else
        printf("[%s] Error: CMSPAR not supported for space parity\n", __func__);
        return UART_ERR;
#endif
    default:
        printf("[%s] Error: Invalid parity %c\n", __func__, (char)parity);
        return UART_ERR;
    }
    return UART_OK;
}

speed_t UartConvertBaud(int baudrate)
{
    switch (baudrate)
    {
    case 2400:
        return B2400;
    case 4800:
        return B4800;
    case 9600:
        return B9600;
    case 19200:
        return B19200;
    case 38400:
        return B38400;
    case 57600:
        return B57600;
    case 115200:
        return B115200;
    case 230400:
        return B230400;
    case 460800:
        return B460800;
#ifdef B921600
    case 921600:
        return B921600;
#endif
#ifdef B1000000
    case 1000000:
        return B1000000;
#endif
#ifdef B1152000
    case 1152000:
        return B1152000;
#endif
    default:
        printf("[%s] Warning: Unsupported baudrate %d, using default 115200\n", __func__, baudrate);
        return B115200;
    }
}

int UartConfigure(int fd, const struct SerialPortConfig *config)
{
    if (fd < 0 || config == NULL)
    {
        printf("[%s] Error: Invalid parameters (fd=%d, config=%p)\n", __func__, fd, config);
        return UART_ERR;
    }

    struct termios options;
    if (tcgetattr(fd, &options) != 0)
    {
        printf("[%s] Error: tcgetattr failed: %s\n", __func__, strerror(errno));
        return UART_ERR;
    }

    const speed_t speed = UartConvertBaud(config->baudrate);
    if (cfsetispeed(&options, speed) != 0 || cfsetospeed(&options, speed) != 0)
    {
        printf("[%s] Error: cfset speed failed: %s\n", __func__, strerror(errno));
        return UART_ERR;
    }

    if (config->raw_mode)
    {
        options.c_iflag = 0;
        options.c_oflag = 0;
        options.c_lflag = 0;
    }

    if (UartSetupDataBits(config->databits, &options) != UART_OK)
    {
        printf("[%s] Error: Failed to setup data bits %d\n", __func__, config->databits);
        return UART_ERR;
    }

    if (UartSetupParity(config->parity, &options) != UART_OK)
    {
        printf("[%s] Error: Failed to setup parity %c\n", __func__, (char)config->parity);
        return UART_ERR;
    }

    switch (config->stopbits)
    {
    case 1:
        options.c_cflag &= ~CSTOPB;
        break;
    case 2:
        options.c_cflag |= CSTOPB;
        break;
    default:
        printf("[%s] Error: Invalid stop bits %d\n", __func__, config->stopbits);
        return UART_ERR;
    }

#ifdef CRTSCTS
    if (config->flow_control == SERIAL_FLOW_HARDWARE)
    {
        options.c_cflag |= CRTSCTS;
    }
    else
    {
        options.c_cflag &= ~CRTSCTS;
    }
#endif

    options.c_cflag |= (CLOCAL | CREAD);

    if (config->vtime >= 0)
    {
        options.c_cc[VTIME] = (cc_t)config->vtime;
    }
    if (config->vmin >= 0)
    {
        options.c_cc[VMIN] = (cc_t)config->vmin;
    }

    tcflush(fd, TCIOFLUSH);
    if (tcsetattr(fd, TCSANOW, &options) != 0)
    {
        printf("[%s] Error: tcsetattr failed: %s\n", __func__, strerror(errno));
        return UART_ERR;
    }

    tcflush(fd, TCIOFLUSH);
    return UART_OK;
}

int UartInit(int fd, const struct SerialPortConfig *config) { return UartConfigure(fd, config); }

int UartOpen(const char *device, int flags)
{
    if (device == NULL)
    {
        printf("[%s] Error: Device path is NULL\n", __func__);
        return UART_ERR;
    }

    int openFlags = flags;
    if ((flags & O_NOCTTY) == 0)
    {
        openFlags |= O_NOCTTY;
    }

    const int fd = open(device, openFlags);
    if (fd < 0)
    {
        printf("[%s] Error: Failed to open device %s: %s\n", __func__, device, strerror(errno));
        return UART_ERR;
    }

    return fd;
}

void UartClose(int fd)
{
    if (fd >= 0)
    {
        close(fd);
    }
}

ssize_t UartRead(int fd, void *buffer, size_t length)
{
    if (fd < 0 || buffer == NULL || length == 0)
    {
        printf("[%s] Error: Invalid parameters (fd=%d, buffer=%p, length=%zu)\n", __func__, fd, buffer, length);
        errno = EINVAL;
        return -1;
    }

    ssize_t result = read(fd, buffer, length);
    if (result < 0)
    {
        printf("[%s] Error: Read failed: %s\n", __func__, strerror(errno));
    }
    // 移除正常读取的调试输出，包括读取到数据时的输出

    return result;
}

ssize_t UartWrite(int fd, const void *buffer, size_t length)
{
    if (fd < 0 || buffer == NULL || length == 0)
    {
        printf("[%s] Error: Invalid parameters (fd=%d, buffer=%p, length=%zu)\n", __func__, fd, buffer, length);
        errno = EINVAL;
        return -1;
    }

    ssize_t result = write(fd, buffer, length);
    if (result < 0)
    {
        printf("[%s] Error: Write failed: %s\n", __func__, strerror(errno));
    }
    // 移除正常写入的调试输出

    return result;
}
