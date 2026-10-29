/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-11-01
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-11-01 16:21:38
 * @FilePath: /OpenHarmony/unionpi_whale_demo/uart_lidar/include/lidar_data_processing.h
 * @Description: 雷达数据处理模块头文件 (C语言版本)
 */

#ifndef LIDAR_DATA_PROCESSING_H
#define LIDAR_DATA_PROCESSING_H

#include "lidar_information.h"
#include "serial_port.h"
#include <stdint.h> 

#ifdef __cplusplus
extern "C"
{
#endif

    /* 雷达数据处理结构 */
    typedef struct
    {
        uint16_t CheckSumCal;
        uint16_t CheckSum;
        uint16_t SampleNumlAndCTCal;
        uint16_t LastSampleAngleCal;
        uint16_t Valu8Tou16;
        uint16_t package_Sample_Index;
        uint16_t package_Sample_Num; /* 当前包的采样点数量 */
        uint16_t FirstSampleAngle;
        uint16_t LastSampleAngle;
        uint8_t scan_frequence;
        int CheckSumResult;
        int has_package_error;
        float IntervalSampleAngle;
        float IntervalSampleAngle_LastPackage;
        float FirstSampleAngle_float; /* 起始角度(浮点型) */
        int package_index;
        float start_t;
        float stop_t;
        float angle_new;
        float angle_bak;
        size_t recvNodeCount;
        uint64_t m_node_time_ns;
        uint64_t m_node_last_time_ns;
        uint32_t m_pointTime;
        int PackageSampleBytes;
    } Lidar_Data_Processing;

    /**
     * @brief 初始化雷达数据处理对象
     * @param processor 数据处理对象指针
     */
    void lidar_data_processing_init(Lidar_Data_Processing *processor);

    /**
     * @brief 向雷达发送命令
     * @param processor 数据处理对象指针
     * @param serial 串口对象指针
     * @param cmd 命令字节
     * @return result_t 执行结果
     */
    result_t lidar_send_command(Lidar_Data_Processing *processor, Serial_Port *serial, uint8_t cmd);

    /**
     * @brief 向雷达发送数据
     * @param serial 串口对象指针
     * @param data 数据缓冲区
     * @param size 数据大小
     * @return result_t 执行结果
     */
    result_t lidar_send_data(Serial_Port *serial, const uint8_t *data, size_t size);

    /**
     * @brief 等待标准M1CT数据包 (0x55 0xAA格式)
     * @param processor 数据处理对象指针
     * @param serial 串口对象指针
     * @param node 节点信息指针
     * @param package_buffer 包数据缓冲区
     * @param globalRecvBuffer 全局接收缓冲区
     * @param timeout 超时时间(毫秒)
     * @return result_t 执行结果
     */
    result_t lidar_wait_package_standard(Lidar_Data_Processing *processor, Serial_Port *serial, node_info *node, uint8_t *package_buffer, uint8_t *globalRecvBuffer, uint32_t timeout);

    /**
     * @brief 等待扫描数据
     * @param processor 数据处理对象指针
     * @param serial 串口对象指针
     * @param nodebuffer 节点缓冲区
     * @param count 节点数量指针
     * @param scan_packages 扫描包数据
     * @param globalRecvBuffer 全局接收缓冲区
     * @param m_intensities 是否包含强度信息
     * @param m_PointTime 点时间
     * @param trans_delay 传输延时
     * @param version 雷达版本
     * @param tim_scan_start 扫描开始时间指针
     * @param tim_scan_end 扫描结束时间指针
     * @param is_connected 连接状态
     * @param timeout 超时时间(毫秒)
     * @return result_t 执行结果
     */
    result_t lidar_wait_scan_data(Lidar_Data_Processing *processor, Serial_Port *serial, node_info *nodebuffer, size_t *count, void *scan_packages, uint8_t *globalRecvBuffer, int m_intensities, uint64_t m_PointTime, uint32_t trans_delay, int version, uint64_t *tim_scan_start, uint64_t *tim_scan_end, int is_connected, uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* LIDAR_DATA_PROCESSING_H */
