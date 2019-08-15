#include "stream_package_decode.h"
#include "utility_tool.h"
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

int stream_package_decode::before_stream(info_av_ptr p_info)
{
    auto ret = avformat_find_stream_info(p_info->pi_fmt_ctx, nullptr);
    if (0 > ret)
    {
        LOG_ERROR("获取流信息失败; 错误代码:" << ret);
        return ES_UNKNOW;
    }
    p_info->streams.resize(p_info->pi_fmt_ctx->nb_streams);
    {
        AVCodec *pi_code = nullptr;
        auto p_info_stream = std::make_shared<info_stream>();
        auto index_stream = av_find_best_stream(p_info->pi_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &(pi_code), 0);
        if (0 > index_stream)
        {
            LOG_ERROR("获取视频索引失败:" << index_stream);
            return ES_UNKNOW;
        }
        p_info_stream->pstream_base = p_info->pi_fmt_ctx->streams[index_stream];
        p_info_stream->pi_code_ctx = avcodec_alloc_context3(pi_code);
        avcodec_parameters_to_context(p_info_stream->pi_code_ctx, p_info_stream->pstream_base->codecpar);
        p_info_stream->pi_code_ctx->framerate = av_guess_frame_rate(p_info->pi_fmt_ctx, p_info_stream->pstream_base, nullptr);
        auto ret = avcodec_open2(p_info_stream->pi_code_ctx, pi_code, nullptr);
        p_info->streams[index_stream] = p_info_stream;
    }
    {
        AVCodec *pi_code = nullptr;
        auto p_info_stream = std::make_shared<info_stream>();
        auto index_stream = av_find_best_stream(p_info->pi_fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &(pi_code), 0);
        if (0 > index_stream)
        {
            LOG_ERROR("获取音频索引失败:" << index_stream);
            return ES_UNKNOW;
        }
        p_info_stream->pstream_base = p_info->pi_fmt_ctx->streams[index_stream];
        p_info_stream->pi_code_ctx = avcodec_alloc_context3(pi_code);
        avcodec_parameters_to_context(p_info_stream->pi_code_ctx, p_info_stream->pstream_base->codecpar);
        p_info_stream->pi_code_ctx->framerate = av_guess_frame_rate(p_info->pi_fmt_ctx, p_info_stream->pstream_base, nullptr);
        auto ret = avcodec_open2(p_info_stream->pi_code_ctx, pi_code, nullptr);
        p_info->streams[index_stream] = p_info_stream;
    }
    return stream_base::before_stream(p_info);
}

int stream_package_decode::do_stream(info_av_ptr p_info)
{
    auto ret = ES_SUCCESS;
    if (0 <= p_info->p_packet->stream_index || static_cast<int>(p_info->streams.size()) > p_info->p_packet->stream_index)
    {
        p_info->p_stream = p_info->streams[p_info->p_packet->stream_index];
        if (nullptr != p_info->p_stream)
        {

            if (AVMEDIA_TYPE_VIDEO == p_info->p_stream->pi_code_ctx->codec_type)
            {
                ret = decode_video(p_info);
            }
            else if (AVMEDIA_TYPE_AUDIO == p_info->p_stream->pi_code_ctx->codec_type)
            {
                ret = decode_audio(p_info);
            }
            else
            {
                return ES_UNKNOW;
            }
        }
        else
        {
            LOG_ERROR("丢弃无法处理的索引数据帧:" << p_info->p_packet->stream_index);
        }
    }
    else
    {
        LOG_ERROR("索引超过范围:" << p_info->p_packet->stream_index << "; 实际最大索引:" << p_info->streams.size());
    }

    if (ES_SUCCESS != ret)
    {
        return ret;
    }

    return ret;
}

int stream_package_decode::decode_video(info_av_ptr p_info)
{
    int ret = 0;
    while (true)
    {
        if (p_info->p_packet->data != nullptr) // not a flush packet
        {
            av_packet_rescale_ts(p_info->p_packet, p_info->p_stream->pstream_base->time_base, p_info->p_stream->pi_code_ctx->time_base);
        }
        ret = avcodec_send_packet(p_info->p_stream->pi_code_ctx, p_info->p_packet);
        if (0 == ret)
        {
        }
        else if (AVERROR(EAGAIN) == ret)
        {
            return ES_AGAIN;
        }
        else if (AVERROR_EOF == ret)
        {
            return ES_EOF;
        }
        else
        {
            return ES_UNKNOW;
        }
        while (true)
        {
            ret = avcodec_receive_frame(p_info->p_stream->pi_code_ctx, p_info->p_frame);
            if (0 != ret)
            {
                if (AVERROR_EOF == ret)
                {
                    avcodec_flush_buffers(p_info->p_stream->pi_code_ctx);
                    return ES_EOF;
                }
                else if (AVERROR(EAGAIN) == ret)
                {
                    return ES_AGAIN;
                }
                else
                {
                    return ES_UNKNOW;
                }
            }
            //if (AV_NOPTS_VALUE == p_info->p_frame->pts)
            //{
            //    p_info->p_frame->pts = p_info->p_frame->best_effort_timestamp;
            //}

            ret = stream_base::do_stream(p_info);
        }
    }
    return ES_SUCCESS;
}

int stream_package_decode::decode_audio(info_av_ptr p_info)
{
    return stream_base::do_stream(p_info);
}