#include "filter_complex.h"
#include <sstream>
extern "C"
{
#include <libavfilter/avfilter.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/buffersrc.h>
}

int filter_complex::before_stream(info_av_ptr p_info)
{
    if (!m_flag_init)
    {
        m_flag_init = true;
        stream_base::before_stream(p_info);

        init_filter_video(mp_video_graph, m_video_input_buffer_cxts, mp_video_output_buffer_cxt, p_info);
        init_filter_video(mp_audio_graph, m_audio_input_buffer_cxts, mp_audio_output_buffer_cxt, p_info);
    }
    return ES_SUCCESS;
}

int filter_complex::init_filter_video(AVFilterGraph *&p_graph, std::vector<AVFilterContext *> &input_buffer_cxts, AVFilterContext *&p_output_buffer_cxt, info_av_ptr p_info)
{
    auto ret = ES_SUCCESS;
    char str_arg[1024], str_name[128];
    std::stringstream filter_params;
    int column = static_cast<int>(ceil(sqrt(p_info->inputs.size())));
    filter_params<<"color=c=black:s="<<1920<<"x"<<1080<<"[x0]";

    input_buffer_cxts.clear();
    std::vector<const AVFilter *> buffersrcs;
    const AVFilter *p_buffersink = nullptr;
    
    std::vector<AVFilterInOut *> outputs;
    AVFilterInOut *p_input = avfilter_inout_alloc();
    p_graph = avfilter_graph_alloc();
    for (std::size_t i = 0; i < p_info->inputs.size(); ++i)
    {
        input_buffer_cxts.push_back(nullptr);
        buffersrcs.push_back(nullptr);
        outputs.push_back(avfilter_inout_alloc());

        for (auto &p_stream : p_info->inputs[i]->streams)
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
		    if (i == p_info->inputs.size() - 1){
                outputs[i]->next = nullptr;
            }else{
                outputs[i]->next = outputs[i + 1];
            }
            filter_params<<";[in_"<<i<<"]scale=w="<<(1920/column)<<":h="<<(1080/column)<<"[inn"<<i<<"];[x"<<i<<"][inn"<<i<<"]overlay="
                <<((1920/column) * (i % column))<<":"<<((1080/column) * (i / column))<<"[x"<<(i + 1)<<"]";
        }
    }
    filter_params<<";[x"<<p_info->inputs.size()<<"]null[out]";
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
    return ES_SUCCESS;
}

int init_filter_audio(AVFilterGraph *&p_graph, std::vector<AVFilterContext *> &input_buffer_cxts, AVFilterContext *&p_output_buffer_cxt, info_av_ptr p_info, const AVMediaType &type)
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
    for (std::size_t i = 0; i < p_info->inputs.size(); ++i)
    {
        input_buffer_cxts.push_back(nullptr);
        buffersrcs.push_back(nullptr);
        outputs.push_back(avfilter_inout_alloc());

        for (auto &p_stream : p_info->inputs[i]->streams)
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
		    if (i == p_info->inputs.size() - 1){
                outputs[i]->next = nullptr;
            }else{
                outputs[i]->next = outputs[i + 1];
            }
            filter_params<<"[in_"<<i<<"]";
        }
    }
    filter_params<<"amix=inputs="<<p_info->inputs.size()<<"[out]";
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
    return ES_SUCCESS;
}

int filter_complex::do_stream(info_av_ptr p_info)
{
    auto ret = ES_SUCCESS;
    if(AVMEDIA_TYPE_VIDEO == p_info->p_stream->pi_code_ctx->codec_type){
        ret = av_buffersrc_add_frame(m_video_input_buffer_cxts[0], p_info->p_frame);
    }else if(AVMEDIA_TYPE_AUDIO == p_info->p_stream->pi_code_ctx->codec_type){
        ret = av_buffersrc_add_frame(m_audio_input_buffer_cxts[0], p_info->p_frame);
    }
	if (ret < 0) {
	    return ES_UNKNOW;
    }
    return ES_SUCCESS;
}
int filter_complex::after_stream(info_av_ptr p_info)
{
    return ES_SUCCESS;
}