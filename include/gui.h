#pragma once

#include "camera_device.h"
#include "video_capture.h"
#include "video_recorder.h"
#include "file_manager.h"
#include "frame_extractor.h"

#include <imgui.h>
#include <vector>
#include <string>
#include <memory>

// OpenGL和GLFW头文件
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <GL/glext.h>

// GUI类
class GUI {
public:
    GUI();
    ~GUI();

    // 初始化GUI
    bool init(int width, int height, const std::string& title);

    // 运行GUI主循环
    void run();

    // 关闭GUI
    void shutdown();

    // 设置摄像头设备列表
    void setCameraDevices(const std::vector<CameraDeviceInfo>& devices);

    // 设置视频文件列表
    void setVideoFiles(const std::vector<VideoFileInfo>& files);

    // 更新预览帧
    void updatePreviewFrame(const cv::Mat& frame);

private:
    // 窗口尺寸
    int m_width;
    int m_height;
    std::string m_title;

    // GLFW窗口和上下文
    GLFWwindow* m_window;

    // 模块引用
    std::shared_ptr<CameraDevice> m_cameraDevice;
    std::shared_ptr<VideoCapture> m_videoCapture;
    std::shared_ptr<VideoRecorder> m_videoRecorder;
    std::shared_ptr<FileManager> m_fileManager;
    std::shared_ptr<FrameExtractor> m_frameExtractor;

    // 数据
    std::vector<CameraDeviceInfo> m_cameraDevices;
    std::vector<VideoFileInfo> m_videoFiles;
    int m_selectedDeviceIndex;
    int m_selectedFileIndex;
    int m_selectedResolutionIndex;
    int m_selectedFramerateIndex;

    // 预览帧
    GLuint m_previewTextureId;
    cv::Mat m_previewFrame;
    bool m_hasNewFrame;

    // 初始化IMGUI
    bool initImGui();

    // 渲染GUI
    void renderGUI();

    // 渲染设备列表面板
    void renderDeviceListPanel();

    // 渲染预览面板
    void renderPreviewPanel();

    // 渲染录制控制面板
    void renderRecordControlPanel();

    // 渲染文件列表面板
    void renderFileListPanel();

    // 渲染分帧控制面板
    void renderFrameExtractionPanel();

    // 更新预览纹理
    void updatePreviewTexture();

    // 清理资源
    void cleanup();
};
