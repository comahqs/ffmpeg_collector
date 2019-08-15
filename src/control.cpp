#include "control.h"

control::control(std::vector<stream_base_ptr> streams) : m_streams(streams)
{
}

bool control::start()
{
    mp_bstop = std::make_shared<std::atomic_bool>(false);
    m_thread = std::thread(std::bind(control::handle_thread, m_streams, mp_bstop));
}

void control::stop()
{
    *mp_bstop = true;
    m_thread.join();
}

void control::handle_thread(std::vector<stream_base_ptr> streams, std::shared_ptr<std::atomic_bool> flag_stop)
{
    auto p_info = std::make_shared<info_av>();
    for(auto& p_stream : streams){
        if(ES_SUCCESS != p_stream->before_step(p_info)){
            return;
        }
    }
    while (!flag_stop)
    {
        int ret = ES_SUCCESS;
        for(auto& p_stream : streams){
            ret = p_stream->before_step(p_info);
            if(ES_SUCCESS != ret){
                break;
            }
        }
        if(ES_SUCCESS == ret){

        }else if(ES_AGAIN == ret){
            continue;
        }else if(ES_EOF == ret){
            break;
        }else{
            break;
        }

        for(auto& p_stream : streams){
            ret = p_stream->step(p_info);
            if(ES_SUCCESS != ret){
                break;
            }
        }
        if(ES_SUCCESS == ret){

        }else if(ES_AGAIN == ret){
            continue;
        }else if(ES_EOF == ret){
            break;
        }else{
            break;
        }

        for(auto& p_stream : streams){
            ret = p_stream->after_step(p_info);
            if(ES_SUCCESS != ret){
                break;
            }
        }
        if(ES_SUCCESS == ret){

        }else if(ES_AGAIN == ret){
            continue;
        }else if(ES_EOF == ret){
            break;
        }else{
            break;
        }
    }

    for(auto iter = streams.rbegin(); iter != streams.rend(); ++iter){
        if(ES_SUCCESS != (*iter)->before_step(p_info)){
            return;
        }
    }
}