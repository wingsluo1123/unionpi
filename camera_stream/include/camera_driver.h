/*
 * @Author: OpenHarmony Camera Team
 * @Date: 2025-11-01 00:00:00
 * @LastEditors: OpenHarmony Camera Team
 * @LastEditTime: 2025-11-01 00:00:00
 * @FilePath: /OpenHarmony/unionpi_whale_demo/camera_video/include/camera_driver.h
 * @Description: 摄像头驱动操作接口定义文件
 */

#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// 通用返回码定义
#define CAMERA_OK 0
#define CAMERA_ERR (-1)

/**
 * @brief 内存映射缓冲区结构体
 */
struct _mmap_buf
{
    void *addr; ///< 缓冲区地址
    int size;   ///< 缓冲区大小
};

/**
 * @brief V4L2视频设备结构体
 */
struct _v4l2_video
{
    int fd;                     ///< 设备文件描述符
    struct _mmap_buf *mmap_buf; ///< 内存映射缓冲区数组
    int mmap_buf_cnt;           ///< 内存映射缓冲区数量
    int width;                  ///< 实际帧宽度
    int height;                 ///< 实际帧高度
    __u32 pixelformat;          ///< 实际像素格式
};

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief 打开视频设备
     * @param device_name 设备名称路径，如"/dev/video0"
     * @return 成功返回文件描述符，失败返回CAMERA_ERR
     */
    int CameraOpen(const char *device_name);

    /**
     * @brief 关闭视频设备并释放资源
     * @param pdevice 指向视频设备结构体的指针
     * @return 成功返回CAMERA_OK，失败返回CAMERA_ERR
     */
    int CameraClose(struct _v4l2_video *pdevice);

    /**
     * @brief 查询视频设备能力
     * @param pdevice 指向视频设备结构体的指针
     * @return 成功返回CAMERA_OK，失败返回CAMERA_ERR
     */
    int CameraQueryCapability(struct _v4l2_video *pdevice);

    /**
     * @brief 查询设备支持的分辨率
     * @param pdevice 指向视频设备结构体的指针
     * @return 成功返回CAMERA_OK，失败返回CAMERA_ERR
     */
    int CameraQueryResolutions(struct _v4l2_video *pdevice);

    /**
     * @brief 设置视频设备参数
     * @param pdevice 指向视频设备结构体的指针
     * @param width 请求的图像宽度
     * @param height 请求的图像高度
     * @return 成功返回CAMERA_OK，失败返回CAMERA_ERR
     */
    int CameraSetParameters(struct _v4l2_video *pdevice, int width, int height);

    /**
     * @brief 设置视频设备内存映射
     * @param pdevice 指向视频设备结构体的指针
     * @param buf_cnt 缓冲区数量
     * @return 成功返回CAMERA_OK，失败返回CAMERA_ERR
     */
    int CameraSetMemoryMap(struct _v4l2_video *pdevice, int buf_cnt);

    /**
     * @brief 将缓冲区放入视频设备队列
     * @param pdevice 指向视频设备结构体的指针
     * @param index 缓冲区索引
     * @return 成功返回CAMERA_OK，失败返回CAMERA_ERR
     */
    int CameraQueueBuffer(struct _v4l2_video *pdevice, int index);

    /**
     * @brief 启动视频设备数据流
     * @param pdevice 指向视频设备结构体的指针
     * @return 成功返回CAMERA_OK，失败返回CAMERA_ERR
     */
    int CameraStreamOn(struct _v4l2_video *pdevice);

    /**
     * @brief 从视频设备读取数据流帧
     * @param pdevice 指向视频设备结构体的指针
     * @param out_buf_index 输出缓冲区索引
     * @param out_bytesused 输出实际使用的字节数
     * @return 成功返回CAMERA_OK，失败返回CAMERA_ERR
     */
    int CameraReadFrame(struct _v4l2_video *pdevice, int *out_buf_index, int *out_bytesused);

    /**
     * @brief 关闭视频设备数据流
     * @param pdevice 指向视频设备结构体的指针
     * @return 成功返回CAMERA_OK，失败返回CAMERA_ERR
     */
    int CameraStreamOff(struct _v4l2_video *pdevice);

    /**
     * @brief 捕获单张JPEG图片
     * @param device 设备路径，如"/dev/video0"
     * @param width 图像宽度
     * @param height 图像高度
     * @param out_jpg_path 输出JPEG文件路径
     * @param errbuf 错误信息缓冲区，可为NULL
     * @param errbuf_sz 错误信息缓冲区大小
     * @return 成功返回CAMERA_OK，若设备不提供MJPEG/JPEG数据返回-2，其他错误返回CAMERA_ERR
     */
    int CameraCaptureSingleJpeg(const char *device, int width, int height, const char *out_jpg_path, char *errbuf, size_t errbuf_sz);

#ifdef __cplusplus
}
#endif

#endif // __CAMERA_H__