#include "test_stream.h"



bool test_stream::start(){
    mp_protocol_pull = std::make_shared<protocol_rtsp_pull>("rtmp://192.168.0.210:1935/live");
    mp_protocol_push = std::make_shared<protocol_rtsp_push>("rtmp://192.168.0.210:1935/hls");
    //mp_protocol_pull = std::make_shared<protocol_rtsp_pull>("/home/com/pq.mp4");
    //mp_protocol_push = std::make_shared<protocol_rtsp_push>("./www.mp4");
    mp_protocol_pull->add_stream(mp_protocol_push);
    mp_protocol_push->start();
    mp_protocol_pull->start();

    return true;
}

void test_stream::stop(){
    mp_protocol_pull->stop();
    mp_protocol_push->stop();
}