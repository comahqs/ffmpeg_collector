#include "control_complex.h"
#include "utility_tool.h"
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

bool operator<(const key_complex &k1, const key_complex &k2)
{
    if (k1.p_info.get() < k2.p_info.get())
    {
        return true;
    }
    else if (k1.p_info.get() == k2.p_info.get())
    {
        return k1.type < k2.type;
    }
    return false;
}

bool less_key_complex::operator()(const key_complex &k1, const key_complex &k2) const
{
    if (k1.p_info.get() < k2.p_info.get())
    {
        return true;
    }
    else if (k1.p_info.get() == k2.p_info.get())
    {
        return k1.type < k2.type;
    }
    return false;
}

control_complex::control_complex(const std::vector<std::string> &url_input, const std::string &url_output) : control("", url_output), m_url_input(url_input), m_url_output(url_output)
{
}

bool control_complex::start()
{
    mp_bstop = std::make_shared<std::atomic_bool>(false);
    m_thread = std::thread(std::bind(&control_complex::handle_thread_complex, this, m_url_input, m_url_output, mp_bstop));
}

void control_complex::stop()
{
    *mp_bstop = true;
    m_thread.join();
}

void control_complex::wait()
{
    m_thread.join();
}

int control_complex::encode(info_control_complex_ptr &p_info)
{
    int ret = ES_SUCCESS;
    if (p_info->inputs[0]->p_packet->stream_index == p_info->inputs[0]->p_video_stream->index)
    {
        if (ES_SUCCESS != (ret = encode_video(p_info)))
        {
            return ret;
        }
    }
    else if (p_info->inputs[0]->p_packet->stream_index == p_info->inputs[0]->p_audio_stream->index)
    {
        if (ES_SUCCESS != (ret = encode_audio(p_info)))
        {
            return ret;
        }
    }
    return ES_SUCCESS;
}

void control_complex::handle_thread_complex(const std::vector<std::string> &url_input, const std::string &url_output, std::shared_ptr<std::atomic_bool> pflag_stop)
{
    try
    {
        auto p_info = std::make_shared<info_control_complex>();
        auto ret = ES_SUCCESS;
        if (ES_SUCCESS != (ret = start_input(p_info, url_input)) || ES_SUCCESS != (ret = start_output(p_info, url_output, p_info->inputs[0]->p_video_code_ctx, p_info->inputs[0]->p_audio_code_ctx)))
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
        ret = stop_output(p_info);
        ret = stop_input(p_info);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("线程发生错误:" << e.what());
    }
}

int control_complex::start_input(info_control_complex_ptr &p_info, const std::vector<std::string> &url_input)
{
    int ret = ES_SUCCESS;
    for (std::size_t i = 0; i < url_input.size(); ++i)
    {
        auto p_gather = std::make_shared<info_gather>();
        ret = avformat_open_input(&p_gather->p_fmt_ctx, url_input[i].c_str(), nullptr, nullptr);
        if (0 != ret)
        {
            LOG_ERROR("打开url失败;  错误代码:" << ret << "; 路径:" << url_input[i]);
            return ES_UNKNOW;
        }
        ret = avformat_find_stream_info(p_gather->p_fmt_ctx, nullptr);
        if (0 > ret)
        {
            LOG_ERROR("获取流信息失败; 错误代码:" << ret);
            return ES_UNKNOW;
        }

        {
            AVCodec *pi_code = nullptr;
            auto index_stream = av_find_best_stream(p_gather->p_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &(pi_code), 0);
            if (0 > index_stream)
            {
                LOG_ERROR("获取视频索引失败:" << index_stream);
                return ES_UNKNOW;
            }
            p_gather->p_video_stream = p_gather->p_fmt_ctx->streams[index_stream];
            p_gather->p_video_code_ctx = avcodec_alloc_context3(pi_code);
            avcodec_parameters_to_context(p_gather->p_video_code_ctx, p_gather->p_video_stream->codecpar);
            p_gather->p_video_code_ctx->framerate = av_guess_frame_rate(p_gather->p_fmt_ctx, p_gather->p_video_stream, nullptr);
            auto ret = avcodec_open2(p_gather->p_video_code_ctx, pi_code, nullptr);
        }

        {
            AVCodec *pi_code = nullptr;
            auto index_stream = av_find_best_stream(p_gather->p_fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &(pi_code), 0);
            if (0 > index_stream)
            {
                LOG_ERROR("获取音频索引失败:" << index_stream);
                return ES_UNKNOW;
            }
            p_gather->p_audio_stream = p_gather->p_fmt_ctx->streams[index_stream];
            p_gather->p_audio_code_ctx = avcodec_alloc_context3(pi_code);
            avcodec_parameters_to_context(p_gather->p_audio_code_ctx, p_gather->p_audio_stream->codecpar);
            p_gather->p_audio_code_ctx->framerate = av_guess_frame_rate(p_gather->p_fmt_ctx, p_gather->p_audio_stream, nullptr);
            auto ret = avcodec_open2(p_gather->p_audio_code_ctx, pi_code, nullptr);
        }
        p_info->inputs.push_back(p_gather);

        printf("---------------- 输入文件信息 ---------------\n");
        av_dump_format(p_gather->p_fmt_ctx, 0, url_input[i].c_str(), 0);
        printf("-------------------------------------------------\n");
    }

    return ES_SUCCESS;
}

int control_complex::start_output(info_control_complex_ptr &p_info, const std::string &url_output, const AVCodecContext *pi_video_code_ctx, const AVCodecContext *pi_audio_code_ctx)
{
    auto ret = ES_SUCCESS;
    auto p_gather = std::make_shared<info_gather>();
    if (0 > avformat_alloc_output_context2(&p_gather->p_fmt_ctx, nullptr, "flv", url_output.c_str()))
    {
        LOG_ERROR("打开输出流失败");
        return ES_UNKNOW;
    }
    //p_info->po_fmt_ctx->duration = p_info->pi_fmt_ctx->duration;
    //p_info->po_fmt_ctx->bit_rate = p_info->pi_fmt_ctx->bit_rate;

    if (!(p_gather->p_fmt_ctx->oformat->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&(p_gather->p_fmt_ctx->pb), url_output.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0)
        {
            return ES_UNKNOW;
        }
    }

    {
        p_gather->p_video_stream = avformat_new_stream(p_gather->p_fmt_ctx, nullptr);
        if (nullptr == p_gather->p_video_stream)
        {
            LOG_ERROR("新建输出流失败");
            return ES_UNKNOW;
        }

        AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        p_gather->p_video_code_ctx = avcodec_alloc_context3(codec);
        p_gather->p_video_code_ctx->width = pi_video_code_ctx->width;   //你想要的宽度
        p_gather->p_video_code_ctx->height = pi_video_code_ctx->height; //你想要的高度
        //p_info_stream->po_code_ctx->width = 480;  //你想要的宽度
        //p_info_stream->po_code_ctx->height = 320; //你想要的高度
        p_gather->p_video_code_ctx->sample_aspect_ratio = pi_video_code_ctx->sample_aspect_ratio;
        if (nullptr != codec->pix_fmts)
        {
            p_gather->p_video_code_ctx->pix_fmt = codec->pix_fmts[0];
        }
        else
        {
            p_gather->p_video_code_ctx->pix_fmt = pi_video_code_ctx->pix_fmt;
        }
        p_gather->p_video_code_ctx->time_base = pi_video_code_ctx->time_base;
        p_gather->p_video_code_ctx->framerate = pi_video_code_ctx->framerate;
        p_gather->p_video_code_ctx->bit_rate = pi_video_code_ctx->bit_rate;

        AVDictionary *opts = nullptr;
        //av_dict_set(&opts, "profile", "baseline", 0);
        //av_dict_set(&opts, "preset", "ultrafast", 0);
        //av_dict_set(&opts, "tune", "zerolatency", 0);
        ret = avcodec_open2(p_gather->p_video_code_ctx, codec, &opts);
        if(0 > ret){
            LOG_ERROR("打开输出解码器失败:"<<ret);
            return ret;
        }
        //av_dict_free(&opts);
        //将AVCodecContext的成员复制到AVCodecParameters结构体。前后两行不能调换顺序
        avcodec_parameters_from_context(p_gather->p_video_stream->codecpar, p_gather->p_video_code_ctx);
        p_gather->p_video_stream->avg_frame_rate = p_gather->p_video_stream->avg_frame_rate;
        p_gather->p_video_stream->r_frame_rate = p_gather->p_video_stream->r_frame_rate;
        p_gather->p_video_code_ctx->gop_size = 30;
        p_gather->p_video_code_ctx->max_b_frames = 0;
        //av_opt_set(p_gather->p_video_code_ctx->priv_data, "preset", "ultrafast", 0);
        //av_opt_set(p_gather->p_video_code_ctx->priv_data, "tune", "zerolatency", 0);
    }
    {
        p_gather->p_audio_stream = avformat_new_stream(p_gather->p_fmt_ctx, nullptr);
        if (nullptr == p_gather->p_audio_stream)
        {
            LOG_ERROR("新建输出流失败");
            return ES_UNKNOW;
        }

        AVCodec *codec = avcodec_find_encoder(pi_audio_code_ctx->codec_id);
        p_gather->p_audio_code_ctx = avcodec_alloc_context3(codec);
        p_gather->p_audio_code_ctx->sample_rate = pi_audio_code_ctx->sample_rate;                                    // 采样率
        p_gather->p_audio_code_ctx->channel_layout = pi_audio_code_ctx->channel_layout;                              // 声道布局
        p_gather->p_audio_code_ctx->channels = av_get_channel_layout_nb_channels(pi_audio_code_ctx->channel_layout); // 声道数量
        p_gather->p_audio_code_ctx->sample_fmt = codec->sample_fmts[0];                                              // 编码器采用所支持的第一种采样格式
        p_gather->p_audio_code_ctx->time_base = (AVRational){1, p_gather->p_audio_code_ctx->sample_rate};            // 时基：编码器采样率取倒数

        avcodec_open2(p_gather->p_audio_code_ctx, codec, nullptr);
        //将AVCodecContext的成员复制到AVCodecParameters结构体。前后两行不能调换顺序
        avcodec_parameters_from_context(p_gather->p_audio_stream->codecpar, p_gather->p_audio_code_ctx);
    }
    p_info->outputs.push_back(p_gather);
    int step = static_cast<int>(ceil(sqrt(p_info->inputs.size())));
    auto step_height = p_gather->p_video_code_ctx->height / step;
    auto step_width = p_gather->p_video_code_ctx->width / step;
    for (auto &p_input : p_info->inputs)
    {
        p_input->p_video_sws_ctx = sws_getContext(p_input->p_video_code_ctx->width, p_input->p_video_code_ctx->height, p_input->p_video_code_ctx->pix_fmt,
                                                  step_width, step_height, p_gather->p_video_code_ctx->pix_fmt, SWS_BICUBIC, nullptr, nullptr, nullptr);

        p_input->po_frame = av_frame_alloc();
        uint8_t *po_tmp_buffer = (uint8_t *)av_malloc(av_image_get_buffer_size(p_gather->p_video_code_ctx->pix_fmt, step_width, step_height, 1));
        av_image_fill_arrays(p_input->po_frame->data, p_input->po_frame->linesize, po_tmp_buffer, p_gather->p_video_code_ctx->pix_fmt, step_width, step_height, 1);
    }

    p_gather->po_frame = av_frame_alloc();
    uint8_t *po_buffer = (uint8_t *)av_malloc(av_image_get_buffer_size(p_gather->p_video_code_ctx->pix_fmt, p_gather->p_video_code_ctx->width, p_gather->p_video_code_ctx->height, 1));
    av_image_fill_arrays(p_gather->po_frame->data, p_gather->po_frame->linesize, po_buffer, p_gather->p_video_code_ctx->pix_fmt, p_gather->p_video_code_ctx->width, p_gather->p_video_code_ctx->height, 1);
    p_gather->po_frame->width = p_gather->p_video_code_ctx->width;
    p_gather->po_frame->height = p_gather->p_video_code_ctx->height;
    p_gather->po_frame->format = p_gather->p_video_code_ctx->pix_fmt;

    ret = avformat_write_header(p_gather->p_fmt_ctx, nullptr);

    printf("---------------- 输出文件信息 ---------------\n");
    av_dump_format(p_gather->p_fmt_ctx, 0, url_output.c_str(), 1);
    printf("-------------------------------------------------\n");
    return ES_SUCCESS;
}

int control_complex::do_stream(info_control_complex_ptr &p_info)
{
    int ret = ES_SUCCESS;
    auto p_output = p_info->outputs[0];
    for (auto &p_gather : p_info->inputs)
    {
        if(nullptr == p_gather->p_packet){
            p_gather->p_packet = av_packet_alloc();
        }else{
            av_packet_unref(p_gather->p_packet);
        }
        ret = av_read_frame(p_gather->p_fmt_ctx, p_gather->p_packet);
        if (0 != ret)
        {
            break;
        }
        ret = decode(p_gather);
        if (0 != ret)
        {
            break;
        }
    }

    if (0 > ret)
    {
        /*
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
            */
        return ret;
    }

    ret = encode(p_info);
    return ret;
}

int control_complex::decode(info_gather_ptr &p_input)
{
    auto ret = ES_SUCCESS;
    if (p_input->p_video_stream->index == p_input->p_packet->stream_index)
    {
        ret = decode_video(p_input);
    }
    else if (p_input->p_audio_stream->index == p_input->p_packet->stream_index)
    {
        ret = decode_audio(p_input);
    }
    else
    {
        return ES_UNKNOW;
    }

    if (ES_SUCCESS != ret)
    {
        return ret;
    }

    return ret;
}

int control_complex::decode_video(info_gather_ptr &p_info)
{
    int ret = ES_SUCCESS;
    if (p_info->p_packet->data != nullptr) // not a flush packet
    {
        av_packet_rescale_ts(p_info->p_packet, p_info->p_video_stream->time_base, p_info->p_video_code_ctx->time_base);
    }
    ret = avcodec_send_packet(p_info->p_video_code_ctx, p_info->p_packet);
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
        AVFrame *p_frame = av_frame_alloc();
        ret = avcodec_receive_frame(p_info->p_video_code_ctx, p_frame);
        if (0 != ret)
        {
            av_frame_free(&p_frame);
            if (AVERROR_EOF == ret)
            {
                avcodec_flush_buffers(p_info->p_video_code_ctx);
                return ES_EOF;
            }
            else if (AVERROR(EAGAIN) == ret)
            {
                break;
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
        if (nullptr == p_info->p_frame)
        {
            p_info->p_frame = p_frame;
        }
        else
        {
            av_frame_free(&p_info->p_frame);
            p_info->p_frame = p_frame;
        }
    }
    return ES_SUCCESS;
}

int control_complex::decode_audio(info_gather_ptr &p_info)
{
    /*
    if (nullptr != p_info->p_audio_packet)
    {
        av_packet_free(&p_info->p_audio_packet);
        p_info->p_audio_packet = nullptr;
    }
    p_info->p_audio_packet = av_packet_clone(p_info->p_packet);
    */
    return ES_SUCCESS;
}

int control_complex::encode_video(info_control_complex_ptr p_info)
{
    auto ret = ES_SUCCESS;

    bool flag_video = false;
    for (std::size_t i = 0; i < p_info->inputs.size(); ++i)
    {
        if (nullptr != p_info->inputs[i]->p_frame)
        {
            flag_video = true;
            break;
        }
    }
    if (!flag_video)
    {
        return ES_SUCCESS;
    }
    auto po_gather = p_info->outputs[0];

    int step = static_cast<int>(ceil(sqrt(p_info->inputs.size())));
    //auto step_height = po_gather->p_video_code_ctx->height / step;
    //auto step_width = po_gather->p_video_code_ctx->width / step;
    // 先缩放，然后再拼接
    auto step_width = po_gather->po_frame->linesize[0] / step;
    auto step_height = po_gather->po_frame->linesize[1] / step;

    memset(po_gather->po_frame->data[0], 0, po_gather->po_frame->width * po_gather->po_frame->height);
    memset(po_gather->po_frame->data[1], 0x80, po_gather->po_frame->width * po_gather->po_frame->height / 4);
    memset(po_gather->po_frame->data[2], 0x80, po_gather->po_frame->width * po_gather->po_frame->height / 4);

    for (std::size_t i = 0; i < p_info->inputs.size(); ++i)
    {
        auto p_input = p_info->inputs[i];
        auto p_frame = (nullptr != p_input->p_frame ? p_input->p_frame : p_input->p_frame_last);
        if (nullptr == p_frame)
        {
            continue;
        }

        sws_scale(p_input->p_video_sws_ctx, (const uint8_t *const *)p_frame->data, p_frame->linesize, 0, p_input->p_video_code_ctx->height,
                  p_input->po_frame->data, p_input->po_frame->linesize);
        int row = static_cast<int>(i / step);
        int column = static_cast<int>(i % step);
        for (int j = 0; j < step_height; ++j)
        {
            memcpy(po_gather->po_frame->data[0] + row * step_height * step_width * step + column * step_width + j * step_width * step, p_input->po_frame->data[0] + j * step_width, step_width);
        }
        for (int j = 0; j < step_height / 2; ++j)
        {
            memcpy(po_gather->po_frame->data[1] + row * step_height * step_width * step / 4 + column * step_width / 2 + j * step_width * step / 2, p_input->po_frame->data[1] + j * step_width / 2, step_width / 2);
            memcpy(po_gather->po_frame->data[2] + row * step_height * step_width * step / 4 + column * step_width / 2 + j * step_width * step / 2, p_input->po_frame->data[2] + j * step_width / 2, step_width / 2);
        }
        po_gather->po_frame->pts = std::max(po_gather->po_frame->pts, p_frame->pts);

        //av_frame_free(&p_input->p_frame);
        //p_input->p_frame = nullptr;
        if (nullptr != p_input->p_frame)
        {
            if (nullptr != p_input->p_frame_last)
            {
                av_frame_free(&p_input->p_frame_last);
            }
            p_input->p_frame_last = p_input->p_frame;
            p_input->p_frame = nullptr;
        }
    }

    static int c = 0, s = 0;
    ++c;
    auto p_frame = (nullptr != p_info->inputs[0]->p_frame ? p_info->inputs[0]->p_frame : p_info->inputs[0]->p_frame_last);
    //LOG_DEBUG(c<<" - "<<s<<" -> "<<p_frame->pict_type);
    if (m_video_pts_last >= po_gather->po_frame->pts)
    {
        po_gather->po_frame->pts++;
        //return ES_SUCCESS;
    }
    m_video_pts_last = po_gather->po_frame->pts;

    ret = avcodec_send_frame(po_gather->p_video_code_ctx, po_gather->po_frame);
    //LOG_DEBUG(po_frame->pts);

    if (0 > ret)
    {
        if (AVERROR_EOF == ret)
        {
            //avcodec_flush_buffers(po_gather->p_video_code_ctx);
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
    ret = ES_SUCCESS;
    while (ES_SUCCESS == ret)
    {
        ret = avcodec_receive_packet(po_gather->p_video_code_ctx, &packet);
        if (0 == ret)
        {
        }
        else if (AVERROR(EAGAIN) == ret)
        {
            break;
        }
        else if (0 > ret)
        {
            return ES_UNKNOW;
        }
        //++s;
        //LOG_DEBUG(c<<" - "<<++s<<" -> "<<(nullptr != p_info->inputs[0]->p_frame ? p_info->inputs[0]->p_frame : p_info->inputs[0]->p_frame_last)->pict_type);
        packet.stream_index = po_gather->p_video_stream->index;
        av_packet_rescale_ts(&packet, po_gather->p_video_code_ctx->time_base, po_gather->p_video_stream->time_base);
        ret = output(&packet, po_gather);

        av_packet_unref(&packet);
    }

    return ret;
}
int control_complex::encode_audio(info_control_complex_ptr &p_info)
{
    int ret = ES_SUCCESS;
    if (0 < p_info->inputs.size() && 0 < p_info->outputs.size())
    {
        auto p_input = p_info->inputs[0];
        auto p_output = p_info->outputs[0];
        p_input->p_packet->stream_index = p_output->p_audio_stream->index;
        ret = output(p_input->p_packet, p_output);
    }
    return ES_SUCCESS;
}

int control_complex::output(AVPacket *p_packet, info_gather_ptr po_gather)
{
    auto ret = av_interleaved_write_frame(po_gather->p_fmt_ctx, p_packet);
    return ret;
}

int control_complex::stop_input(info_control_complex_ptr &p_info)
{
    return ES_SUCCESS;
}

int control_complex::stop_output(info_control_complex_ptr &p_info)
{
    return ES_SUCCESS;
}