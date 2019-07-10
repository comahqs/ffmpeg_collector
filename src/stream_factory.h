#ifndef STREAM_FACTORY_H
#define STREAM_FACTORY_H

#include "i_stream.h"
#include "utility_tool.h"
#include <map>
#include <mutex>

class stream_factory
{
public:
    static std::shared_ptr<stream_factory> get_instance();

    stream_factory();
    virtual ~stream_factory();

    bool add_factory(const std::string& name, std::function<i_stream_ptr ()>& fun){
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto iter = m_creates.find(name);
            if(m_creates.end() != iter){
                LOG_ERROR("名称已存在:"<<name);
                return false;
            }
            auto p_proxy = std::make_shared<proxy_param1<std::function<i_stream_ptr ()>>>();
            p_proxy->param1 = fun;
            return m_creates.insert(std::make_pair(name, p_proxy)).second;
        }
    }

    template<typename T1>
    bool add_factory(const std::string& name, std::function<i_stream_ptr (const T1&)>& fun){
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto iter = m_creates.find(name);
            if(m_creates.end() != iter){
                LOG_ERROR("名称已存在:"<<name);
                return false;
            }
            auto p_proxy = std::make_shared<proxy_param1<std::function<i_stream_ptr (const T1&)>>>();
            p_proxy->param1 = fun;
            return m_creates.insert(std::make_pair(name, p_proxy)).second;
        }
    }

    i_stream_ptr create_stream(const std::string &name){
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto iter = m_creates.find(name);
            if(m_creates.end() == iter){
                LOG_ERROR("找不到对应的名称:"<<name);
                return i_stream_ptr();
            }
            try{
                auto p_proxy = std::dynamic_pointer_cast<proxy_param1<std::function<i_stream_ptr ()>>>(iter->second);
                if(!p_proxy){
                    LOG_ERROR("转换失败; 名称:"<<name);
                    return i_stream_ptr();
                }
                return (p_proxy->param1)();
            }catch(const std::exception& e){
                LOG_ERROR("调用失败:"<<e.what()<<"; 名称:"<<name);
            }
        }
        return i_stream_ptr();
    }

    template <typename T1>
    i_stream_ptr create_stream(const std::string &name, const T1 &param1){
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto iter = m_creates.find(name);
            if(m_creates.end() == iter){
                LOG_ERROR("找不到对应的名称:"<<name);
                return i_stream_ptr();
            }
            try{
                auto p_proxy = std::dynamic_pointer_cast<proxy_param1<std::function<i_stream_ptr (const T1&)>>>(iter->second);
                if(!p_proxy){
                    LOG_ERROR("转换失败; 名称:"<<name);
                    return i_stream_ptr();
                }
                return (p_proxy->param1)();
            }catch(const std::exception& e){
                LOG_ERROR("调用失败:"<<e.what()<<"; 名称:"<<name);
            }
        }
        return i_stream_ptr();
    }

    template <typename T1, typename T2>
    i_stream_ptr create_stream(const std::string &name, const T1 &param1, const T2 &param2){
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto iter = m_creates.find(name);
            if(m_creates.end() == iter){
                LOG_ERROR("找不到对应的名称:"<<name);
                return i_stream_ptr();
            }
            try{
                auto p_proxy = std::dynamic_pointer_cast<proxy_param1<std::function<i_stream_ptr (const T1&, const T2&)>>>(iter->second);
                if(!p_proxy){
                    LOG_ERROR("转换失败; 名称:"<<name);
                    return i_stream_ptr();
                }
                return (p_proxy->param1)();
            }catch(const std::exception& e){
                LOG_ERROR("调用失败:"<<e.what()<<"; 名称:"<<name);
            }
        }
        return i_stream_ptr();
    }

protected:
    std::map<std::string, proxy_base_ptr> m_creates;
    std::mutex m_mutex;
};
using stream_factory_ptr = std::shared_ptr<stream_factory>;

#endif // STREAM_FACTORY_H