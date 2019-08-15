#ifndef PROTOCOL_RTSP_PUSH_H
#define PROTOCOL_RTSP_PUSH_H

#include "i_stream.h"
#include <thread>
#include <atomic>
#include <string>



class protocol_rtsp_push : public stream_base{
public:
    protocol_rtsp_push(const std::string& url);

    virtual int before_stream(info_av_ptr p_info);
    virtual int do_stream(info_av_ptr p_info);
    virtual int after_stream(info_av_ptr p_info);
protected:
    std::string m_url;
};
typedef std::shared_ptr<protocol_rtsp_push> protocol_rtsp_push_ptr;



#endif // PROTOCOL_RTSP_PUSH_H