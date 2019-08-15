#ifndef CONTROL_H
#define CONTROL_H

#include "i_stream.h"
#include <thread>
#include <atomic>

class control : public i_plugin
{
public:
    control(std::vector<stream_base_ptr> streams);
    virtual bool start();
    virtual void stop();
protected:
    static void handle_thread(std::vector<stream_base_ptr> streams, std::shared_ptr<std::atomic_bool> flag_stop);

     std::vector<stream_base_ptr> m_streams;
     std::thread m_thread;
     std::shared_ptr<std::atomic_bool> mp_bstop;
};
typedef std::shared_ptr<control> control_ptr;


#endif // CONTROL_H