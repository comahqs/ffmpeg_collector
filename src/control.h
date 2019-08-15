#ifndef CONTROL_H
#define CONTROL_H

#include "i_stream.h"
#include <thread>
#include <atomic>

class control : public i_plugin
{
public:
    control(const std::string& url_input, const std::string& url_output);
    virtual bool start();
    virtual void stop();
protected:
    static void handle_thread(const std::string& url_input, const std::string& url_output, std::shared_ptr<std::atomic_bool> flag_stop);

    std::string m_url_input;
    std::string m_url_output;
    std::thread m_thread;
    std::shared_ptr<std::atomic_bool> mp_bstop;
};
typedef std::shared_ptr<control> control_ptr;


#endif // CONTROL_H