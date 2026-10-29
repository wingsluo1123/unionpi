/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-11-01
 * @FilePath: /OpenHarmony/unionpi_whale_demo/uart_lidar/src/lidar_data_processing.c
 * @Description: 雷达数据处理模块实现 (完整版)
 */

#include "../include/lidar_data_processing.h"
#include "../include/mtime.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief 初始化雷达数据处理对象
 * @param processor 数据处理对象指针
 */
void lidar_data_processing_init(Lidar_Data_Processing *processor)
{
    if (!processor)
        return;

    memset(processor, 0, sizeof(Lidar_Data_Processing));
    processor->CheckSumCal = 0;
    processor->CheckSum = 0;
    processor->SampleNumlAndCTCal = 0;
    processor->LastSampleAngleCal = 0;
    processor->Valu8Tou16 = 0;
    processor->package_Sample_Index = 0;
    processor->FirstSampleAngle = 0;
    processor->LastSampleAngle = 0;
    processor->scan_frequence = 0;
    processor->CheckSumResult = 0;
    processor->has_package_error = 0;
    processor->IntervalSampleAngle = 0.0f;
    processor->IntervalSampleAngle_LastPackage = 0.0f;
    processor->package_index = 0;
    processor->PackageSampleBytes = 2;
    processor->recvNodeCount = 0;
    processor->m_pointTime = 1000000000 / 5000;
}

/**
 * @brief 向雷达发送命令
 * @param processor 数据处理对象指针
 * @param serial 串口对象指针
 * @param cmd 命令字节
 * @return result_t 执行结果
 */
result_t lidar_send_command(Lidar_Data_Processing *processor, Serial_Port *serial, uint8_t cmd)
{
    uint8_t pkt_header[10];

    switch (cmd)
    {
    case 0x60:
        break;
    case 0x65:
        break;
    case 0x63:
        pkt_header[0] = 0xAA;
        pkt_header[1] = 0x55;
        pkt_header[2] = 0xF0;
        pkt_header[3] = 0x0F;
        serial_port_write(serial, pkt_header, 4);
        break;
    default:
        break;
    }
    return RESULT_OK;
}

/**
 * @brief 向雷达发送数据
 * @param serial 串口对象指针
 * @param data 数据缓冲区
 * @param size 数据大小
 * @return result_t 执行结果
 */
result_t lidar_send_data(Serial_Port *serial, const uint8_t *data, size_t size)
{
    if (!data || size == 0)
    {
        return RESULT_FAIL;
    }

    size_t r;
    while (size)
    {
        r = serial_port_write(serial, data, size);
        if (r < 1)
        {
            return RESULT_FAIL;
        }
        size -= r;
        data += r;
    }
    return RESULT_OK;
}

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
result_t lidar_wait_package_standard(Lidar_Data_Processing *processor, Serial_Port *serial, node_info *node, uint8_t *package_buffer, uint8_t *globalRecvBuffer, uint32_t timeout)
{
    int recvPos = 0;
    uint32_t startTs = getms();
    uint32_t waitTime = 0;

    uint8_t LSN = 0;  // 采样点数量
    uint16_t FSA = 0; // 起始角度
    uint16_t LSA = 0; // 结束角度
    uint16_t CS = 0;  // 校验码

    static int debug_count = 0;

    if (processor->package_Sample_Index == 0)
    {
        recvPos = 0;

        if (debug_count++ < 3)
        {
            printf("[%s] Waiting for standard M1CT package (0xAA 0x55)...\n", __func__);
        }

        /* 读取包头和元数据：PH(2) + CT(1) + LSN(1) + FSA(2) + LSA(2) + CS(2) = 10字节 */
        /* 注意：实际包头是 0xAA 0x55 (大端)，不是手册说的 0x55 0xAA */
        while ((waitTime = getms() - startTs) <= timeout && recvPos < 10)
        {
            size_t remainSize = 10 - recvPos;
            size_t recvSize = 0;
            result_t ans = serial_port_wait_for_data(serial, remainSize, timeout - waitTime, &recvSize);

            if (!IS_OK(ans))
            {
                if (debug_count <= 3)
                {
                    printf("[%s] serial_port_wait_for_data failed: %d\n", __func__, ans);
                }
                return ans;
            }
            if (recvSize > remainSize)
                recvSize = remainSize;

            serial_port_read(serial, globalRecvBuffer, recvSize);

            for (size_t pos = 0; pos < recvSize; ++pos)
            {
                uint8_t currentByte = globalRecvBuffer[pos];

                // 减少调试输出，只在找到包头时打印

                switch (recvPos)
                {
                case 0: // PH高字节 - 寻找 0xAA
                    if (currentByte != 0xAA)
                    {
                        // 不是包头，继续寻找，不增加recvPos
                        continue;
                    }
                    if (debug_count <= 3)
                    {
                        printf("[%s] Found 0xAA (PH high byte)\n", __func__);
                    }
                    package_buffer[recvPos++] = currentByte;
                    break;
                case 1: // PH低字节 - 检查 0x55
                    if (currentByte != 0x55)
                    {
                        if (debug_count <= 3)
                        {
                            printf("[%s] After 0xAA, got 0x%02X (expecting 0x55), resetting\n", __func__, currentByte);
                        }
                        // 不匹配，重新开始寻找0xAA
                        recvPos = 0;
                        // 检查当前字节是否是0xAA
                        if (currentByte == 0xAA)
                        {
                            package_buffer[recvPos++] = currentByte;
                        }
                        continue;
                    }
                    if (debug_count <= 3)
                    {
                        printf("[%s] Found 0x55 (PH low byte) - Header complete!\n", __func__);
                    }
                    package_buffer[recvPos++] = currentByte;
                    break;
                default:
                    // 其他位置直接保存
                    package_buffer[recvPos++] = currentByte;

                    // 根据位置打印相关信息
                    if (recvPos == 3)
                    { // CT
                        if (debug_count <= 5)
                        {
                            uint8_t CT = currentByte;
                            printf("[%s] CT=0x%02X (type=%d, freq=%d)\n", __func__, CT, CT & 0x01, CT >> 1);
                        }
                    }
                    else if (recvPos == 4)
                    { // LSN
                        LSN = currentByte;
                        if (debug_count <= 5)
                        {
                            printf("[%s] LSN=%d (sample count)\n", __func__, LSN);
                        }
                    }
                    else if (recvPos == 6)
                    { // FSA完成
                        FSA = package_buffer[4] | (package_buffer[5] << 8);
                        if (debug_count <= 5)
                        {
                            printf("[%s] FSA=0x%04X (%f degrees)\n", __func__, FSA, (FSA >> 1) / 64.0f);
                        }
                    }
                    else if (recvPos == 8)
                    { // LSA完成
                        LSA = package_buffer[6] | (package_buffer[7] << 8);
                        if (debug_count <= 5)
                        {
                            printf("[%s] LSA=0x%04X (%f degrees)\n", __func__, LSA, (LSA >> 1) / 64.0f);
                        }
                    }
                    else if (recvPos == 10)
                    { // CS完成
                        CS = package_buffer[8] | (package_buffer[9] << 8);
                        if (debug_count <= 5)
                        {
                            printf("[%s] CS=0x%04X (checksum)\n", __func__, CS);
                        }
                    }
                    break;
                }
            }
        }

        if (recvPos < 10)
        {
            if (debug_count <= 3)
            {
                printf("[%s] Failed to receive header, only got %d bytes\n", __func__, recvPos);
            }
            return RESULT_FAIL;
        }

        /* 读取采样数据：LSN * 3 字节 */
        int sample_bytes = LSN * 3;
        recvPos = 0;
        startTs = getms();

        if (debug_count <= 5)
        {
            printf("[%s] Reading %d sample bytes (%d samples * 3)...\n", __func__, sample_bytes, LSN);
        }

        while ((waitTime = getms() - startTs) <= timeout && recvPos < sample_bytes)
        {
            size_t remainSize = sample_bytes - recvPos;
            size_t recvSize = 0;
            result_t ans = serial_port_wait_for_data(serial, remainSize, timeout - waitTime, &recvSize);

            if (!IS_OK(ans))
                return ans;
            if (recvSize > remainSize)
                recvSize = remainSize;

            serial_port_read(serial, globalRecvBuffer, recvSize);

            for (size_t pos = 0; pos < recvSize; ++pos)
            {
                package_buffer[10 + recvPos++] = globalRecvBuffer[pos];
            }
        }

        if (recvPos < sample_bytes)
        {
            if (debug_count <= 3)
            {
                printf("[%s] Failed to receive samples, only got %d/%d bytes\n", __func__, recvPos, sample_bytes);
            }
            return RESULT_FAIL;
        }

        /* 校验和验证 - 双字节异或 */
        /* 根据M1CT数据手册：校验码采用双字节异或对当前数据包进行校验 */
        /* 校验范围：PH(2字节) ^ CT(1字节) ^ LSN(1字节) ^ FSA(2字节) ^ LSA(2字节) ^ 所有采样数据 */
        uint16_t checksum = 0;

        // PH: 0xAA55
        checksum ^= 0xAA55;

        // CT (1字节，扩展为2字节)
        checksum ^= package_buffer[2];

        // LSN (1字节，扩展为2字节)
        checksum ^= package_buffer[3];

        // FSA (2字节，小端)
        checksum ^= (package_buffer[4] | (package_buffer[5] << 8));

        // LSA (2字节，小端)
        checksum ^= (package_buffer[6] | (package_buffer[7] << 8));

        // 采样数据 (每个采样点3字节：距离2字节+强度1字节)
        for (int i = 0; i < sample_bytes; i += 2)
        {
            if (i + 1 < sample_bytes)
            {
                checksum ^= (package_buffer[10 + i] | (package_buffer[10 + i + 1] << 8));
            }
            else
            {
                // 最后一个字节（如果是奇数个字节）
                checksum ^= package_buffer[10 + i];
            }
        }

        if (checksum != CS)
        {
            if (debug_count <= 5)
            {
                printf("[%s] Checksum mismatch: calculated=0x%04X, expected=0x%04X\n", __func__, checksum, CS);
                // 打印前几个采样数据用于调试
                printf("[%s] First 9 sample bytes: ", __func__);
                for (int i = 0; i < 9 && i < sample_bytes; i++)
                {
                    printf("0x%02X ", package_buffer[10 + i]);
                }
                printf("\n");
            }
            processor->package_Sample_Index = 0;
            return RESULT_FAIL;
        }

        if (debug_count <= 3)
        {
            printf("[%s] Package received successfully: LSN=%d, FSA=%f°, LSA=%f°\n", __func__, LSN, (FSA >> 1) / 64.0f, (LSA >> 1) / 64.0f);
        }

        /* 解析采样点 */
        processor->package_Sample_Num = LSN;
        processor->package_Sample_Index = 0;
        processor->IntervalSampleAngle = ((LSA >> 1) / 64.0f - (FSA >> 1) / 64.0f) / (LSN - 1);
        processor->FirstSampleAngle_float = (FSA >> 1) / 64.0f;
    }

    /* 返回当前采样点数据 */
    if (processor->package_Sample_Index < processor->package_Sample_Num)
    {
        int sample_offset = 10 + processor->package_Sample_Index * 3;
        uint16_t distance = package_buffer[sample_offset] | (package_buffer[sample_offset + 1] << 8);
        uint8_t intensity = package_buffer[sample_offset + 2];

        float angle = processor->FirstSampleAngle_float + processor->IntervalSampleAngle * processor->package_Sample_Index;

        node->sync_quality = intensity;
        node->angle_q6_checkbit = (uint16_t)(angle * 100);
        node->distance_q2 = distance;
        node->stamp = 0;

        /* 检测一圈的开始 */
        if (processor->angle_bak > 350 && angle < 10)
        {
            node->sync_flag = Node_Sync;
        }
        else
        {
            node->sync_flag = Node_NotSync;
        }
        processor->angle_bak = angle;

        processor->package_Sample_Index++;

        if (processor->package_Sample_Index >= processor->package_Sample_Num)
        {
            processor->package_Sample_Index = 0; /* 准备接收下一个包 */
        }

        return RESULT_OK;
    }

    return RESULT_FAIL;
}

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
result_t lidar_wait_scan_data(Lidar_Data_Processing *processor, Serial_Port *serial, node_info *nodebuffer, size_t *count, void *scan_packages, uint8_t *globalRecvBuffer, int m_intensities, uint64_t m_PointTime, uint32_t trans_delay, int version, uint64_t *tim_scan_start, uint64_t *tim_scan_end, int is_connected, uint32_t timeout)
{
    if (!is_connected)
    {
        *count = 0;
        return RESULT_FAIL;
    }

    processor->recvNodeCount = 0;
    uint32_t startTs = getms();
    uint32_t waitTime = 0;
    result_t ans = RESULT_FAIL;

    while ((waitTime = getms() - startTs) <= timeout && processor->recvNodeCount < *count)
    {
        node_info node;
        memset(&node, 0, sizeof(node_info));

        /* 使用标准M1CT格式解析 (0x55 0xAA) */
        uint8_t package_buffer[512]; /* 最大支持 (512-10)/3 = 167个采样点 */
        ans = lidar_wait_package_standard(processor, serial, &node, package_buffer, globalRecvBuffer, timeout - waitTime);

        if (!IS_OK(ans))
        {
            *count = processor->recvNodeCount;
            return ans;
        }

        nodebuffer[processor->recvNodeCount++] = node;

        if (node.sync_flag & LIDAR_RESP_MEASUREMENT_SYNCBIT)
        {
            *count = processor->recvNodeCount;
            return RESULT_OK;
        }

        if (processor->recvNodeCount == *count)
        {
            return RESULT_OK;
        }
    }

    *count = processor->recvNodeCount;
    return RESULT_FAIL;
}
 