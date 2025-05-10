# 摄像头采集软件开发日志

## 项目信息

- **项目名称**: 摄像头采集软件
- **开发平台**: Linux (Debian/Ubuntu)
- **CPU类型**: ARM64 (Radxa)
- **操作系统**: Debian Bookworm

## 依赖库

- libv4l-dev: 用于访问V4L2设备
- libimgui-dev: 用于GUI界面
- libglfw3-dev: 用于窗口管理
- libopencv-dev: 用于视频处理

## 项目结构

```
captureVideo/
├── CMakeLists.txt
├── include/
│   ├── camera_device.h
│   ├── video_capture.h
│   ├── video_recorder.h
│   ├── file_manager.h
│   ├── frame_extractor.h
│   ├── gui.h
│   └── utils.h
└── src/
    ├── main.cpp
    ├── camera_device.cpp
    ├── video_capture.cpp
    ├── video_recorder.cpp
    ├── file_manager.cpp
    ├── frame_extractor.cpp
    ├── gui.cpp
    └── utils.cpp
```

## 功能模块

1. **摄像头设备管理 (camera_device)**
   - 扫描系统中的V4L2设备
   - 获取设备支持的分辨率和帧率
   - 设置设备参数

2. **视频采集 (video_capture)**
   - 从摄像头获取视频流
   - 提供帧回调机制

3. **视频录制 (video_recorder)**
   - 将视频流保存为文件
   - 生成包含日期时间、分辨率和帧率的文件名

4. **文件管理 (file_manager)**
   - 管理录制的视频文件
   - 提供文件列表和删除功能

5. **视频分帧 (frame_extractor)**
   - 将视频文件转换为静帧
   - 保存到相同文件名的子目录中

6. **GUI界面 (gui)**
   - 设备列表
   - 预览窗口
   - 录制控制
   - 文件列表
   - 分帧控制

7. **工具函数 (utils)**
   - 日期时间格式化
   - 文件大小格式化
   - 时间格式化
   - 目录操作

## 开发日志

### 2024-05-10

- 初始化项目结构
- 创建CMakeLists.txt
- 实现基本的摄像头设备管理模块
- 实现基本的视频采集模块
- 实现基本的视频录制模块
- 实现基本的文件管理模块
- 实现基本的视频分帧模块
- 实现基本的GUI界面
- 实现工具函数

## 编译与运行

### 编译

```bash
mkdir build
cd build
cmake ..
make
```

### 运行

```bash
./capture_video
```

## 待办事项

- [ ] 添加错误处理和日志记录
- [ ] 优化GUI界面
- [ ] 添加视频预览功能
- [ ] 添加视频编码选项
- [ ] 添加多语言支持
- [ ] 添加配置保存功能
