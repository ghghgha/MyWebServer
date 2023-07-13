/*
 * @Author: Wang
 * @Date: 2022-04-07 23:32:44
 * @Description: socket类的实现
 * @LastEditTime: 2022-04-08 14:47:45
 * @FilePath: \C++\NetCode\socket_epoll.cpp
 */

 #include"socket.h"
//初始化套接字并监听
bool ListenSocket::Init(int port,bool openLinger){
    LOG_DEBUG("开始创建监听socket...\n");
    if(!this->CreateSocket()){
        return false;
    }
    if(!this->SetLinger(openLinger)){
        return false;
    }
    if(!this->Bind_IPv4(port)){
        return false;
    }
    if(!this->Listen(5)){
        return false;
    }
    LOG_DEBUG("监听socket创建成功并开始监听...\n");
    return true;
}
// 获取本机IP地址
const char * ListenSocket::GetIP(){
    char name[256];
    gethostname(name,sizeof(name));
    struct hostent* host=gethostbyname(name);
    char ipStr[32];
    inet_ntop(host->h_addrtype,host->h_addr_list[0],ipStr,sizeof(ipStr));
    char * p_str=nullptr;
    strncpy(p_str,ipStr,strlen(ipStr));// 将局部变量复制给指针
    //printf("IP:%s  Port:%d \n",  ipStr);
    return p_str;
}
// 创建socket
bool ListenSocket::CreateSocket(){
     // 创建一个监听套接字
    m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
    if(m_listenfd == -1) {
        LOG_ERROR("创建socket出错：%s(errno:%d)\n",strerror(errno),errno);
        return false;
    }
    else {
        LOG_DEBUG("创建socket成功\n");
        return true;
    }
 }
// 绑定socket地址
bool ListenSocket::Bind_IPv4(int port){
    if(port > 65535 || port < 1024) {
        LOG_ERROR("Port:%d 不符合要求!",  port);
        return false;
    }
    m_port=port;
    // 创建一个IPv4的socket地址
    struct sockaddr_in address;//socket地址
    bzero(&address, sizeof(address));//将该地址清空为0
    address.sin_family = AF_INET;//设置IPv4的地址族
    //inet_pton(AF_INET,ip,&address.sin_addr);//手动设置IP地址
    address.sin_addr.s_addr = htonl(INADDR_ANY);// 自动获取本机的IP地址并且设置
    address.sin_port = htons(m_port);//端口号
    //绑定socket地址
    int flag = 1;
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));//允许重用本地地址和端口
    int ret = bind(m_listenfd, (struct sockaddr *)&address, sizeof(address));//绑定socket地址
    if(ret == -1) {
        LOG_ERROR("绑定socket出错：%s(errno:%d)\n",strerror(errno),errno);
        return false;
    }
    else {
        LOG_DEBUG("绑定socket成功\n");
        return true;
    }
}
// 设置TCP的断开方式
bool ListenSocket::SetLinger(bool openLinger_){
    // TCP的优雅断开
    struct linger optLinger = { 0 };
    if(openLinger_) {
        /* 优雅关闭: 直到所剩数据发送完毕或超时 */
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
    }
    int ret = setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if(ret < 0) {
        this->Close();
        LOG_ERROR("设置TCP的优雅断开（linger）出错!");
        return false;
    }
    /* 端口复用 */
    /* 只有最后一个套接字会正常接收数据。 */
    int optval = 1;
    ret = setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    if(ret == -1) {
        this->Close();
        LOG_ERROR("set socket setsockopt error !");
        return false;
    }
    return true;
}
// 监听socket
bool ListenSocket::Listen(int backlog){
    // 开启监听
    int ret = listen(m_listenfd, backlog);
    if(ret == -1) {
        LOG_ERROR("监听socket出错：%s(errno:%d)\n",strerror(errno),errno);
        return false;
    }
    else {
        LOG_DEBUG("监听socket成功\n");
        return true;
    }
}
int ListenSocket::Accept(struct sockaddr_in* addr_){
    socklen_t len = sizeof(*addr_);
    int fd = accept(m_listenfd, (struct sockaddr *)addr_, &len);
    return fd;
}
// 设置socket为非阻塞模式
int ListenSocket::SetFdNonblock(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}

