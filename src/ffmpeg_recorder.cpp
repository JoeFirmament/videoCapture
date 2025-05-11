#include "ffmpeg_recorder.h"
#include "utils.h"
#include <iostream>
#include <filesystem>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;

FFmpegRecorder::FFmpegRecorder() : m_isRecording(false), m_ffmpegPid(-1) {
}

FFmpegRecorder::~FFmpegRecorder() {
    stopRecording();
}

bool FFmpegRecorder::init(const std::string& outputDir) {
    m_outputDir = outputDir;
    
    // 确保输出目录存在
    if (!Utils::ensureDirectoryExists(m_outputDir)) {
        std::cerr << "无法创建输出目录: " << m_outputDir << std::endl;
        return false;
    }
    
    return true;
}

bool FFmpegRecorder::startRecording(const std::string& devicePath, const Resolution& resolution, int framerate) {
    if (m_isRecording) {
        return true;  // 已经在录制中
    }
    
    // 生成文件名
    m_currentFilePath = generateFileName(resolution, framerate);
    
    // 记录开始时间
    m_startTime = std::chrono::steady_clock::now();
    
    // 设置录制标志
    m_isRecording = true;
    
    // 启动录制线程
    m_recordingThread = std::thread(&FFmpegRecorder::recordingThreadFunc, this, devicePath, resolution, framerate);
    
    return true;
}

void FFmpegRecorder::stopRecording() {
    if (!m_isRecording) {
        return;  // 没有在录制
    }
    
    // 清除录制标志
    m_isRecording = false;
    
    // 终止FFmpeg进程
    terminateFFmpegProcess();
    
    // 等待录制线程结束
    if (m_recordingThread.joinable()) {
        m_recordingThread.join();
    }
}

double FFmpegRecorder::getRecordingDuration() const {
    if (!m_isRecording) {
        return 0.0;
    }
    
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(now - m_startTime).count();
}

void FFmpegRecorder::recordingThreadFunc(const std::string& devicePath, const Resolution& resolution, int framerate) {
    // 构建FFmpeg命令
    std::string command = buildFFmpegCommand(devicePath, resolution, framerate, m_currentFilePath);
    
    // 执行FFmpeg命令
    std::cout << "执行FFmpeg命令: " << command << std::endl;
    m_ffmpegPid = executeCommand(command);
    
    if (m_ffmpegPid <= 0) {
        std::cerr << "无法启动FFmpeg进程" << std::endl;
        m_isRecording = false;
        return;
    }
    
    std::cout << "FFmpeg进程已启动，PID: " << m_ffmpegPid << std::endl;
    
    // 等待FFmpeg进程结束或录制被停止
    int status;
    pid_t result;
    
    do {
        result = waitpid(m_ffmpegPid, &status, WNOHANG);
        if (result == 0) {
            // 进程仍在运行，检查是否应该停止
            if (!m_isRecording) {
                terminateFFmpegProcess();
                waitpid(m_ffmpegPid, &status, 0);  // 等待进程结束
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } while (result == 0);
    
    if (result > 0) {
        if (WIFEXITED(status)) {
            std::cout << "FFmpeg进程正常退出，退出码: " << WEXITSTATUS(status) << std::endl;
        } else if (WIFSIGNALED(status)) {
            std::cout << "FFmpeg进程被信号终止，信号: " << WTERMSIG(status) << std::endl;
        }
    } else if (result < 0) {
        std::cerr << "等待FFmpeg进程时出错: " << strerror(errno) << std::endl;
    }
    
    m_ffmpegPid = -1;
    m_isRecording = false;
}

std::string FFmpegRecorder::generateFileName(const Resolution& resolution, int framerate) {
    // 获取当前日期时间
    std::string dateTime = Utils::getCurrentDateTimeString();
    
    // 生成文件名：日期时间_分辨率_帧率.mp4
    std::string fileName = dateTime + "_" + 
                          std::to_string(resolution.width) + "x" + std::to_string(resolution.height) + 
                          "_" + std::to_string(framerate) + "fps.mp4";
    
    // 完整路径
    return fs::path(m_outputDir) / fileName;
}

std::string FFmpegRecorder::buildFFmpegCommand(const std::string& devicePath, const Resolution& resolution, int framerate, const std::string& outputPath) {
    // 构建FFmpeg命令
    std::string command = "ffmpeg -y";  // 覆盖输出文件
    
    // 输入设备
    command += " -f v4l2";  // 使用V4L2
    command += " -input_format mjpeg";  // 使用MJPEG格式（如果摄像头支持）
    command += " -video_size " + std::to_string(resolution.width) + "x" + std::to_string(resolution.height);
    command += " -framerate " + std::to_string(framerate);
    command += " -i " + devicePath;
    
    // 输出选项
    command += " -c:v libx264";  // 使用H.264编码
    command += " -preset ultrafast";  // 使用最快的编码预设
    command += " -tune zerolatency";  // 优化低延迟
    command += " -pix_fmt yuv420p";  // 使用YUV420P像素格式
    command += " -r " + std::to_string(framerate);  // 设置输出帧率
    command += " -b:v 2000k";  // 设置视频比特率
    
    // 输出文件
    command += " \"" + outputPath + "\"";
    
    // 添加后台运行和错误重定向
    command += " 2>/dev/null &";
    
    return command;
}

int FFmpegRecorder::executeCommand(const std::string& command) {
    // 执行命令并获取进程ID
    FILE* pipe = popen((command + " echo $!").c_str(), "r");
    if (!pipe) {
        return -1;
    }
    
    // 读取进程ID
    char buffer[128];
    std::string result = "";
    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != NULL) {
            result += buffer;
        }
    }
    
    // 关闭管道
    pclose(pipe);
    
    // 解析进程ID
    try {
        return std::stoi(result);
    } catch (...) {
        return -1;
    }
}

void FFmpegRecorder::terminateFFmpegProcess() {
    if (m_ffmpegPid > 0) {
        std::cout << "终止FFmpeg进程，PID: " << m_ffmpegPid << std::endl;
        kill(m_ffmpegPid, SIGTERM);
    }
}
