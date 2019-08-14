#include "stream_package_encode.h"
#include "utility_tool.h"
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

bool stream_package_encode::add_stream(std::shared_ptr<i_stream> p_stream)
{
    mp_stream = p_stream;
    return true;
}

bool stream_package_encode::before_stream(info_av_ptr p_info)
{
    mp_stream->before_stream(p_info);

    for (auto &p_info_stream : p_info->streams)
    {
        if (AVMEDIA_TYPE_VIDEO == p_info_stream->pi_code_ctx->codec_type)
        {
            p_info_stream->po_stream = avformat_new_stream(p_info->po_fmt_ctx, nullptr);
            if (nullptr == p_info_stream->po_stream)
            {
                LOG_ERROR("新建输出流失败");
                return false;
            }

            AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
            p_info_stream->po_code_ctx = avcodec_alloc_context3(codec);

            p_info_stream->po_code_ctx->codec_id = AV_CODEC_ID_H264;

            p_info_stream->po_code_ctx->width = p_info_stream->pi_code_ctx->width;   //你想要的宽度
            p_info_stream->po_code_ctx->height = p_info_stream->pi_code_ctx->height; //你想要的高度
            //p_info_stream->po_code_ctx->width = 480;  //你想要的宽度
            //p_info_stream->po_code_ctx->height = 320; //你想要的高度
            p_info_stream->po_code_ctx->sample_aspect_ratio = p_info_stream->pi_code_ctx->sample_aspect_ratio;
            if (nullptr != codec->pix_fmts)
            {
                p_info_stream->po_code_ctx->pix_fmt = codec->pix_fmts[0];
            }
            else
            {
                p_info_stream->po_code_ctx->pix_fmt = p_info_stream->pi_code_ctx->pix_fmt;
            }
            p_info_stream->po_code_ctx->time_base = p_info_stream->pi_code_ctx->time_base;
            p_info_stream->po_code_ctx->framerate = p_info_stream->pi_code_ctx->framerate;
            p_info_stream->po_code_ctx->bit_rate = p_info_stream->pi_code_ctx->bit_rate;

            avcodec_open2(p_info_stream->po_code_ctx, codec, nullptr);
            //将AVCodecContext的成员复制到AVCodecParameters结构体。前后两行不能调换顺序
            avcodec_parameters_from_context(p_info_stream->po_stream->codecpar, p_info_stream->po_code_ctx);
            //p_info_stream->po_stream->avg_frame_rate = p_info_stream->pi_stream->avg_frame_rate;
            //p_info_stream->po_stream->r_frame_rate = p_info_stream->pi_stream->r_frame_rate;
        }
        else if (AVMEDIA_TYPE_AUDIO == p_info_stream->pi_code_ctx->codec_type)
        {
            p_info_stream->po_stream = avformat_new_stream(p_info->po_fmt_ctx, nullptr);
            if (nullptr == p_info_stream->po_stream)
            {
                LOG_ERROR("新建输出流失败");
                return false;
            }

            AVCodec *codec = avcodec_find_encoder(p_info_stream->pi_code_ctx->codec_id);
            p_info_stream->po_code_ctx = avcodec_alloc_context3(codec);

            p_info_stream->po_code_ctx->codec_id = p_info_stream->pi_code_ctx->codec_id;

            p_info_stream->po_code_ctx->sample_rate = p_info_stream->pi_code_ctx->sample_rate;                                    // 采样率
            p_info_stream->po_code_ctx->channel_layout = p_info_stream->pi_code_ctx->channel_layout;                              // 声道布局
            p_info_stream->po_code_ctx->channels = av_get_channel_layout_nb_channels(p_info_stream->pi_code_ctx->channel_layout); // 声道数量
            p_info_stream->po_code_ctx->sample_fmt = codec->sample_fmts[0];                                                       // 编码器采用所支持的第一种采样格式
            p_info_stream->po_code_ctx->time_base = (AVRational){1, p_info_stream->po_code_ctx->sample_rate};                     // 时基：编码器采样率取倒数

            avcodec_open2(p_info_stream->po_code_ctx, codec, nullptr);
            //将AVCodecContext的成员复制到AVCodecParameters结构体。前后两行不能调换顺序
            avcodec_parameters_from_context(p_info_stream->po_stream->codecpar, p_info_stream->po_code_ctx);
        }
    }
    return true;
}

bool stream_package_encode::do_stream(info_av_ptr p_info)
{
    if (AVMEDIA_TYPE_VIDEO == p_info->p_stream->pi_code_ctx->codec_type)
    {
        auto ret = avcodec_send_frame(p_info->p_stream->po_code_ctx, p_info->p_frame);
        if (0 > ret)
        {
            if (AVERROR_EOF == ret)
            {
                avcodec_flush_buffers(p_info->p_stream->pi_code_ctx);
            }
            else if (AVERROR(EAGAIN) == ret)
            {
            }
            else
            {
                return false;
            }
        }
        AVPacket o_packet;
        av_init_packet(&o_packet);
        ret = avcodec_receive_packet(p_info->p_stream->po_code_ctx, &o_packet);
        if (0 > ret)
        {
            return false;
        }
        o_packet.stream_index = p_info->p_stream->po_stream->index;
        av_packet_rescale_ts(&o_packet, p_info->p_stream->po_code_ctx->time_base, p_info->p_stream->po_stream->time_base);

        auto pi_package = p_info->p_packet;
        p_info->p_packet = &o_packet;
        mp_stream->do_stream(p_info);
        p_info->p_packet = pi_package;
        av_packet_unref(&o_packet);
    }
    else if (AVMEDIA_TYPE_AUDIO == p_info->p_stream->pi_code_ctx->codec_type)
    {
        mp_stream->do_stream(p_info);
    }
    return true;
}

bool stream_package_encode::after_stream(info_av_ptr p_info)
{
    return true;
}