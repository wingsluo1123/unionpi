/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-31 15:00:00
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-11-27 16:36:59
 * @FilePath: /OpenHarmony/unionpi_whale_demo/spi_ws2812/include/spi_base.h
 * @Description: SPI基础驱动接口定义文件
 */

#ifndef __SPI_BASE_H__
#define __SPI_BASE_H__

#include <stdint.h> 
#include <sys/types.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // SPI配置结构体
    typedef struct
    {
        uint8_t mode;          // SPI模式 (0-3)
        uint8_t bits_per_word; // 每个字的位数
        uint32_t speed_hz;     // SPI频率
    } SpiConfig;

// 错误码定义
#define SPI_OK 0
#define SPI_ERR (-1)

    /**
     * @brief 打开SPI设备
     * @param device SPI设备路径
     * @return 成功返回文件描述符，失败返回SPI_ERR
     */
    int SpiOpen(const char *device);

    /**
     * @brief 关闭SPI设备
     * @param fd SPI设备文件描述符
     */
    void SpiClose(int fd);

    /**
     * @brief 配置SPI参数
     * @param fd SPI设备文件描述符
     * @param config SPI配置参数
     * @return 成功返回SPI_OK，失败返回SPI_ERR
     */
    int SpiConfigure(int fd, const SpiConfig *config);

    /**
     * @brief SPI数据传输
     * @param fd SPI设备文件描述符
     * @param tx_buffer 发送缓冲区
     * @param rx_buffer 接收缓冲区（可为NULL）
     * @param length 传输数据长度
     * @param speed_hz 传输速度（0使用默认速度）
     * @return 成功返回传输的字节数，失败返回SPI_ERR
     */
    int SpiTransfer(int fd, const uint8_t *tx_buffer, uint8_t *rx_buffer, size_t length, uint32_t speed_hz);

#ifdef __cplusplus
}
#endif

#endif /* __SPI_BASE_H__ */