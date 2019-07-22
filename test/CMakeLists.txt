cmake_minimum_required(VERSION 2.8)

#在项目中显示所有源码
file(GLOB files_current ../src/*.cpp ../src/*.h)

#编译项目所需源码
set(src_app main.cpp ../src/utility_tool.cpp)

#g++编译参数
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -W -pthread -m64 -std=c++11")

#输出项目名称
project(test_ffmpeg_collector)

link_directories("/usr/local/lib" "/usr/local/x264/lib" "/usr/local/ffmpeg/lib")
include_directories("/usr/local/x264/include" "/usr/local/ffmpeg/include")
add_executable(${PROJECT_NAME} ${src_app})
target_link_libraries (${PROJECT_NAME} avcodec avformat avutil)