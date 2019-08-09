#ifndef I_STREAM_H
#define I_STREAM_H


#include <memory>
#include <vector>



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
    AVPacket* p_packet = nullptr;
    int o_stream_index = -1;
};
using info_stream_ptr = std::shared_ptr<info_stream>;

class info_av{
public:
    AVFormatContext* pi_fmt_ctx = nullptr;
    AVFormatContext* po_fmt_ctx = nullptr;

    std::vector<info_stream_ptr> streams;
    AVPacket* p_packet = nullptr;
};
using info_av_ptr = std::shared_ptr<info_av>;

class i_stream{
public:
    virtual bool add_stream(std::shared_ptr<i_stream> p_stream) = 0;
    virtual bool do_stream(info_av_ptr p_info) = 0;
    virtual bool start() = 0;
    virtual void stop() = 0;
};
using i_stream_ptr = std::shared_ptr<i_stream>;


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




#endif // I_STREAM_H