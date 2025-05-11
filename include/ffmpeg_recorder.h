#pragma once

#include "camera_device.h"
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>

// FFmpeg录制类
class FFmpegRecorder {
public:
    FFmpegRecorder();
    ~FFmpegRecorder();

    // 初始化录制器
    bool init(const std::string& outputDir);
    
    // 开始录制
    bool startRecording(const std::string& devicePath, const Resolution& resolution, int framerate);
    
    // 停止录制
    void stopRecording();
    
    // 是否正在录制
    bool isRecording() const { return m_isRecording; }
    
    // 获取当前录制文件路径
    std::string getCurrentFilePath() const { return m_currentFilePath; }
    
    // 获取录制时长（秒）
    double getRecordingDuration() const;

private:
    std::string m_outputDir;  // 输出目录
    std::string m_currentFilePath;  // 当前录制文件路径
    
    std::atomic<bool> m_isRecording;  // 是否正在录制
    std::chrono::time_point<std::chrono::steady_clock> m_startTime;  // 开始录制时间
    
    std::thread m_recordingThread;  // 录制线程
    int m_ffmpegPid;  // FFmpeg进程ID
    
    // 录制线程函数
    void recordingThreadFunc(const std::string& devicePath, const Resolution& resolution, int framerate);
    
    // 生成文件名（包含日期时间、分辨率和帧率）
    std::string generateFileName(const Resolution& resolution, int framerate);
    
    // 构建FFmpeg命令
    std::string buildFFmpegCommand(const std::string& devicePath, const Resolution& resolution, int framerate, const std::string& outputPath);
    
    // 执行系统命令
    int executeCommand(const std::string& command);
    
    // 终止FFmpeg进程
    void terminateFFmpegProcess();
};
