/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-11-27 15:00:00
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-11-27 16:36:51
 * @FilePath: /OpenHarmony/unionpi_whale_demo/spi_loopback/include/main.h
 * @Description: SPI回环测试主程序头文件
 */

#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdint.h>

#ifdef __cplusplus 
extern "C"
{
#endif

    // 默认发送数据
    extern uint8_t default_tx[];
    extern uint8_t default_rx[];
    extern char *input_tx;

    // 全局配置参数
    extern const char *device;
    extern uint32_t mode;
    extern uint8_t bits;
    extern char *input_file;
    extern char *output_file;
    extern uint32_t speed;
    extern uint16_t delay;
    extern int verbose;

    // 函数声明
    void hex_dump(const void *src, size_t length, size_t line_size, char *prefix);
    int unescape(char *_dst, char *_src);
    void transfer_escaped_string(int fd, char *str);
    void transfer_file(int fd, char *filename);
    void transfer_default(int fd);
    void print_usage(const char *prog);
    void parse_opts(int argc, char *argv[]);
    void pabort(const char *s);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H__ */