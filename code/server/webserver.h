/*
 * @Author: Wang
 * @Date: 2022-03-21 20:44:42
 * @Description: WebServer类的头文件
 * @LastEditTime: 2022-04-08 15:38:02
 * @FilePath: \WebServer\server\webserver.h
 */
#ifndef WJ_WEB_SERVER_H_
#define WJ_WEB_SERVER_H_


#include <unistd.h>     
#include <arpa/inet.h>  
#include <unordered_map>
//#include <fcntl.h>       // fcntl()

#include<string>
#include<netdb.h>

#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>


#include"socket.h"
#include "epoller.h" // IO复用
#include "../timer/heaptimer.h"
#include "../pool/threadpool.h"
#include "../pool/sqlconnRAII.h"
#include "../http/httpconn.h"
#include "../pool/sqlconnpool.h" // 数据库连接系统
#include "../log/log.h" // 日志系统
// Web服务器类
class WebServer{
public:
    WebServer(
            // 端口号，epoll触发模式
            int port, int trigMode, int timeoutMS, bool OptLinger,
            // SQL端口号，SQL用户名，SQL密码， 数据库名称，连接池数量
            int sqlPort, const char* sqlUser, const  char* sqlPwd,const char* dbName, int connPoolNum, 
            int threadNum,
            // 日志系统：
            bool openLog, int logLevel, int logQueSize
    );//构造函数
    ~WebServer();// 析构函数
public:
    void Run();//启动
private:
    void InitHttpConn();
    void InitListenSocket(int port,bool OptLinger);
    void InitEpoller(int trigMode);
    void _InitEventMode(int trigMode);

    // 处理客户端的请求
    void AddClient_(int fd, sockaddr_in addr);
    void DealListen_();
    void DealWrite_(HttpConn* client);
    void DealRead_(HttpConn* client);

    void SendError_(int fd, const char*info);
    void ExtentTime_(HttpConn* client);
    void CloseConn_(HttpConn* client);

    void OnRead_(HttpConn* client);
    void OnWrite_(HttpConn* client);
    void OnProcess(HttpConn* client);

    static const int MAX_FD = 65536;// 最大连接数量
private:
    char* srcDir_;// 当前网页资源所在目录
    bool isClose_;// 是否关闭服务器
    int timeoutMS_;  /* 毫秒MS */
    uint32_t listenEvent_;// 监听socket触发模式
    uint32_t connEvent_;// 连接socket触发模式
    // 基础服务模块
    std::unique_ptr<HeapTimer> m_pTimer;// 定时器模块
    std::unique_ptr<ListenSocket> m_pSocket;//套接字模块指针
    std::unique_ptr<Epoller> m_pEpoller;//IO复用模块epoll
    std::unique_ptr<ThreadPool> m_pThreadPool; // 线程池模块
    std::unordered_map<int, HttpConn> m_UsersDict;// http连接模块  <连接套接字,HttpConn对象>
    // 连接池、日志为单例模式

};


#endif // MRL_WEB_SERVER_H_