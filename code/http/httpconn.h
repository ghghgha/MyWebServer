/*
 * @Author: Wang
 * @Date: 2022-03-21 22:36:08
 * @Description: HttpConn类的头文件
 * @LastEditTime: 2022-04-06 22:35:31
 * @FilePath: \WebServer\http\httpconn.h
 */
#ifndef WJ_HTTP_CONN_H_
#define WJ_HTTP_CONN_H_
using namespace std;


#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      // atoi()
#include <errno.h>      

#include "../log/log.h"
//#include "../pool/sqlconnRAII.h"
#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

// HTTP连接类
class HttpConn
{
public:
    static bool isET;// 公共的是否ET模式
    static atomic<int> userCount;// 公共的当前用户数
    static const char* srcDir;// 公共的当前工作目录
public:
    HttpConn(/* args */);//构造函数
    ~HttpConn();//析构函数
    void init(int fd, const sockaddr_in& addr);//初始化
    void Close();// 关闭
    ssize_t read(int* saveErrno);
    ssize_t write(int* saveErrno);
    bool process();
    int ToWriteBytes() { 
        return iov_[0].iov_len + iov_[1].iov_len; 
    }

    bool IsKeepAlive() const {
        return request_.IsKeepAlive();
    }
    // 获取私有属性
    int GetFd() const;
    int GetPort() const;
    const char* GetIP() const;
    sockaddr_in GetAddr() const;
private:
    int fd_; //连接socket
    struct  sockaddr_in addr_;// socket地址
    bool isClose_;// 是否关闭
    Buffer readBuff_; // 读缓冲区
    Buffer writeBuff_; // 写缓冲区
    // HTTP请求
    HttpRequest request_;
    HttpResponse response_;
    int iovCnt_;
    struct iovec iov_[2];
};






#endif //MRL_HTTP_CONN_H_