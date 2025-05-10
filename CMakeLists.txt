cmake_minimum_required(VERSION 3.10)
project(CaptureVideo VERSION 1.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用静态链接
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-as-needed")

# 优化设置
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -flto")

# 查找必要的包
find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBV4L2 REQUIRED libv4l2)

# 查找IMGUI和GLFW
find_path(IMGUI_INCLUDE_DIR imgui.h PATH_SUFFIXES imgui)
find_library(IMGUI_LIBRARY NAMES imgui)
find_package(glfw3 REQUIRED)

# 添加源文件
set(SOURCES
    src/main.cpp
    src/camera_device.cpp
    src/video_capture.cpp
    src/video_recorder.cpp
    src/file_manager.cpp
    src/frame_extractor.cpp
    src/gui.cpp
    src/utils.cpp
)

# 添加头文件目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
    ${LIBV4L2_INCLUDE_DIRS}
    ${IMGUI_INCLUDE_DIR}
)

# 创建可执行文件
add_executable(capture_video ${SOURCES})

# 链接库
target_link_libraries(capture_video
    ${OpenCV_LIBS}
    ${LIBV4L2_LIBRARIES}
    ${IMGUI_LIBRARY}
    glfw
    GL
    dl
    pthread
    stb
)

# 安装目标
install(TARGETS capture_video DESTINATION bin)
