#ifndef STREAM_PACKAGE_ENCODE_H
#define STREAM_PACKAGE_ENCODE_H

#include "i_stream.h"

class stream_package_encode : public stream_base
{
public:
    virtual int before_stream(info_av_ptr p_info);
    virtual int do_stream(info_av_ptr p_info);

protected:
    virtual int encode_video(info_av_ptr p_info);
    virtual int encode_audio(info_av_ptr p_info);
};
typedef std::shared_ptr<stream_package_encode> stream_package_encode_ptr;


#endif // STREAM_PACKAGE_ENCODE_H