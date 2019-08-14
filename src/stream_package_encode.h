#ifndef STREAM_PACKAGE_ENCODE_H
#define STREAM_PACKAGE_ENCODE_H

#include "i_stream.h"

class stream_package_encode : public i_stream
{
public:
    virtual bool add_stream(std::shared_ptr<i_stream> p_stream);
    virtual bool before_stream(info_av_ptr p_info);
    virtual bool do_stream(info_av_ptr p_info);
    virtual bool after_stream(info_av_ptr p_info);

protected:
     std::shared_ptr<i_stream> mp_stream;
};
typedef std::shared_ptr<stream_package_encode> stream_package_encode_ptr;


#endif // STREAM_PACKAGE_ENCODE_H