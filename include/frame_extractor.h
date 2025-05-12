#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>

// 视频分帧类
class FrameExtractor {
public:
    FrameExtractor();
    ~FrameExtractor();

    // 开始分帧
    bool startExtraction(const std::string& videoFilePath);

    // 停止分帧
    void stopExtraction();

    // 是否正在分帧
    bool isExtracting() const { return m_isExtracting; }

    // 获取进度（0.0-1.0）
    float getProgress() const { return m_progress; }

    // 设置进度回调
    void setProgressCallback(std::function<void(float)> callback);

    // 设置完成回调
    void setCompletionCallback(std::function<void(const std::string&)> callback);

private:
    std::string m_videoFilePath;  // 视频文件路径
    std::string m_outputDir;      // 输出目录

    std::atomic<bool> m_isExtracting;  // 是否正在分帧
    std::atomic<float> m_progress;     // 进度

    std::thread m_extractionThread;  // 分帧线程

    std::function<void(float)> m_progressCallback;  // 进度回调
    std::function<void(const std::string&)> m_completionCallback;  // 完成回调

    // 分帧线程函数（保留但不再使用）
    void extractionThreadFunc();

    // 创建输出目录
    bool createOutputDir();

    // 同步分帧方法
    void extractFrames();
};
