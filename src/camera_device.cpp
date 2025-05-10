#include "camera_device.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <string.h>
#include <algorithm>
#include <set>

CameraDevice::CameraDevice() : m_fd(-1) {
}

CameraDevice::~CameraDevice() {
    closeDevice();
}

std::vector<CameraDeviceInfo> CameraDevice::scanDevices() {
    std::vector<CameraDeviceInfo> devices;
    
    // 打开/dev目录
    DIR* dir = opendir("/dev");
    if (!dir) {
        std::cerr << "无法打开/dev目录" << std::endl;
        return devices;
    }
    
    // 遍历/dev目录，查找video设备
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        
        // 检查是否为video设备
        if (name.find("video") == 0) {
            std::string devicePath = "/dev/" + name;
            
            // 尝试打开设备
            int fd = open(devicePath.c_str(), O_RDWR);
            if (fd < 0) {
                continue;  // 无法打开，跳过
            }
            
            // 获取设备信息
            struct v4l2_capability cap;
            if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
                close(fd);
                continue;  // 无法获取设备信息，跳过
            }
            
            // 检查是否为视频捕获设备
            if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
                close(fd);
                continue;  // 不是视频捕获设备，跳过
            }
            
            // 创建设备信息
            CameraDeviceInfo deviceInfo;
            deviceInfo.devicePath = devicePath;
            deviceInfo.deviceName = reinterpret_cast<const char*>(cap.card);
            
            // 查询设备支持的格式
            if (queryDeviceFormats(deviceInfo)) {
                devices.push_back(deviceInfo);
            }
            
            close(fd);
        }
    }
    
    closedir(dir);
    return devices;
}

bool CameraDevice::openDevice(const std::string& devicePath) {
    // 关闭已打开的设备
    closeDevice();
    
    // 打开新设备
    m_fd = open(devicePath.c_str(), O_RDWR);
    if (m_fd < 0) {
        std::cerr << "无法打开设备: " << devicePath << std::endl;
        return false;
    }
    
    // 获取设备信息
    struct v4l2_capability cap;
    if (ioctl(m_fd, VIDIOC_QUERYCAP, &cap) < 0) {
        std::cerr << "无法获取设备信息" << std::endl;
        closeDevice();
        return false;
    }
    
    // 检查是否为视频捕获设备
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        std::cerr << "不是视频捕获设备" << std::endl;
        closeDevice();
        return false;
    }
    
    // 设置当前设备信息
    m_currentDevice.devicePath = devicePath;
    m_currentDevice.deviceName = reinterpret_cast<const char*>(cap.card);
    
    // 查询设备支持的格式
    if (!queryDeviceFormats(m_currentDevice)) {
        closeDevice();
        return false;
    }
    
    return true;
}

void CameraDevice::closeDevice() {
    if (m_fd >= 0) {
        close(m_fd);
        m_fd = -1;
    }
}

std::vector<Resolution> CameraDevice::getSupportedResolutions() {
    return m_currentDevice.supportedResolutions;
}

std::vector<int> CameraDevice::getSupportedFramerates(const Resolution& resolution) {
    if (m_fd < 0) {
        return {};
    }
    
    std::vector<int> framerates;
    
    // 设置格式
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = resolution.width;
    fmt.fmt.pix.height = resolution.height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;  // 使用常见的YUYV格式
    fmt.fmt.pix.field = V4L2_FIELD_ANY;
    
    if (ioctl(m_fd, VIDIOC_S_FMT, &fmt) < 0) {
        return {};
    }
    
    // 查询支持的帧率
    struct v4l2_streamparm parm;
    memset(&parm, 0, sizeof(parm));
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    if (ioctl(m_fd, VIDIOC_G_PARM, &parm) < 0) {
        return {};
    }
    
    // 检查是否支持设置帧率
    if (!(parm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME)) {
        // 不支持设置帧率，返回默认帧率
        framerates.push_back(30);
        return framerates;
    }
    
    // 常见帧率列表
    std::vector<int> commonFramerates = {15, 30, 60};
    
    for (int fps : commonFramerates) {
        parm.parm.capture.timeperframe.numerator = 1;
        parm.parm.capture.timeperframe.denominator = fps;
        
        // 尝试设置帧率
        if (ioctl(m_fd, VIDIOC_S_PARM, &parm) >= 0) {
            // 获取实际设置的帧率
            int actualFps = parm.parm.capture.timeperframe.denominator / 
                           parm.parm.capture.timeperframe.numerator;
            
            // 添加到列表（避免重复）
            if (std::find(framerates.begin(), framerates.end(), actualFps) == framerates.end()) {
                framerates.push_back(actualFps);
            }
        }
    }
    
    return framerates;
}

bool CameraDevice::setResolutionAndFramerate(const Resolution& resolution, int framerate) {
    if (m_fd < 0) {
        return false;
    }
    
    // 设置格式
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = resolution.width;
    fmt.fmt.pix.height = resolution.height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;  // 使用常见的YUYV格式
    fmt.fmt.pix.field = V4L2_FIELD_ANY;
    
    if (ioctl(m_fd, VIDIOC_S_FMT, &fmt) < 0) {
        std::cerr << "无法设置视频格式" << std::endl;
        return false;
    }
    
    // 设置帧率
    struct v4l2_streamparm parm;
    memset(&parm, 0, sizeof(parm));
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    if (ioctl(m_fd, VIDIOC_G_PARM, &parm) < 0) {
        std::cerr << "无法获取流参数" << std::endl;
        return false;
    }
    
    // 检查是否支持设置帧率
    if (!(parm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME)) {
        std::cerr << "设备不支持设置帧率" << std::endl;
        return false;
    }
    
    parm.parm.capture.timeperframe.numerator = 1;
    parm.parm.capture.timeperframe.denominator = framerate;
    
    if (ioctl(m_fd, VIDIOC_S_PARM, &parm) < 0) {
        std::cerr << "无法设置帧率" << std::endl;
        return false;
    }
    
    return true;
}

bool CameraDevice::queryDeviceFormats(CameraDeviceInfo& deviceInfo) {
    int fd = -1;
    
    // 如果已经打开设备，使用当前文件描述符
    if (m_fd >= 0 && deviceInfo.devicePath == m_currentDevice.devicePath) {
        fd = m_fd;
    } else {
        // 否则打开设备
        fd = open(deviceInfo.devicePath.c_str(), O_RDWR);
        if (fd < 0) {
            return false;
        }
    }
    
    // 查询支持的格式
    struct v4l2_fmtdesc fmtdesc;
    memset(&fmtdesc, 0, sizeof(fmtdesc));
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    std::set<Resolution> resolutions;
    
    // 遍历所有支持的格式
    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) >= 0) {
        // 查询该格式支持的分辨率
        struct v4l2_frmsizeenum frmsize;
        memset(&frmsize, 0, sizeof(frmsize));
        frmsize.pixel_format = fmtdesc.pixelformat;
        frmsize.index = 0;
        
        while (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) >= 0) {
            if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                // 离散分辨率
                Resolution res(frmsize.discrete.width, frmsize.discrete.height);
                resolutions.insert(res);
            } else if (frmsize.type == V4L2_FRMSIZE_TYPE_STEPWISE) {
                // 步进分辨率，添加一些常见分辨率
                for (uint32_t width = frmsize.stepwise.min_width; 
                     width <= frmsize.stepwise.max_width; 
                     width += frmsize.stepwise.step_width) {
                    for (uint32_t height = frmsize.stepwise.min_height; 
                         height <= frmsize.stepwise.max_height; 
                         height += frmsize.stepwise.step_height) {
                        Resolution res(width, height);
                        resolutions.insert(res);
                    }
                }
            }
            
            frmsize.index++;
        }
        
        fmtdesc.index++;
    }
    
    // 将分辨率集合转换为向量
    deviceInfo.supportedResolutions.clear();
    for (const auto& res : resolutions) {
        deviceInfo.supportedResolutions.push_back(res);
    }
    
    // 如果不是当前打开的设备，关闭文件描述符
    if (fd != m_fd) {
        close(fd);
    }
    
    return !deviceInfo.supportedResolutions.empty();
}
