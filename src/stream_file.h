#ifndef STREAM_FILE_H__
#define STREAM_FILE_H__

#include "i_stream.h"

class AVStream;
class stream_file : public i_stream{
public:
    stream_file(const std::string& file_path);

    virtual bool add_stream(std::shared_ptr<i_stream> p_stream);
    virtual bool do_stream(info_stream_ptr p_info);
    virtual bool start();
    virtual void stop();
protected:
    std::string m_file_path;
    AVFormatContext* mp_fmt_ctx = nullptr;
    AVStream* mp_stream = nullptr;
};
using stream_file_ptr = std::shared_ptr<stream_file>;







#endif // STREAM_FILE_H__