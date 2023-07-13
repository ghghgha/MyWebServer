/*
 * @Author: Wang
 * @Date:  2022-03-28 15:34:03
 * @Description: WebServer类的头文件
 * @LastEditTime: 2022-03-29 00:51:26
 * @FilePath:  /WebServer/log/log.cpp
 */

#include "log.h"
using namespace std;
// 构造函数
Log::Log() {
    lineCount_ = 0;
    isAsync_ = false;
    writeThread_ = nullptr;
    deque_ = nullptr;
    toDay_ = 0;
    fp_ = nullptr;
}
// 析沟函数
Log::~Log() {
    if(writeThread_ && writeThread_->joinable()) {
        while(!deque_->empty()) {
            deque_->flush();
        };
        deque_->Close();
        writeThread_->join();
    }
    if(fp_) {
        lock_guard<mutex> locker(mtx_);
        flush();
        fclose(fp_);
    }
}
// 获取静态单例
Log* Log::Instance() {
    static Log inst;
    return &inst;
}



// 初始化日志系统
void Log::init(int level = 1, //默认日志等级
    const char* path, //日志保存文件
    const char* suffix,//日志保存后缀
    int maxQueueSize//日志异步队列容量
    ) {
    isOpen_ = true;
    level_ = level;
    path_ = path;
    suffix_ = suffix;
    // 如果设置了maxQueueSize，则设置为异步
    if(maxQueueSize > 0) {
        isAsync_ = true;
        if(!deque_) {
            unique_ptr<BlockDeque<std::string>> newDeque(new BlockDeque<std::string>);
            deque_ = move(newDeque);
            
            std::unique_ptr<std::thread> NewThread(new thread(FlushLogThread));
            writeThread_ = move(NewThread);
        }
    } else {
        isAsync_ = false;
    }

    lineCount_ = 0;
    // 生成日志文件
    time_t timer = time(nullptr);//获取1970-1-1开始到现在的秒数
    struct tm *sysTime = localtime(&timer);//将time_t类型的秒数转换为tm类型的时间
    struct tm t = *sysTime;
    toDay_ = t.tm_mday;//保存日
    // 拼接日志文件名：path_/2022_03_28.log
    char fileName[LOG_NAME_LEN] = {0};
    snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s", 
            path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
    // 
    {
        lock_guard<mutex> locker(mtx_);//在声明空间内枷锁
        buff_.RetrieveAll();
        if(fp_) { 
            flush();
            fclose(fp_); 
        }
        // 打开文件fileName
        fp_ = fopen(fileName, "a");
        // 如果打开失败
        if(fp_ == nullptr) {
            mkdir(path_, 0777);//创建目录
            fp_ = fopen(fileName, "a");//重新打开文件fileName
        } 
        assert(fp_ != nullptr);
    }
}
// 写日志
void Log::write(int level, const char *format, ...) {
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t tSec = now.tv_sec;
    struct tm *sysTime = localtime(&tSec);
    struct tm t = *sysTime;
    va_list vaList;

    /* 如果当前日志文件不是今天的日期 或者写入的日志行数是最大行的倍数，则创建新的日志文件*/
    if (toDay_ != t.tm_mday || (lineCount_ && (lineCount_  %  MAX_LINES == 0)))
    {
        unique_lock<mutex> locker(mtx_);
        locker.unlock();
        
        char newFile[LOG_NAME_LEN];
        char tail[36] = {0};
        //格式化日志名中的时间部分
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
        //如果当前日志文件的日期不是今天日期，创建今天的日志文件
        if (toDay_ != t.tm_mday)
        {
            //格式化今天的日志
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", path_, tail, suffix_);
            toDay_ = t.tm_mday;
            lineCount_ = 0;
        }
        // 如果超过了最大行，在之前的日志名基础上加后缀，
        else {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", path_, tail, (lineCount_  / MAX_LINES), suffix_);
        }
        
        locker.lock();
        flush();
        fclose(fp_);//关闭原有日志文件
        fp_ = fopen(newFile, "a");//打开新的日志文件
        assert(fp_ != nullptr);
    }

    {
        unique_lock<mutex> locker(mtx_);//加锁
        lineCount_++;//更新现有行数
        // 向缓存区添加时间格式
        int n = snprintf(buff_.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                    t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
                 
        buff_.HasWritten(n);
        // 向缓存区添加日志头缀
        AppendLogLevelTitle_(level);
        // 向缓存区添加内容format
        va_start(vaList, format);
        int m = vsnprintf(buff_.BeginWrite(), buff_.WritableBytes(), format, vaList);
        va_end(vaList);
        buff_.HasWritten(m);
        buff_.Append("\n\0", 2);
        // 如果异步且阻塞队列没有满，则将日志信息加入阻塞队列
        if(isAsync_ && deque_ && !deque_->full()) {
            deque_->push_back(buff_.RetrieveAllToStr());
        }
        // 如果同步则将日志信息写入文件 
        else {
            fputs(buff_.Peek(), fp_);
        }
        //
        buff_.RetrieveAll();
    }
}
// 添加日志分级头缀
void Log::AppendLogLevelTitle_(int level) {
    switch(level) {
    case 0:
        buff_.Append("[debug]: ", 9);
        break;
    case 1:
        buff_.Append("[info] : ", 9);
        break;
    case 2:
        buff_.Append("[warn] : ", 9);
        break;
    case 3:
        buff_.Append("[error]: ", 9);
        break;
    default:
        buff_.Append("[info] : ", 9);
        break;
    }
}

void Log::flush() {
    if(isAsync_) { 
        deque_->flush(); 
    }
    fflush(fp_);
}

void Log::AsyncWrite_() {
    string str = "";
    // 从阻塞队列中不断取出字符串
    while(deque_->pop(str)) {
        lock_guard<mutex> locker(mtx_);
        fputs(str.c_str(), fp_);//将字符串写入字符
    }
}

// 刷新
void Log::FlushLogThread() {
    Log::Instance()->AsyncWrite_();
}

int Log::GetLevel() {
    lock_guard<mutex> locker(mtx_);
    return level_;
}

void Log::SetLevel(int level) {
    lock_guard<mutex> locker(mtx_);
    level_ = level;
}
