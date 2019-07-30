#ifndef PROTOCOL_RTSP_H
#define PROTOCOL_RTSP_H

#include "i_stream.h"
#include <thread>
#include <atomic>
#include <string>


class protocol_rtsp_pull : public i_stream{
public:
    protocol_rtsp_pull(const std::string& url);

    virtual bool add_stream(std::shared_ptr<i_stream> p_stream);
    virtual bool do_stream(info_stream_ptr p_info);
    virtual bool start();
    virtual void stop();
protected:
    static void handle_thread(std::string url, i_stream_ptr p_stream, std::shared_ptr<std::atomic_bool> p_bstop);

    std::string m_url;
    i_stream_ptr mp_stream;
    std::thread m_thread;
    std::shared_ptr<std::atomic_bool> mp_bstop;
};
typedef std::shared_ptr<protocol_rtsp_pull> protocol_rtsp_pull_ptr;



#endif // PROTOCOL_RTSP_H