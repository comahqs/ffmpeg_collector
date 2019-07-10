#include "utility_tool.h"
#include <iostream>
#include <mutex>
#include <fstream>
#include <string.h>

std::chrono::system_clock::time_point str_to_ptime(const std::string& data){
    try{
        // 2019-01-01 00:00:00
        if(19 != data.size()){
            LOG_ERROR("非法的时间格式:"<<data);
            return std::chrono::system_clock::time_point();
        }
        std::tm t;
        t.tm_year = 1900 + atoi(data.substr(0, 4).c_str());
        t.tm_mon = atoi(data.substr(5, 2).c_str()) - 1;
        t.tm_mday = atoi(data.substr(8, 2).c_str());
        t.tm_hour = atoi(data.substr(11, 2).c_str());
        t.tm_min = atoi(data.substr(14, 2).c_str());
        t.tm_sec = atoi(data.substr(17, 2).c_str());
        if(1900 > t.tm_year || 2050 < t.tm_year || 0 > t.tm_mon || 11 < t.tm_mon || 1 > t.tm_mday || 31 < t.tm_mday 
            || 0 > t.tm_hour || 23 < t.tm_hour || 0 > t.tm_min || 59 < t.tm_min || 0 > t.tm_sec || 59 < t.tm_sec){
                LOG_ERROR("非法的时间内容:"<<data);
                return std::chrono::system_clock::time_point();
        }
        auto time_c = mktime(&t);
        return std::chrono::system_clock::from_time_t(time_c);
    }catch(const std::exception& e){
        LOG_ERROR("转换失败:"<<e.what()<<"; 数据:"<<data);
    }
    return std::chrono::system_clock::time_point();
}

bool split(std::vector<std::string>& params, const std::string& data, const std::string& seporator){
    return false;
}

void log_write(const std::string& le, const std::string& msg){
    std::cout<<ptime_to_str(std::chrono::system_clock::now())<<"\t"<<le<<"\t"<<msg<<std::endl;
}


std::string ptime_to_str(const std::chrono::system_clock::time_point& time_point){
    try
    {
        if(std::chrono::system_clock::time_point::duration::zero() == time_point.time_since_epoch()){
            return std::string();
        }
        char str_tmp[100];
        auto t = std::chrono::system_clock::to_time_t(time_point);
        tm tm_data;
        localtime_r(&t, &tm_data);
        sprintf(str_tmp, "%04d-%02d-%02d %02d:%02d:%02d", 1900 + tm_data.tm_year, 1 + tm_data.tm_mon, tm_data.tm_mday, tm_data.tm_hour, tm_data.tm_min, tm_data.tm_sec);
        return std::string(str_tmp);
    }
    catch(const std::exception& )
    {
    }
    return std::string("");
}

const char* get_file_name(const char* file){
    const char* p_end = file + strlen(file);
    for(const char* p_start = p_end; p_start != file; --p_start){
        if('/' == *p_start || '\\' == *p_start){
            if(p_start != p_end){
                return p_start + 1;
            }else{
                return p_start;
            }
        }
    }
    return file;
}

