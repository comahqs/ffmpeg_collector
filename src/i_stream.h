#ifndef I_STREAM_H
#define I_STREAM_H


#include <memory>



const static std::size_t STREAM_BUFFER_MAX = 4096;

class AVPacket;
class AVFrame;
class AVFormatContext;
class AVStream;
class info_stream{
public:
    using data_ptr = std::shared_ptr<std::array<unsigned char, STREAM_BUFFER_MAX>>;

    data_ptr p_data;
    std::size_t len = 0;

    AVFormatContext* p_fmt_ctx = nullptr;
    AVStream* p_stream = nullptr;
    AVPacket* p_packet = nullptr;
    AVFrame* p_frame = nullptr;
};
using info_stream_ptr = std::shared_ptr<info_stream>;

class i_stream{
public:
    virtual bool add_stream(std::shared_ptr<i_stream>& p_stream) = 0;
    virtual bool do_stream(info_stream_ptr& p_info) = 0;
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