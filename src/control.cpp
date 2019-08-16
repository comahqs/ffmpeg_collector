#include "control.h"
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}
#include "utility_tool.h"


control::control(const std::string& url_input, const std::string& url_output) : m_url_input(url_input), m_url_output(url_output)
{
}

bool control::start()
{
    mp_bstop = std::make_shared<std::atomic_bool>(false);
    m_thread = std::thread(std::bind(control::handle_thread, this, m_url_input, m_url_output, mp_bstop));
}

void control::stop()
{
    *mp_bstop = true;
    m_thread.join();
}

void control::wait(){
    m_thread.join();
}

int control::start_input(info_av_ptr& p_info, const std::string& url_input){
    auto ret = avformat_open_input(&p_info->pi_fmt_ctx, url_input.c_str(), nullptr, nullptr);
    if (0 != ret)
    {
        LOG_ERROR("打开url失败;  错误代码:" << ret << "; 路径:" << url_input);
        return ES_UNKNOW;
    }
    p_info->p_packet = av_packet_alloc();
    p_info->p_frame = av_frame_alloc();

    ret = avformat_find_stream_info(p_info->pi_fmt_ctx, nullptr);
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
        p_info_stream->pi_stream = p_info->pi_fmt_ctx->streams[index_stream];
        p_info_stream->pi_code_ctx = avcodec_alloc_context3(pi_code);
        avcodec_parameters_to_context(p_info_stream->pi_code_ctx, p_info_stream->pi_stream->codecpar);
        p_info_stream->pi_code_ctx->framerate = av_guess_frame_rate(p_info->pi_fmt_ctx, p_info_stream->pi_stream, nullptr);
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
        p_info_stream->pi_stream = p_info->pi_fmt_ctx->streams[index_stream];
        p_info_stream->pi_code_ctx = avcodec_alloc_context3(pi_code);
        avcodec_parameters_to_context(p_info_stream->pi_code_ctx, p_info_stream->pi_stream->codecpar);
        p_info_stream->pi_code_ctx->framerate = av_guess_frame_rate(p_info->pi_fmt_ctx, p_info_stream->pi_stream, nullptr);
        auto ret = avcodec_open2(p_info_stream->pi_code_ctx, pi_code, nullptr);
        p_info->streams[index_stream] = p_info_stream;
    }

    printf("---------------- 输入文件信息 ---------------\n");
    av_dump_format(p_info->pi_fmt_ctx, 0, url_input.c_str(), 0);
    printf("-------------------------------------------------\n");
    return ES_SUCCESS;
}

int control::start_output(info_av_ptr& p_info, const std::string& url_output){
    auto ret = ES_SUCCESS;
    if (0 > avformat_alloc_output_context2(&p_info->po_fmt_ctx, nullptr, "flv", url_output.c_str()))
    {
        LOG_ERROR("打开输出流失败");
        return ES_UNKNOW;
    }
    //p_info->po_fmt_ctx->duration = p_info->pi_fmt_ctx->duration;
    //p_info->po_fmt_ctx->bit_rate = p_info->pi_fmt_ctx->bit_rate;

    if (!(p_info->po_fmt_ctx->oformat->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&(p_info->po_fmt_ctx->pb), url_output.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0)
        {
            return ES_UNKNOW;
        }
    }

    for (auto &p_info_stream : p_info->streams)
    {
        if (AVMEDIA_TYPE_VIDEO == p_info_stream->pi_code_ctx->codec_type)
        {
            p_info_stream->po_stream = avformat_new_stream(p_info->po_fmt_ctx, nullptr);
            if (nullptr == p_info_stream->po_stream)
            {
                LOG_ERROR("新建输出流失败");
                return ES_UNKNOW;
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
            p_info_stream->po_stream->avg_frame_rate = p_info_stream->pi_stream->avg_frame_rate;
            p_info_stream->po_stream->r_frame_rate = p_info_stream->pi_stream->r_frame_rate;
        }
        else if (AVMEDIA_TYPE_AUDIO == p_info_stream->pi_code_ctx->codec_type)
        {
            p_info_stream->po_stream = avformat_new_stream(p_info->po_fmt_ctx, nullptr);
            if (nullptr == p_info_stream->po_stream)
            {
                LOG_ERROR("新建输出流失败");
                return ES_UNKNOW;
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

    ret = avformat_write_header(p_info->po_fmt_ctx, nullptr);

    printf("---------------- 输出文件信息 ---------------\n");
    av_dump_format(p_info->po_fmt_ctx, 0, url_output.c_str(), 1);
    printf("-------------------------------------------------\n");
    return ES_SUCCESS;
}

int control::do_stream(info_av_ptr& p_info){
    auto ret = av_read_frame(p_info->pi_fmt_ctx, p_info->p_packet);
    if (0 > ret)
    {
        if ((ret == AVERROR_EOF) || avio_feof(p_info->pi_fmt_ctx->pb))
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
    ret = decode(p_info);
    if (nullptr != p_info->p_packet)
    {
        av_packet_unref(p_info->p_packet);
    }
}

int control::decode(info_av_ptr& p_info){
    auto ret = ES_SUCCESS;
    if (0 <= p_info->p_packet->stream_index || static_cast<int>(p_info->streams.size()) > p_info->p_packet->stream_index)
    {
        auto p_stream = p_info->streams[ p_info->p_packet->stream_index];
        if (nullptr != p_stream)
        {

            if (AVMEDIA_TYPE_VIDEO == p_stream->pi_code_ctx->codec_type)
            {
                ret = decode_video(p_info);
            }
            else if (AVMEDIA_TYPE_AUDIO == p_stream->pi_code_ctx->codec_type)
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

int control::decode_video(info_av_ptr& p_info)
{
    int ret = 0;
    auto p_stream = p_info->streams[ p_info->p_packet->stream_index];
    while (true)
    {
        if (p_info->p_packet->data != nullptr) // not a flush packet
        {
            av_packet_rescale_ts(p_info->p_packet, p_stream->pi_stream->time_base, p_stream->pi_code_ctx->time_base);
        }
        ret = avcodec_send_packet(p_stream->pi_code_ctx, p_info->p_packet);
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
            ret = avcodec_receive_frame(p_stream->pi_code_ctx, p_info->p_frame);
            if (0 != ret)
            {
                if (AVERROR_EOF == ret)
                {
                    avcodec_flush_buffers(p_stream->pi_code_ctx);
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

            ret = encode(p_info);
        }
    }
    return ES_SUCCESS;
}

int control::decode_audio(info_av_ptr& p_info)
{
    return encode(p_info);
}

int control::encode(info_av_ptr& p_info)
{
    auto ret = ES_SUCCESS;
    auto p_stream = p_info->streams[ p_info->p_packet->stream_index];
    if (AVMEDIA_TYPE_VIDEO == p_stream->pi_code_ctx->codec_type)
    {
        ret = encode_video(p_info);
    }
    else if (AVMEDIA_TYPE_AUDIO == p_stream->pi_code_ctx->codec_type)
    {
        ret = encode_audio(p_info);
    }
    else
    {
        return ES_UNKNOW;
    }
    if(ES_SUCCESS != ret){
        return ret;
    }
    return ret;
}

int control::encode_video(info_av_ptr& p_info)
{
    auto p_stream = p_info->streams[ p_info->p_packet->stream_index];
    auto ret = avcodec_send_frame(p_stream->po_code_ctx, p_info->p_frame);
    if (0 > ret)
    {
        if (AVERROR_EOF == ret)
        {
            avcodec_flush_buffers(p_stream->pi_code_ctx);
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
    AVPacket packet;
    av_init_packet(&packet);
    ret = avcodec_receive_packet(p_stream->po_code_ctx, &packet);
    if (0 > ret)
    {
        return ES_UNKNOW;
    }
    packet.stream_index = p_stream->po_stream->index;
    av_packet_rescale_ts(&packet, p_stream->po_code_ctx->time_base, p_stream->po_stream->time_base);

    auto p_packet_old = p_info->p_packet;
    p_info->p_packet = &packet;
    ret =  output(p_info);
    p_info->p_packet = p_packet_old;
    return ret;
}

int control::encode_audio(info_av_ptr& p_info)
{
    return output(p_info);
}

int control::output(info_av_ptr& p_info)
{   
    if (!(p_info->p_packet))
    {
        // 结束了
        return ES_SUCCESS;
    }
    auto ret = av_interleaved_write_frame(p_info->po_fmt_ctx, p_info->p_packet);
    return ES_SUCCESS;
}

int control::stop_input(info_av_ptr& p_info)
{
    if (nullptr != p_info->p_frame)
    {
        av_frame_free(&p_info->p_frame);
        p_info->p_frame = nullptr;
    }
    if (nullptr != p_info->p_packet)
    {
        av_packet_free(&p_info->p_packet);
        p_info->p_packet = nullptr;
    }

    if (nullptr != p_info->pi_fmt_ctx)
    {
        avformat_close_input(&p_info->pi_fmt_ctx);
        p_info->pi_fmt_ctx = nullptr;
    }

    return ES_SUCCESS;
}

int control::stop_output(info_av_ptr& p_info)
{
    if(nullptr != p_info->po_fmt_ctx){
        auto ret = av_write_trailer(p_info->po_fmt_ctx);
        avformat_free_context(p_info->po_fmt_ctx);
        p_info->po_fmt_ctx = nullptr;
    }
    return ES_SUCCESS;
}

void control::handle_thread(const std::string& url_input, const std::string& url_output, std::shared_ptr<std::atomic_bool> pflag_stop)
{
    LOG_INFO("开始处理视频流:" << url_input);
    auto p_info = std::make_shared<info_av>();
    auto ret = ES_SUCCESS;
    if (ES_SUCCESS != (ret = start_input(p_info, url_input.c_str())) || ES_SUCCESS != (ret = start_output(p_info, url_output)))
    {
        return;
    }
    
    while (!(*pflag_stop))
    {
        ret = do_stream(p_info);
        if (ES_SUCCESS == ret)
        {
        }
        else if (ES_AGAIN == ret)
        {
            continue;
        }
        else if (ES_EOF == ret)
        {
            break;
        }
        else
        {
            break;
        }
    }
    if (ES_SUCCESS != (ret = stop_output(p_info) || ES_SUCCESS != (ret = stop_input(p_info))))
    {
        return;
    }
}