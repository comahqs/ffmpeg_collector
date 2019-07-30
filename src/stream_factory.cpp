#include "stream_factory.h"
extern "C"
{
#include <libavformat/avformat.h>
}


static std::mutex s_mutex;
static stream_factory_ptr s_instance = std::make_shared<stream_factory>();

std::shared_ptr<stream_factory> stream_factory::get_instance(){
    if(!s_instance){
        std::lock_guard<std::mutex> lock(s_mutex);
        if(!s_instance){
            s_instance = std::make_shared<stream_factory>();
        }
    }
    return s_instance;
}

stream_factory::stream_factory(){
    av_register_all();
    avformat_network_init();
}

stream_factory::~stream_factory(){
    avformat_network_deinit();
}