#ifndef TEST_STREAM_H__
#define TEST_STREAM_H__

#include <memory>
#include "thread_proxy.h"
#include "../src/protocol_rtsp_pull.h"
#include "../src/protocol_rtsp_push.h"

class test_stream{
public:
    virtual bool start();
    virtual void stop();

protected:
    thread_proxy_ptr mp_proxy;
    protocol_rtsp_pull_ptr mp_protocol_pull;
    protocol_rtsp_push_ptr mp_protocol_push;
};
typedef std::shared_ptr<test_stream> test_stream_ptr;






#endif // TEST_STREAM_H__