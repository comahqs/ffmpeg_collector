#include "protocol_rtsp_push.h"
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}
#include "utility_tool.h"

protocol_rtsp_push::protocol_rtsp_push(const std::string &url) : m_url(url)
{
}

bool protocol_rtsp_push::add_stream(std::shared_ptr<i_stream>)
{
    return true;
}
bool protocol_rtsp_push::do_stream(info_stream_ptr p_info)
{
    if (nullptr == mp_fmt_cnt)
    {
        if (0 > avformat_alloc_output_context2(&mp_fmt_cnt, nullptr, nullptr, m_url.c_str()))
        {
            LOG_ERROR("打开输出流失败");
            return false;
        }
        mp_stream = avformat_new_stream(mp_fmt_cnt, nullptr);
        if (nullptr == mp_stream)
        {
            LOG_ERROR("新建输出流失败");
            return false;
        }
        int ret = 0;

        AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);

        codec_ctx->codec_id = AV_CODEC_ID_H264;

        codec_ctx->width = p_info->p_stream->codecpar->width;   //你想要的宽度
        codec_ctx->height = p_info->p_stream->codecpar->height; //你想要的高度
        codec_ctx->pix_fmt = p_info->p_code_cnt->pix_fmt;       //受codec->pix_fmts数组限制

        codec_ctx->time_base = p_info->p_stream->time_base; //应该根据帧率设置
        codec_ctx->bit_rate = p_info->p_stream->codecpar->bit_rate;

        avcodec_open2(codec_ctx, codec, nullptr);
        //将AVCodecContext的成员复制到AVCodecParameters结构体。前后两行不能调换顺序
        avcodec_parameters_from_context(mp_stream->codecpar, p_info->p_code_cnt);

        av_stream_set_r_frame_rate(mp_stream, {1, 25});

        if (!(mp_fmt_cnt->oformat->flags & AVFMT_NOFILE))
        {
            ret = avio_open(&mp_fmt_cnt->pb, m_url.c_str(), AVIO_FLAG_WRITE);
            if (ret < 0)
            {
                return false;
            }
        }
        ret = avformat_write_header(mp_fmt_cnt, nullptr);
    }
    p_info->p_packet->pts = av_rescale_q_rnd(p_info->p_packet->pts, p_info->p_stream->time_base, mp_stream->time_base, AV_ROUND_NEAR_INF);
    p_info->p_packet->dts = av_rescale_q_rnd(p_info->p_packet->dts, p_info->p_stream->time_base, mp_stream->time_base, AV_ROUND_NEAR_INF);
    p_info->p_packet->duration = av_rescale_q_rnd(p_info->p_packet->duration, p_info->p_stream->time_base, mp_stream->time_base, AV_ROUND_NEAR_INF);
    p_info->p_packet->pos = -1;
    auto ret = av_interleaved_write_frame(mp_fmt_cnt, p_info->p_packet);
    return true;
}
bool protocol_rtsp_push::start()
{
    return true;
}
void protocol_rtsp_push::stop()
{
    if (nullptr != mp_fmt_cnt)
    {
        auto ret = av_write_trailer(mp_fmt_cnt);
        avformat_free_context(mp_fmt_cnt);
        mp_fmt_cnt = nullptr;
    }
}