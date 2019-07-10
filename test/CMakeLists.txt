cmake_minimum_required(VERSION 2.8)

#在项目中显示所有源码
file(GLOB files_current ../src/*.cpp ../src/*.h)

#编译项目所需源码
set(src_app ${files_current})

#g++编译参数
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -pthread -m64 -std=c++11")

#输出项目名称
project(test_ffmpeg_collector)

add_executable(${PROJECT_NAME} main.cpp ${src_app})