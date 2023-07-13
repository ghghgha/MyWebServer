/*
 * @Author: Wang
 * @Date: 2022-03-29 14:36:42
 * @Description: Linux下的log日志工具
 * @LastEditTime: 2022-04-08 01:56:45
 * @FilePath: \C++\NetCode\log.cpp
 */
 # include "log.h"

// 构造函数
Logger::Logger(){
    this->target = terminal;
    this->level = debug;
}

Logger::Logger(log_target target, log_level level, string path){
        this->target = target;
        this->path = path;
        this->level = level;
        string tmp = "";  // 双引号下的常量不能直接相加，所以用一个string类型做转换
        if (target != terminal){
            this->outfile.open(path, ios::out | ios::app);   // 打开输出文件
        }
    }

void Logger::output( log_level act_level,const char *format,...){
    // 分析输出等级
    string prefix;
    switch(act_level){
        case log_level::debug:
            #ifndef DEBUG_OUT
                return;
            #endif
            prefix = "[DEBUG]   ";
            break;
        case log_level::info:
            #ifndef INFO_OUT
                return;
            #endif
            prefix = "[INFO]    ";
            break;
        case log_level::warning:
            #ifndef WARNING_OUT
                return;
            #endif
            prefix = "[WARNING]    ";
            break;
        case log_level::error:
            #ifndef ERROR_OUT
                return;
            #endif
            prefix = "[ERROR]    ";
            break;
        default:
            prefix = "";
    }
    //prefix += __FILE__;
    prefix += " "+ currTime() + " : ";
    string output_content = prefix + format + "\n";
    if(this->level <= act_level && this->target != file){
        // 当前等级设定的等级才会显示在终端，且不能是只文件模式
        cout<<(prefix);
        va_list args;// 定义一个va_list类型的变量，用来存储单个参数
        va_start(args,format);// 使args指向可变参数的第一个参数
        vprintf(format,args);
        va_end(args);//结束可变参数的获取
        cout<<endl;
    }
    if(this->target != terminal)
        outfile << output_content;
}

