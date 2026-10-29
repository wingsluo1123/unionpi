/*
 * @Author: OpenHarmony Camera Team
 * @Date: 2025-11-01 00:00:00
 * @LastEditors: OpenHarmony Camera Team
 * @LastEditTime: 2025-11-01 00:00:00
 * @FilePath: /OpenHarmony/unionpi_whale_demo/camera_video/src/camera_driver.c
 * @Description: 摄像头驱动操作实现文件
 */

#include "camera_driver.h"

/**
 * @brief 分辨率结构体
 */
struct resolution
{
    int width;  ///< 宽度
    int height; ///< 高度
};

/**
 * @brief 比较两个分辨率的大小
 *
 * @param a 指向第一个分辨率结构体的指针
 * @param b 指向第二个分辨率结构体的指针
 * @return a大于b返回正数，a小于b返回负数，相等返回0
 */
int compare_res(const void *a, const void *b)
{
    struct resolution *ra = (struct resolution *)a;
    struct resolution *rb = (struct resolution *)b;
    if (ra->width != rb->width)
        return ra->width - rb->width;
    return ra->height - rb->height;
}

int CameraOpen(const char *device_name)
{
    int fd = 0;

    if (device_name == NULL)
    {
        printf("[%s] Error: device_name is NULL\n", __func__);
        return CAMERA_ERR;
    }

    fd = open(device_name, O_RDWR); /* 以读写方式打开；以只读方式打开导致内存映射出错 */
    if (fd < 0)
    {
        printf("[%s] open video device failed: %s\n", __func__, strerror(errno));
        return CAMERA_ERR;
    };
    return fd;
}

int CameraClose(struct _v4l2_video *pdevice)
{
    int ret = 0;
    int i;

    for (i = 0; i < pdevice->mmap_buf_cnt; i++)
    {
        if (pdevice->mmap_buf[i].addr != 0x00)
        {
            ret = munmap(pdevice->mmap_buf[i].addr, pdevice->mmap_buf[i].size);
            if (ret < 0)
            {
                printf("[%s] munmap failed: %s\n", __func__, strerror(errno));
                continue;
            }
        }
    }

    if (pdevice->mmap_buf != 0x00)
    {
        free(pdevice->mmap_buf);
    }

    if (pdevice->fd != 0x00)
    {
        ret = close(pdevice->fd);
        if (ret < 0)
        {
            printf("[%s] close fd failed: %s\n", __func__, strerror(errno));
        }
    }
    return ret == 0 ? CAMERA_OK : CAMERA_ERR;
}

int CameraQueryCapability(struct _v4l2_video *pdevice)
{
    int ret = 0;
    struct v4l2_capability cap = {0};
    struct v4l2_fmtdesc fmtdesc = {0};

    /* 查询摄像头信息 */
    ret = ioctl(pdevice->fd, VIDIOC_QUERYCAP, &cap);
    if (ret < 0)
    {
        printf("[%s] ioctl call 'VIDEO_QUERYCAP' failed: %s\n", __func__, strerror(errno));
        return CAMERA_ERR;
    }
    printf("[%s] video driver name:%s\n", __func__, cap.driver);
    printf("[%s] video device name:%s\n", __func__, cap.card);
    printf("[%s] video bus information:%s\n", __func__, cap.bus_info);
    printf("[%s] video driver version:%d\n", __func__, cap.version);
    printf("[%s] video capabilities:%x\n", __func__, cap.capabilities);

    /* 检查设备是否支持视频捕获 */
    if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)
    {
        printf("[%s] the video device support capture\n", __func__);
    }

    /* 检查设备是否支持数据流 */
    if (!(cap.capabilities & V4L2_CAP_STREAMING))
    {
        printf("[%s] the video device not support stream\n", __func__);
    }

    /* 查询设备支持的输出格式 */
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    printf("[%s] the video device support format:\n", __func__);
    while (ioctl(pdevice->fd, VIDIOC_ENUM_FMT, &fmtdesc) != -1)
    {
        printf("[%s] %d.%s\n", __func__, fmtdesc.index + 1, fmtdesc.description);
        fmtdesc.index++;
    }
    return CAMERA_OK;
}

int CameraQueryResolutions(struct _v4l2_video *pdevice)
{
    struct v4l2_fmtdesc fmtdesc = {0};
    struct v4l2_frmsizeenum frmsize = {0};

    /* 查询设备支持的输出格式 */
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    printf("[%s] Supported pixel formats and resolutions:\n", __func__);
    while (ioctl(pdevice->fd, VIDIOC_ENUM_FMT, &fmtdesc) != -1)
    {
        printf("[%s] Format %d: %s (%c%c%c%c)\n", __func__, fmtdesc.index + 1, fmtdesc.description, fmtdesc.pixelformat & 0xFF, (fmtdesc.pixelformat >> 8) & 0xFF, (fmtdesc.pixelformat >> 16) & 0xFF, (fmtdesc.pixelformat >> 24) & 0xFF);

        /* 为每个格式枚举分辨率 */
        struct resolution res_list[100];
        int res_count = 0;
        frmsize.index = 0;
        frmsize.pixel_format = fmtdesc.pixelformat;
        while (ioctl(pdevice->fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) != -1)
        {
            if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE)
            {
                // 检查是否已存在
                int exists = 0;
                for (int i = 0; i < res_count; i++)
                {
                    if (res_list[i].width == (int)frmsize.discrete.width && res_list[i].height == (int)frmsize.discrete.height)
                    {
                        exists = 1;
                        break;
                    }
                }
                if (!exists && res_count < 100)
                {
                    res_list[res_count].width = frmsize.discrete.width;
                    res_list[res_count].height = frmsize.discrete.height;
                    res_count++;
                }
            }
            else if (frmsize.type == V4L2_FRMSIZE_TYPE_CONTINUOUS || frmsize.type == V4L2_FRMSIZE_TYPE_STEPWISE)
            {
                printf("[%s]   Resolution range: %dx%d to %dx%d\n", __func__, frmsize.stepwise.min_width, frmsize.stepwise.min_height, frmsize.stepwise.max_width, frmsize.stepwise.max_height);
            }
            frmsize.index++;
        }
        // 排序分辨率
        qsort(res_list, res_count, sizeof(struct resolution), compare_res);
        // 打印排序后的分辨率
        for (int i = 0; i < res_count; i++)
        {
            printf("[%s]   Resolution: %dx%d\n", __func__, res_list[i].width, res_list[i].height);
        }
        fmtdesc.index++;
    }
    return CAMERA_OK;
}

int CameraSetParameters(struct _v4l2_video *pdevice, int width, int height)
{
    int ret = 0;
    struct v4l2_format format;

    /* 设置帧输出格式 */
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = width;                    /* 像素宽度 */
    format.fmt.pix.height = height;                  /* 像素高度 */
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG; /* 输出格式，前提是摄像头支持该格式 */
    format.fmt.pix.field = V4L2_FIELD_NONE;

    ret = ioctl(pdevice->fd, VIDIOC_S_FMT, &format);
    if (ret < 0)
    {
        printf("[%s] ioctl call 'VIDIOC_S_FMT' failed: %s\n", __func__, strerror(errno));
    }

    // Query back the actual negotiated format
    struct v4l2_format gfmt = {0};
    gfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(pdevice->fd, VIDIOC_G_FMT, &gfmt) == 0)
    {
        pdevice->width = gfmt.fmt.pix.width;
        pdevice->height = gfmt.fmt.pix.height;
        pdevice->pixelformat = gfmt.fmt.pix.pixelformat;
        __u32 f = pdevice->pixelformat;
        printf("[%s] Negotiated format: %c%c%c%c %dx%d\n", __func__, f & 0xFF, (f >> 8) & 0xFF, (f >> 16) & 0xFF, (f >> 24) & 0xFF, pdevice->width, pdevice->height);
        // 如果请求的分辨率不支持，直接返回错误
        if ((int)gfmt.fmt.pix.width != width || (int)gfmt.fmt.pix.height != height)
        {
            printf("[%s] Requested resolution %dx%d not supported\n", __func__, width, height);
            return CAMERA_ERR;
        }
    }
    else
    {
        printf("[%s] ioctl VIDIOC_G_FMT failed: %s\n", __func__, strerror(errno));
    }
    return ret == 0 ? CAMERA_OK : CAMERA_ERR;
}

int CameraSetMemoryMap(struct _v4l2_video *pdevice, int buf_cnt)
{
    int ret = 0;
    int i = 0;
    struct v4l2_requestbuffers req_buf = {0};
    struct v4l2_buffer buf = {0};

    /* 申请内核缓存区 */
    pdevice->mmap_buf_cnt = buf_cnt;
    req_buf.count = buf_cnt; /* 缓存数目 */
    req_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req_buf.memory = V4L2_MEMORY_MMAP;
    ret = ioctl(pdevice->fd, VIDIOC_REQBUFS, &req_buf);
    if (ret < 0)
    {
        printf("[%s] ioctl call 'VIDIOC_REQBUFS' failed: %s\n", __func__, strerror(errno));
        return CAMERA_ERR;
    }

    pdevice->mmap_buf = malloc(req_buf.count * sizeof(struct _mmap_buf));
    if (pdevice->mmap_buf == NULL)
    {
        printf("[%s] malloc memory failed\n", __func__);
        return CAMERA_ERR;
    }

    /* 将内核态内存映射到用户态 */
    for (i = 0; i < (int)req_buf.count; i++)
    {
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        ret = ioctl(pdevice->fd, VIDIOC_QUERYBUF, &buf);
        if (ret < 0)
        {
            printf("[%s] ioctl call 'VIDIOC_QUERYBUF' failed: %s\n", __func__, strerror(errno));
            return CAMERA_ERR;
        }

        pdevice->mmap_buf[i].size = buf.length;
        pdevice->mmap_buf[i].addr = (char *)mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, pdevice->fd, buf.m.offset);

        if (MAP_FAILED == pdevice->mmap_buf[i].addr)
        {
            printf("[%s] mmap failed: %s\n", __func__, strerror(errno));
            return CAMERA_ERR;
        }
    }
    return CAMERA_OK;
}

int CameraQueueBuffer(struct _v4l2_video *pdevice, int index)
{
    int ret = 0;
    struct v4l2_buffer buf = {0};

    /* 将内核缓存放入队列 */
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = index;
    ret = ioctl(pdevice->fd, VIDIOC_QBUF, &buf);
    if (ret < 0)
    {
        printf("[%s] ioctl call 'VIDIOC_QBUF' failed: %s\n", __func__, strerror(errno));
        return CAMERA_ERR;
    }
    return CAMERA_OK;
}

int CameraStreamOn(struct _v4l2_video *pdevice)
{
    int ret = 0;
    int i;
    enum v4l2_buf_type type;

    /* 将内核缓存放入队列 */
    for (i = 0; i < pdevice->mmap_buf_cnt; i++)
    {
        ret = CameraQueueBuffer(pdevice, i);
        if (ret < 0)
        {
            return CAMERA_ERR;
        }
    }

    /* 开启数据流 */
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(pdevice->fd, VIDIOC_STREAMON, &type);
    if (ret < 0)
    {
        printf("[%s] ioctl call 'VIDIOC_STREAMON' failed: %s\n", __func__, strerror(errno));
        return CAMERA_ERR;
    }

    return CAMERA_OK;
}

int CameraReadFrame(struct _v4l2_video *pdevice, int *out_buf_index, int *out_bytesused)
{
    int ret = 0;
    struct v4l2_buffer buf = {0};

    /* 从队列取出数据 */
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    ret = ioctl(pdevice->fd, VIDIOC_DQBUF, &buf);
    if (ret < 0)
    {
        printf("[%s] ioctl call 'VIDIOC_DQBUF' failed: %s\n", __func__, strerror(errno));
        return CAMERA_ERR;
    }
    if ((int)buf.index > pdevice->mmap_buf_cnt)
    {
        printf("[%s] buf overflow[%d]\n", __func__, buf.index);
    }
    *out_buf_index = buf.index;
    if (out_bytesused)
    {
        *out_bytesused = buf.bytesused;
    }

    return CAMERA_OK;
}

int CameraStreamOff(struct _v4l2_video *pdevice)
{
    int ret = 0;
    enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(pdevice->fd, VIDIOC_STREAMOFF, &type); /* 关闭数据流 */
    if (ret < 0)
    {
        printf("[%s] ioctl call 'VIDIOC_STREAMOFF' failed: %s\n", __func__, strerror(errno));
        return CAMERA_ERR;
    }
    return CAMERA_OK;
}

int CameraCaptureSingleJpeg(const char *device, int width, int height, const char *out_jpg_path, char *errbuf, size_t errbuf_sz)
{
    struct _v4l2_video video = {0};
    int ret = 0, index = 0, bytesused = 0;
    FILE *fp = NULL;
    fd_set fds;
    struct timeval tv;

    if (!device || !out_jpg_path)
    {
        if (errbuf && errbuf_sz)
            snprintf(errbuf, errbuf_sz, "[%s] invalid arguments", __func__);
        return CAMERA_ERR;
    }

    video.fd = CameraOpen(device);
    if (video.fd < 0)
    {
        if (errbuf && errbuf_sz)
            snprintf(errbuf, errbuf_sz, "[%s] open device failed", __func__);
        return CAMERA_ERR;
    }
    if (CameraQueryCapability(&video) < 0)
    {
        if (errbuf && errbuf_sz)
            snprintf(errbuf, errbuf_sz, "[%s] query cap failed", __func__);
        ret = CAMERA_ERR;
        goto cleanup;
    }
    if (CameraSetParameters(&video, width, height) < 0)
    {
        if (errbuf && errbuf_sz)
            snprintf(errbuf, errbuf_sz, "[%s] set fmt failed", __func__);
        ret = CAMERA_ERR;
        goto cleanup;
    }
    if (CameraSetMemoryMap(&video, 1) < 0)
    {
        if (errbuf && errbuf_sz)
            snprintf(errbuf, errbuf_sz, "[%s] mmap failed", __func__);
        ret = CAMERA_ERR;
        goto cleanup;
    }
    if (CameraStreamOn(&video) < 0)
    {
        if (errbuf && errbuf_sz)
            snprintf(errbuf, errbuf_sz, "[%s] stream on failed", __func__);
        ret = CAMERA_ERR;
        goto cleanup;
    }

    // wait one frame
    FD_ZERO(&fds);
    FD_SET(video.fd, &fds);
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    ret = select(video.fd + 1, &fds, NULL, NULL, &tv);
    if (ret <= 0)
    {
        if (errbuf && errbuf_sz)
            snprintf(errbuf, errbuf_sz, ret == 0 ? "[%s] select timeout" : "[%s] select error", __func__);
        ret = CAMERA_ERR;
        goto cleanup;
    }
    if (CameraReadFrame(&video, &index, &bytesused) < 0)
    {
        if (errbuf && errbuf_sz)
            snprintf(errbuf, errbuf_sz, "[%s] dequeue failed", __func__);
        ret = CAMERA_ERR;
        goto cleanup;
    }

    // Require JPEG/MJPEG data for now
    unsigned char *framePtr = (unsigned char *)video.mmap_buf[index].addr;
    int is_jpeg = (bytesused >= 2 && framePtr[0] == 0xFF && framePtr[1] == 0xD8);
    if (!is_jpeg)
    {
        if (errbuf && errbuf_sz)
            snprintf(errbuf, errbuf_sz, "[%s] device not providing MJPEG/JPEG frame", __func__);
        ret = -2; // special code: not JPEG
        goto requeue_only;
    }

    // Ensure output directory exists (best-effort)
    // Note: using system calls could be avoided; here we just try to open directly.
    fp = fopen(out_jpg_path, "wb");
    if (!fp)
    {
        if (errbuf && errbuf_sz)
            snprintf(errbuf, errbuf_sz, "[%s] open output failed: %s", __func__, strerror(errno));
        ret = CAMERA_ERR;
        goto requeue_only;
    }
    if (bytesused <= 0 || bytesused > video.mmap_buf[index].size)
    {
        bytesused = video.mmap_buf[index].size;
    }
    fwrite(video.mmap_buf[index].addr, bytesused, 1, fp);
    fclose(fp);
    ret = CAMERA_OK;

requeue_only:
    CameraQueueBuffer(&video, index);
cleanup:
    CameraStreamOff(&video);
    CameraClose(&video);
    return ret;
}