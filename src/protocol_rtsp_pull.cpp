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
bool protocol_rtsp_pull::do_stream(info_stream_ptr)
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
    auto p_info = std::make_shared<info_stream>();
    if (0 != avformat_open_input(&p_info->pi_fmt_ctx, url.c_str(), nullptr, nullptr))
    {
        LOG_ERROR("打开url失败:" << url);
        return;
    }
    if (0 > avformat_find_stream_info(p_info->pi_fmt_ctx, nullptr))
    {
        LOG_ERROR("获取流信息失败");
        avformat_close_input(&p_info->pi_fmt_ctx);
        return;
    }
    AVCodec* pi_code = nullptr;
    p_info->index_video = av_find_best_stream(p_info->pi_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &(pi_code), 0);
    if (0 > p_info->index_video)
    {
        LOG_ERROR("获取视频索引失败");
        avformat_close_input(&p_info->pi_fmt_ctx);
        return;
    }
    p_info->pi_stream = p_info->pi_fmt_ctx->streams[p_info->index_video];
    p_info->pi_code_ctx = avcodec_alloc_context3(pi_code);
    avcodec_parameters_to_context(p_info->pi_code_ctx, p_info->pi_stream->codecpar);
    p_info->pi_code_ctx->framerate = av_guess_frame_rate(p_info->pi_fmt_ctx, p_info->pi_stream, nullptr);
    auto ret = avcodec_open2(p_info->pi_code_ctx, pi_code, nullptr);

    printf("---------------- 文件信息 ---------------\n");
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

        if (p_info->p_packet->stream_index == p_info->index_video)
        {
            p_stream->do_stream(p_info);
        }
        av_packet_unref(p_info->p_packet);
    }

    avformat_close_input(&p_info->pi_fmt_ctx);
    LOG_INFO("结束处理视频流:" << url);
}