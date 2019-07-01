#ifndef PROTOCOL_RTSP_H
#define PROTOCOL_RTSP_H

#include "i_stream.h"


class protocol_rtsp : public i_stream{
public:
    protocol_rtsp(const std::string& url);

    virtual bool add_stream(std::shared_ptr<i_stream>& p_stream);
    virtual bool do_stream(info_stream_ptr& p_info);
    virtual bool start();
    virtual void stop();
protected:
    std::string m_url;
};
using protocol_rtsp_ptr = std::shared_ptr<protocol_rtsp>;



#endif // PROTOCOL_RTSP_H