/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-11-27 15:00:00
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-11-27 16:36:51
 * @FilePath: /OpenHarmony/unionpi_whale_demo/spi_loopback/include/main.h
 * @Description: SPI回环测试主程序文件
 */

#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "spi_base.h" 

// SPI端口（必须通过命令行指定）
static const char *spi_port = NULL;
static const uint8_t default_tx[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x40, 0x00, 0x00, 0x00, 0x95,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xF0, 0x0D,
};

static void print_usage(const char *prog)
{
    printf("[%s] Usage: %s --spi-port PORT\n", __func__, prog);
    printf("[%s] Options:\n", __func__);
    printf("[%s]   --spi-port PORT    SPI device port (required)\n", __func__);
    printf("[%s] Example: %s --spi-port /dev/spidev0.0\n", __func__, prog);
}

static void parse_opts(int argc, char *argv[])
{
    while (1) {
        static const struct option lopts[] = {
            { "spi-port", 1, 0, 'p' },
            { NULL, 0, 0, 0 },
        };
        int c;

        c = getopt_long(argc, argv, "p:", lopts, NULL);

        if (c == -1)
            break;

        switch (c) {
        case 'p':
            spi_port = optarg;
            break;
        default:
            print_usage(argv[0]);
            exit(1);
            break;
        }
    }

    if (spi_port == NULL || strlen(spi_port) == 0) {
        printf("[%s] Error: --spi-port is required\n", __func__);
        print_usage(argv[0]);
        exit(1);
    }
}

// 十六进制转储函数
static void hex_dump(const uint8_t *data, size_t len, const char *prefix)
{
    printf("[%s] %s: ", __func__, prefix);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0 && i + 1 < len) {
            printf("\n[%s] %s: ", __func__, prefix);
        }
    }
    printf("\n");
}

int main(int argc, char *argv[])
{
    int fd;
    int ret;
    size_t data_len = sizeof(default_tx);
    uint8_t *rx_data;

    parse_opts(argc, argv);

    // 分配接收缓冲区
    rx_data = malloc(data_len);
    if (!rx_data) {
        printf("[%s] Error: Cannot allocate rx buffer\n", __func__);
        return -1;
    }

    // 打开SPI设备
    fd = SpiOpen(spi_port);
    if (fd < 0) {
        printf("[%s] Error: Can't open SPI device %s\n", __func__, spi_port);
        free(rx_data);
        return -1;
    }

    // 配置SPI参数
    SpiConfig config = {
        .mode = 0,          // SPI模式0
        .bits_per_word = 8, // 8位字宽
        .speed_hz = 500000  // 500KHz
    };

    ret = SpiConfigure(fd, &config);
    if (ret != SPI_OK) {
        printf("[%s] Error: Can't configure SPI device\n", __func__);
        SpiClose(fd);
        free(rx_data);
        return -1;
    }

    // 打印配置信息
    printf("[%s] SPI Port: %s\n", __func__, spi_port);
    printf("[%s] SPI Mode: 0x%x\n", __func__, config.mode);
    printf("[%s] Bits per word: %d\n", __func__, config.bits_per_word);
    printf("[%s] Max speed: %d Hz (%d KHz)\n", __func__, config.speed_hz, config.speed_hz / 1000);
    printf("\n");

    // 发送默认数据
    ret = SpiTransfer(fd, default_tx, rx_data, data_len, config.speed_hz);
    if (ret < 0) {
        printf("[%s] Error: SPI transfer failed\n", __func__);
        SpiClose(fd);
        free(rx_data);
        return -1;
    }

    // 打印发送和接收的数据
    printf("[%s] TX data (%zu bytes):\n", __func__, data_len);
    hex_dump(default_tx, data_len, "TX");
    printf("\n");

    printf("[%s] RX data (%zu bytes):\n", __func__, data_len);
    hex_dump(rx_data, data_len, "RX");
    printf("\n");

    // 简单验证回环测试
    if (memcmp(default_tx, rx_data, data_len) == 0) {
        printf("[%s] Loopback test: PASSED\n", __func__);
    } else {
        printf("[%s] Loopback test: FAILED\n", __func__);
    }

    // 清理资源
    SpiClose(fd);
    free(rx_data);

    return 0;
}