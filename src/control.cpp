#include "control.h"
#include "protocol_rtsp_pull.h"
#include "protocol_rtsp_push.h"
#include "stream_package_decode.h"
#include "stream_package_encode.h"


control::control(const std::string& url_input, const std::string& url_output) : m_url_input(url_input), m_url_output(url_output)
{
}

bool control::start()
{
    mp_bstop = std::make_shared<std::atomic_bool>(false);
    m_thread = std::thread(std::bind(control::handle_thread, m_url_input, m_url_output, mp_bstop));
}

void control::stop()
{
    *mp_bstop = true;
    m_thread.join();
}

void control::wait(){
    m_thread.join();
}

void control::handle_thread(const std::string& url_input, const std::string& url_output, std::shared_ptr<std::atomic_bool> pflag_stop)
{
    auto p_protocol_push = std::make_shared<protocol_rtsp_push>(url_output);
    auto p_encode = std::make_shared<stream_package_encode>();
    p_encode->add_stream(p_protocol_push);
    auto p_decode = std::make_shared<stream_package_decode>();
    p_decode->add_stream(p_encode);
    auto p_protocol_pull = std::make_shared<protocol_rtsp_pull>(url_input);
    p_protocol_pull->add_stream(p_decode);
    
    auto p_stream = p_protocol_pull;
    auto p_info = std::make_shared<info_av>();
    auto ret = p_stream->before_stream(p_info);
    if (ES_SUCCESS != ret)
    {
        return;
    }
    while (!(*pflag_stop))
    {
        ret = p_stream->do_stream(p_info);
        if (ES_SUCCESS == ret)
        {
        }
        else if (ES_AGAIN == ret)
        {
            continue;
        }
        else if (ES_EOF == ret)
        {
            break;
        }
        else
        {
            break;
        }
    }
    ret = p_stream->after_stream(p_info);
    if (ES_SUCCESS != ret)
    {
        return;
    }
}