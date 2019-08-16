#include "protocol_rtsp_pull.h"
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}
#include "utility_tool.h"

protocol_rtsp_pull::protocol_rtsp_pull(const std::string &url) : m_url(url)
{
}

int protocol_rtsp_pull::before_stream(info_av_ptr p_info)
{
    LOG_INFO("开始处理视频流:" << m_url);
    auto p_info_gather = p_info->inputs[p_info->index_input];
    auto ret = avformat_open_input(&p_info_gather->pi_fmt_ctx, m_url.c_str(), nullptr, nullptr);
    if (0 != ret)
    {
        LOG_ERROR("打开url失败;  错误代码:" << ret << "; 路径:" << m_url);
        return ES_UNKNOW;
    }
    p_info_gather->p_packet = av_packet_alloc();
    p_info_gather->p_frame = av_frame_alloc();

    ret = stream_base::before_stream(p_info);

    printf("---------------- 输入文件信息 ---------------\n");
    av_dump_format(p_info_gather->pi_fmt_ctx, 0, m_url.c_str(), 0);
    printf("-------------------------------------------------\n");
    return ret;
}

int protocol_rtsp_pull::do_stream(info_av_ptr p_info)
{
    auto p_info_gather = p_info->inputs[p_info->index_input];
    auto ret = av_read_frame(p_info_gather->pi_fmt_ctx, p_info_gather->p_packet);
    if (0 > ret)
    {
        if ((ret == AVERROR_EOF) || avio_feof(p_info_gather->pi_fmt_ctx->pb))
        {
            // 已经读取完了
            return ES_EOF;
        }
        else
        {
            LOG_ERROR("获取输入帧时发生错误:" << ret);
            return ES_UNKNOW;
        }
    }
    ret = stream_base::do_stream(p_info);
    if (nullptr != p_info_gather->p_packet)
    {
        av_packet_unref(p_info_gather->p_packet);
    }
    return ES_SUCCESS;
}

int protocol_rtsp_pull::after_stream(info_av_ptr p_info)
{
    stream_base::after_stream(p_info);

    auto p_info_gather = p_info->inputs[p_info->index_input];
    if (nullptr != p_info_gather->p_frame)
    {
        av_frame_free(&p_info_gather->p_frame);
        p_info_gather->p_frame = nullptr;
    }
    if (nullptr != p_info_gather->p_packet)
    {
        av_packet_free(&p_info_gather->p_packet);
        p_info_gather->p_packet = nullptr;
    }

    if (nullptr != p_info_gather->pi_fmt_ctx)
    {
        avformat_close_input(&p_info_gather->pi_fmt_ctx);
        p_info_gather->pi_fmt_ctx = nullptr;
    }

    LOG_INFO("结束处理视频流:" << m_url);
    return ES_SUCCESS;
}