#pragma once

#include "camera_device.h"
#include <opencv2/opencv.hpp>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>

// 视频采集类
class VideoCapture {
public:
    VideoCapture();
    ~VideoCapture();

    // 初始化视频采集
    bool init(CameraDevice& device, const Resolution& resolution, int framerate);
    
    // 开始采集
    bool start();
    
    // 停止采集
    void stop();
    
    // 获取当前帧
    cv::Mat getCurrentFrame();
    
    // 设置帧回调函数
    void setFrameCallback(std::function<void(const cv::Mat&)> callback);
    
    // 是否正在采集
    bool isCapturing() const { return m_isCapturing; }
    
    // 获取当前分辨率
    Resolution getCurrentResolution() const { return m_currentResolution; }
    
    // 获取当前帧率
    int getCurrentFramerate() const { return m_currentFramerate; }

private:
    CameraDevice* m_device;  // 摄像头设备
    Resolution m_currentResolution;  // 当前分辨率
    int m_currentFramerate;  // 当前帧率
    
    std::atomic<bool> m_isCapturing;  // 是否正在采集
    std::thread m_captureThread;  // 采集线程
    std::mutex m_frameMutex;  // 帧互斥锁
    cv::Mat m_currentFrame;  // 当前帧
    
    std::function<void(const cv::Mat&)> m_frameCallback;  // 帧回调函数
    
    // 采集线程函数
    void captureThreadFunc();
    
    // 处理采集到的帧
    void processFrame(const cv::Mat& frame);
};
