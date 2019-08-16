#ifndef FILTER_COMPLEX_H
#define FILTER_COMPLEX_H

#include "i_stream.h"
#include <thread>
#include <atomic>

class AVFilterGraph;
class AVFilter;
class AVFilterContext;
class AVFilterInOut;
class filter_complex : public stream_base
{
public:
    virtual int before_stream(info_av_ptr p_info);
    virtual int do_stream(info_av_ptr p_info);
    virtual int after_stream(info_av_ptr p_info);

protected:
    virtual int init_filter_video(AVFilterGraph*& p_graph, std::vector<AVFilterContext*>& input_buffer_cxts, AVFilterContext*& p_output_buffer_cxt, info_av_ptr p_info);
    virtual int init_filter_audio(AVFilterGraph*& p_graph, std::vector<AVFilterContext*>& input_buffer_cxts, AVFilterContext*& p_output_buffer_cxt, info_av_ptr p_info);

    bool m_flag_init = false;
    AVFilterGraph* mp_video_graph = nullptr;
    std::vector<AVFilterContext*> m_video_input_buffer_cxts;
    AVFilterContext* mp_video_output_buffer_cxt = nullptr;
    AVFilterGraph* mp_audio_graph = nullptr;
    std::vector<AVFilterContext*> m_audio_input_buffer_cxts;
    AVFilterContext* mp_audio_output_buffer_cxt = nullptr;
};
typedef std::shared_ptr<filter_complex> filter_complex_ptr;


#endif // FILTER_COMPLEX_H