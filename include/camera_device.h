#pragma once

#include <string>
#include <vector>
#include <linux/videodev2.h>

// 分辨率结构体
struct Resolution {
    int width;
    int height;

    Resolution(int w, int h) : width(w), height(h) {}

    std::string toString() const {
        return std::to_string(width) + "x" + std::to_string(height);
    }

    bool operator==(const Resolution& other) const {
        return width == other.width && height == other.height;
    }

    // 添加小于运算符，用于std::set排序
    bool operator<(const Resolution& other) const {
        if (width != other.width)
            return width < other.width;
        return height < other.height;
    }
};

// 摄像头设备信息结构体
struct CameraDeviceInfo {
    std::string devicePath;      // 设备路径，如 /dev/video0
    std::string deviceName;      // 设备名称
    std::vector<Resolution> supportedResolutions;  // 支持的分辨率列表
    std::vector<int> supportedFramerates;          // 支持的帧率列表
};

// 摄像头设备管理类
class CameraDevice {
public:
    CameraDevice();
    ~CameraDevice();

    // 扫描系统中的所有摄像头设备
    std::vector<CameraDeviceInfo> scanDevices();

    // 打开指定的摄像头设备
    bool openDevice(const std::string& devicePath);

    // 关闭当前打开的设备
    void closeDevice();

    // 获取设备支持的分辨率列表
    std::vector<Resolution> getSupportedResolutions();

    // 获取设备支持的帧率列表
    std::vector<int> getSupportedFramerates(const Resolution& resolution);

    // 设置分辨率和帧率
    bool setResolutionAndFramerate(const Resolution& resolution, int framerate);

    // 获取当前设备文件描述符
    int getDeviceFd() const { return m_fd; }

    // 获取当前设备信息
    const CameraDeviceInfo& getCurrentDeviceInfo() const { return m_currentDevice; }

private:
    int m_fd;  // 设备文件描述符
    CameraDeviceInfo m_currentDevice;  // 当前设备信息

    // 查询设备支持的格式
    bool queryDeviceFormats(CameraDeviceInfo& deviceInfo);
};
