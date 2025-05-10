#include "file_manager.h"
#include "utils.h"
#include <iostream>
#include <algorithm>
#include <regex>
#include <opencv2/opencv.hpp>

FileManager::FileManager() {
}

FileManager::~FileManager() {
}

bool FileManager::init(const std::string& baseDir) {
    m_baseDir = baseDir;
    
    // 确保目录存在
    if (!Utils::ensureDirectoryExists(m_baseDir)) {
        std::cerr << "无法创建基础目录: " << m_baseDir << std::endl;
        return false;
    }
    
    return true;
}

std::vector<VideoFileInfo> FileManager::getVideoFileList() {
    std::vector<VideoFileInfo> videoFiles;
    
    try {
        // 遍历目录
        for (const auto& entry : fs::directory_iterator(m_baseDir)) {
            if (entry.is_regular_file()) {
                std::string filePath = entry.path().string();
                
                // 检查是否为视频文件
                if (Utils::isVideoFile(filePath)) {
                    // 解析文件信息
                    VideoFileInfo fileInfo = parseFileName(entry.path());
                    
                    // 获取文件大小
                    fileInfo.fileSize = entry.file_size();
                    
                    // 获取视频时长和其他信息
                    double duration = 0.0;
                    int width = 0, height = 0, framerate = 0;
                    if (Utils::getVideoFileInfo(filePath, duration, width, height, framerate)) {
                        fileInfo.duration = duration;
                    }
                    
                    videoFiles.push_back(fileInfo);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "获取视频文件列表时出错: " << e.what() << std::endl;
    }
    
    // 按日期时间排序（最新的在前）
    std::sort(videoFiles.begin(), videoFiles.end(), 
             [](const VideoFileInfo& a, const VideoFileInfo& b) {
                 return a.dateTime > b.dateTime;
             });
    
    return videoFiles;
}

bool FileManager::deleteVideoFile(const std::string& filePath) {
    try {
        // 检查文件是否存在
        if (!fs::exists(filePath)) {
            std::cerr << "文件不存在: " << filePath << std::endl;
            return false;
        }
        
        // 删除文件
        fs::remove(filePath);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "删除文件时出错: " << e.what() << std::endl;
        return false;
    }
}

bool FileManager::createDirectory(const std::string& dirPath) {
    return Utils::ensureDirectoryExists(dirPath);
}

VideoFileInfo FileManager::parseFileName(const fs::path& filePath) {
    VideoFileInfo fileInfo;
    fileInfo.filePath = filePath.string();
    fileInfo.fileName = filePath.filename().string();
    
    // 使用正则表达式解析文件名
    // 格式：日期时间_分辨率_帧率.mp4
    // 例如：20230101_120000_1920x1080_30fps.mp4
    std::regex pattern(R"((\d{8}_\d{6})_(\d+x\d+)_(\d+)fps\..+)");
    std::smatch matches;
    
    if (std::regex_match(fileInfo.fileName, matches, pattern) && matches.size() == 4) {
        fileInfo.dateTime = matches[1].str();
        fileInfo.resolution = matches[2].str();
        fileInfo.framerate = std::stoi(matches[3].str());
    } else {
        // 如果无法解析，设置默认值
        fileInfo.dateTime = "未知";
        fileInfo.resolution = "未知";
        fileInfo.framerate = 0;
    }
    
    return fileInfo;
}
