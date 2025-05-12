#include "camera_device.h"
#include "video_capture.h"
#include "video_recorder.h"
#include "ffmpeg_recorder.h"
#include "file_manager.h"
#include "frame_extractor.h"
#include "gui.h"
#include "utils.h"

#include <iostream>
#include <memory>
#include <string>
#include <filesystem>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

// 显示帮助信息
void showHelp(const char* programName) {
    std::cout << "用法: " << programName << " [选项]" << std::endl;
    std::cout << "选项:" << std::endl;
    std::cout << "  --help, -h       显示此帮助信息" << std::endl;
    std::cout << "  --gui            使用图形界面模式（默认）" << std::endl;
    std::cout << "  --cli            使用命令行界面模式" << std::endl;
    std::cout << "命令行模式下的子命令:" << std::endl;
    std::cout << "  list-devices     列出可用的摄像头设备" << std::endl;
    std::cout << "  list-files       列出已录制的视频文件" << std::endl;
    std::cout << "  record           开始录制视频" << std::endl;
    std::cout << "    --device=N     使用设备索引N（默认为0）" << std::endl;
    std::cout << "    --width=W      设置宽度为W（默认为640）" << std::endl;
    std::cout << "    --height=H     设置高度为H（默认为480）" << std::endl;
    std::cout << "    --fps=F        设置帧率为F（默认为30）" << std::endl;
    std::cout << "    --time=T       录制T秒后停止（默认为10）" << std::endl;
    std::cout << "  extract          从视频文件中提取帧" << std::endl;
    std::cout << "    --file=PATH    指定视频文件路径" << std::endl;
}

// 解析命令行参数
std::vector<std::string> parseArgs(int argc, char** argv) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }
    return args;
}

// 检查参数是否存在
bool hasArg(const std::vector<std::string>& args, const std::string& arg) {
    return std::find(args.begin(), args.end(), arg) != args.end();
}

// 获取参数值
std::string getArgValue(const std::vector<std::string>& args, const std::string& prefix, const std::string& defaultValue = "") {
    for (const auto& arg : args) {
        if (arg.find(prefix) == 0) {
            return arg.substr(prefix.length());
        }
    }
    return defaultValue;
}

int main(int argc, char** argv) {
    // 解析命令行参数
    std::vector<std::string> args = parseArgs(argc, argv);

    // 显示帮助信息
    if (args.empty() || hasArg(args, "--help") || hasArg(args, "-h")) {
        showHelp(argv[0]);
        return 0;
    }

    // 创建应用程序目录
    std::string appDir = fs::path(getenv("HOME")) / "captureVideo";
    if (!Utils::ensureDirectoryExists(appDir)) {
        std::cerr << "无法创建应用程序目录: " << appDir << std::endl;
        return 1;
    }

    // 创建视频输出目录
    std::string videoDir = fs::path(appDir) / "videos";
    if (!Utils::ensureDirectoryExists(videoDir)) {
        std::cerr << "无法创建视频目录: " << videoDir << std::endl;
        return 1;
    }

    // 初始化基本模块
    auto cameraDevice = std::make_shared<CameraDevice>();
    auto fileManager = std::make_shared<FileManager>();

    // 初始化文件管理器
    if (!fileManager->init(videoDir)) {
        std::cerr << "无法初始化文件管理器" << std::endl;
        return 1;
    }

    // 判断运行模式：GUI或CLI
    bool useGui = !hasArg(args, "--cli");

    // 命令行模式
    if (!useGui) {
        std::cout << "运行命令行模式..." << std::endl;

        // 列出设备
        if (hasArg(args, "list-devices")) {
            std::cout << "扫描摄像头设备..." << std::endl;
            std::vector<CameraDeviceInfo> devices = cameraDevice->scanDevices();
            std::cout << "找到 " << devices.size() << " 个摄像头设备:" << std::endl;

            for (size_t i = 0; i < devices.size(); ++i) {
                const auto& device = devices[i];
                std::cout << "[" << i << "] " << device.deviceName << " (" << device.devicePath << ")" << std::endl;

                // 显示支持的分辨率
                std::cout << "  支持的分辨率:" << std::endl;
                for (const auto& res : device.supportedResolutions) {
                    std::cout << "    " << res.toString() << std::endl;
                }
            }
            return 0;
        }

        // 列出文件
        if (hasArg(args, "list-files")) {
            std::cout << "扫描视频文件..." << std::endl;
            std::vector<VideoFileInfo> videoFiles = fileManager->getVideoFileList();
            std::cout << "找到 " << videoFiles.size() << " 个视频文件:" << std::endl;

            for (size_t i = 0; i < videoFiles.size(); ++i) {
                const auto& file = videoFiles[i];
                std::cout << "[" << i << "] " << file.fileName << std::endl;
                std::cout << "  路径: " << file.filePath << std::endl;
                std::cout << "  日期时间: " << file.dateTime << std::endl;
                std::cout << "  分辨率: " << file.resolution << std::endl;
                std::cout << "  帧率: " << file.framerate << " fps" << std::endl;
                std::cout << "  大小: " << Utils::formatFileSize(file.fileSize) << std::endl;
                std::cout << "  时长: " << Utils::formatTime(file.duration) << std::endl;
            }
            return 0;
        }

        // 录制视频
        if (hasArg(args, "record")) {
            // 解析参数
            int deviceIndex = std::stoi(getArgValue(args, "--device=", "0"));
            int width = std::stoi(getArgValue(args, "--width=", "640"));
            int height = std::stoi(getArgValue(args, "--height=", "480"));
            int fps = std::stoi(getArgValue(args, "--fps=", "30"));
            int recordTime = std::stoi(getArgValue(args, "--time=", "10"));

            // 扫描设备
            std::vector<CameraDeviceInfo> devices = cameraDevice->scanDevices();
            if (devices.empty()) {
                std::cerr << "未找到摄像头设备" << std::endl;
                return 1;
            }

            if (deviceIndex < 0 || deviceIndex >= devices.size()) {
                std::cerr << "无效的设备索引: " << deviceIndex << std::endl;
                return 1;
            }

            // 获取设备路径
            std::string devicePath = devices[deviceIndex].devicePath;
            std::cout << "使用设备: " << devices[deviceIndex].deviceName << " (" << devicePath << ")" << std::endl;

            // 初始化FFmpeg录制器
            auto ffmpegRecorder = std::make_shared<FFmpegRecorder>();

            if (!ffmpegRecorder->init(videoDir)) {
                std::cerr << "无法初始化FFmpeg录制器" << std::endl;
                return 1;
            }

            // 设置分辨率和帧率
            Resolution resolution(width, height);
            std::cout << "设置分辨率: " << resolution.toString() << ", 帧率: " << fps << std::endl;

            // 开始录制
            std::cout << "开始录制..." << std::endl;
            if (!ffmpegRecorder->startRecording(devicePath, resolution, fps, recordTime)) {
                std::cerr << "无法开始视频录制" << std::endl;
                return 1;
            }

            // 录制指定时间
            std::cout << "录制 " << recordTime << " 秒..." << std::endl;
            for (int i = 0; i < recordTime; ++i) {
                std::cout << "已录制 " << (i + 1) << "/" << recordTime << " 秒\r" << std::flush;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            std::cout << std::endl;

            // 停止录制
            std::cout << "停止录制..." << std::endl;
            ffmpegRecorder->stopRecording();

            std::cout << "录制完成，文件保存至: " << ffmpegRecorder->getCurrentFilePath() << std::endl;
            return 0;
        }

        // 提取帧
        if (hasArg(args, "extract")) {
            try {
                std::string filePath = getArgValue(args, "--file=");
                if (filePath.empty()) {
                    std::cerr << "请指定视频文件路径，例如: --file=/path/to/video.mp4" << std::endl;
                    return 1;
                }

                // 检查文件是否存在
                if (!fs::exists(filePath)) {
                    std::cerr << "文件不存在: " << filePath << std::endl;
                    return 1;
                }

                // 初始化帧提取器
                auto frameExtractor = std::make_shared<FrameExtractor>();

                // 设置进度回调
                frameExtractor->setProgressCallback([](float progress) {
                    int percent = static_cast<int>(progress * 100);
                    std::cout << "提取进度: " << percent << "%\r" << std::flush;
                });

                // 设置完成回调
                frameExtractor->setCompletionCallback([](const std::string& outputDir) {
                    std::cout << std::endl << "提取完成，帧保存至: " << outputDir << std::endl;
                });

                // 开始提取
                std::cout << "开始从 " << filePath << " 提取帧..." << std::endl;
                if (!frameExtractor->startExtraction(filePath)) {
                    std::cerr << "无法开始帧提取" << std::endl;
                    return 1;
                }

                // 不需要等待，因为startExtraction现在是同步的

                // 清理资源
                std::cout << "清理资源..." << std::endl;
                frameExtractor = nullptr;
                std::cout << "帧提取完成" << std::endl;

                return 0;
            } catch (const std::exception& e) {
                std::cerr << "帧提取过程中发生异常: " << e.what() << std::endl;
                return 1;
            } catch (...) {
                std::cerr << "帧提取过程中发生未知异常" << std::endl;
                return 1;
            }
        }

        // 未知命令
        std::cerr << "未知的命令，请使用 --help 查看帮助" << std::endl;
        return 1;
    }

    // GUI模式
    else {
        // 检查环境变量
        const char* display = getenv("DISPLAY");
        if (!display) {
            std::cerr << "错误: 未设置DISPLAY环境变量，无法初始化图形界面" << std::endl;
            std::cerr << "请在X环境下运行此程序，或使用 --cli 参数运行命令行模式" << std::endl;
            return 1;
        }

        std::cout << "运行图形界面模式..." << std::endl;
        std::cout << "DISPLAY环境变量: " << display << std::endl;
        std::cout << "应用程序目录: " << appDir << std::endl;
        std::cout << "视频输出目录: " << videoDir << std::endl;

        try {
            // 初始化其他模块
            std::cout << "初始化模块..." << std::endl;
            auto videoCapture = std::make_shared<VideoCapture>();
            auto videoRecorder = std::make_shared<VideoRecorder>();
            auto frameExtractor = std::make_shared<FrameExtractor>();

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
    }

    std::cout << "程序正常退出" << std::endl;
    return 0;
}
