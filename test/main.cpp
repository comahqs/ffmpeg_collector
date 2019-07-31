#include <iostream>
#include <unistd.h>
#include "../src/utility_tool.h"


#include "test_stream.h"

int main()
{
    LOG_INFO("");
    LOG_INFO("");
    LOG_INFO("程序开始运行");
    {
        test_stream stream;
        stream.start();
        stream.stop();
    }

    LOG_INFO("程序结束运行");
    return 0;
}