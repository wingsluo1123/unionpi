# Whale Car 多设备透传服务器

## 项目简介

`whale_car` 是一个基于 OpenHarmony OS 的多设备数据透传服务器项目。该项目结合了 `camera_net` 和 `uart_to_tcp` 示例的功能，实现了一个统一的服务器，能够同时处理多个串口设备和摄像头设备的数据传输。

## 功能特性

- **多设备支持**：同时支持 4 个 UART 串口设备（IMU、AMC、Radar、Lidar）和 1 个摄像头设备
- **TCP 透传**：所有设备数据通过 TCP 端口进行透传
- **多线程架构**：每个设备在独立的线程中运行，确保并发处理
- **摄像头 MJPEG 流**：摄像头设备支持 MJPEG 格式的视频流传输，可配置分辨率和帧率
- **串口数据透传**：串口设备支持双向数据透传（网络→串口、串口→网络）
- **网络信息显示**：启动时显示服务器的 IP 地址信息
- **优雅关闭**：支持 SIGINT/SIGTERM 信号的优雅关闭

## 端口分配

| 设备类型 | 设备名称 | TCP 端口 | 波特率/格式 | 参数 |
|----------|----------|----------|-------------|------|
| UART     | IMU      | 8000     | 115200      | -    |
| UART     | AMC      | 8001     | 115200      | -    |
| UART     | Radar    | 8002     | 115200      | -    |
| UART     | Lidar    | 8003     | 230400      | -    |
| Camera   | Camera   | 8004     | MJPEG       | 640x480@30fps (默认) |

## 硬件要求

- **开发板**：UnionPi Whale 或兼容的 OpenHarmony 开发板
- **串口设备**：4 个串口设备，连接到相应的 UART 接口
- **摄像头**：支持 V4L2 的摄像头设备（例如 /dev/video21）
- **网络**：支持 TCP/IP 网络连接

## 软件依赖

- **OpenHarmony OS**：v4.0 或更高版本
- **编译工具链**：hb (HarmonyOS Build)
- **依赖库**：
  - c_utils：通用工具库
  - camera_driver：摄像头驱动
  - uart_base：串口基础库

## 构建说明

### 环境准备

确保 OpenHarmony 开发环境已正确配置，包括：
- OpenHarmony SDK
- hb 构建工具
- 交叉编译工具链

### 构建步骤

1. 进入 OpenHarmony 源码根目录：
   ```bash
   cd <OpenHarmony源码根目录>
   ```

2. 构建项目：
   ```bash
   hb build -T whale_car
   ```

3. 快速重新构建（修改代码后）：
   ```bash
   hb build -T whale_car --fast-rebuild
   ```

## 运行说明

### 命令行参数

程序需要指定所有 5 个设备的路径，以及可选的摄像头参数：

```bash
./whale_car --IMU <imu_device> --AMC <amc_device> --Radar <radar_device> --Lidar <lidar_device> --Camera <camera_device> [选项]
```

#### 必需参数

- `--IMU <device>`: IMU 串口设备路径
- `--AMC <device>`: AMC 串口设备路径
- `--Radar <device>`: Radar 串口设备路径
- `--Lidar <device>`: Lidar 串口设备路径
- `--Camera <device>`: 摄像头设备路径

#### 可选参数

- `--width <width>`: 摄像头宽度 (默认: 640)
- `--height <height>`: 摄像头高度 (默认: 480)
- `--fps <fps>`: 摄像头帧率 (默认: 30)

### 示例

```bash
# 使用默认摄像头参数
./whale_car --IMU /dev/ttyUSB0 --AMC /dev/ttyUSB1 --Radar /dev/ttyUSB2 --Lidar /dev/ttyUSB3 --Camera /dev/video21

# 指定摄像头参数
./whale_car --IMU /dev/ttyUSB0 --AMC /dev/ttyUSB1 --Radar /dev/ttyUSB2 --Lidar /dev/ttyUSB3 --Camera /dev/video21 --width 1280 --height 720 --fps 25
```

### 网络连接

启动后，程序会显示服务器的 IP 地址信息。客户端可以通过以下方式连接：

- **串口设备**：使用 TCP 客户端连接相应端口，进行双向数据透传
- **摄像头设备**：连接端口 8004，接收 MJPEG 视频流

## 架构说明

### 整体架构

```
┌─────────────────┐
│   main.c        │  主程序入口
│                 │  ├── 参数解析
│                 │  ├── 网络信息显示
│                 │  ├── 摄像头初始化
│                 │  └── 线程创建
└─────────────────┘
         │
    ┌────┴────┐
    │ 线程池  │  5个设备线程
    └────┴────┘
         │
    ┌────┼────┐
    │         │
┌───┴───┐ ┌───┴───┐
│ UART  │ │Camera│
│线程    │ │线程  │
└───┬───┘ └───┬───┘
    │         │
    │         │
┌───┴───┐ ┌───┴───┐
│ TCP   │ │ TCP   │
│服务器  │ │服务器  │
└───┬───┘ └───┬───┘
    │         │
    │         │
┌───┴───┐ ┌───┴───┐
│串口数据│ │MJPEG │
│透传    │ │视频流│
└───────┘ └───────┘
```

### 主要组件

- **main.c**：主程序逻辑，负责初始化、线程管理和信号处理
- **camera_driver.c**：摄像头驱动，处理 V4L2 摄像头操作
- **uart_base.c**：串口基础操作，提供串口配置和 I/O 函数

### 数据流

1. **串口设备**：
   - 客户端连接 → TCP 服务器 → 串口设备
   - 串口设备 → TCP 服务器 → 客户端

2. **摄像头设备**：
   - 客户端连接 → TCP 服务器 → MJPEG 帧数据流

## 开发历史

### v1.0.0 (初始版本)
- 实现基本的多设备透传功能
- 支持 UART 和摄像头设备
- 多线程架构实现

### 最新修改
- 添加摄像头参数配置支持：--width、--height、--fps（可选，默认640x480@30fps）
- 使用 `__func__` 宏统一输出格式
- 移除调试信息，提高输出清洁度
- 修复编译警告和错误
- 优化网络信息显示

## 故障排除

### 常见问题

1. **设备路径错误**
   - 确保所有设备路径正确
   - 检查设备权限

2. **端口占用**
   - 确保 8000-8004 端口未被其他程序占用

3. **摄像头初始化失败**
   - 检查摄像头设备路径
   - 确认摄像头支持 MJPEG 格式

4. **串口通信失败**
   - 检查串口设备连接
   - 确认波特率设置正确

### 日志分析

程序使用统一的日志格式 `[function_name] message`，便于调试和监控。

## 许可证

本项目遵循 OpenHarmony 开源许可证。

## 贡献

欢迎提交 Issue 和 Pull Request 来改进项目。

## 联系方式

如有问题，请通过 GitHub Issues 联系项目维护者。
