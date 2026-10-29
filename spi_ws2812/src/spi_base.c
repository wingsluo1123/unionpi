/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-31 15:00:00
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-31 13:54:35
 * @FilePath: /OpenHarmony/unionpi_whale_demo/spi_ws2812/src/spi_base.c
 * @Description: SPI基础驱动实现文件
 */

#include "../include/spi_base.h"
#include "../include/spidev.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

/**
 * @brief 打开SPI设备
 * @param device SPI设备路径
 * @return 成功返回文件描述符，失败返回SPI_ERR
 */
int SpiOpen(const char *device)
{
    if (device == NULL)
    {
        printf("[%s] Error: Device path is NULL\n", __func__);
        return SPI_ERR;
    }

    int fd = open(device, O_RDWR);
    if (fd < 0)
    {
        printf("[%s] Error: Failed to open SPI device %s: %s\n", __func__, device, strerror(errno));
        return SPI_ERR;
    }

    return fd;
}

/**
 * @brief 关闭SPI设备
 * @param fd SPI设备文件描述符
 */
void SpiClose(int fd)
{
    if (fd >= 0)
    {
        close(fd);
    }
}

/**
 * @brief 配置SPI参数
 * @param fd SPI设备文件描述符
 * @param config SPI配置参数
 * @return 成功返回SPI_OK，失败返回SPI_ERR
 */
int SpiConfigure(int fd, const SpiConfig *config)
{
    if (fd < 0 || config == NULL)
    {
        printf("[%s] Error: Invalid parameters (fd=%d, config=%p)\n", __func__, fd, (void *)config);
        return SPI_ERR;
    }

    // 配置SPI模式
    if (ioctl(fd, SPI_IOC_WR_MODE, &config->mode) == -1)
    {
        printf("[%s] Error: Cannot set SPI mode: %s\n", __func__, strerror(errno));
        return SPI_ERR;
    }

    // 配置位宽
    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &config->bits_per_word) == -1)
    {
        printf("[%s] Error: Cannot set bits per word: %s\n", __func__, strerror(errno));
        return SPI_ERR;
    }

    // 配置速度
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &config->speed_hz) == -1)
    {
        printf("[%s] Error: Cannot set max speed: %s\n", __func__, strerror(errno));
        return SPI_ERR;
    }

    return SPI_OK;
}

/**
 * @brief SPI数据传输
 * @param fd SPI设备文件描述符
 * @param tx_buffer 发送缓冲区
 * @param rx_buffer 接收缓冲区（可为NULL）
 * @param length 传输数据长度
 * @param speed_hz 传输速度（0使用默认速度）
 * @return 成功返回传输的字节数，失败返回SPI_ERR
 */
int SpiTransfer(int fd, const uint8_t *tx_buffer, uint8_t *rx_buffer, size_t length, uint32_t speed_hz)
{
    if (fd < 0 || tx_buffer == NULL || length == 0)
    {
        printf("[%s] Error: Invalid parameters (fd=%d, tx_buffer=%p, length=%zu)\n", __func__, fd, (void *)tx_buffer, length);
        return SPI_ERR;
    }

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx_buffer,
        .rx_buf = (unsigned long)rx_buffer,
        .len = length,
        .speed_hz = speed_hz,
        .bits_per_word = 8,
    };

    int ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 1)
    {
        printf("[%s] Error: SPI transfer failed: %s\n", __func__, strerror(errno));
        return SPI_ERR;
    }

    return ret;
}