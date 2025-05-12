#include "frame_extractor.h"
#include "utils.h"
#include <iostream>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

FrameExtractor::FrameExtractor() : m_isExtracting(false), m_progress(0.0f) {
}

FrameExtractor::~FrameExtractor() {
    // 不再需要特殊处理，因为我们不再使用线程
}

bool FrameExtractor::startExtraction(const std::string& videoFilePath) {
    if (m_isExtracting) {
        return false;  // 已经在分帧中
    }
    
    // 检查文件是否存在
    if (!fs::exists(videoFilePath)) {
        std::cerr << "视频文件不存在: " << videoFilePath << std::endl;
        return false;
    }
    
    // 设置文件路径
    m_videoFilePath = videoFilePath;
    
    // 创建输出目录
    if (!createOutputDir()) {
        std::cerr << "无法创建输出目录" << std::endl;
        return false;
    }
    
    // 重置进度
    m_progress = 0.0f;
    
    // 设置分帧标志
    m_isExtracting = true;
    
    // 直接执行分帧，不使用线程
    extractFrames();
    
    return true;
}

void FrameExtractor::stopExtraction() {
    m_isExtracting = false;
}

void FrameExtractor::setProgressCallback(std::function<void(float)> callback) {
    m_progressCallback = callback;
}

void FrameExtractor::setCompletionCallback(std::function<void(const std::string&)> callback) {
    m_completionCallback = callback;
}

bool FrameExtractor::createOutputDir() {
    // 从视频文件路径中提取文件名（不含扩展名）
    fs::path videoPath(m_videoFilePath);
    std::string fileName = videoPath.stem().string();
    
    // 创建与视频文件同名的子目录
    m_outputDir = fs::path(videoPath).parent_path() / fileName;
    
    return Utils::ensureDirectoryExists(m_outputDir);
}

// 新的同步分帧方法，替代原来的线程函数
void FrameExtractor::extractFrames() {
    try {
        // 打开视频文件
        cv::VideoCapture cap(m_videoFilePath);
        
        if (!cap.isOpened()) {
            std::cerr << "无法打开视频文件: " << m_videoFilePath << std::endl;
            m_isExtracting = false;
            return;
        }
        
        // 获取视频信息
        int frameCount = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));
        int frameWidth = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
        int frameHeight = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
        
        // 检查帧数是否有效
        if (frameCount <= 0) {
            std::cerr << "无法获取视频帧数" << std::endl;
            m_isExtracting = false;
            return;
        }
        
        std::cout << "视频信息: " << frameWidth << "x" << frameHeight << ", " 
                  << frameCount << " 帧" << std::endl;
        
        // 分帧循环
        int currentFrame = 0;
        cv::Mat frame;
        
        while (m_isExtracting && cap.read(frame)) {
            try {
                // 生成帧文件名
                std::stringstream ss;
                ss << "frame_" << std::setw(6) << std::setfill('0') << currentFrame << ".jpg";
                std::string framePath = fs::path(m_outputDir) / ss.str();
                
                // 保存帧
                cv::imwrite(framePath, frame);
                
                // 更新进度
                currentFrame++;
                m_progress = static_cast<float>(currentFrame) / frameCount;
                
                // 调用进度回调
                if (m_progressCallback) {
                    m_progressCallback(m_progress);
                }
            } catch (const std::exception& e) {
                std::cerr << "处理帧 " << currentFrame << " 时发生异常: " << e.what() << std::endl;
                // 继续处理下一帧
            }
        }
        
        // 关闭视频
        cap.release();
        
        // 调用完成回调
        if (m_completionCallback && m_isExtracting) {
            m_completionCallback(m_outputDir);
        }
    } catch (const std::exception& e) {
        std::cerr << "分帧过程异常: " << e.what() << std::endl;
    }
    
    // 清除分帧标志
    m_isExtracting = false;
}

// 不再需要原来的线程函数
void FrameExtractor::extractionThreadFunc() {
    // 这个函数不再使用，但为了保持接口兼容性，我们保留它
    // 实际的分帧逻辑已经移到了extractFrames()方法中
}
