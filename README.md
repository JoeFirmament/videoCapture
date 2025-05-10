# 摄像头采集软件

基于IMGUI的摄像头采集软件，支持设备识别、预览、录像、文件管理和视频分帧功能。

## 功能特点

- 使用V4L2识别USB摄像头设备
- 显示摄像头支持的分辨率和帧率
- 实时预览摄像头画面
- 录制视频，文件名包含日期时间、分辨率和帧率信息
- 管理录制的视频文件
- 将视频文件分帧为静态图像

## 系统要求

- Linux操作系统
- V4L2兼容的摄像头设备
- OpenGL 3.3+支持

## 依赖库

- libv4l-dev
- libimgui-dev
- libglfw3-dev
- libopencv-dev

## 安装依赖

```bash
sudo apt-get install -y libv4l-dev libimgui-dev libglfw3-dev libopencv-dev
```

## 编译

```bash
mkdir build
cd build
cmake ..
make
```

## 运行

```bash
./capture_video
```

## 使用说明

### 设备选择

1. 启动程序后，在左侧"设备列表"面板中选择一个摄像头设备
2. 在"录制控制"面板中选择分辨率和帧率
3. 点击"开始预览"按钮开始预览摄像头画面

### 录制视频

1. 在预览状态下，点击"开始录像"按钮开始录制
2. 录制过程中会显示录制时长
3. 点击"停止录像"按钮停止录制
4. 录制的视频文件会自动保存到`~/captureVideo/videos`目录下

### 文件管理

1. 在左侧"文件列表"面板中可以查看所有录制的视频文件
2. 右键点击文件可以选择删除

### 视频分帧

1. 在"文件列表"中选择一个视频文件
2. 在"视频分帧"面板中点击"开始分帧"按钮
3. 分帧过程中会显示进度
4. 分帧完成后，静态图像会保存到与视频文件同名的子目录中

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

## 许可证

MIT
