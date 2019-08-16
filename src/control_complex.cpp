#include "control_complex.h"
#include "protocol_rtsp_pull.h"
#include "protocol_rtsp_push.h"
#include "stream_package_decode.h"
#include "stream_package_encode.h"
#include "filter_complex.h"

control_complex::control_complex(const std::vector<std::string> &url_input, const std::string &url_output) : m_url_input(url_input), m_url_output(url_output)
{
}

bool control_complex::start()
{
    if (4 != m_url_input.size())
    {
        return false;
    }
    mp_bstop = std::make_shared<std::atomic_bool>(false);
    m_thread = std::thread(std::bind(control_complex::handle_thread, m_url_input, m_url_output, mp_bstop));
}

void control_complex::stop()
{
    *mp_bstop = true;
    m_thread.join();
}

void control_complex::wait()
{
    m_thread.join();
}

void control_complex::handle_thread(const std::vector<std::string> &url_input, const std::string &url_output, std::shared_ptr<std::atomic_bool> pflag_stop)
{
    std::vector<stream_base_ptr> streams;
    auto p_protocol_push = std::make_shared<protocol_rtsp_push>(url_output);
    auto p_encode = std::make_shared<stream_package_encode>();
    p_encode->add_stream(p_protocol_push);
    auto p_filter = std::make_shared<filter_complex>();
    p_filter->add_stream(p_encode);
    for (auto &url : url_input)
    {
        auto p_decode = std::make_shared<stream_package_decode>();
        p_decode->add_stream(p_filter);
        auto p_protocol_pull = std::make_shared<protocol_rtsp_pull>(url);
        p_protocol_pull->add_stream(p_decode);
        streams.push_back(p_protocol_pull);
    }
    auto ret = ES_SUCCESS;
    auto p_info = std::make_shared<info_av>();
    for (std::size_t i = 0; i < streams.size(); ++i)
    {
        p_info->index_input = i;
        ret =  streams[i]->before_stream(p_info);
        if (ES_SUCCESS != ret)
        {
            return;
        }
    }

    while (!(*pflag_stop))
    {
        for (std::size_t i = 0; i < streams.size(); ++i)
        {
            p_info->index_input = i;
            ret = streams[i]->do_stream(p_info);
            if (ES_SUCCESS != ret)
            {
                break;
            }
        }
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

    for (std::size_t i = 0; i < streams.size(); ++i)
    {
        p_info->index_input = i;
        ret = streams[i]->after_stream(p_info);
        if (ES_SUCCESS != ret)
        {
            return;
        }
    }
}