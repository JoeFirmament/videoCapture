#include "camera_device.h"
#include "video_capture.h"
#include "video_recorder.h"
#include "file_manager.h"
#include "frame_extractor.h"
#include "gui.h"
#include "utils.h"

#include <iostream>
#include <memory>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

int main(int argc, char** argv) {
    // 检查环境变量
    const char* display = getenv("DISPLAY");
    if (!display) {
        std::cerr << "错误: 未设置DISPLAY环境变量，无法初始化图形界面" << std::endl;
        std::cerr << "请在X环境下运行此程序" << std::endl;
        return 1;
    }

    std::cout << "DISPLAY环境变量: " << display << std::endl;

    // 创建应用程序目录
    std::string appDir = fs::path(getenv("HOME")) / "captureVideo";
    if (!Utils::ensureDirectoryExists(appDir)) {
        std::cerr << "无法创建应用程序目录: " << appDir << std::endl;
        return 1;
    }

    std::cout << "应用程序目录: " << appDir << std::endl;

    // 创建视频输出目录
    std::string videoDir = fs::path(appDir) / "videos";
    if (!Utils::ensureDirectoryExists(videoDir)) {
        std::cerr << "无法创建视频目录: " << videoDir << std::endl;
        return 1;
    }

    std::cout << "视频输出目录: " << videoDir << std::endl;

    try {
        // 初始化模块
        std::cout << "初始化模块..." << std::endl;
        auto cameraDevice = std::make_shared<CameraDevice>();
        auto videoCapture = std::make_shared<VideoCapture>();
        auto videoRecorder = std::make_shared<VideoRecorder>();
        auto fileManager = std::make_shared<FileManager>();
        auto frameExtractor = std::make_shared<FrameExtractor>();

        // 初始化文件管理器
        std::cout << "初始化文件管理器..." << std::endl;
        if (!fileManager->init(videoDir)) {
            std::cerr << "无法初始化文件管理器" << std::endl;
            return 1;
        }

        // 初始化录制器
        std::cout << "初始化视频录制器..." << std::endl;
        if (!videoRecorder->init(videoDir)) {
            std::cerr << "无法初始化视频录制器" << std::endl;
            return 1;
        }

        // 扫描摄像头设备
        std::cout << "扫描摄像头设备..." << std::endl;
        std::vector<CameraDeviceInfo> devices = cameraDevice->scanDevices();
        std::cout << "找到 " << devices.size() << " 个摄像头设备" << std::endl;

        // 扫描视频文件
        std::cout << "扫描视频文件..." << std::endl;
        std::vector<VideoFileInfo> videoFiles = fileManager->getVideoFileList();
        std::cout << "找到 " << videoFiles.size() << " 个视频文件" << std::endl;

        // 初始化GUI
        std::cout << "初始化GUI..." << std::endl;
        GUI gui;
        if (!gui.init(1280, 720, "摄像头采集软件")) {
            std::cerr << "无法初始化GUI" << std::endl;
            return 1;
        }

        gui.setCameraDevices(devices);
        gui.setVideoFiles(videoFiles);

        // 运行GUI主循环
        std::cout << "运行GUI主循环..." << std::endl;
        gui.run();

        // 关闭GUI
        std::cout << "关闭GUI..." << std::endl;
        gui.shutdown();

    } catch (const std::exception& e) {
        std::cerr << "发生异常: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "发生未知异常" << std::endl;
        return 1;
    }

    std::cout << "程序正常退出" << std::endl;
    return 0;
}
