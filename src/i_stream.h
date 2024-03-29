#ifndef stream_base_H
#define stream_base_H


#include <memory>
#include <vector>


const static int ES_SUCCESS = 0;
const static int ES_AGAIN = -11;
const static int ES_EOF = 2;

const static int ES_UNKNOW = -1;

const static std::size_t STREAM_BUFFER_MAX = 4096;


class AVPacket;
class AVFrame;
class AVFormatContext;
class AVStream;
class AVCodec;
class AVCodecContext;

class info_stream{
public:
    AVStream* pi_stream = nullptr;
    AVCodecContext* pi_code_ctx = nullptr;
    AVStream* po_stream = nullptr;
    AVCodecContext* po_code_ctx = nullptr;
};
using info_stream_ptr = std::shared_ptr<info_stream>;

class info_av{
public:
    AVFormatContext* pi_fmt_ctx = nullptr;
    AVFormatContext* po_fmt_ctx = nullptr;
    std::vector<info_stream_ptr> streams;
    AVPacket* p_packet = nullptr;
    AVFrame *p_frame = nullptr;
};
using info_av_ptr = std::shared_ptr<info_av>;


class stream_base{
public:
    virtual bool add_stream( std::shared_ptr<stream_base> p_stream){mp_stream = p_stream;};
    virtual int before_stream(info_av_ptr p_info) {if(nullptr != mp_stream){return mp_stream->before_stream(p_info);}else{return ES_SUCCESS;}};
    virtual int do_stream(info_av_ptr p_info) {if(nullptr != mp_stream){return mp_stream->do_stream(p_info);}else{return ES_SUCCESS;}};
    virtual int after_stream(info_av_ptr p_info) {if(nullptr != mp_stream){return mp_stream->after_stream(p_info);}else{return ES_SUCCESS;}};

protected:
     std::shared_ptr<stream_base>mp_stream;
};
using stream_base_ptr = std::shared_ptr<stream_base>;

class i_plugin{
public:
     virtual bool start() = 0;
     virtual void stop() = 0;   
};
using i_plugin_ptr = std::shared_ptr<i_plugin>;


class proxy_base{
public:
    virtual ~proxy_base(){};
};
using proxy_base_ptr = std::shared_ptr<proxy_base>;

template<typename T1>
class proxy_param1:public proxy_base{
public:
    T1 param1;
};

template<typename T1, typename T2>
class proxy_param2:public proxy_base{
public:
    T1 param1;
    T2 param2;
};




#endif // stream_base_H