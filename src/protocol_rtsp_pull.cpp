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

bool protocol_rtsp_pull::add_stream(std::shared_ptr<i_stream> p_stream)
{
    mp_stream = p_stream;
    return true;
}
bool protocol_rtsp_pull::do_stream(info_av_ptr)
{
    return false;
}
bool protocol_rtsp_pull::start()
{
    mp_bstop = std::make_shared<std::atomic_bool>(false);
    //m_thread = std::thread(std::bind(protocol_rtsp_pull::handle_thread, m_url, mp_stream, mp_bstop));
    handle_thread(m_url, mp_stream, mp_bstop);
    return true;
}
void protocol_rtsp_pull::stop()
{
    auto p_bstop = mp_bstop;
    if (p_bstop)
    {
        *p_bstop = true;
    }
}

void protocol_rtsp_pull::handle_thread(std::string url, i_stream_ptr p_stream, std::shared_ptr<std::atomic_bool> p_bstop)
{
    LOG_INFO("开始处理视频流:" << url);
    auto p_info = std::make_shared<info_av>();
    if (0 != avformat_open_input(&p_info->pi_fmt_ctx, url.c_str(), nullptr, nullptr))
    {
        LOG_ERROR("打开url失败:" << url);
        return;
    }
    if(!p_stream->before_stream(p_info)){
        return;
    }
    p_info->p_frame = av_frame_alloc();

    printf("---------------- 输入文件信息 ---------------\n");
    av_dump_format(p_info->pi_fmt_ctx, 0, url.c_str(), 0);
    printf("-------------------------------------------------\n");

    p_info->p_packet = (AVPacket *)av_malloc(sizeof(AVPacket));

    while (!(*p_bstop))
    {
        auto ret = av_read_frame(p_info->pi_fmt_ctx, p_info->p_packet);
        if (0 > ret)
        {
            if ((ret == AVERROR_EOF) || avio_feof(p_info->pi_fmt_ctx->pb))
            {
                // 已经读取完了
            }
            else
            {
            }
            break;
        }
        av_packet_unref(p_info->p_packet);
    }

    p_stream->after_stream(p_info);

    avformat_close_input(&p_info->pi_fmt_ctx);
    LOG_INFO("结束处理视频流:" << url);
}