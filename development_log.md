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

### 2024-05-11

- 修复了帧提取功能的异常终止问题
  - 重新设计了帧提取的实现方式，从异步线程改为同步实现
  - 添加了更多的异常处理和错误检查
  - 改进了资源管理，确保所有资源在使用后被正确释放

- 优化了GUI界面
  - 强制使用OpenGL ES 3.1，以适应Radxa Rock 5C的Mali-G610 GPU
  - 修改了GLFW窗口提示，使用更兼容的设置
  - 在GUI中使用单独的线程执行帧提取，避免阻塞GUI

### 2024-05-11 - GPU兼容性分析

#### RK3588（Mali-G610）的 OpenGL/GLES 支持情况

- **OpenGL 3.1**（核心规范）
- **OpenGL ES 3.1**（嵌入式规范）
- **GLSL 版本**：1.40（OpenGL） / GLSL ES 3.10（OpenGL ES）
- **驱动**：Mesa + Panfrost（开源 Mali 驱动）

#### 对 ImGui 的配置选择

经过测试，我们发现在Radxa Rock 5C上：

1. **首选方案**：强制使用 OpenGL ES 3.1
   - 设置GLFW窗口提示为`GLFW_OPENGL_ES_API`
   - 设置OpenGL ES版本为3.1
   - 为ImGui设置GLSL ES版本为`#version 310 es`

2. **避免使用**：OpenGL 3.3核心配置
   - 在X11转发环境下可能会导致"无法创建GLFW窗口"错误
   - 可能与Mali-G610的驱动兼容性有关

## 编译与运行

### 编译

```bash
mkdir build
cd build
cmake ..
make
```

### 运行

#### GUI模式（默认）

```bash
./capture_video
# 或
./capture_video --gui
```

#### 命令行模式

列出设备：
```bash
./capture_video --cli list-devices
```

列出文件：
```bash
./capture_video --cli list-files
```

录制视频：
```bash
./capture_video --cli record --device=0 --width=640 --height=480 --fps=30 --time=10
```

提取帧：
```bash
./capture_video --cli extract --file=/path/to/video.mp4
```

### 测试X11转发

如果需要在远程机器上运行GUI模式，确保X11转发正常工作：

```bash
echo $DISPLAY  # 应该显示类似 :0 或 hostname:10.0 的输出
```

可以使用简单的X11测试程序验证转发是否正常：

```c
// test_x11.c
#include <X11/Xlib.h>
#include <stdio.h>

int main() {
    Display *display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }

    printf("Hello X11\n");
    XCloseDisplay(display);
    return 0;
}
```

编译和运行：
```bash
gcc -o test_x11 test_x11.c -lX11
./test_x11
```

## 待办事项

- [x] 添加错误处理和日志记录
- [x] 优化GUI界面（适配OpenGL ES 3.1）
- [x] 添加视频预览功能
- [ ] 改进GUI模式下的帧提取功能
  - [ ] 添加进度显示优化
  - [ ] 添加估计剩余时间功能
- [ ] 添加视频编码选项
  - [ ] 支持不同的编码器（H.264, H.265等）
  - [ ] 支持不同的比特率设置
- [ ] 添加多语言支持
- [ ] 添加配置保存功能
- [ ] 性能优化
  - [ ] 使用多线程并行处理多个帧，提高处理速度
  - [ ] 添加帧提取参数，如提取间隔、分辨率调整等
