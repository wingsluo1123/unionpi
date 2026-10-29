/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-11-01
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-11-01 16:21:40
 * @FilePath: /OpenHarmony/unionpi_whale_demo/uart_lidar/include/lidar_information.h
 * @Description: 激光雷达信息定义头文件 (C语言版本)
 */

#ifndef LIDAR_INFORMATION_H
#define LIDAR_INFORMATION_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* 常量定义 */
#define FRAME_ANGLE_RANGE (90 * 256)
#define FRAME_ANGLE_BASE (0xA000)
#define PI 3.141592654
#define ULTRASONIC_ANGLE_INC_DEG 0.5
#define ANGLESTRLENMAX 32

/* 协议定义 */
#define LIDAR_CMD_SYNC_BYTE 0xA5
#define LIDAR_CMDFLAG_HAS_PAYLOAD 0x80
#define LIDAR_ANS_SYNC_BYTE1 0xA5
#define LIDAR_ANS_SYNC_BYTE2 0x5A
#define LIDAR_RESP_MEASUREMENT_CHECKBIT (0x1 << 0)
#define LIDAR_RESP_MEASUREMENT_ANGLE_SAMPLE_SHIFT 8
#define LIDAR_RESP_MEASUREMENT_SYNCBIT (0x1 << 0)
#define LIDAR_RESP_MEASUREMENT_ANGLE_SHIFT 1

/* 包相关定义 */
#define Node_Default_Quality (10)
#define Node_Sync 1
#define Node_NotSync 2
#define PackagePaidBytes 10
#define PH 0x55AA
#define NORMAL_PACKAGE_SIZE 90
#define INTENSITY_NORMAL_PACKAGE_SIZE 130
#define PackageSampleMaxLngth 0x100

/* 超时和扫描点数定义 */
#define DEFAULT_TIMEOUT 2000
#define DEFAULT_HEART_BEAT 1000
#define MAX_SCAN_NODES 800
#define DEFAULT_TIMEOUT_COUNT 10

/* 返回值定义 */
#define RESULT_OK 0
#define RESULT_TIMEOUT -1
#define RESULT_FAIL -2

#define IS_OK(x) ((x) == RESULT_OK)
#define IS_TIMEOUT(x) ((x) == RESULT_TIMEOUT)
#define IS_FAIL(x) ((x) == RESULT_FAIL)

    typedef int32_t result_t;

    /* 雷达启动停止命令 */
    extern const uint8_t start_lidar[4];
    extern const uint8_t end_lidar[4];
    extern const uint8_t low_exposure[4];
    extern const uint8_t high_exposure[4];
    extern const uint8_t get_version[4];
    extern const uint8_t high_speed[4];
    extern const uint8_t low_speed[4];

    /* CT类型枚举 */
    typedef enum
    {
        CT_Normal = 0,
        CT_RingStart = 1,
        CT_Tail,
    } CT;

    /* 雷达版本枚举 */
    typedef enum
    {
        M1C1_Mini_v1 = 1,
        M1C1_Mini_v2,
        M1CT_Coin_Plus,
        M1CT_Coin_D2,
    } LIDAR_VERSION;

    /* 雷达应用类型枚举 */
    typedef enum
    {
        LIDAR_FOR_MOTION = 0,
        LIDAR_FOR_BLOCKED = 1,
    } MOTION_AND_BLOCKED;

    /* 雷达覆盖角度结构 */
    typedef struct
    {
        float f_begin;
        float f_end;
    } LidarCoverAngleStr;

    /* 陀螺仪航向角结构 */
    typedef struct
    { 
        uint64_t timeStamp;
        float GyroYaw;
    } GyroYawStr;

    /* 激光配置结构 */
    typedef struct
    {
        float min_angle;
        float max_angle;
        float angle_increment;
        float time_increment;
        float scan_time;
        float min_range;
        float max_range;
    } LaserConfig;

    /* 激光点结构 */
    typedef struct
    {
        float angle;        /* 角度 */
        float range;        /* 距离 */
        uint16_t intensity; /* 强度 */
        short range_check;  /* 距离检查值 */
    } LaserPoint;

    /* 激光扫描数据结构 */
    typedef struct
    {
        uint64_t stamp;        /* 时间戳 */
        LaserPoint *points;    /* 激光点数组 */
        size_t point_count;    /* 激光点数量 */
        size_t point_capacity; /* 激光点数组容量 */
        LaserConfig config;    /* 激光配置 */
    } LaserScan;

    /* 节点信息结构 */
    typedef struct
    {
        uint8_t sync_flag;          /* 同步标志 */
        uint16_t sync_quality;      /* 信号质量 */
        uint16_t angle_q6_checkbit; /* 测距点角度 */
        uint16_t distance_q2;       /* 当前测距点距离 */
        uint64_t stamp;             /* 时间戳 */
        uint8_t scan_frequence;     /* 扫描频率 */
        uint8_t exp_m;              /* 当前是否为高反光材质 */
        uint8_t debug_info[12];     /* 调试信息 */
        uint8_t index;              /* 索引 */
    } node_info;

    /* 包采样节点结构 */
    typedef struct
    {
        uint8_t PakageSampleQuality;
        uint16_t PakageSampleDistance;
    } __attribute__((packed)) PackageNode;

    /* 雷达数据包结构 (M1C1_Mini v2版本，带强度) */
    typedef struct
    {
        uint16_t package_Head;
        uint8_t package_CT;
        uint8_t nowPackageNum;
        uint16_t packageFirstSampleAngle;
        uint16_t packageLastSampleAngle;
        uint16_t checkSum;
        uint8_t packageSampleDistance[PackageSampleMaxLngth];
    } __attribute__((packed)) node_package;

    /* 雷达数据包结构 (M1C1_Mini v1版本) */
    typedef struct
    {
        uint16_t package_Head;
        uint8_t package_CT;
        uint8_t nowPackageNum;
        uint16_t packageFirstSampleAngle;
        uint16_t packageLastSampleAngle;
        uint16_t checkSum;
        uint16_t packageSampleDistance[PackageSampleMaxLngth];
    } __attribute__((packed)) node_packages;

    /* 雷达数据包结构 (M1CT_Coin_Plus版本) */
    typedef struct
    {
        uint8_t headL;
        uint8_t headH;
        uint8_t info;
        uint8_t sampleinfo;
        uint8_t speedL;
        uint8_t speedH;
        uint8_t startAngleL;
        uint8_t startAngleH;
        struct
        {
            uint8_t d0;
            uint8_t d1;
            uint8_t d2;
        } data[8];
        uint8_t stopAngleL;
        uint8_t stopAngleH;
        uint8_t csL;
        uint8_t csH;
    } __attribute__((packed)) node_package_coin;

    /* 命令包结构 */
    typedef struct
    {
        uint8_t syncByte;
        uint8_t cmd_flag;
        uint8_t size;
        uint8_t data;
    } __attribute__((packed)) cmd_packet;

    /* 应答头结构 */
    typedef struct
    {
        uint8_t syncByte1;
        uint8_t syncByte2;
        uint32_t size : 30;
        uint32_t subType : 2;
        uint8_t type;
    } __attribute__((packed)) lidar_ans_header;

    /* 陀螺仪节点结构 */
    typedef struct
    {
        float angle;
        int count;
        uint64_t time;
    } node_Gyro;

    /* LaserScan辅助函数 */
    LaserScan *laser_scan_create(size_t capacity);
    void laser_scan_destroy(LaserScan *scan);
    int laser_scan_add_point(LaserScan *scan, const LaserPoint *point);
    void laser_scan_clear(LaserScan *scan);

#ifdef __cplusplus
}
#endif

#endif /* LIDAR_INFORMATION_H */
