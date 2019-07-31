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
    int ret = 0;
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

        AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        p_info->p_o_code_cnt = avcodec_alloc_context3(codec);

        p_info->p_o_code_cnt->codec_id = AV_CODEC_ID_H264;

        p_info->p_o_code_cnt->width = p_info->p_code_cnt->width;   //你想要的宽度
        p_info->p_o_code_cnt->height = p_info->p_code_cnt->height; //你想要的高度
        p_info->p_o_code_cnt->sample_aspect_ratio = p_info->p_code_cnt->sample_aspect_ratio;
        if (nullptr != codec->pix_fmts)
        {
            p_info->p_o_code_cnt->pix_fmt = codec->pix_fmts[0];
        }
        else
        {
            p_info->p_o_code_cnt->pix_fmt = p_info->p_code_cnt->pix_fmt;
        }

        p_info->p_o_code_cnt->time_base = av_inv_q(p_info->p_code_cnt->framerate); //应该根据帧率设置
        p_info->p_o_code_cnt->framerate = p_info->p_code_cnt->framerate;
        //codec_ctx->bit_rate = p_info->p_stream->codecpar->bit_rate;

        avcodec_open2(p_info->p_o_code_cnt, codec, nullptr);
        //将AVCodecContext的成员复制到AVCodecParameters结构体。前后两行不能调换顺序
        avcodec_parameters_from_context(mp_stream->codecpar, p_info->p_o_code_cnt);

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

    AVFrame *p_frame = av_frame_alloc();
    AVPacket o_packet;
    av_init_packet(&o_packet);
    while (true)
    {
        if (p_info->p_packet->data != nullptr) // not a flush packet
        {
            av_packet_rescale_ts(p_info->p_packet, p_info->p_stream->time_base, p_info->p_o_code_cnt->time_base);
        }
        ret = avcodec_send_packet(p_info->p_code_cnt, p_info->p_packet);
        while (true)
        {
            ret = avcodec_receive_frame(p_info->p_code_cnt, p_frame);
            if (0 > ret)
            {
                if (AVERROR_EOF == ret)
                {
                    avcodec_flush_buffers(p_info->p_code_cnt);
                    return true;
                }
                else if (AVERROR(EAGAIN) == ret)
                {
                    return true;
                }
                else
                {
                    return false;
                }
                
            }
            if(AV_NOPTS_VALUE == p_frame->pts){
                // 
                p_frame->pts = p_frame->best_effort_timestamp;
            }
            ret = avcodec_send_frame(p_info->p_o_code_cnt, p_frame);
            if(0 > ret){
                if (AVERROR_EOF == ret)
                {
                    avcodec_flush_buffers(p_info->p_code_cnt);
                }
                else if (AVERROR(EAGAIN) == ret)
                {
                }
                else
                {
                    return false;
                }
            }
            ret = avcodec_receive_packet(p_info->p_o_code_cnt, &o_packet);
            if(0 > ret){
                return false;
            }
            o_packet.stream_index = p_info->index_video;
            av_packet_rescale_ts(&o_packet, p_info->p_o_code_cnt->time_base, mp_stream->time_base);
            ret = av_interleaved_write_frame(mp_fmt_cnt, &o_packet);
            av_packet_unref(&o_packet);
        }
    }
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