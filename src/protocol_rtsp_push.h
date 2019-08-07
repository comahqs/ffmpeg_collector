#ifndef PROTOCOL_RTSP_PUSH_H
#define PROTOCOL_RTSP_PUSH_H

#include "i_stream.h"
#include <thread>
#include <atomic>
#include <string>



class protocol_rtsp_push : public i_stream{
public:
    protocol_rtsp_push(const std::string& url);

    virtual bool add_stream(std::shared_ptr<i_stream> p_stream);
    virtual bool do_stream(info_av_ptr p_info);
    virtual bool start();
    virtual void stop();
protected:
    std::string m_url;
    info_av_ptr mp_info;
};
typedef std::shared_ptr<protocol_rtsp_push> protocol_rtsp_push_ptr;



#endif // PROTOCOL_RTSP_PUSH_H