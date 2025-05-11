#include "video_capture.h"
#include <iostream>
#include <chrono>

VideoCapture::VideoCapture()
    : m_device(nullptr),
      m_currentResolution(0, 0),
      m_currentFramerate(0),
      m_isCapturing(false) {
}

VideoCapture::~VideoCapture() {
    stop();
}

bool VideoCapture::init(CameraDevice& device, const Resolution& resolution, int framerate) {
    // 停止当前采集
    stop();

    // 设置设备
    m_device = &device;

    // 设置分辨率和帧率
    if (!m_device->setResolutionAndFramerate(resolution, framerate)) {
        std::cerr << "无法设置分辨率和帧率" << std::endl;
        return false;
    }

    // 保存当前设置
    m_currentResolution = resolution;
    m_currentFramerate = framerate;

    return true;
}

bool VideoCapture::start() {
    if (m_isCapturing) {
        return true;  // 已经在采集中
    }

    if (!m_device || m_device->getDeviceFd() < 0) {
        std::cerr << "设备未初始化" << std::endl;
        return false;
    }

    // 设置采集标志
    m_isCapturing = true;

    // 启动采集线程
    m_captureThread = std::thread(&VideoCapture::captureThreadFunc, this);

    return true;
}

void VideoCapture::stop() {
    if (!m_isCapturing) {
        return;  // 没有在采集
    }

    // 清除采集标志
    m_isCapturing = false;

    // 等待采集线程结束
    if (m_captureThread.joinable()) {
        m_captureThread.join();
    }
}

cv::Mat VideoCapture::getCurrentFrame() {
    std::lock_guard<std::mutex> lock(m_frameMutex);
    return m_currentFrame.clone();
}

void VideoCapture::setFrameCallback(std::function<void(const cv::Mat&)> callback) {
    m_frameCallback = callback;
}

void VideoCapture::captureThreadFunc() {
    // 构建GStreamer管道字符串
    std::string devicePath = m_device->getCurrentDeviceInfo().devicePath;
    std::string gstPipeline = "v4l2src device=" + devicePath +
                             " ! video/x-raw,width=" + std::to_string(m_currentResolution.width) +
                             ",height=" + std::to_string(m_currentResolution.height) +
                             ",framerate=" + std::to_string(m_currentFramerate) + "/1" +
                             " ! videoconvert ! appsink";

    std::cout << "使用GStreamer管道: " << gstPipeline << std::endl;

    // 打开OpenCV视频捕获，使用GStreamer后端
    cv::VideoCapture cap(gstPipeline, cv::CAP_GSTREAMER);

    if (!cap.isOpened()) {
        std::cerr << "无法打开摄像头，GStreamer管道: " << gstPipeline << std::endl;
        m_isCapturing = false;
        return;
    }

    // 不需要再设置分辨率和帧率，因为已经在GStreamer管道中指定了

    // 计算帧间隔（毫秒）
    int frameInterval = 1000 / m_currentFramerate;

    // 采集循环
    while (m_isCapturing) {
        auto startTime = std::chrono::steady_clock::now();

        // 捕获帧
        cv::Mat frame;
        if (!cap.read(frame)) {
            std::cerr << "无法读取帧" << std::endl;
            break;
        }

        if (!frame.empty()) {
            // 处理帧
            processFrame(frame);
        }

        // 计算剩余时间
        auto endTime = std::chrono::steady_clock::now();
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        // 等待，保持帧率
        if (elapsedMs < frameInterval) {
            std::this_thread::sleep_for(std::chrono::milliseconds(frameInterval - elapsedMs));
        }
    }

    // 释放资源
    cap.release();
}

void VideoCapture::processFrame(const cv::Mat& frame) {
    // 更新当前帧
    {
        std::lock_guard<std::mutex> lock(m_frameMutex);
        m_currentFrame = frame.clone();
    }

    // 调用回调函数
    if (m_frameCallback) {
        m_frameCallback(frame);
    }
}
