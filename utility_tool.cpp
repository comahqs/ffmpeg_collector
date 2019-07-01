#include "utility_tool.h"
#include <iostream>
#include <mutex>
#include <fstream>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/locale.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/attributes/timer.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/expressions/keyword.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/common.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/severity_feature.hpp>
#include <boost/log/utility/setup/console.hpp>

static std::shared_ptr<boost::property_tree::ptree> sp_config;
static std::mutex s_mutex;

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace keywords = boost::log::keywords;
namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;

enum severity_level
{
    trace,
    debug,
    info,
    warning,
    error,
    fatal
};
template< typename CharT, typename TraitsT >
inline std::basic_ostream< CharT, TraitsT >& operator<< (
    std::basic_ostream< CharT, TraitsT >& strm, severity_level lvl)
{
    static const char* const str[] =
    {
        "TRACE",
        "DEBUG",
        "INFO",
        "WARN",
        "ERROR",
        "FATA"
    };
    if (static_cast< std::size_t >(lvl) < (sizeof(str) / sizeof(*str)))
        strm << str[lvl];
    else
        strm << static_cast< int >(lvl);
    return strm;
}

BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(logger,src::severity_logger_mt<severity_level >);

typedef std::shared_ptr<boost::log::sources::severity_logger_mt<severity_level >> severity_log_ptr;


class LogProxy{
public:
    LogProxy(){
        if (boost::filesystem::exists("./log") == false)
        {
            boost::filesystem::create_directories("./log");
        }

        auto pSinkFile = logging::add_file_log(
             keywords::open_mode = std::ios::app,
             keywords::file_name = "./log/log_%Y%m%d.log",
             keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
             keywords::format = expr::stream<< expr::message);
        pSinkFile->locked_backend()->auto_flush(true);
        logging::core::get()->add_sink(pSinkFile);

        auto pSinkError = logging::add_file_log(
            keywords::open_mode = std::ios::app,
            keywords::file_name = "./log/error_%N.log",
            keywords::rotation_size = 20 * 1024 * 1024,
            keywords::time_based_rotation = sinks::file::rotation_at_time_point(boost::gregorian::greg_day(1)),
            keywords::filter = expr::attr< severity_level >("Severity") >= error,
            keywords::format = expr::stream<< expr::message);
        pSinkError->locked_backend()->auto_flush(true);
        logging::core::get()->add_sink(pSinkError);

        auto pSinkConsole = logging::add_console_log();
        logging::core::get()->add_sink(pSinkConsole);

        logging::add_common_attributes();
        mp_serverity = std::make_shared<severity_log_ptr::element_type>();
    }

    severity_log_ptr mp_serverity;
};
static LogProxy s_proxy;

IPlugin::~IPlugin(){}

boost::posix_time::ptime str_to_ptime(const std::string& data){
    try{
        return boost::posix_time::time_from_string(data);
    }catch(const std::exception& e){
        LOG_ERROR("转换失败:"<<e.what()<<"; 数据:"<<data);
    }
    return boost::posix_time::ptime();
}

bool split(std::vector<std::string>& params, const std::string& data, const std::string& seporator){
    boost::split(params, data, boost::is_any_of(seporator));
    return true;
}

void log_write(const std::string& le, const std::string& msg){
    if ("trace" == le)
        {
            BOOST_LOG_SEV((logger::get()),(trace))<<ptime_to_str(boost::posix_time::second_clock::local_time())<<"\t"<<le<<"\t"<<msg;
            return;
        }
        if ("debug" == le)
        {
            BOOST_LOG_SEV((logger::get()),(debug))<<ptime_to_str(boost::posix_time::second_clock::local_time())<<"\t"<<le<<"\t"<<msg;
            return;
        }
        if ("info" == le)
        {
            BOOST_LOG_SEV((logger::get()),(info))<<ptime_to_str(boost::posix_time::second_clock::local_time())<<"\t"<<le<<"\t"<<msg;
            return;
        }
        if ("warn" == le)
        {
            BOOST_LOG_SEV((logger::get()),(warning))<<ptime_to_str(boost::posix_time::second_clock::local_time())<<"\t"<<le<<"\t"<<msg;
            return;
        }
        if ("error" == le)
        {
            BOOST_LOG_SEV((logger::get()),(error))<<ptime_to_str(boost::posix_time::second_clock::local_time())<<"\t"<<le<<"\t"<<msg;
            return;
        }
}

boost::property_tree::ptree& get_config(){

    if(!sp_config){
        std::lock_guard<std::mutex> lock(s_mutex);
        if(!sp_config){
            sp_config = std::make_shared<boost::property_tree::ptree>();
            try{
                std::fstream stream;
                stream.open("./config.ini");
                if(!stream.is_open()){
                    LOG_ERROR("打开配置失败");
                }else{
                    boost::property_tree::read_ini(stream, *sp_config);
                }
            }catch(const std::exception& e){
                LOG_ERROR("打开配置文件时发生错误:"<<e.what());
            }
        }
    }
    return *sp_config;
}

void set_config(const boost::property_tree::ptree& config){
    std::lock_guard<std::mutex> lock(s_mutex);
    try{
        std::ofstream stream;
        stream.open("./config.ini");
        if(!stream.is_open()){
            LOG_ERROR("打开配置文件失败");
        }else{
            boost::property_tree::write_ini(stream, config);
        }
        stream.close();
    }catch(const std::exception& e){
        LOG_ERROR("保存配置文件时发生错误:"<<e.what());
    }
}

std::string ptime_to_str(const boost::posix_time::ptime& time_point){
    try
    {
        return boost::gregorian::to_iso_extended_string(time_point.date()) + " " + boost::posix_time::to_simple_string(time_point.time_of_day());
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

