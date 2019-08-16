#ifndef CONTROL_H
#define CONTROL_H

#include "i_stream.h"
#include <thread>
#include <atomic>

class control : public i_plugin
{
public:
    control(const std::string& url_input, const std::string& url_output);
    virtual bool start();
    virtual void stop();

    virtual void wait();
protected:
    virtual void handle_thread(const std::string& url_input, const std::string& url_output, std::shared_ptr<std::atomic_bool> flag_stop);
    virtual int start_input(info_av_ptr& p_info, const std::string& url_input);
    virtual int start_output(info_av_ptr& p_info, const std::string& url_output);
    virtual int do_stream(info_av_ptr& p_info);
    virtual int decode(info_av_ptr& p_info);
    virtual int encode(info_av_ptr& p_info);
    virtual int output(info_av_ptr& p_info);
    virtual int decode_video(info_av_ptr& p_info);
    virtual int decode_audio(info_av_ptr& p_info);
    virtual int encode_video(info_av_ptr& p_info);
    virtual int encode_audio(info_av_ptr& p_info);
    virtual int stop_input(info_av_ptr& p_info);
    virtual int stop_output(info_av_ptr& p_info);

    std::string m_url_input;
    std::string m_url_output;
    std::thread m_thread;
    std::shared_ptr<std::atomic_bool> mp_bstop;
};
typedef std::shared_ptr<control> control_ptr;


#endif // CONTROL_H