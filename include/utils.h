#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <opencv2/opencv.hpp>

namespace Utils {
    // 获取当前日期时间字符串（格式：YYYYMMDD_HHMMSS）
    std::string getCurrentDateTimeString();
    
    // 格式化文件大小（转换为KB/MB/GB）
    std::string formatFileSize(size_t sizeInBytes);
    
    // 格式化时间（秒转为HH:MM:SS）
    std::string formatTime(double seconds);
    
    // 从文件名中提取信息（日期时间、分辨率、帧率）
    bool extractInfoFromFileName(const std::string& fileName, 
                                std::string& dateTime, 
                                std::string& resolution, 
                                int& framerate);
    
    // 将OpenCV Mat转换为ImGui纹理数据
    void matToTexture(const cv::Mat& mat, unsigned char* textureData, int width, int height);
    
    // 检查目录是否存在，不存在则创建
    bool ensureDirectoryExists(const std::string& path);
    
    // 获取文件扩展名
    std::string getFileExtension(const std::string& filePath);
    
    // 检查文件是否为视频文件
    bool isVideoFile(const std::string& filePath);
    
    // 获取视频文件信息（时长、分辨率等）
    bool getVideoFileInfo(const std::string& filePath, 
                         double& duration, 
                         int& width, 
                         int& height, 
                         int& framerate);
};
