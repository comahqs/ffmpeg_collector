cmake_minimum_required(VERSION 2.8)

#在项目中显示所有源码
file(GLOB files_current ./src/*.cpp ./src/*.h)

#编译项目所需源码
set(src_app ${files_current})

#g++编译参数
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -pthread -m64 -std=c++11")

#输出项目名称
project(ffmpeg_collector)

#link_directories("/usr/local/lib")
#include_directories("/usr/local/include" "/usr/local/include/boost")
add_executable(${PROJECT_NAME} ${src_app})
#target_link_libraries (${PROJECT_NAME} boost_system boost_date_time boost_thread boost_filesystem boost_log)