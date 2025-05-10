#include "video_recorder.h"
#include "utils.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

VideoRecorder::VideoRecorder() : m_isRecording(false) {
}

VideoRecorder::~VideoRecorder() {
    stopRecording();
}

bool VideoRecorder::init(const std::string& outputDir) {
    m_outputDir = outputDir;
    
    // 确保输出目录存在
    if (!Utils::ensureDirectoryExists(m_outputDir)) {
        std::cerr << "无法创建输出目录: " << m_outputDir << std::endl;
        return false;
    }
    
    return true;
}

bool VideoRecorder::startRecording(const Resolution& resolution, int framerate) {
    if (m_isRecording) {
        return true;  // 已经在录制中
    }
    
    // 生成文件名
    m_currentFilePath = generateFileName(resolution, framerate);
    
    // 创建视频写入器
    {
        std::lock_guard<std::mutex> lock(m_writerMutex);
        
        // 使用H.264编码
        int fourcc = cv::VideoWriter::fourcc('H', '2', '6', '4');
        
        // 创建视频写入器
        m_videoWriter.open(m_currentFilePath, fourcc, framerate, 
                          cv::Size(resolution.width, resolution.height));
        
        if (!m_videoWriter.isOpened()) {
            std::cerr << "无法创建视频写入器" << std::endl;
            return false;
        }
    }
    
    // 记录开始时间
    m_startTime = std::chrono::steady_clock::now();
    
    // 设置录制标志
    m_isRecording = true;
    
    return true;
}

void VideoRecorder::stopRecording() {
    if (!m_isRecording) {
        return;  // 没有在录制
    }
    
    // 清除录制标志
    m_isRecording = false;
    
    // 关闭视频写入器
    {
        std::lock_guard<std::mutex> lock(m_writerMutex);
        m_videoWriter.release();
    }
}

void VideoRecorder::processFrame(const cv::Mat& frame) {
    if (!m_isRecording) {
        return;  // 没有在录制
    }
    
    // 写入帧
    std::lock_guard<std::mutex> lock(m_writerMutex);
    if (m_videoWriter.isOpened()) {
        m_videoWriter.write(frame);
    }
}

double VideoRecorder::getRecordingDuration() const {
    if (!m_isRecording) {
        return 0.0;
    }
    
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(now - m_startTime).count();
}

std::string VideoRecorder::generateFileName(const Resolution& resolution, int framerate) {
    // 获取当前日期时间
    std::string dateTime = Utils::getCurrentDateTimeString();
    
    // 生成文件名：日期时间_分辨率_帧率.mp4
    std::string fileName = dateTime + "_" + 
                          std::to_string(resolution.width) + "x" + std::to_string(resolution.height) + 
                          "_" + std::to_string(framerate) + "fps.mp4";
    
    // 完整路径
    return fs::path(m_outputDir) / fileName;
}
