#include "control_complex.h"
#include "utility_tool.h"
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
}

control_complex::control_complex(const std::vector<std::string> &url_input, const std::string &url_output) : control("", url_output), m_url_input(url_input), m_url_output(url_output)
{
}

bool control_complex::start()
{
    if (4 != m_url_input.size())
    {
        return false;
    }
    mp_bstop = std::make_shared<std::atomic_bool>(false);
    m_thread = std::thread(std::bind(control_complex::handle_thread_complex, this, m_url_input, m_url_output, mp_bstop));
}

void control_complex::stop()
{
    *mp_bstop = true;
    m_thread.join();
}

void control_complex::wait()
{
    m_thread.join();
}

int control_complex::init_filter_video(AVFilterGraph *&p_graph, std::vector<AVFilterContext *> &input_buffer_cxts, AVFilterContext *&p_output_buffer_cxt, std::vector<info_av_ptr>& infos)
{
    auto ret = ES_SUCCESS;
    char str_arg[1024], str_name[128];
    std::stringstream filter_params;
    int column = static_cast<int>(ceil(sqrt(infos.size())));
    filter_params<<"color=c=black:s="<<1920<<"x"<<1080<<"[x0]";

    input_buffer_cxts.clear();
    std::vector<const AVFilter *> buffersrcs;
    const AVFilter *p_buffersink = nullptr;
    
    std::vector<AVFilterInOut *> outputs;
    AVFilterInOut *p_input = avfilter_inout_alloc();
    p_graph = avfilter_graph_alloc();
    for (std::size_t i = 0; i < infos.size(); ++i)
    {
        input_buffer_cxts.push_back(nullptr);
        buffersrcs.push_back(nullptr);
        outputs.push_back(avfilter_inout_alloc());

        for (auto &p_stream : infos[i]->streams)
        {
            if(AVMEDIA_TYPE_VIDEO != p_stream->pi_code_ctx->codec_type){
                continue;
            }
            buffersrcs[i] = avfilter_get_by_name("buffer");
            sprintf(str_name, "in_%d", i);
            sprintf(str_arg, "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d", p_stream->pi_code_ctx->width, p_stream->pi_code_ctx->height, p_stream->pi_code_ctx->pix_fmt, p_stream->pi_code_ctx->time_base.num, p_stream->pi_code_ctx->time_base.den, p_stream->pi_code_ctx->sample_aspect_ratio.num, p_stream->pi_code_ctx->sample_aspect_ratio.den);
            ret = avfilter_graph_create_filter(&(input_buffer_cxts[i]), buffersrcs[i], str_name, str_arg, nullptr, p_graph);
            if (0 > ret)
            {
                return ES_UNKNOW;
            }

            outputs[i]->name = av_strdup(str_name);
		    outputs[i]->filter_ctx = input_buffer_cxts[i];
		    outputs[i]->pad_idx = 0;
		    if (i == infos.size() - 1){
                outputs[i]->next = nullptr;
            }else{
                outputs[i]->next = outputs[i + 1];
            }
            filter_params<<";[in_"<<i<<"]scale=w="<<(1920/column)<<":h="<<(1080/column)<<"[inn"<<i<<"];[x"<<i<<"][inn"<<i<<"]overlay="
                <<((1920/column) * (i % column))<<":"<<((1080/column) * (i / column))<<"[x"<<(i + 1)<<"]";

            
        }
        m_map_cxts.insert(std::make_pair(key_complex(infos[i], AVMEDIA_TYPE_VIDEO), input_buffer_cxts[i]));
    }
    filter_params<<";[x"<<infos.size()<<"]null[out]";
    p_buffersink = avfilter_get_by_name("buffersink");
    ret = avfilter_graph_create_filter(&p_output_buffer_cxt, p_buffersink, "out", nullptr, nullptr, p_graph);
    if (0 > ret)
    {
        return ES_UNKNOW;
    }
    p_input->name = av_strdup("out");
	p_input->filter_ctx = p_output_buffer_cxt;
	p_input->pad_idx = 0;
	p_input->next = nullptr;
	if ((ret = avfilter_graph_parse_ptr(p_graph, filter_params.str().c_str(), &p_input, outputs.data(), nullptr)) < 0){
        return ES_UNKNOW;
    }
	if ((ret = avfilter_graph_config(p_graph, nullptr)) < 0){
        return ES_UNKNOW;
    }
    avfilter_inout_free(&p_input);
    for(auto& p_output : outputs){
        avfilter_inout_free(&p_output);
    }

    m_map_cxts.insert(std::make_pair(key_complex(info_av_ptr(), AVMEDIA_TYPE_VIDEO), p_output_buffer_cxt));
    return ES_SUCCESS;
}

int control_complex::init_filter_audio(AVFilterGraph *&p_graph, std::vector<AVFilterContext *> &input_buffer_cxts, AVFilterContext *&p_output_buffer_cxt, std::vector<info_av_ptr>& infos)
{
    auto ret = ES_SUCCESS;
    char str_arg[1024], str_name[128];
    std::stringstream filter_params;
    input_buffer_cxts.clear();
    std::vector<const AVFilter *> buffersrcs;
    const AVFilter *p_buffersink = nullptr;
    
    std::vector<AVFilterInOut *> outputs;
    AVFilterInOut *p_input = avfilter_inout_alloc();
    p_graph = avfilter_graph_alloc();
    for (std::size_t i = 0; i < infos.size(); ++i)
    {
        input_buffer_cxts.push_back(nullptr);
        buffersrcs.push_back(nullptr);
        outputs.push_back(avfilter_inout_alloc());

        for (auto &p_stream : infos[i]->streams)
        {
            if(AVMEDIA_TYPE_VIDEO != p_stream->pi_code_ctx->codec_type){
                continue;
            }
            buffersrcs[i] = avfilter_get_by_name("abuffer");
            sprintf(str_name, "in_%d", i);
            sprintf(str_arg, "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%I64x"
                , p_stream->pi_code_ctx->time_base.num, p_stream->pi_code_ctx->time_base.den
                , p_stream->pi_code_ctx->sample_rate, av_get_sample_fmt_name(p_stream->pi_code_ctx->sample_fmt), p_stream->pi_code_ctx->channel_layout);
            ret = avfilter_graph_create_filter(&(input_buffer_cxts[i]), buffersrcs[i], str_name, str_arg, nullptr, p_graph);
            if (0 > ret)
            {
                return ES_UNKNOW;
            }

            outputs[i]->name = av_strdup(str_name);
		    outputs[i]->filter_ctx = input_buffer_cxts[i];
		    outputs[i]->pad_idx = 0;
		    if (i ==infos.size() - 1){
                outputs[i]->next = nullptr;
            }else{
                outputs[i]->next = outputs[i + 1];
            }
            filter_params<<"[in_"<<i<<"]";
        }

        m_map_cxts.insert(std::make_pair(key_complex(infos[i], AVMEDIA_TYPE_AUDIO), input_buffer_cxts[i]));
    }
    filter_params<<"amix=inputs="<<infos.size()<<"[out]";
    p_buffersink = avfilter_get_by_name("abuffersink");
    ret = avfilter_graph_create_filter(&p_output_buffer_cxt, p_buffersink, "out", nullptr, nullptr, p_graph);
    if (0 > ret)
    {
        return ES_UNKNOW;
    }
    p_input->name = av_strdup("out");
	p_input->filter_ctx = p_output_buffer_cxt;
	p_input->pad_idx = 0;
	p_input->next = nullptr;
	if ((ret = avfilter_graph_parse_ptr(p_graph, filter_params.str().c_str(), &p_input, outputs.data(), nullptr)) < 0){
        return ES_UNKNOW;
    }
	if ((ret = avfilter_graph_config(p_graph, nullptr)) < 0){
        return ES_UNKNOW;
    }
    avfilter_inout_free(&p_input);
    for(auto& p_output : outputs){
        avfilter_inout_free(&p_output);
    }

    m_map_cxts.insert(std::make_pair(key_complex(info_av_ptr(), AVMEDIA_TYPE_AUDIO), p_output_buffer_cxt));
    return ES_SUCCESS;
}

int control_complex::encode(info_av_ptr& p_info){
    auto ret = ES_SUCCESS;
    auto iter = m_map_cxts.find(key_complex(p_info, p_info->streams[p_info->p_packet->stream_index]->pi_code_ctx->codec_type));
    if(m_map_cxts.end() == iter){
        return ES_UNKNOW;
    }
    ret = av_buffersrc_add_frame(iter->second, p_info->p_frame);
	if (ret < 0) {
	    return ES_UNKNOW;
    }

    AVFrame frame;
    while (true)
    {
        ret = av_buffersink_get_frame_flags(iter->second, &frame, 0);
        if(AVERROR(EAGAIN) == ret || AVERROR_EOF == ret){
            break;
        }else if(0 == ret){
            auto p_frame_old = p_info->p_frame;
            p_info->p_frame = &frame;
            ret = control::encode(p_info);
            p_info->p_frame = p_frame_old;
            if(0 > ret){
                return ret;
            }
        }else if(0 > ret){
            return ret;
        }
    }
    return ES_SUCCESS;
}

int control_complex::encode_complex(info_av_ptr& p_info){
    return ES_SUCCESS;
}

void control_complex::handle_thread_complex(const std::vector<std::string> &url_input, const std::string &url_output, std::shared_ptr<std::atomic_bool> pflag_stop)
{
    std::vector<info_av_ptr> inputs;
    auto ret = ES_SUCCESS;
    for (auto &url : url_input)
    {
        auto p_info = std::make_shared<info_av>();
        if (ES_SUCCESS != (ret = start_input(p_info, url)))
        {
            return;
        }
    }
    if (ES_SUCCESS != (ret = start_output(inputs[0], url_output)))
    {
        return;
    }
    init_filter_video(mp_video_graph, m_video_input_buffer_cxts, mp_video_output_buffer_cxt, inputs);
    init_filter_audio(mp_audio_graph, m_audio_input_buffer_cxts, mp_audio_output_buffer_cxt, inputs);
    while (!(*pflag_stop))
    {
        for (auto &p_info : inputs)
        {
            ret = do_stream(p_info);
        }
        if (ES_SUCCESS == ret)
        {
        }
        else if (ES_AGAIN == ret)
        {
            continue;
        }
        else if (ES_EOF == ret)
        {
            break;
        }
        else
        {
            break;
        }
    }
    ret = stop_output(inputs[0]);
    for (auto &p_info : inputs)
    {
        ret = stop_input(p_info);
    }
}