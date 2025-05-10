#include "utils.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <regex>
#include <cmath>

namespace fs = std::filesystem;

namespace Utils {

std::string getCurrentDateTimeString() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
    
    return ss.str();
}

std::string formatFileSize(size_t sizeInBytes) {
    static const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = static_cast<double>(sizeInBytes);
    
    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << size << " " << units[unitIndex];
    
    return ss.str();
}

std::string formatTime(double seconds) {
    int hours = static_cast<int>(seconds) / 3600;
    int minutes = (static_cast<int>(seconds) % 3600) / 60;
    int secs = static_cast<int>(seconds) % 60;
    
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << hours << ":" 
       << std::setfill('0') << std::setw(2) << minutes << ":" 
       << std::setfill('0') << std::setw(2) << secs;
    
    return ss.str();
}

bool extractInfoFromFileName(const std::string& fileName, 
                           std::string& dateTime, 
                           std::string& resolution, 
                           int& framerate) {
    // 使用正则表达式解析文件名
    // 格式：日期时间_分辨率_帧率.mp4
    // 例如：20230101_120000_1920x1080_30fps.mp4
    std::regex pattern(R"((\d{8}_\d{6})_(\d+x\d+)_(\d+)fps\..+)");
    std::smatch matches;
    
    if (std::regex_match(fileName, matches, pattern) && matches.size() == 4) {
        dateTime = matches[1].str();
        resolution = matches[2].str();
        framerate = std::stoi(matches[3].str());
        return true;
    }
    
    return false;
}

void matToTexture(const cv::Mat& mat, unsigned char* textureData, int width, int height) {
    // 确保Mat是RGB格式
    cv::Mat rgbMat;
    if (mat.channels() == 3) {
        if (mat.type() == CV_8UC3) {
            // BGR转RGB
            cv::cvtColor(mat, rgbMat, cv::COLOR_BGR2RGB);
        } else {
            rgbMat = mat.clone();
        }
    } else if (mat.channels() == 1) {
        // 灰度转RGB
        cv::cvtColor(mat, rgbMat, cv::COLOR_GRAY2RGB);
    } else {
        // 不支持的格式
        return;
    }
    
    // 调整大小
    cv::Mat resizedMat;
    if (rgbMat.cols != width || rgbMat.rows != height) {
        cv::resize(rgbMat, resizedMat, cv::Size(width, height));
    } else {
        resizedMat = rgbMat;
    }
    
    // 复制数据
    memcpy(textureData, resizedMat.data, width * height * 3);
}

bool ensureDirectoryExists(const std::string& path) {
    try {
        // 检查目录是否存在
        if (fs::exists(path)) {
            if (fs::is_directory(path)) {
                return true;  // 目录已存在
            } else {
                std::cerr << "路径存在但不是目录: " << path << std::endl;
                return false;
            }
        }
        
        // 创建目录
        return fs::create_directories(path);
    } catch (const std::exception& e) {
        std::cerr << "创建目录时出错: " << e.what() << std::endl;
        return false;
    }
}

std::string getFileExtension(const std::string& filePath) {
    fs::path path(filePath);
    return path.extension().string();
}

bool isVideoFile(const std::string& filePath) {
    std::string ext = getFileExtension(filePath);
    
    // 转换为小写
    std::transform(ext.begin(), ext.end(), ext.begin(), 
                  [](unsigned char c) { return std::tolower(c); });
    
    // 检查是否为视频扩展名
    static const std::vector<std::string> videoExtensions = {
        ".mp4", ".avi", ".mkv", ".mov", ".wmv", ".flv", ".webm"
    };
    
    return std::find(videoExtensions.begin(), videoExtensions.end(), ext) != videoExtensions.end();
}

bool getVideoFileInfo(const std::string& filePath, 
                     double& duration, 
                     int& width, 
                     int& height, 
                     int& framerate) {
    try {
        // 打开视频文件
        cv::VideoCapture cap(filePath);
        
        if (!cap.isOpened()) {
            return false;
        }
        
        // 获取视频信息
        width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
        height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
        framerate = static_cast<int>(cap.get(cv::CAP_PROP_FPS));
        int frameCount = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));
        
        // 计算时长
        if (framerate > 0) {
            duration = static_cast<double>(frameCount) / framerate;
        } else {
            duration = 0.0;
        }
        
        // 关闭视频
        cap.release();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "获取视频信息时出错: " << e.what() << std::endl;
        return false;
    }
}

} // namespace Utils
