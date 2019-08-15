#ifndef STREAM_PACKAGE_DECODE_H
#define STREAM_PACKAGE_DECODE_H

#include "i_stream.h"

class stream_package_decode : public stream_base
{
public:
    virtual int before_stream(info_av_ptr p_info);
    virtual int step(info_av_ptr p_info);
    virtual int after_stream(info_av_ptr p_info);

protected:
    virtual int decode_video(info_av_ptr p_info);
    virtual int decode_audio(info_av_ptr p_info);
};
typedef std::shared_ptr<stream_package_decode> stream_package_decode_ptr;


#endif // STREAM_PACKAGE_DECODE_H