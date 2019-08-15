#ifndef PROTOCOL_RTSP_H
#define PROTOCOL_RTSP_H

#include "i_stream.h"
#include <string>

class protocol_rtsp_pull : public stream_base{
public:
    protocol_rtsp_pull(const std::string& url);
    virtual int before_stream(info_av_ptr p_info);
    virtual int step(info_av_ptr p_info);
    virtual int after_step(info_av_ptr p_info);
    virtual int after_stream(info_av_ptr p_info);
protected:
    std::string m_url;
};
typedef std::shared_ptr<protocol_rtsp_pull> protocol_rtsp_pull_ptr;



#endif // PROTOCOL_RTSP_H