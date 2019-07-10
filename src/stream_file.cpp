#include "stream_file.h"
#include "utility_tool.h"
#include <libavformat/avformat.h>

stream_file::stream_file(const std::string &file_path) : m_file_path(file_path)
{
}

bool stream_file::start()
{
    return true;
}

void stream_file::stop()
{
    if(nullptr != mp_fmt_ctx){
        av_write_trailer(mp_fmt_ctx);
    }

    if(nullptr != mp_stream){
        //avcodec_close(mp_stream);
        mp_stream = nullptr;
    }
    if(nullptr != mp_fmt_ctx){
        avformat_free_context(mp_fmt_ctx);
        mp_fmt_ctx = nullptr;
    }
}

bool stream_file::do_stream(info_stream_ptr &p_info)
{
    if (nullptr == mp_fmt_ctx)
    {
        // 初始化
        if (0 > avformat_alloc_output_context2(&mp_fmt_ctx, nullptr, nullptr, m_file_path.c_str()))
        {
            LOG_ERROR("打开输出流失败");
            return false;
        }
        mp_stream = avformat_new_stream(p_info->p_fmt_ctx, nullptr);
        if (nullptr == mp_stream)
        {
            LOG_ERROR("新建输出流失败");
            return false;
        }
        auto *codec = avcodec_find_encoder(mp_fmt_ctx->oformat->video_codec);
        auto *codec_ctx = avcodec_alloc_context3(codec);

        codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
        codec_ctx->codec_id = mp_fmt_ctx->oformat->video_codec;

        codec_ctx->width = 1280;                 //你想要的宽度
        codec_ctx->height = 720;                 //你想要的高度
        codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P; //受codec->pix_fmts数组限制

        codec_ctx->gop_size = 12;

        codec_ctx->time_base = AVRational{1, 25}; //应该根据帧率设置
        codec_ctx->bit_rate = 1400 * 1000;

        avcodec_open2(codec_ctx, codec, nullptr);
        //将AVCodecContext的成员复制到AVCodecParameters结构体。前后两行不能调换顺序
        avcodec_parameters_from_context(mp_stream->codecpar, codec_ctx);
        avcodec_free_context(&codec_ctx);

        av_stream_set_r_frame_rate(mp_stream, {1, 25});
        //AVDictionary *opt = nullptr;
        //av_dict_set_int(&opt, "video_track_timescale", 25, 0);
        //avformat_write_header(mp_fmt_ctx, &opt);
        avformat_write_header(mp_fmt_ctx, nullptr);
    }

    if(nullptr != p_info->p_packet){
        av_interleaved_write_frame(mp_fmt_ctx, p_info->p_packet);
    }
    return true;
}