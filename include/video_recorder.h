#pragma once

#include "video_capture.h"
#include <string>
#include <opencv2/opencv.hpp>
#include <mutex>
#include <atomic>

// 视频录制类
class VideoRecorder {
public:
    VideoRecorder();
    ~VideoRecorder();

    // 初始化录制器
    bool init(const std::string& outputDir);
    
    // 开始录制
    bool startRecording(const Resolution& resolution, int framerate);
    
    // 停止录制
    void stopRecording();
    
    // 处理帧（由VideoCapture回调）
    void processFrame(const cv::Mat& frame);
    
    // 是否正在录制
    bool isRecording() const { return m_isRecording; }
    
    // 获取当前录制文件路径
    std::string getCurrentFilePath() const { return m_currentFilePath; }
    
    // 获取录制时长（秒）
    double getRecordingDuration() const;

private:
    std::string m_outputDir;  // 输出目录
    std::string m_currentFilePath;  // 当前录制文件路径
    
    cv::VideoWriter m_videoWriter;  // OpenCV视频写入器
    std::mutex m_writerMutex;  // 写入器互斥锁
    
    std::atomic<bool> m_isRecording;  // 是否正在录制
    std::chrono::time_point<std::chrono::steady_clock> m_startTime;  // 开始录制时间
    
    // 生成文件名（包含日期时间、分辨率和帧率）
    std::string generateFileName(const Resolution& resolution, int framerate);
};
