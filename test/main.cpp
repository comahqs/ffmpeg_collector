#include <iostream>
#include <unistd.h>
#include "../src/utility_tool.h"


#include "../src/control.h"

int main()
{
    LOG_INFO("");
    LOG_INFO("");
    LOG_INFO("程序开始运行");
    control stream("rtmp://192.168.0.210:1935/live", "rtmp://192.168.0.210:1935/hls");
    //control stream("/home/com/pq.mp4", "./wwd.mp4");
    stream.start();
    stream.wait();
    //stream.stop();

    LOG_INFO("程序结束运行");
    return 0;
}