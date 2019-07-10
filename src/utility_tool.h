/**
 * @brief 公共库
 * @author 陈欧美
 * @date 2019-06-24
 * @version v1.0.0
 */
#ifndef UTILITYTOOL_H
#define UTILITYTOOL_H



#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <chrono>
#include <thread>



/// @brief 分割函数，使用指定的分隔字符串分割字符串
/// @param[out] params 输出分割后的字符串数组
/// @param data 需要分割的字符串
/// @param seporator 分隔字符，其中的每个字符都会当作一个分隔符
/// @return 操作是否成功
/// @retval true 成功
/// @retval false 失败
bool split(std::vector<std::string>& params, const std::string& data, const std::string& seporator);

/// @brief 字符串转ptime函数
/// @param data 输入字符串；输入字符串格式必须是yyyy-mm-dd hh:mi:ss，例如2019-06-13 10:00:00
/// @return 转换后的ptime，若转换失败，返回无效的ptime
std::chrono::system_clock::time_point str_to_ptime(const std::string& data);

/// @brief ptime转字符串函数
/// @param time_point 输入时间点
/// @return 输出字符串; 字符串格式yyyy-mm-dd hh:mi:ss，例如2019-06-13 10:00:00; 若失败则返回空字符串
std::string ptime_to_str(const std::chrono::system_clock::time_point& time_point);

/// @brief 日志输出函数
/// @param info 日志等级字符串
/// @param msg 日志内容
void log_write(const std::string& info, const std::string& msg);

/// @brief 获取文件绝对路径中的文件名
/// @param file 绝对路径
/// @return 文件名
const char* get_file_name(const char* file);

/// @brief 创建统一日志消息内容
#define CREATE_LOG_MSG(MSG) std::stringstream abc123;abc123<<"["<<std::this_thread::get_id()<<"]["<<get_file_name(__FILE__)<<"::"<<__FUNCTION__<<"@"<<__LINE__<<"]"<<MSG

/// @brief 输出调试日志
#define LOG_DEBUG(MSG) {CREATE_LOG_MSG(MSG);log_write("debug", abc123.str());}

/// @brief 输出警告日志
#define LOG_WARN(MSG) {CREATE_LOG_MSG(MSG);log_write("warn", abc123.str());}

/// @brief 输出错误日志
#define LOG_ERROR(MSG) {CREATE_LOG_MSG(MSG);log_write("error", abc123.str());}

/// @brief 输出通知日志
#define LOG_INFO(MSG) {CREATE_LOG_MSG(MSG);log_write("info", abc123.str());}

#endif // UTILITYTOOL_H
