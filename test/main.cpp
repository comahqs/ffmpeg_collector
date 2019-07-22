#include <iostream>
#include <unistd.h>
#include "../src/utility_tool.h"

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

using namespace std;

#include <sys/time.h>
#define TIME_DEN 1000

// Returns the local date/time formatted as 2014-03-19 11:11:52
char *getFormattedTime(void);

// Remove path from filename
#define __SHORT_FILE__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

// Main log macro
//#define __LOG__(format, loglevel, ...) printf("%s %-5s [%s] [%s:%d] " format "\n", getFormattedTime(), loglevel, __func__, __SHORT_FILE__, __LINE__, ## __VA_ARGS__)
#define __LOG__(format, loglevel, ...) printf("%ld %-5s [%s] [%s:%d] " format "\n", current_timestamp(), loglevel, __func__, __SHORT_FILE__, __LINE__, ##__VA_ARGS__)

// Specific log macros with
#define LOGDEBUG(format, ...) __LOG__(format, "DEBUG", ##__VA_ARGS__)
#define LOGWARN(format, ...) __LOG__(format, "WARN", ##__VA_ARGS__)
#define LOGERROR(format, ...) __LOG__(format, "ERROR", ##__VA_ARGS__)
#define LOGINFO(format, ...) __LOG__(format, "INFO", ##__VA_ARGS__)

// Returns the local date/time formatted as 2014-03-19 11:11:52
char *getFormattedTime(void)
{

    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // Must be static, otherwise won't work
    static char _retval[26];
    strftime(_retval, sizeof(_retval), "%Y-%m-%d %H:%M:%S", timeinfo);

    return _retval;
}

long long current_timestamp()
{
    struct timeval te;
    gettimeofday(&te, NULL);                                         // get current time
    long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000; // calculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}

static int encode_and_save_pkt(AVCodecContext *enc_ctx, AVFormatContext *ofmt_ctx, AVStream *out_stream)
{
    AVPacket enc_pkt;
    av_init_packet(&enc_pkt);
    enc_pkt.data = NULL;
    enc_pkt.size = 0;

    int ret = 0;
    while (ret >= 0)
    {
        ret = avcodec_receive_packet(enc_ctx, &enc_pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            ret = 0;
            break;
        }

        else if (ret < 0)
        {
            printf("[avcodec_receive_packet]Error during encoding, ret:%d\n", ret);
            break;
        }
        //LOGDEBUG("encode type:%d pts:%d dts:%d\n", enc_ctx->codec_type, enc_pkt.pts, enc_pkt.dts);

        /* rescale output packet timestamp values from codec to stream timebase */
        av_packet_rescale_ts(&enc_pkt, enc_ctx->time_base, out_stream->time_base);
        enc_pkt.stream_index = out_stream->index;

        ret = av_interleaved_write_frame(ofmt_ctx, &enc_pkt);
        if (ret < 0)
        {
            printf("write frame error, ret:%d\n", ret);
            break;
        }

        av_packet_unref(&enc_pkt);
    }
    return ret;
}

static int decode_and_send_frame(AVCodecContext *dec_ctx, AVCodecContext *enc_ctx)
{
    AVFrame *frame = av_frame_alloc();
    int ret = 0;

    while (ret >= 0)
    {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            ret = 0;
            break;
        }
        else if (ret < 0)
        {
            printf("Error while receiving a frame from the decoder");
            break;
        }
        int pts = frame->pts;
        frame->pict_type = AV_PICTURE_TYPE_NONE;
        // 修改pts
        //frame->pts  = pts - start*TIME_DEN;
        //printf("pts:%d\n", frame->pts);

        ret = avcodec_send_frame(enc_ctx, frame);
        if (ret < 0)
        {
            printf("Error sending a frame for encoding\n");
            break;
        }
    }
    av_frame_free(&frame);
    return ret;
}

static int open_decodec_context(int *stream_idx,
                                AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type)
{
    int ret, stream_index;
    AVStream *st;
    AVCodec *dec = NULL;
    AVDictionary *opts = NULL;

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, &dec, 0);
    if (ret < 0)
    {
        fprintf(stderr, "Could not find %s stream in input file \n",
                av_get_media_type_string(type));
        return ret;
    }
    else
    {
        stream_index = ret;
        st = fmt_ctx->streams[stream_index];

        /* Allocate a codec context for the decoder */
        *dec_ctx = avcodec_alloc_context3(dec);
        if (!*dec_ctx)
        {
            fprintf(stderr, "Failed to allocate the %s codec context\n",
                    av_get_media_type_string(type));
            return AVERROR(ENOMEM);
        }

        /* Copy codec parameters from input stream to output codec context */
        if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0)
        {
            fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
                    av_get_media_type_string(type));
            return ret;
        }

        if ((*dec_ctx)->codec_type == AVMEDIA_TYPE_VIDEO)
            (*dec_ctx)->framerate = av_guess_frame_rate(fmt_ctx, st, NULL);

        /* Init the decoders, with or without reference counting */
        av_dict_set_int(&opts, "refcounted_frames", 1, 0);
        if ((ret = avcodec_open2(*dec_ctx, dec, &opts)) < 0)
        {
            fprintf(stderr, "Failed to open %s codec\n",
                    av_get_media_type_string(type));
            return ret;
        }
        *stream_idx = stream_index;
    }

    return 0;
}

static int set_encode_option(AVCodecContext *dec_ctx, AVDictionary **opt)
{
    const char *profile = avcodec_profile_name(dec_ctx->codec_id, dec_ctx->profile);
    if (profile)
    {
        if (!strcasecmp(profile, "high"))
        {
            av_dict_set(opt, "profile", "high", 0);
        }
    }
    else
    {
        av_dict_set(opt, "profile", "main", 0);
    }

    av_dict_set(opt, "threads", "16", 0);

    av_dict_set(opt, "preset", "slow", 0);
    av_dict_set(opt, "level", "4.0", 0);

    return 0;
}

static int open_encodec_context(int stream_index, AVCodecContext **oenc_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type)
{
    int ret;
    AVStream *st;
    AVCodec *encoder = NULL;
    AVDictionary *opts = NULL;
    AVCodecContext *enc_ctx;

    st = fmt_ctx->streams[stream_index];

    /* find encoder for the stream */
    encoder = avcodec_find_encoder(st->codecpar->codec_id);
    if (!encoder)
    {
        fprintf(stderr, "Failed to find %s codec\n",
                av_get_media_type_string(type));
        return AVERROR(EINVAL);
    }

    enc_ctx = avcodec_alloc_context3(encoder);
    if (!enc_ctx)
    {
        printf("Failed to allocate the encoder context\n");
        return AVERROR(ENOMEM);
    }

    AVCodecContext *dec_ctx = st->codec;
    if (type == AVMEDIA_TYPE_VIDEO)
    {
        enc_ctx->height = dec_ctx->height;
        enc_ctx->width = dec_ctx->width;
        enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;

        enc_ctx->bit_rate = dec_ctx->bit_rate;
        //enc_ctx->rc_max_rate = dec_ctx->bit_rate;
        //enc_ctx->rc_buffer_size = dec_ctx->bit_rate;
        enc_ctx->bit_rate_tolerance = 0;
        // use yuv420P
        enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
        // set frame rate
        enc_ctx->time_base.num = 1;
        enc_ctx->time_base.den = TIME_DEN;

        enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        enc_ctx->has_b_frames = false;
        enc_ctx->max_b_frames = 0;
        enc_ctx->gop_size = 120;

        set_encode_option(dec_ctx, &opts);
    }
    else if (type == AVMEDIA_TYPE_AUDIO)
    {
        enc_ctx->sample_rate = dec_ctx->sample_rate;
        enc_ctx->channel_layout = dec_ctx->channel_layout;
        enc_ctx->channels = av_get_channel_layout_nb_channels(enc_ctx->channel_layout);
        /* take first format from list of supported formats */
        enc_ctx->sample_fmt = encoder->sample_fmts[0];
        enc_ctx->time_base = (AVRational){1, TIME_DEN};
        enc_ctx->bit_rate = dec_ctx->bit_rate;
        enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    else
    {
        ret = avcodec_copy_context(enc_ctx, st->codec);
        if (ret < 0)
        {
            fprintf(stderr, "Failed to copy context from input to output stream codec context\n");
            return ret;
        }
    }

    if ((ret = avcodec_open2(enc_ctx, encoder, &opts)) < 0)
    {
        fprintf(stderr, "Failed to open %s codec\n",
                av_get_media_type_string(type));
        return ret;
    }

    *oenc_ctx = enc_ctx;
    return 0;
}
int test_cut(char *input_file, char *output_file)
{
    int ret = 0;
    // 输入流
    AVFormatContext *ifmt_ctx = NULL;
    AVCodecContext *video_dec_ctx = NULL;
    AVCodecContext *audio_dec_ctx = NULL;
    char *flv_name = input_file;
    int video_stream_idx = 0;
    int audio_stream_idx = 1;

    // 输出流
    AVFormatContext *ofmt_ctx = NULL;
    AVCodecContext *audio_enc_ctx = NULL;
    AVCodecContext *video_enc_ctx = NULL;

    if ((ret = avformat_open_input(&ifmt_ctx, flv_name, 0, 0)) < 0)
    {
        printf("Could not open input file '%s' ret:%d\n", flv_name, ret);
        goto end;
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0)
    {
        printf("Failed to retrieve input stream information");
        goto end;
    }

    if (open_decodec_context(&video_stream_idx, &video_dec_ctx, ifmt_ctx, AVMEDIA_TYPE_VIDEO) < 0)
    {
        printf("fail to open vedio decode context, ret:%d\n", ret);
        goto end;
    }
    av_dump_format(ifmt_ctx, 0, input_file, 0);

    // 设置输出
    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, output_file);
    if (!ofmt_ctx)
    {
        printf("can not open ouout context");
        goto end;
    }

    // video stream
    AVStream *out_stream;
    out_stream = avformat_new_stream(ofmt_ctx, NULL);
    if (!out_stream)
    {
        printf("Failed allocating output stream\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }
    if ((ret = open_encodec_context(video_stream_idx, &video_enc_ctx, ifmt_ctx, AVMEDIA_TYPE_VIDEO)) < 0)
    {
        printf("video enc ctx init err\n");
        goto end;
    }
    ret = avcodec_parameters_from_context(out_stream->codecpar, video_enc_ctx);
    if (ret < 0)
    {
        printf("Failed to copy codec parameters\n");
        goto end;
    }
    video_enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    out_stream->time_base = video_enc_ctx->time_base;
    out_stream->codecpar->codec_tag = 0;

    av_dump_format(ofmt_ctx, 0, output_file, 1);

    // 打开文件
    if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&ofmt_ctx->pb, output_file, AVIO_FLAG_WRITE);
        if (ret < 0)
        {
            printf("Could not open output file '%s'\n", output_file);
            goto end;
        }
    }

    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0)
    {
        printf("Error occurred when opening output file\n");
        goto end;
    }

    AVPacket flv_pkt;
    int stream_index;
    while (1)
    {
        AVPacket *pkt = &flv_pkt;
        ret = av_read_frame(ifmt_ctx, pkt);
        if (ret < 0)
        {
            break;
        }

        if (pkt->stream_index == video_stream_idx && (pkt->flags & AV_PKT_FLAG_KEY))
        {
            printf("pkt.dts = %ld pkt.pts = %ld pkt.stream_index = %d is key frame\n", pkt->dts, pkt->pts, pkt->stream_index);
        }
        stream_index = pkt->stream_index;

        if (pkt->stream_index == video_stream_idx)
        {
            ret = avcodec_send_packet(video_dec_ctx, pkt);
            //LOGDEBUG("read type:%d pts:%d dts:%d\n", video_dec_ctx->codec_type, pkt->pts, pkt->dts);

            ret = decode_and_send_frame(video_dec_ctx, video_enc_ctx);
            ret = encode_and_save_pkt(video_enc_ctx, ofmt_ctx, ofmt_ctx->streams[0]);
            if (ret < 0)
            {
                printf("re encode video error, ret:%d\n", ret);
            }
        }
        av_packet_unref(pkt);
    }
    // fflush
    // fflush encode
    avcodec_send_packet(video_dec_ctx, NULL);
    decode_and_send_frame(video_dec_ctx, video_enc_ctx);

    // fflush decode
    avcodec_send_frame(video_enc_ctx, NULL);
    encode_and_save_pkt(video_enc_ctx, ofmt_ctx, ofmt_ctx->streams[0]);

    LOGDEBUG("stream end\n");
    av_write_trailer(ofmt_ctx);

end:
    avformat_close_input(&ifmt_ctx);
    if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);

    avcodec_free_context(&video_dec_ctx);

    return ret;
}

void play()
{
    avformat_network_init();

    //std::string url("rtsp://192.168.0.203:1000/wwd");
    std::string url("/home/com/pq.mp4");
    LOG_INFO("开始处理视频流:" << url);
    AVFormatContext *p_input_fmt_ctx = nullptr;
    if (0 != avformat_open_input(&p_input_fmt_ctx, url.c_str(), nullptr, nullptr))
    {
        LOG_ERROR("打开url失败:" << url);
        return;
    }
    if (0 > avformat_find_stream_info(p_input_fmt_ctx, nullptr))
    {
        LOG_ERROR("获取流信息失败");
        avformat_close_input(&p_input_fmt_ctx);
        return;
    }
    AVCodec *p_decoder = nullptr;
    auto index_video = av_find_best_stream(p_input_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &p_decoder, 0);
    if (0 > index_video)
    {
        LOG_ERROR("获取视频索引失败");
        avformat_close_input(&p_input_fmt_ctx);
        return;
    }
    auto video = p_input_fmt_ctx->streams[index_video];

    printf("---------------- 文件信息 ---------------\n");
    av_dump_format(p_input_fmt_ctx, 0, url.c_str(), 0);
    printf("-------------------------------------------------\n");

    AVCodecContext *p_dec_ctx = nullptr;
    p_dec_ctx = avcodec_alloc_context3(p_decoder);
    if (nullptr == p_dec_ctx)
    {
        return;
    }
    int ret = 0;
    if ((ret = avcodec_parameters_to_context(p_dec_ctx, video->codecpar)) < 0)
    {
        LOG_ERROR("复制解码参数失败");
        return;
    }

    p_dec_ctx->framerate = av_guess_frame_rate(p_input_fmt_ctx, video, nullptr);

    /* Init the decoders, with or without reference counting */
    AVDictionary *opts = nullptr;
    av_dict_set_int(&opts, "refcounted_frames", 1, 0);
    if ((ret = avcodec_open2(p_dec_ctx, p_decoder, &opts)) < 0)
    {
        LOG_ERROR("打开解码上下文失败");
        return;
    }

    //////////////////////////
    AVFormatContext *p_output_fmt_ctx = nullptr;
    if (0 > avformat_alloc_output_context2(&p_output_fmt_ctx, nullptr, nullptr, "./wwd.mp4"))
    {
        LOG_ERROR("打开输出流失败");
        return;
    }
    auto p_output_stream = avformat_new_stream(p_output_fmt_ctx, nullptr);
    if (nullptr == p_output_stream)
    {
        LOG_ERROR("新建输出流失败");
        return;
    }

    auto encoder = avcodec_find_encoder(video->codecpar->codec_id);
    if (!encoder)
    {
        LOG_ERROR("获取编码器失败");
        return;
    }

    auto enc_ctx = avcodec_alloc_context3(encoder);
    if (!enc_ctx)
    {
        LOG_ERROR("获取编码上下文失败");
        return;
    }

    {
        

//ret = avcodec_copy_context(enc_ctx, video->codec);
    //ret = avcodec_parameters_copy(p_output_stream->codecpar, video->codecpar);
    avcodec_parameters_to_context(p_dec_ctx, video->codecpar);



    avcodec_parameters_from_context(p_output_stream->codecpar, p_dec_ctx);
    enc_ctx->height = p_dec_ctx->height;
        enc_ctx->width = p_dec_ctx->width;
        enc_ctx->sample_aspect_ratio = p_dec_ctx->sample_aspect_ratio;
    enc_ctx->pix_fmt = p_dec_ctx->pix_fmt;
    enc_ctx->time_base = p_dec_ctx->time_base;
    enc_ctx->framerate =p_dec_ctx->framerate;
    enc_ctx->bit_rate = p_dec_ctx->bit_rate;
        if ((ret = avcodec_open2(enc_ctx, encoder, nullptr)) < 0)
        {
            LOG_ERROR("打开编码器失败");
            return;
        }

/* 
AVCodecContext *dec_ctx = video->codec;
        enc_ctx->height = dec_ctx->height;
        enc_ctx->width = dec_ctx->width;
        enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;

        //enc_ctx->bit_rate = dec_ctx->bit_rate;
        //enc_ctx->rc_max_rate = dec_ctx->bit_rate;
        //enc_ctx->rc_buffer_size = dec_ctx->bit_rate;
        //enc_ctx->bit_rate_tolerance = 0;
        // use yuv420P
        enc_ctx->pix_fmt = dec_ctx->pix_fmt;
        // set frame rate
        enc_ctx->time_base.num = 1;
        enc_ctx->time_base.den = TIME_DEN;

        //enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
       // enc_ctx->has_b_frames = false;
        //enc_ctx->max_b_frames = 0;
        //enc_ctx->gop_size = 120;

        set_encode_option(dec_ctx, &opts);

        if ((ret = avcodec_open2(enc_ctx, encoder, &opts)) < 0)
        {
            LOG_ERROR("打开编码器失败");
            return;
        }
        */
       
    }

    ret = avcodec_parameters_from_context(p_output_stream->codecpar, enc_ctx);
    if (ret < 0)
    {
        return;
    }
    enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    p_output_stream->time_base = enc_ctx->time_base;
    p_output_stream->codecpar->codec_tag = 0;

    av_dump_format(p_output_fmt_ctx, 0, "./wwd.mp4", 1);

    // 打开文件
    if (!(p_output_fmt_ctx->oformat->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&p_output_fmt_ctx->pb, "./wwd.mp4", AVIO_FLAG_WRITE);
        if (ret < 0)
        {
            return;
        }
    }

    auto p_frame = av_frame_alloc();
    if (nullptr == p_frame)
    {
        LOG_ERROR("请求av_frame失败");
        return;
    }
    AVPacket p;
    AVPacket *p_packet = &p;

    AVPacket enc_pkt;
    av_init_packet(&enc_pkt);
    enc_pkt.data = nullptr;
    enc_pkt.size = 0;
    auto p_packet_encode = &enc_pkt;

    LOG_DEBUG("0");

    if (0 > avformat_write_header(p_output_fmt_ctx, nullptr))
    {
        LOG_ERROR("写入视频头信息到输出流失败");
        return;
    }
    LOG_DEBUG("1");

    while (0 <= av_read_frame(p_input_fmt_ctx, p_packet))
    {
        if (p_packet->stream_index == index_video)
        {
            ret = avcodec_send_packet(p_dec_ctx, p_packet);

            AVFrame *frame = av_frame_alloc();
            while (ret >= 0)
            {
                ret = avcodec_receive_frame(p_dec_ctx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                {
                    ret = 0;
                    break;
                }
                else if (ret < 0)
                {
                    break;
                }
                int pts = frame->pts;
                frame->pict_type = AV_PICTURE_TYPE_NONE;
                // 修改pts
                //frame->pts  = pts - start*TIME_DEN;
                //printf("pts:%d\n", frame->pts);

                ret = avcodec_send_frame(enc_ctx, frame);
                if (ret < 0)
                {
                    printf("Error sending a frame for encoding\n");
                    break;
                }
            }
            av_frame_free(&frame);


            while (ret >= 0)
            {
                ret = avcodec_receive_packet(enc_ctx, p_packet_encode);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                {
                    ret = 0;
                    break;
                }

                else if (ret < 0)
                {
                    break;
                }
                //LOGDEBUG("encode type:%d pts:%d dts:%d\n", enc_ctx->codec_type, enc_pkt.pts, enc_pkt.dts);

                /* rescale output packet timestamp values from codec to stream timebase */
                av_packet_rescale_ts(p_packet_encode, enc_ctx->time_base, video->time_base);
                p_packet_encode->stream_index = video->index;

                ret = av_interleaved_write_frame(p_output_fmt_ctx, p_packet_encode);
                if (ret < 0)
                {
                    printf("write frame error, ret:%d\n", ret);
                    break;
                }

                av_packet_unref(p_packet_encode);
            }

            //av_packet_rescale_ts(p_packet, enc_ctx->time_base, video->time_base);
            //p_packet->stream_index = video->index;
            //av_interleaved_write_frame(p_output_fmt_ctx, p_packet);
            //av_write_frame(p_output_fmt_ctx, p_packet);
        }
        av_packet_unref(p_packet);
    }
    LOG_DEBUG("2");

    av_write_trailer(p_output_fmt_ctx);

    av_frame_free(&p_frame);
    avformat_close_input(&p_input_fmt_ctx);
    avformat_free_context(p_output_fmt_ctx);
    LOG_INFO("结束处理视频流:" << url);

    avformat_network_deinit();
}

int main()
{
    LOG_INFO("");
    LOG_INFO("");
    LOG_INFO("程序开始运行");
    play();
    LOG_INFO("程序结束运行");
    return 0;
}