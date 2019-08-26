#ifndef CONTROL_COMPLEX_H
#define CONTROL_COMPLEX_H

#include "control.h"
#include <thread>
#include <atomic>
#include <map>
extern "C"
{
#include <libavutil/avutil.h>
}


class AVFilterGraph;
class AVFilterContext;
class key_complex
{
public:
    key_complex(const info_av_ptr &p, const AVMediaType &t) : p_info(p), type(t) {}
    info_av_ptr p_info;
    AVMediaType type;
};
bool operator<(const key_complex& k1, const key_complex& k2);

class less_key_complex{
public:
    bool operator()(const key_complex& k1, const key_complex& k2) const;
};

class info_gather{
public:
    AVFormatContext* p_fmt_ctx = nullptr;
    AVPacket* p_packet = nullptr;
    AVFrame *p_frame = nullptr;

    AVStream* p_video_stream = nullptr;
    AVCodecContext* p_video_code_ctx = nullptr;
    AVStream* p_audio_stream = nullptr;
    AVCodecContext* p_audio_code_ctx = nullptr;

    struct SwsContext* p_video_sws_ctx = nullptr;
    AVFrame* po_frame = nullptr;
};
using info_gather_ptr  = std::shared_ptr<info_gather>;

class info_control_complex{
public:
    std::vector<info_gather_ptr> inputs;
    std::vector<info_gather_ptr> outputs;
};
using info_control_complex_ptr  = std::shared_ptr<info_control_complex>;


class control_complex : public control
{
public:
    control_complex(const std::vector<std::string>& url_input, const std::string& url_output);
    virtual bool start();
    virtual void stop();

    virtual void wait();
protected:
    virtual void handle_thread_complex(const std::vector<std::string>& url_input, const std::string& url_output, std::shared_ptr<std::atomic_bool> flag_stop);

     virtual int start_input(info_control_complex_ptr& p_info, const std::vector<std::string>& url_input);
    virtual int start_output(info_control_complex_ptr& p_info, const std::string& url_output, const AVCodecContext* pi_video_code_ctx, const AVCodecContext* pi_audio_code_ctx);
    virtual int do_stream(info_control_complex_ptr& p_info);
    virtual int decode(info_gather_ptr &p_input);
    virtual int encode(info_control_complex_ptr &p_info);
    virtual int output(AVPacket* p_packet, info_gather_ptr po_gather);
    virtual int decode_video(info_gather_ptr& p_info);
    virtual int decode_audio(info_gather_ptr& p_info);
    virtual int encode_video(info_control_complex_ptr p_info);
    virtual int encode_audio(info_control_complex_ptr& p_info);
    virtual int stop_input(info_control_complex_ptr& p_info);
    virtual int stop_output(info_control_complex_ptr& p_info);
    
    std::vector<std::string> m_url_input;
    std::string m_url_output;
    std::thread m_thread;
    std::shared_ptr<std::atomic_bool> mp_bstop;
    int m_video_pts_last = -1;
};
typedef std::shared_ptr<control_complex> control_complex_ptr;


#endif // CONTROL_COMPLEX_H