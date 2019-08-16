#ifndef CONTROL_COMPLEX_H
#define CONTROL_COMPLEX_H

#include "i_stream.h"
#include <thread>
#include <atomic>


class control_complex : public i_plugin
{
public:
    control_complex(const std::vector<std::string>& url_input, const std::string& url_output);
    virtual bool start();
    virtual void stop();

    virtual void wait();
protected:
    static void handle_thread(const std::vector<std::string>& url_input, const std::string& url_output, std::shared_ptr<std::atomic_bool> flag_stop);

    std::vector<std::string> m_url_input;
    std::string m_url_output;
    std::thread m_thread;
    std::shared_ptr<std::atomic_bool> mp_bstop;
};
typedef std::shared_ptr<control_complex> control_complex_ptr;


#endif // CONTROL_COMPLEX_H