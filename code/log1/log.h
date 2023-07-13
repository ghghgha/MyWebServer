/*
 * @Author: Wang
 * @Date:  2022-03-28 15:34:03
 * @Description: log类的头文件
 * @LastEditTime: 2022-04-06 15:30:17
 * @FilePath:  /WebServer/log/log.h
 */
#ifndef WJ_LOG_H_
#define WJ_LOG_H_

#include <mutex>//互斥锁
#include <string>
#include <thread>
#include <sys/time.h>//时间日期
#include <string.h>
#include <stdarg.h>           // vastart va_end
#include <assert.h>
#include <sys/stat.h>         //mkdir
#include "blockqueue.h"// 自定义阻塞队列
#include "../buffer/buffer.h" // 自定义缓存区

// 日志系统类
class Log {
public:
    static Log* Instance();//静态获取单例的方法
    // 初始化日志系统
    void init(int level, //默认日志等级
            const char* path = "./log", //日志保存路径
            const char* suffix =".log",//日志文件后缀
            int maxQueueCapacity = 1024);//日志异步队列容量

    
    static void FlushLogThread();// 刷新日志线程

    void write(int level, const char *format,...);//写日志
    void flush();

    int GetLevel();
    void SetLevel(int level);
    bool IsOpen() { return isOpen_; }
    
private:
    Log();// 私有构造函数
    virtual ~Log();//私有析构函数
    void AppendLogLevelTitle_(int level);
    // 异步写入日志
    void AsyncWrite_();

private:
    static const int LOG_PATH_LEN = 256;//路径+日志名的最大长度
    static const int LOG_NAME_LEN = 256;//路径+日志名的最大长度
    static const int MAX_LINES = 50000;

    const char* path_;//日志文件保存路径
    const char* suffix_;//日志文件后缀名

    int MAX_LINES_;//最大行数

    int lineCount_;//当前行数
    int toDay_;//当前日

    bool isOpen_;
 
    Buffer buff_;//缓存区
    int level_;//日志默认输出等级
    bool isAsync_;//是否支持异步

    FILE* fp_;//文件指针
    std::unique_ptr<BlockDeque<std::string>> deque_; //定义存储字符串的阻塞队列的智能指针
    std::unique_ptr<std::thread> writeThread_;//定义写线程的智能指针
    std::mutex mtx_;// 互斥锁
};
#define LOG_BASE(level, format, ...) \
    do {\
        Log* log = Log::Instance();\
        if (log->IsOpen() && log->GetLevel() <= level) {\
            log->write(level, format, ##__VA_ARGS__); \
            log->flush();\
        }\
    } while(0);
#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);

#endif /*MRL_LOG_H_*/