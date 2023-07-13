/*
 * @Author: Wang
 * @Date: 2022-03-21 22:36:14
 * @Description: HttpConn类的实现
 * @LastEditTime: 2022-06-03 19:56:43
 * @FilePath: \WebServer\http\httpconn.cpp
 */
#include"httpconn.h"
using namespace std;

bool HttpConn::isET;
const char* HttpConn::srcDir;
std::atomic<int> HttpConn::userCount;

// 构造函数
HttpConn::HttpConn()
{
     fd_ = -1;
    addr_ = { 0 };
    isClose_ = true;
}

// 析构函数
HttpConn::~HttpConn()
{
    this->Close();
}

/*初始化Http连接*/
void HttpConn::init(int fd, const sockaddr_in& addr) {
    assert(fd > 0);
    userCount++;
    fd_ = fd;
    addr_ = addr;
    isClose_ = false;
    writeBuff_.RetrieveAll();
    readBuff_.RetrieveAll();
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
}
/*关闭Http连接*/
void HttpConn::Close() {
    // response_.UnmapFile();
    if(isClose_ == false){
        isClose_ = true; 
        userCount--;
        close(fd_);
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
    }
}

ssize_t HttpConn::read(int* saveErrno) {
    ssize_t len = -1;
    // 从fd_读缓存
    do {
        // 分散读
        len = readBuff_.ReadFd(fd_, saveErrno);
        if (len <= 0) {
            break;
        }
    } while (isET);
    return len;
}

ssize_t HttpConn::write(int* saveErrno) {
    ssize_t len = -1;
    // 集中写
    do {
        len = writev(fd_, iov_, iovCnt_);
        if(len <= 0) {
            *saveErrno = errno;
            break;
        }
        if(iov_[0].iov_len + iov_[1].iov_len  == 0) { break; } /* 传输结束 */
        else if(static_cast<size_t>(len) > iov_[0].iov_len) {
            iov_[1].iov_base = (uint8_t*) iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if(iov_[0].iov_len) {
                writeBuff_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        }
        else {
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len; 
            iov_[0].iov_len -= len; 
            writeBuff_.Retrieve(len);
        }
    } while(isET || ToWriteBytes() > 10240);
    return len;
}

bool HttpConn::process() {
    request_.Init();//初始化HTTP请求
    if(readBuff_.ReadableBytes() <= 0) {
        return false;
    }
    else if(request_.parse(readBuff_)) {
        LOG_DEBUG("%s", request_.path().c_str());//解析成功后的初始化HTTP应答
        response_.Init(srcDir, request_.path(), request_.IsKeepAlive(), 200);//初始化一个HTTP应答
    } else {
        response_.Init(srcDir, request_.path(), false, 400);//解析失败的初始化HTTP应答
    }

    response_.MakeResponse(writeBuff_);//制作HTTP应答
    /* 响应头 */
    iov_[0].iov_base = const_cast<char*>(writeBuff_.Peek());
    iov_[0].iov_len = writeBuff_.ReadableBytes();
    iovCnt_ = 1;

    /* 文件 */
    if(response_.FileLen() > 0  && response_.File()) {
        iov_[1].iov_base = response_.File();
        iov_[1].iov_len = response_.FileLen();
        iovCnt_ = 2;
    }
    LOG_DEBUG("filesize:%d, %d  to %d", response_.FileLen() , iovCnt_, ToWriteBytes());
    return true;
}




int HttpConn::GetFd() const {
    return fd_;
};

struct sockaddr_in HttpConn::GetAddr() const {
    return addr_;
}

const char* HttpConn::GetIP() const {
    return inet_ntoa(addr_.sin_addr);
}

int HttpConn::GetPort() const {
    return addr_.sin_port;
}