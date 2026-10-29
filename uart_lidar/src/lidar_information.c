/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-11-01
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-11-01
 * @FilePath: /OpenHarmony/unionpi_whale_demo/uart_lidar/src/lidar_information.c
 * @Description: 激光雷达信息实现文件 (C语言版本)
 */

#include "../include/lidar_information.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 雷达控制命令定义 */
const uint8_t start_lidar[4] = {0xAA, 0x55, 0xF0, 0x0F};
const uint8_t end_lidar[4] = {0xAA, 0x55, 0xF5, 0x0A};
const uint8_t low_exposure[4] = {0xAA, 0x55, 0xF1, 0x0E};
const uint8_t high_exposure[4] = {0xAA, 0x55, 0xF2, 0x0D};
const uint8_t get_version[4] = {0xAA, 0x55, 0xFC, 0x03};
const uint8_t high_speed[4] = {0xAA, 0x55, 0xF2, 0x0D};
const uint8_t low_speed[4] = {0xAA, 0x55, 0xF1, 0x0E};

/**
 * @brief 创建激光扫描对象
 * @param capacity 初始容量
 * @return LaserScan* 激光扫描对象指针
 */
LaserScan *laser_scan_create(size_t capacity)
{
    LaserScan *scan = (LaserScan *)malloc(sizeof(LaserScan));
    if (scan == NULL)
    {
        printf("[%s] Failed to allocate LaserScan\n", __func__);
        return NULL;
    }

    scan->points = (LaserPoint *)malloc(capacity * sizeof(LaserPoint));
    if (scan->points == NULL)
    {
        printf("[%s] Failed to allocate points array\n", __func__);
        free(scan);
        return NULL;
    }

    scan->stamp = 0;
    scan->point_count = 0;
    scan->point_capacity = capacity;

    memset(&scan->config, 0, sizeof(LaserConfig));
    memset(scan->points, 0, capacity * sizeof(LaserPoint));

    return scan;
}

/**
 * @brief 销毁激光扫描对象
 * @param scan 激光扫描对象指针
 */
void laser_scan_destroy(LaserScan *scan)
{
    if (scan != NULL)
    {
        if (scan->points != NULL)
        {
            free(scan->points);
            scan->points = NULL;
        }
        free(scan);
    }
}

/**
 * @brief 向激光扫描对象添加点
 * @param scan 激光扫描对象指针
 * @param point 激光点指针
 * @return 成功返回0，失败返回-1
 */
int laser_scan_add_point(LaserScan *scan, const LaserPoint *point)
{
    if (scan == NULL || point == NULL)
    {
        return -1;
    }

    if (scan->point_count >= scan->point_capacity)
    {
        /* 需要扩容 */
        size_t new_capacity = scan->point_capacity * 2;
        LaserPoint *new_points = (LaserPoint *)realloc(scan->points, new_capacity * sizeof(LaserPoint));
        if (new_points == NULL)
        {
            printf("[%s] Failed to reallocate points array\n", __func__);
            return -1;
        }
        scan->points = new_points;
        scan->point_capacity = new_capacity;
    }

    scan->points[scan->point_count] = *point;
    scan->point_count++;
    return 0;
}

/**
 * @brief 清空激光扫描对象
 * @param scan 激光扫描对象指针
 */
void laser_scan_clear(LaserScan *scan)
{
    if (scan != NULL)
    {
        scan->point_count = 0;
        scan->stamp = 0;
    }
}