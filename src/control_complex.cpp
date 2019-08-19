#include "control_complex.h"
#include "utility_tool.h"
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavutil/avutil.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/opt.h>
}



bool operator<(const key_complex &k1, const key_complex &k2)
{
    if (k1.p_info.get() < k2.p_info.get())
    {
        return true;
    }
    else if (k1.p_info.get() == k2.p_info.get())
    {
        return k1.type < k2.type;
    }
    return false;
}

bool less_key_complex::operator()(const key_complex& k1, const key_complex& k2) const{
    if (k1.p_info.get() < k2.p_info.get())
    {
        return true;
    }
    else if (k1.p_info.get() == k2.p_info.get())
    {
        return k1.type < k2.type;
    }
    return false;
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
    m_thread = std::thread(std::bind(&control_complex::handle_thread_complex, this, m_url_input, m_url_output, mp_bstop));
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

int control_complex::init_filter_video(AVFilterGraph *&p_graph, std::vector<AVFilterContext *> &input_buffer_cxts, AVFilterContext *&p_output_buffer_cxt, std::vector<info_av_ptr> &infos)
{
    auto ret = ES_SUCCESS;
    char str_arg[1024], str_name[128];
    std::stringstream filter_params;
    int column = static_cast<int>(ceil(sqrt(infos.size())));
    

    input_buffer_cxts.clear();
    std::vector<const AVFilter *> buffersrcs;
    const AVFilter *p_buffersink = nullptr;

    std::vector<AVFilterInOut *> outputs;
    AVFilterInOut *p_input = avfilter_inout_alloc();
    p_graph = avfilter_graph_alloc();
    AVCodecContext* p_code_cxt = nullptr;
    for (std::size_t i = 0; i < infos.size(); ++i)
    {
        input_buffer_cxts.push_back(nullptr);
        buffersrcs.push_back(nullptr);
        outputs.push_back(avfilter_inout_alloc());
    }
    for (std::size_t i = 0; i < infos.size(); ++i)
    {
        for (auto &p_stream : infos[i]->streams)
        {
            if (AVMEDIA_TYPE_VIDEO != p_stream->pi_code_ctx->codec_type)
            {
                continue;
            }
            if(nullptr == p_code_cxt){
                p_code_cxt = p_stream->pi_code_ctx;
                filter_params << "color=c=black:s=" << p_code_cxt->width << "x" << p_code_cxt->height << "[x0]";
            }
            buffersrcs[i] = avfilter_get_by_name("buffer");
            sprintf(str_name, "inv%d", i);
            sprintf(str_arg, "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d", p_stream->pi_code_ctx->width, p_stream->pi_code_ctx->height, p_stream->pi_code_ctx->pix_fmt, p_stream->pi_code_ctx->time_base.num, p_stream->pi_code_ctx->time_base.den, p_stream->pi_code_ctx->sample_aspect_ratio.num, p_stream->pi_code_ctx->sample_aspect_ratio.den);
            ret = avfilter_graph_create_filter(&(input_buffer_cxts[i]), buffersrcs[i], str_name, str_arg, nullptr, p_graph);
            if (0 > ret)
            {
                return ES_UNKNOW;
            }

            outputs[i]->name = av_strdup(str_name);
            outputs[i]->filter_ctx = input_buffer_cxts[i];
            outputs[i]->pad_idx = 0;
            if (i == infos.size() - 1)
            {
                outputs[i]->next = nullptr;
            }
            else
            {
                outputs[i]->next = outputs[i + 1];
            }
            filter_params << ";[inv" << i << "]scale=w=" << (p_code_cxt->width / column) << ":h=" << (p_code_cxt->height / column) << "[innv" << i << "];[x" << i << "][innv" << i << "]overlay="
                          << ((p_code_cxt->width / column) * (i % column)) << ":" << ((p_code_cxt->height / column) * (i / column)) << "[x" << (i + 1) << "]";
        }
        m_map_cxts.insert(std::make_pair(key_complex(infos[i], AVMEDIA_TYPE_VIDEO), input_buffer_cxts[i]));
    }
    filter_params << ";[x" << infos.size() << "]null[out]";
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
    auto filter_str = filter_params.str();
    if ((ret = avfilter_graph_parse_ptr(p_graph, filter_str.c_str(), &p_input, outputs.data(), nullptr)) < 0)
    {
        return ES_UNKNOW;
    }
    if ((ret = avfilter_graph_config(p_graph, nullptr)) < 0)
    {
        return ES_UNKNOW;
    }
    avfilter_inout_free(&p_input);
    avfilter_inout_free(outputs.data());

    m_map_cxts.insert(std::make_pair(key_complex(info_av_ptr(), AVMEDIA_TYPE_VIDEO), p_output_buffer_cxt));
    return ES_SUCCESS;
}

int control_complex::init_filter_audio(AVFilterGraph *&p_graph, std::vector<AVFilterContext *> &input_buffer_cxts, AVFilterContext *&p_output_buffer_cxt, std::vector<info_av_ptr> &infos)
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
    AVCodecContext* p_code_cxt = nullptr;
    for (std::size_t i = 0; i < infos.size(); ++i)
    {
        input_buffer_cxts.push_back(nullptr);
        buffersrcs.push_back(nullptr);
        outputs.push_back(avfilter_inout_alloc());
    }
    for (std::size_t i = 0; i < infos.size(); ++i)
    {
        for (auto &p_stream : infos[i]->streams)
        {
            if (AVMEDIA_TYPE_AUDIO != p_stream->pi_code_ctx->codec_type)
            {
                continue;
            }
            if(nullptr == p_code_cxt){
                p_code_cxt = p_stream->pi_code_ctx;
            }
            buffersrcs[i] = avfilter_get_by_name("abuffer");
            sprintf(str_name, "ina%d", i);
            sprintf(str_arg, "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64, p_stream->pi_code_ctx->time_base.num, p_stream->pi_code_ctx->time_base.den, p_stream->pi_code_ctx->sample_rate, av_get_sample_fmt_name(p_stream->pi_code_ctx->sample_fmt), p_stream->pi_code_ctx->channel_layout);
            //sprintf(str_arg, "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64, p_stream->pi_code_ctx->time_base.num, p_stream->pi_code_ctx->time_base.den, p_stream->pi_code_ctx->sample_rate, av_get_sample_fmt_name(p_stream->pi_code_ctx->sample_fmt), p_stream->pi_code_ctx->channel_layout);
            ret = avfilter_graph_create_filter(&(input_buffer_cxts[i]), buffersrcs[i], str_name, str_arg, nullptr, p_graph);
            if (0 > ret)
            {
                return ES_UNKNOW;
            }

            outputs[i]->name = av_strdup(str_name);
            outputs[i]->filter_ctx = input_buffer_cxts[i];
            outputs[i]->pad_idx = 0;
            if (i == infos.size() - 1)
            {
                outputs[i]->next = nullptr;
            }
            else
            {
                outputs[i]->next = outputs[i + 1];
            }
            filter_params << "[ina" << i << "]";
        }

        m_map_cxts.insert(std::make_pair(key_complex(infos[i], AVMEDIA_TYPE_AUDIO), input_buffer_cxts[i]));
    }
    filter_params << "amix=inputs=" << infos.size() << "[out]";
    p_buffersink = avfilter_get_by_name("abuffersink");
    ret = avfilter_graph_create_filter(&p_output_buffer_cxt, p_buffersink, "out", nullptr, nullptr, p_graph);
    if (0 > ret)
    {
        return ES_UNKNOW;
    }

    if(0 > (ret = av_opt_set_bin(p_output_buffer_cxt, "sample_fmts", (uint8_t*)&p_code_cxt->sample_fmt, sizeof(p_code_cxt->sample_fmt), AV_OPT_SEARCH_CHILDREN))
        || 0 > (ret = av_opt_set_bin(p_output_buffer_cxt, "channel_layouts", (uint8_t*)&p_code_cxt->channel_layout, sizeof(p_code_cxt->channel_layout), AV_OPT_SEARCH_CHILDREN))
        || 0 > (ret = av_opt_set_bin(p_output_buffer_cxt, "sample_rates", (uint8_t*)&p_code_cxt->sample_rate, sizeof(p_code_cxt->sample_rate), AV_OPT_SEARCH_CHILDREN))){
        return ret;
    }

    p_input->name = av_strdup("out");
    p_input->filter_ctx = p_output_buffer_cxt;
    p_input->pad_idx = 0;
    p_input->next = nullptr;
    auto filter_str = filter_params.str();
    if ((ret = avfilter_graph_parse_ptr(p_graph, filter_str.c_str(), &p_input, outputs.data(), nullptr)) < 0)
    {
        return ES_UNKNOW;
    }
    if ((ret = avfilter_graph_config(p_graph, nullptr)) < 0)
    {
        return ES_UNKNOW;
    }
    avfilter_inout_free(&p_input);
    avfilter_inout_free(outputs.data());

    m_map_cxts.insert(std::make_pair(key_complex(info_av_ptr(), AVMEDIA_TYPE_AUDIO), p_output_buffer_cxt));
    return ES_SUCCESS;
}

int control_complex::encode(info_av_ptr &p_info)
{
    auto ret = ES_SUCCESS;
    auto iter = m_map_cxts.find(key_complex(p_info, p_info->streams[p_info->p_packet->stream_index]->pi_code_ctx->codec_type));
    auto iter_desc = m_map_cxts.find(key_complex(info_av_ptr(), p_info->streams[p_info->p_packet->stream_index]->pi_code_ctx->codec_type));
    if (m_map_cxts.end() == iter || m_map_cxts.end() == iter_desc)
    {
        return ES_UNKNOW;
    }
    ret = av_buffersrc_add_frame(iter->second, p_info->p_frame);
    if (ret < 0)
    {
        return ES_UNKNOW;
    }

    AVFrame* p_frame = av_frame_alloc();
    while (true)
    {
        ret = av_buffersink_get_frame_flags(iter_desc->second, p_frame, 0);
        if (AVERROR(EAGAIN) == ret || AVERROR_EOF == ret)
        {
            break;
        }
        else if (0 == ret)
        {
            auto p_frame_old = p_info->p_frame;
            p_info->p_frame =p_frame;
            ret = control::encode(p_info);
            p_info->p_frame = p_frame_old;
            if (0 > ret)
            {
                return ret;
            }
            av_frame_unref(p_frame);
        }
        else if (0 > ret)
        {
            return ret;
        }
    }
    av_frame_free(&p_frame);
    return ES_SUCCESS;
}

int control_complex::encode_complex(info_av_ptr &p_info)
{
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
        inputs.push_back(p_info);
    }
    if (ES_SUCCESS != (ret = start_output(inputs[0], url_output)))
    {
        return;
    }
    auto p_out = inputs[0];
    for(auto& p_info : inputs){
        p_info->po_fmt_ctx = p_out->po_fmt_ctx;
        for(auto& p_stream : p_info->streams){
            for(auto& p_stream_out : p_out->streams){
                if(p_stream->pi_code_ctx->codec_type == p_stream_out->pi_code_ctx->codec_type){
                    p_stream->po_code_ctx = p_stream_out->po_code_ctx;
                    p_stream->po_stream = p_stream_out->po_stream;
                    break;
                }
            }
        }
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

int control_complex::decode_audio(info_av_ptr& p_info){
    return ES_SUCCESS;
}