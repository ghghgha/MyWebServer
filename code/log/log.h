/*
 * @Author: Wang
 * @Date: 2022-03-29 14:36:42
 * @Description: Linux下的log日志工具
 * @LastEditTime: 2022-04-17 22:51:41
 * @FilePath: \C++\NetCode\linux_socket.cpp
 */
# ifndef WJ_LOGGER_H_
# define WJ_LOGGER_H_

# include <iostream>
# include <fstream>
# include <string>
# include <time.h>
# include <stdio.h>
# include <stdlib.h>
#include<stdarg.h>
using std::cout;
using std::string;
using std::endl;
using std::to_string;
using std::ios;


class Logger{
public:
    enum log_level{debug, info, warning, error};// 日志等级
    enum log_target{file, terminal, file_and_terminal};// 日志输出目标
private:
    Logger();  // 默认构造函数
    Logger(log_target target, log_level level, string path);
    std::ofstream outfile;    // 将日志输出到文件的流对象
    log_target target;        // 日志输出目标
    string path;              // 日志文件路径
    log_level level;          // 日志等级
    // 获取当前时间，并格式化表示
    string currTime(){
        char tmp[64];
        time_t ptime;
        time(&ptime);  // time_t time (time_t* timer);
        strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&ptime));
        return tmp;
    }
public:
    // 单例模式
    static Logger *Instance(){
        static Logger  _instance;
        return &_instance;
    }

    void output( log_level act_level,const char* format,...);            // 输出行为
};
// 是否输出相关等级信息
//#define DEBUG_OUT   1
#define INFO_OUT   1
#define WARNING_OUT   1
#define ERROR_OUT   1
// 输出宏工具
#define LOG_INFO(format,...) {Logger::Instance()->output(Logger::log_level::info,format,##__VA_ARGS__);}
#define LOG_DEBUG(format,...) {Logger::Instance()->output(Logger::log_level::debug,format,##__VA_ARGS__);}
#define LOG_WARN(format,...) {Logger::Instance()->output(Logger::log_level::warning,format,##__VA_ARGS__);}
#define LOG_ERROR(format,...) {Logger::Instance()->output(Logger::log_level::error,format,##__VA_ARGS__);}

# endif