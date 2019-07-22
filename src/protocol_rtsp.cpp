#include "protocol_rtsp.h"
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include "utility_tool.h"

protocol_rtsp::protocol_rtsp(const std::string& url):m_url(url){

}

bool protocol_rtsp::add_stream(std::shared_ptr<i_stream>& p_stream){
    mp_stream = p_stream;
    return true;
}
bool protocol_rtsp::do_stream(info_stream_ptr& p_info){
    return false;
}
bool protocol_rtsp::start(){
    mp_bstop = std::make_shared<std::atomic_bool>(false);
    m_thread = std::thread(std::bind(protocol_rtsp::handle_thread, m_url, mp_stream, mp_bstop));
    return true;
}
void protocol_rtsp::stop(){
    auto p_bstop = mp_bstop;
    if(p_bstop){
        *p_bstop = true;
    }
}

void protocol_rtsp::handle_thread(std::string url, i_stream_ptr p_stream, std::shared_ptr<std::atomic_bool> p_bstop){
    LOG_INFO("开始处理视频流:"<<url);
    AVFormatContext* p_fmt_ctx = nullptr;
    if(0 != avformat_open_input(&p_fmt_ctx, url.c_str(), nullptr, nullptr)){
        LOG_ERROR("打开url失败:"<<url);
        return;
    }
    if(0 > avformat_find_stream_info(p_fmt_ctx, nullptr)){
        LOG_ERROR("获取流信息失败");
        avformat_close_input(&p_fmt_ctx);
        return;
    }
    AVCodec* p_decoder = nullptr;
    auto index_video = av_find_best_stream(p_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &p_decoder, 0);
    if(0 > index_video){
        LOG_ERROR("获取视频索引失败");
        avformat_close_input(&p_fmt_ctx);
        return;
    }
    auto p_codec_ctx =  avcodec_alloc_context3(p_decoder);
    if(nullptr == p_codec_ctx){

    }
    auto video = p_fmt_ctx->streams[index_video];
     if (0 > avcodec_parameters_to_context(p_codec_ctx, video->codecpar)){

     }
    if (0 > avcodec_open2(p_codec_ctx, p_decoder, nullptr))
	{
		LOG_ERROR("打开解码器失败; 解码器id:"<<p_codec_ctx->codec_id);
        avformat_close_input(&p_fmt_ctx);
        return;
	}
    auto p_frame = av_frame_alloc();
    if(nullptr == p_frame){
        LOG_ERROR("请求av_frame失败");
        avcodec_close(p_codec_ctx);
        avformat_close_input(&p_fmt_ctx);
        return;
    }
	auto p_frame_YUV = av_frame_alloc();
    if(nullptr == p_frame_YUV){
        LOG_ERROR("请求av_frame失败");
        av_frame_free(&p_frame);
        avcodec_close(p_codec_ctx);
        avformat_close_input(&p_fmt_ctx);
        return;
    }
    
	auto p_out_buffer = (uint8_t *)av_malloc(av_image_get_buffer_size(AVPixelFormat::AV_PIX_FMT_YUV420P, p_codec_ctx->width, p_codec_ctx->height, 1));
	av_image_fill_arrays(p_frame_YUV->data, p_frame_YUV->linesize, p_out_buffer, AVPixelFormat::AV_PIX_FMT_YUV420P, p_codec_ctx->width, p_codec_ctx->height, 1);
 
	printf("---------------- 文件信息 ---------------\n");
	av_dump_format(p_fmt_ctx, 0, url.c_str(), 0);
	printf("-------------------------------------------------\n");
 
	auto p_img_convert_ctx = sws_getContext(p_codec_ctx->width, p_codec_ctx->height, p_codec_ctx->pix_fmt,
		p_codec_ctx->width, p_codec_ctx->height, AVPixelFormat::AV_PIX_FMT_YUV420P, SWS_BICUBIC, nullptr, nullptr, nullptr);
	auto p_packet = (AVPacket *)av_malloc(sizeof(AVPacket));

    while (0 <= av_read_frame(p_fmt_ctx, p_packet))
	{
		if (p_packet->stream_index == index_video){
				
		}
		av_free_packet(p_packet);
	}
 
	av_frame_free(&p_frame_YUV);
	av_frame_free(&p_frame);
	avcodec_close(p_codec_ctx);
	avformat_close_input(&p_fmt_ctx);
    LOG_INFO("结束处理视频流:"<<url);
}