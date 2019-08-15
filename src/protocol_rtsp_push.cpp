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

int protocol_rtsp_push::before_stream(info_av_ptr p_info)
{
    auto ret = ES_SUCCESS;
    if (0 > avformat_alloc_output_context2(&p_info->po_fmt_ctx, nullptr, "flv", m_url.c_str()))
    {
        LOG_ERROR("打开输出流失败");
        return ES_UNKNOW;
    }
    p_info->po_fmt_ctx->duration = p_info->pi_fmt_ctx->duration;
    p_info->po_fmt_ctx->bit_rate = p_info->pi_fmt_ctx->bit_rate;

    if (!(p_info->po_fmt_ctx->oformat->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&(p_info->po_fmt_ctx->pb), m_url.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0)
        {
            return ES_UNKNOW;
        }
    }
    return stream_base::before_stream(p_info);
}

int protocol_rtsp_push::do_stream(info_av_ptr p_info)
{
    int ret = 0;
    if(!m_flag_header){
        m_flag_header = true;
        ret = avformat_write_header(p_info->po_fmt_ctx, nullptr);

        printf("---------------- 输出文件信息 ---------------\n");
        av_dump_format(p_info->po_fmt_ctx, 0, m_url.c_str(), 1);
        printf("-------------------------------------------------\n");
    }
    
    if (!(p_info->p_packet))
    {
        // 结束了
        return ES_SUCCESS;
    }
    ret = av_interleaved_write_frame(p_info->po_fmt_ctx, p_info->p_packet);
    return stream_base::do_stream(p_info);
}

int protocol_rtsp_push::after_stream(info_av_ptr p_info)
{
    stream_base::after_stream(p_info);
    
    auto ret = av_write_trailer(p_info->po_fmt_ctx);
    avformat_free_context(p_info->po_fmt_ctx);
    p_info->po_fmt_ctx = nullptr;
    return ES_SUCCESS;
}