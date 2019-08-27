#include <iostream>
#include <unistd.h>
#include "../src/utility_tool.h"

#include "../src/control.h"
#include "../src/control_complex.h"

extern "C"
{
 #include <libavutil/log.h>
}
    

int main()
{
    LOG_INFO("");
    LOG_INFO("");
    LOG_INFO("程序开始运行");

    //av_log_set_level(AV_LOG_DEBUG);
#if 1
    {
        std::vector<std::string> urls;
        
        urls.push_back("rtmp://192.168.0.210:1935/live");
        urls.push_back("rtmp://192.168.0.210:1935/live");
        urls.push_back("rtmp://192.168.0.210:1935/live");
        urls.push_back("rtmp://192.168.0.210:1935/live");
       
        //urls.push_back("/home/com/pq.mp4");
        //urls.push_back("/home/com/pq.mp4");
        //urls.push_back("/home/com/pq.mp4");
        //urls.push_back("/home/com/pq.mp4");
        
        control_complex stream(urls, "rtmp://192.168.0.210:1935/hls");
        stream.start();
        stream.wait();
    }
#else
    {
        control stream("rtmp://192.168.0.210:1935/live", "rtmp://192.168.0.210:1935/hls");
        //control stream("/home/com/pq.mp4", "./wwd.mp4");
        stream.start();
        stream.wait();
    }
#endif


    //stream.stop();

    LOG_INFO("程序结束运行");
    return 0;
}