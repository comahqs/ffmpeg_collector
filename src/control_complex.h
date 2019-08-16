#ifndef CONTROL_COMPLEX_H
#define CONTROL_COMPLEX_H

#include "control.h"
#include <thread>
#include <atomic>
#include <map>


class AVFilterGraph;
class AVFilterContext;
enum AVMediaType;
class key_complex{
public:
    key_complex(const info_av_ptr& p, const AVMediaType& t):p_info(p), type(t){}

    info_av_ptr p_info;
    AVMediaType type;
};


class control_complex : public control
{
public:
    control_complex(const std::vector<std::string>& url_input, const std::string& url_output);
    virtual bool start();
    virtual void stop();

    virtual void wait();
protected:
    virtual void handle_thread_complex(const std::vector<std::string>& url_input, const std::string& url_output, std::shared_ptr<std::atomic_bool> flag_stop);
    virtual int encode(info_av_ptr& p_info);
    virtual int encode_complex(info_av_ptr& p_info);
    virtual int init_filter_video(AVFilterGraph*& p_graph, std::vector<AVFilterContext*>& input_buffer_cxts, AVFilterContext*& p_output_buffer_cxt, std::vector<info_av_ptr>& infos);
    virtual int init_filter_audio(AVFilterGraph*& p_graph, std::vector<AVFilterContext*>& input_buffer_cxts, AVFilterContext*& p_output_buffer_cxt, std::vector<info_av_ptr>& infos);

    std::vector<std::string> m_url_input;
    std::string m_url_output;
    std::thread m_thread;
    std::shared_ptr<std::atomic_bool> mp_bstop;
    AVFilterGraph* mp_video_graph = nullptr;
    std::vector<AVFilterContext*> m_video_input_buffer_cxts;
    AVFilterContext* mp_video_output_buffer_cxt = nullptr;
    AVFilterGraph* mp_audio_graph = nullptr;
    std::vector<AVFilterContext*> m_audio_input_buffer_cxts;
    AVFilterContext* mp_audio_output_buffer_cxt = nullptr;

    std::map<key_complex, AVFilterContext*> m_map_cxts;
};
typedef std::shared_ptr<control_complex> control_complex_ptr;


#endif // CONTROL_COMPLEX_H