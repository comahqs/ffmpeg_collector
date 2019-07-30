#ifndef THREAD_PROXY_H__
#define THREAD_PROXY_H__



#include <memory>
#include <thread>
#include <atomic>

class thread_proxy{
public:
    typedef std::shared_ptr<std::atomic<bool>> flag_ptr;
    typedef std::function<void (flag_ptr)> fun_type;
    thread_proxy(fun_type fun) : m_fun(fun){
        mp_flag = std::make_shared<std::atomic<bool>>(false);
    }
    virtual ~thread_proxy(){}
    virtual bool start(){
        *mp_flag = true;
        m_thread = std::thread(std::bind(thread_proxy::handle_thread, m_fun, mp_flag));
        return true;
    }
    virtual void stop(){
        if(mp_flag){
            *mp_flag = false;
        }
        m_thread.join();
    }

protected:
    static void handle_thread(fun_type fun, flag_ptr p_flag){
        try{
            fun(p_flag);
        }catch(const std::exception&){

        }
    }

    std::thread m_thread;
    fun_type m_fun;
    flag_ptr mp_flag;
};
typedef std::shared_ptr<thread_proxy> thread_proxy_ptr;



class thread_proxy_none{
public:
    typedef std::shared_ptr<std::atomic<bool>> flag_ptr;
    typedef std::function<void ()> fun_type;
    thread_proxy_none(fun_type fun) : m_fun(fun){
    }
    virtual ~thread_proxy_none(){}
    virtual bool start(){
        m_thread = std::thread(std::bind(thread_proxy_none::handle_thread, m_fun));
        return true;
    }
    virtual void stop(){
        m_thread.join();
    }

protected:
    static void handle_thread(fun_type fun){
        try{
            fun();
        }catch(const std::exception&){

        }
    }

    std::thread m_thread;
    fun_type m_fun;
};
typedef std::shared_ptr<thread_proxy_none> thread_proxy_none_ptr;


#endif // THREAD_PROXY_H__