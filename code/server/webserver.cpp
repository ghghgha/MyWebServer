/*
 * @Author: Wang
 * @Date: 2022-03-21 20:48:47
 * @Description: WebServer类的实现
 * @LastEditTime: 2022-04-08 23:18:39
 * @FilePath: \WebServer\server\webserver.cpp
 */
#include"webserver.h"
#include<iostream>
using namespace std;

// 构造函数
WebServer::WebServer(
            // 端口号，epoll触发模式,最大等待时间，是否优雅断开socket
            int port, int trigMode, int timeoutMS, bool OptLinger,
            // SQL端口号，SQL用户名，SQL密码， 数据库名称，连接池数量
            int sqlPort, const char* sqlUser, const  char* sqlPwd,const char* dbName, int connPoolNum, 
            int threadNum,
            // 是否开启日志，日志，日志队列的容量
            bool openLog, int logLevel, int logQueSize
    ):isClose_(false),timeoutMS_(timeoutMS),m_pTimer(new HeapTimer()),
    m_pSocket(new ListenSocket()),m_pEpoller(new Epoller()), m_pThreadPool(new ThreadPool(threadNum)) {
    this->InitHttpConn();
    this->InitListenSocket(port,OptLinger); // 初始化监听socket
    this->InitEpoller(trigMode);// 初始化epoll模式
    //  初始化MySQL数据库连接池
    SqlConnPool::Instance()->InitPool("localhost", sqlPort, sqlUser, sqlPwd, dbName, connPoolNum);
    // 初始化日志系统
    if(openLog) {
        //Log::Instance()->init(logLevel, "./log", ".log", logQueSize);
        if(isClose_) { LOG_ERROR("========== Server init error!=========="); }
        else {
            LOG_INFO("========== Server init ==========");
            LOG_INFO("Port:%d, OpenLinger: %s", port, OptLinger? "true":"false");
            LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                            (listenEvent_ & EPOLLET ? "ET": "LT"),
                            (connEvent_ & EPOLLET ? "ET": "LT"));
            LOG_INFO("LogSys level: %d", logLevel);
            LOG_INFO("srcDir: %s", HttpConn::srcDir);
            LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", connPoolNum, threadNum);
        }
    }
}

// 析构函数
WebServer::~WebServer(){
    isClose_ = true;
    free(srcDir_);
    SqlConnPool::Instance()->ClosePool();
}

// 启动
void WebServer::Run(){
    int timeMS = -1;  /* epoll wait timeout == -1 无事件将阻塞 */
    if(!isClose_) { 
        LOG_INFO("========== Server start ==========");
        cout<<"服务器程序运行.."<<endl;
     }
    // 循环监听epoll事件
    int _listenFd=m_pSocket->GetListenFd();
    while(!isClose_) {
        if(timeoutMS_ > 0) {
            timeMS = m_pTimer->GetNextTick();
        }
        int eventCnt = m_pEpoller->Wait(timeMS);
        for(int i = 0; i < eventCnt; i++) {
            /* 处理事件 */
            int fd = m_pEpoller->GetEventFd(i);
            uint32_t events = m_pEpoller->GetEvents(i);
            // 处理客户端的连接事件
            if(fd == _listenFd) {
                DealListen_();
            }
            // 处理客户端的关闭/出错
            else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(m_UsersDict.count(fd) > 0);// 如果没有找到该用户
                CloseConn_(&m_UsersDict[fd]);
            }
            // 处理可读事件
            else if(events & EPOLLIN) {
                assert(m_UsersDict.count(fd) > 0);
                DealRead_(&m_UsersDict[fd]);
            }
            // 处理可写事件
            else if(events & EPOLLOUT) {
                assert(m_UsersDict.count(fd) > 0);
                DealWrite_(&m_UsersDict[fd]);
            } else {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}
void WebServer::InitHttpConn(){
    // 初始化数据连接的变量
    srcDir_ = getcwd(nullptr, 256);//当前工作目录
    assert(srcDir_);
    strncat(srcDir_, "/web/", 16);
    HttpConn::userCount = 0;
    HttpConn::srcDir = srcDir_;
}
void WebServer::InitListenSocket(int port,bool OptLinger){
    // 初始化监听socket
    if(!m_pSocket->Init(port,OptLinger)){
        isClose_ = true;
    }
}
void WebServer::InitEpoller(int trigMode){
    this->_InitEventMode(trigMode);
    int _listenFd=m_pSocket->GetListenFd();
    // 注册epoll事件
    int ret = m_pEpoller->AddFd(_listenFd,  listenEvent_ | EPOLLIN);
    if(ret == 0) {
        LOG_ERROR("Add listen error!");
        m_pSocket->Close();
        isClose_ = true;
    }
    m_pSocket->SetFdNonblock(_listenFd);
}

void WebServer::_InitEventMode(int trigMode) {
    listenEvent_ = EPOLLRDHUP;
    connEvent_ = EPOLLONESHOT | EPOLLRDHUP;
    switch (trigMode)
    {
    case 0:
        break;
    case 1:
        connEvent_ |= EPOLLET;
        break;
    case 2:
        listenEvent_ |= EPOLLET;
        break;
    case 3:
        listenEvent_ |= EPOLLET;
        connEvent_ |= EPOLLET;
        break;
    default:
        listenEvent_ |= EPOLLET;
        connEvent_ |= EPOLLET;
        break;
    }
    HttpConn::isET = (connEvent_ & EPOLLET);
}

void WebServer::SendError_(int fd, const char*info) {
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if(ret < 0) {
        LOG_WARN("send error to client[%d] error!", fd);
    }
    close(fd);
}

void WebServer::CloseConn_(HttpConn* client) {
    assert(client);
    LOG_INFO("Client[%d] quit!", client->GetFd());
    m_pEpoller->DelFd(client->GetFd());
    client->Close();
}

void WebServer::AddClient_(int fd, sockaddr_in addr) {
    assert(fd > 0);
    m_UsersDict[fd].init(fd, addr);// 初始化一个http连接
    if(timeoutMS_ > 0) {
        m_pTimer->add(fd, timeoutMS_, std::bind(&WebServer::CloseConn_, this, &m_UsersDict[fd]));
    }
    m_pEpoller->AddFd(fd, EPOLLIN | connEvent_);
    m_pSocket->SetFdNonblock(fd);
    LOG_INFO("Client[%d] in!", m_UsersDict[fd].GetFd());
}

void WebServer::DealListen_() {
    struct sockaddr_in addr;
    do {
        int fd =m_pSocket->Accept(&addr);
        if(fd <= 0) { return;}
        else if(HttpConn::userCount >= MAX_FD) {
            SendError_(fd, "Server busy!");
            LOG_WARN("Clients is full!");
            return;
        }
        AddClient_(fd, addr);
    } while(listenEvent_ & EPOLLET);
}

void WebServer::DealRead_(HttpConn* client) {
    assert(client);
    ExtentTime_(client);
    m_pThreadPool->AddTask(std::bind(&WebServer::OnRead_, this, client));
}

void WebServer::DealWrite_(HttpConn* client) {
    assert(client);
    ExtentTime_(client);
    m_pThreadPool->AddTask(std::bind(&WebServer::OnWrite_, this, client));
}

void WebServer::ExtentTime_(HttpConn* client) {
    assert(client);
    if(timeoutMS_ > 0) { m_pTimer->adjust(client->GetFd(), timeoutMS_); }
}

void WebServer::OnRead_(HttpConn* client) {
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->read(&readErrno);
    if(ret <= 0 && readErrno != EAGAIN) {
        CloseConn_(client);
        return;
    }
    OnProcess(client);//
}

void WebServer::OnProcess(HttpConn* client) {
    if(client->process()) {
        m_pEpoller->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);//修改事件
    } else {
        m_pEpoller->ModFd(client->GetFd(), connEvent_ | EPOLLIN);//修改事件
    }
}

void WebServer::OnWrite_(HttpConn* client) {
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);
    if(client->ToWriteBytes() == 0) {
        /* 传输完成 */
        if(client->IsKeepAlive()) {
            OnProcess(client);
            return;
        }
    }
    else if(ret < 0) {
        if(writeErrno == EAGAIN) {
            /* 继续传输 */
            m_pEpoller->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
            return;
        }
    }
    CloseConn_(client);
}