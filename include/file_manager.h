#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

// 视频文件信息结构体
struct VideoFileInfo {
    std::string filePath;      // 文件路径
    std::string fileName;      // 文件名
    std::string dateTime;      // 日期时间
    std::string resolution;    // 分辨率
    int framerate;             // 帧率
    size_t fileSize;           // 文件大小（字节）
    double duration;           // 视频时长（秒）
};

// 文件管理类
class FileManager {
public:
    FileManager();
    ~FileManager();

    // 初始化文件管理器
    bool init(const std::string& baseDir);
    
    // 获取视频文件列表
    std::vector<VideoFileInfo> getVideoFileList();
    
    // 删除视频文件
    bool deleteVideoFile(const std::string& filePath);
    
    // 创建目录
    bool createDirectory(const std::string& dirPath);
    
    // 获取基础目录
    std::string getBaseDir() const { return m_baseDir; }
    
    // 解析文件名（从文件名中提取日期时间、分辨率和帧率）
    static VideoFileInfo parseFileName(const fs::path& filePath);

private:
    std::string m_baseDir;  // 基础目录
};
