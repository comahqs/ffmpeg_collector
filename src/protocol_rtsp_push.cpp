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

bool protocol_rtsp_push::before_stream(info_av_ptr p_info)
{
    if (0 > avformat_alloc_output_context2(&p_info->po_fmt_ctx, nullptr, "flv", m_url.c_str()))
    {
        LOG_ERROR("打开输出流失败");
        return false;
    }
    p_info->po_fmt_ctx->duration = p_info->pi_fmt_ctx->duration;
    p_info->po_fmt_ctx->bit_rate = p_info->pi_fmt_ctx->bit_rate;
    return true;
}

bool protocol_rtsp_push::do_stream(info_av_ptr p_info)
{
    int ret = 0;
    if (!(p_info->p_packet))
    {
        // 结束了
        stop();
        return true;
    }

    if (nullptr == mp_info)
    {
        mp_info = p_info;

        if (!(p_info->po_fmt_ctx->oformat->flags & AVFMT_NOFILE))
        {
            ret = avio_open(&(p_info->po_fmt_ctx->pb), m_url.c_str(), AVIO_FLAG_WRITE);
            if (ret < 0)
            {
                return false;
            }
        }
        ret = avformat_write_header(p_info->po_fmt_ctx, nullptr);
    }

    ret = av_interleaved_write_frame(p_info->po_fmt_ctx, p_info->p_packet);
    return true;
}

bool after_stream(info_av_ptr p_info)
{
    auto ret = av_write_trailer(p_info->po_fmt_ctx);
    avformat_free_context(p_info->po_fmt_ctx);
    p_info->po_fmt_ctx = nullptr;
    return true;
}