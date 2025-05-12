#include "gui.h"
#include "utils.h"
#include <iostream>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <opencv2/imgproc.hpp>

GUI::GUI()
    : m_width(0),
      m_height(0),
      m_window(nullptr),
      m_previewTextureId(0),
      m_selectedDeviceIndex(-1),
      m_selectedFileIndex(-1),
      m_selectedResolutionIndex(0),
      m_selectedFramerateIndex(0),
      m_hasNewFrame(false),
      m_useFFmpeg(true) {  // 默认使用FFmpeg录制

    // 创建模块实例
    m_cameraDevice = std::make_shared<CameraDevice>();
    m_videoCapture = std::make_shared<VideoCapture>();
    m_videoRecorder = std::make_shared<VideoRecorder>();
    m_ffmpegRecorder = std::make_shared<FFmpegRecorder>();
    m_fileManager = std::make_shared<FileManager>();
    m_frameExtractor = std::make_shared<FrameExtractor>();
}

GUI::~GUI() {
    shutdown();
}

bool GUI::init(int width, int height, const std::string& title) {
    m_width = width;
    m_height = height;
    m_title = title;

    // 初始化GLFW
    if (!glfwInit()) {
        std::cerr << "无法初始化GLFW" << std::endl;
        return false;
    }

    // 创建窗口
    // 设置GLFW窗口提示，强制使用OpenGL ES 3.1
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    // 添加更多的窗口提示以提高兼容性
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
    if (!m_window) {
        std::cerr << "无法创建GLFW窗口" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);  // 启用垂直同步

    // 初始化ImGui
    if (!initImGui()) {
        std::cerr << "无法初始化ImGui" << std::endl;
        glfwDestroyWindow(m_window);
        glfwTerminate();
        return false;
    }

    // 创建预览纹理
    glGenTextures(1, &m_previewTextureId);
    glBindTexture(GL_TEXTURE_2D, m_previewTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 初始化FFmpeg录制器
    if (!m_ffmpegRecorder->init(m_fileManager->getBaseDir())) {
        std::cerr << "无法初始化FFmpeg录制器" << std::endl;
        return false;
    }

    // 设置视频捕获回调
    m_videoCapture->setFrameCallback([this](const cv::Mat& frame) {
        updatePreviewFrame(frame);

        // 如果正在录制，处理帧
        if (!m_useFFmpeg && m_videoRecorder->isRecording()) {
            m_videoRecorder->processFrame(frame);
        }
    });

    return true;
}

void GUI::run() {
    // 主循环
    while (!glfwWindowShouldClose(m_window)) {
        // 轮询事件
        glfwPollEvents();

        // 开始ImGui帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 渲染GUI
        renderGUI();

        // 渲染ImGui
        ImGui::Render();

        // 获取帧缓冲大小
        int displayWidth, displayHeight;
        glfwGetFramebufferSize(m_window, &displayWidth, &displayHeight);

        // 设置视口
        glViewport(0, 0, displayWidth, displayHeight);

        // 清除屏幕
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 渲染ImGui绘制数据
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // 交换缓冲区
        glfwSwapBuffers(m_window);
    }
}

void GUI::shutdown() {
    // 停止视频捕获
    if (m_videoCapture) {
        m_videoCapture->stop();
    }

    // 停止视频录制
    if (m_videoRecorder) {
        m_videoRecorder->stopRecording();
    }

    // 停止FFmpeg录制
    if (m_ffmpegRecorder) {
        m_ffmpegRecorder->stopRecording();
    }

    // 停止分帧
    if (m_frameExtractor && m_frameExtractor->isExtracting()) {
        std::cout << "停止分帧进程..." << std::endl;
        m_frameExtractor->stopExtraction();
    }

    // 删除预览纹理
    if (m_previewTextureId) {
        glDeleteTextures(1, &m_previewTextureId);
        m_previewTextureId = 0;
    }

    // 清理ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // 清理GLFW
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }

    glfwTerminate();
}

void GUI::setCameraDevices(const std::vector<CameraDeviceInfo>& devices) {
    m_cameraDevices = devices;

    // 如果有设备，默认选择第一个
    if (!m_cameraDevices.empty() && m_selectedDeviceIndex < 0) {
        m_selectedDeviceIndex = 0;
    }
}

void GUI::setVideoFiles(const std::vector<VideoFileInfo>& files) {
    m_videoFiles = files;
}

void GUI::updatePreviewFrame(const cv::Mat& frame) {
    if (frame.empty()) {
        return;
    }

    // 转换为RGB
    cv::Mat rgbFrame;
    cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);

    // 保存预览帧
    {
        m_previewFrame = rgbFrame.clone();
        m_hasNewFrame = true;
    }
}

bool GUI::initImGui() {
    // 创建ImGui上下文
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // 设置ImGui IO
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // 启用键盘控制

    // 设置ImGui样式
    ImGui::StyleColorsDark();

    // 初始化ImGui后端
    if (!ImGui_ImplGlfw_InitForOpenGL(m_window, true)) {
        return false;
    }

    // 使用OpenGL ES 3.1
    if (!ImGui_ImplOpenGL3_Init("#version 310 es")) {
        return false;
    }

    return true;
}

void GUI::renderGUI() {
    // 设置窗口大小和位置
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(m_width, m_height));

    // 创建主窗口
    ImGui::Begin("摄像头采集软件", nullptr,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_MenuBar);

    // 菜单栏
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("文件")) {
            if (ImGui::MenuItem("刷新设备列表")) {
                // 扫描摄像头设备
                std::vector<CameraDeviceInfo> devices = m_cameraDevice->scanDevices();
                setCameraDevices(devices);
            }

            if (ImGui::MenuItem("刷新文件列表")) {
                // 扫描视频文件
                std::vector<VideoFileInfo> videoFiles = m_fileManager->getVideoFileList();
                setVideoFiles(videoFiles);
            }

            if (ImGui::MenuItem("退出")) {
                glfwSetWindowShouldClose(m_window, GLFW_TRUE);
            }

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    // 创建左右分栏
    ImGui::Columns(2, "MainColumns", true);

    // 左侧面板
    {
        // 设备列表面板
        renderDeviceListPanel();

        // 文件列表面板
        renderFileListPanel();

        // 分帧控制面板
        renderFrameExtractionPanel();
    }

    ImGui::NextColumn();

    // 右侧面板
    {
        // 预览面板
        renderPreviewPanel();

        // 录制控制面板
        renderRecordControlPanel();
    }

    ImGui::Columns(1);

    ImGui::End();

    // 更新预览纹理
    if (m_hasNewFrame) {
        updatePreviewTexture();
        m_hasNewFrame = false;
    }
}

void GUI::renderDeviceListPanel() {
    if (ImGui::CollapsingHeader("设备列表", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BeginChild("DeviceListChild", ImVec2(0, 150), true);

        for (int i = 0; i < m_cameraDevices.size(); i++) {
            const auto& device = m_cameraDevices[i];

            // 设备名称和路径
            std::string label = device.deviceName + " (" + device.devicePath + ")";

            if (ImGui::Selectable(label.c_str(), m_selectedDeviceIndex == i)) {
                // 选择设备
                m_selectedDeviceIndex = i;

                // 打开设备
                if (m_cameraDevice->openDevice(device.devicePath)) {
                    // 获取支持的分辨率
                    auto resolutions = m_cameraDevice->getSupportedResolutions();

                    // 如果有分辨率，默认选择第一个
                    if (!resolutions.empty()) {
                        m_selectedResolutionIndex = 0;

                        // 获取支持的帧率
                        auto framerates = m_cameraDevice->getSupportedFramerates(resolutions[0]);

                        // 如果有帧率，默认选择第一个
                        if (!framerates.empty()) {
                            m_selectedFramerateIndex = 0;
                        }
                    }
                }
            }
        }

        ImGui::EndChild();
    }
}

void GUI::renderPreviewPanel() {
    if (ImGui::CollapsingHeader("预览", ImGuiTreeNodeFlags_DefaultOpen)) {
        // 预览控制按钮
        if (m_selectedDeviceIndex >= 0) {
            if (!m_videoCapture->isCapturing()) {
                if (ImGui::Button("开始预览")) {
                    // 获取选中的设备
                    const auto& device = m_cameraDevices[m_selectedDeviceIndex];

                    // 获取选中的分辨率
                    auto resolutions = m_cameraDevice->getSupportedResolutions();
                    if (!resolutions.empty() && m_selectedResolutionIndex < resolutions.size()) {
                        auto resolution = resolutions[m_selectedResolutionIndex];

                        // 获取选中的帧率
                        auto framerates = m_cameraDevice->getSupportedFramerates(resolution);
                        if (!framerates.empty() && m_selectedFramerateIndex < framerates.size()) {
                            int framerate = framerates[m_selectedFramerateIndex];

                            // 初始化视频捕获
                            if (m_videoCapture->init(*m_cameraDevice, resolution, framerate)) {
                                // 开始捕获
                                m_videoCapture->start();
                            }
                        }
                    }
                }
            } else {
                if (ImGui::Button("停止预览")) {
                    // 停止捕获
                    m_videoCapture->stop();
                }
            }
        }

        // 预览窗口
        ImGui::BeginChild("PreviewChild", ImVec2(0, 400), true);

        if (m_previewTextureId) {
            // 计算预览窗口大小
            ImVec2 windowSize = ImGui::GetContentRegionAvail();

            // 计算纹理尺寸
            float textureWidth = m_previewFrame.cols;
            float textureHeight = m_previewFrame.rows;

            // 计算缩放比例
            float scale = std::min(windowSize.x / textureWidth, windowSize.y / textureHeight);

            // 计算显示尺寸
            float displayWidth = textureWidth * scale;
            float displayHeight = textureHeight * scale;

            // 计算居中位置
            float posX = (windowSize.x - displayWidth) * 0.5f;
            float posY = (windowSize.y - displayHeight) * 0.5f;

            // 显示预览
            ImGui::SetCursorPos(ImVec2(posX, posY));
            ImGui::Image((void*)(intptr_t)m_previewTextureId,
                        ImVec2(displayWidth, displayHeight),
                        ImVec2(0, 0), ImVec2(1, 1));
        } else {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "无预览");
        }

        ImGui::EndChild();
    }
}

void GUI::renderRecordControlPanel() {
    if (ImGui::CollapsingHeader("录制控制", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BeginChild("RecordControlChild", ImVec2(0, 150), true);

        // 分辨率选择
        if (m_selectedDeviceIndex >= 0) {
            auto resolutions = m_cameraDevice->getSupportedResolutions();

            if (!resolutions.empty()) {
                // 创建分辨率选项
                std::vector<const char*> resolutionItems;
                for (const auto& res : resolutions) {
                    resolutionItems.push_back(res.toString().c_str());
                }

                // 分辨率下拉框
                if (ImGui::Combo("分辨率", &m_selectedResolutionIndex,
                                resolutionItems.data(), resolutionItems.size())) {
                    // 更新帧率列表
                    auto framerates = m_cameraDevice->getSupportedFramerates(
                                    resolutions[m_selectedResolutionIndex]);

                    // 如果有帧率，默认选择第一个
                    if (!framerates.empty()) {
                        m_selectedFramerateIndex = 0;
                    }
                }

                // 帧率选择
                auto framerates = m_cameraDevice->getSupportedFramerates(
                                resolutions[m_selectedResolutionIndex]);

                if (!framerates.empty()) {
                    // 创建帧率选项
                    std::vector<std::string> framerateStrings;
                    std::vector<const char*> framerateItems;

                    for (int fps : framerates) {
                        framerateStrings.push_back(std::to_string(fps) + " fps");
                        framerateItems.push_back(framerateStrings.back().c_str());
                    }

                    // 帧率下拉框
                    ImGui::Combo("帧率", &m_selectedFramerateIndex,
                                framerateItems.data(), framerateItems.size());
                }
            }
        }

        // 录制模式选择
        ImGui::Checkbox("使用FFmpeg录制", &m_useFFmpeg);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("使用FFmpeg录制可以获得更好的视频质量和兼容性");
        }

        // 录制控制按钮
        if (m_videoCapture->isCapturing()) {
            bool isRecording = m_useFFmpeg ? m_ffmpegRecorder->isRecording() : m_videoRecorder->isRecording();

            if (!isRecording) {
                if (ImGui::Button("开始录像")) {
                    // 获取当前分辨率和帧率
                    Resolution resolution = m_videoCapture->getCurrentResolution();
                    int framerate = m_videoCapture->getCurrentFramerate();

                    if (m_useFFmpeg) {
                        // 使用FFmpeg录制
                        std::string devicePath = m_cameraDevice->getCurrentDeviceInfo().devicePath;
                        m_ffmpegRecorder->startRecording(devicePath, resolution, framerate);
                    } else {
                        // 使用OpenCV录制
                        m_videoRecorder->startRecording(resolution, framerate);
                    }
                }
            } else {
                // 显示录制时长
                double duration = m_useFFmpeg ?
                                 m_ffmpegRecorder->getRecordingDuration() :
                                 m_videoRecorder->getRecordingDuration();
                ImGui::Text("录制时长: %s", Utils::formatTime(duration).c_str());

                if (ImGui::Button("停止录像")) {
                    // 停止录制
                    if (m_useFFmpeg) {
                        m_ffmpegRecorder->stopRecording();
                    } else {
                        m_videoRecorder->stopRecording();
                    }

                    // 刷新文件列表
                    std::vector<VideoFileInfo> videoFiles = m_fileManager->getVideoFileList();
                    setVideoFiles(videoFiles);
                }
            }
        } else {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "请先开始预览");
        }

        ImGui::EndChild();
    }
}

void GUI::renderFileListPanel() {
    if (ImGui::CollapsingHeader("文件列表", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BeginChild("FileListChild", ImVec2(0, 200), true);

        for (int i = 0; i < m_videoFiles.size(); i++) {
            const auto& file = m_videoFiles[i];

            // 文件信息
            std::string label = file.fileName + "\n" +
                              "大小: " + Utils::formatFileSize(file.fileSize) + ", " +
                              "时长: " + Utils::formatTime(file.duration);

            if (ImGui::Selectable(label.c_str(), m_selectedFileIndex == i)) {
                m_selectedFileIndex = i;
            }

            // 右键菜单
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("删除")) {
                    // 删除文件
                    if (m_fileManager->deleteVideoFile(file.filePath)) {
                        // 刷新文件列表
                        std::vector<VideoFileInfo> videoFiles = m_fileManager->getVideoFileList();
                        setVideoFiles(videoFiles);

                        // 重置选择
                        if (m_selectedFileIndex >= m_videoFiles.size()) {
                            m_selectedFileIndex = -1;
                        }
                    }
                }

                ImGui::EndPopup();
            }
        }

        ImGui::EndChild();
    }
}

void GUI::renderFrameExtractionPanel() {
    if (ImGui::CollapsingHeader("视频分帧", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BeginChild("FrameExtractionChild", ImVec2(0, 100), true);

        if (m_selectedFileIndex >= 0 && m_selectedFileIndex < m_videoFiles.size()) {
            const auto& file = m_videoFiles[m_selectedFileIndex];

            ImGui::Text("选中文件: %s", file.fileName.c_str());

            if (!m_frameExtractor->isExtracting()) {
                if (ImGui::Button("开始分帧")) {
                    // 创建一个新线程来执行分帧，避免阻塞GUI
                    std::thread([this, filePath = file.filePath]() {
                        // 设置进度回调
                        m_frameExtractor->setProgressCallback([](float progress) {
                            // 进度回调在另一个线程中，不能直接更新UI
                        });

                        // 设置完成回调
                        m_frameExtractor->setCompletionCallback([](const std::string& outputDir) {
                            std::cout << "分帧完成，帧保存至: " << outputDir << std::endl;
                        });

                        // 开始分帧
                        m_frameExtractor->startExtraction(filePath);
                    }).detach();  // 分离线程，让它在后台运行
                }
            } else {
                // 显示进度
                float progress = m_frameExtractor->getProgress();
                ImGui::ProgressBar(progress, ImVec2(-1, 0),
                                 (std::to_string(static_cast<int>(progress * 100)) + "%").c_str());

                if (ImGui::Button("停止分帧")) {
                    // 停止分帧
                    m_frameExtractor->stopExtraction();
                }
            }
        } else {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "请先选择一个视频文件");
        }

        ImGui::EndChild();
    }
}

void GUI::updatePreviewTexture() {
    if (m_previewFrame.empty()) {
        return;
    }

    // 绑定纹理
    glBindTexture(GL_TEXTURE_2D, m_previewTextureId);

    // 上传纹理数据
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                m_previewFrame.cols, m_previewFrame.rows, 0,
                GL_RGB, GL_UNSIGNED_BYTE, m_previewFrame.data);
}
