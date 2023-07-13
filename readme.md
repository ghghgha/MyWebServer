# 基于Linux的C++高性能web服务器    

## 技术栈

* 利用I/O复用技术Epoll 与线程池实现多线程的Reactor高并发模型;
* 利用标准库容器封装字符，实现自动增长的缓冲区，可以一次性将所有的数据读入，大大提高了效率；
* 利用正则与状态机解析 HTTP 请报文，实现处理静态资源的请求;
* 基于堆结构实现定时器，关闭超时的非活动连接，有效提高服务器的性能；
* 利用单例模式与阻塞队列实现异步的日志系统，记录服务器的运行状态；
* 利用RAII机制实现数据库连接池，减少数据库连接建立与关闭的开销，同时实现用户注册登录功能。

## 环境要求
* Linux操作系统
* C++14编程语言
* MySql数据库
* MakeFIle项目编译工具

## 目录树
```tex
.
├── code           源代码
│   ├── buffer     缓存区模块    
│   ├── http       HTTP模块
│   ├── log        简易日志模块
│   ├── log1       异步日志模块
│   ├── pool       线程池和MySQL连接池
│   ├── server     socket模块、Epoll模块和主程序
│   ├── timer      定时器模块
│   └── main.cpp   入口函数
├── web            Web静态资源
│   ├── index.html 网站首页
│   ├── images     图片资源
│   ├── video      视频资源
│   ├── js         脚本资源
│   └── css        样式资源
├── bin            可执行文件
│   └── WebServer_v1.0
├── log            日志文件
├── webbench-1.5   第三方压力测试工具
├── build          编译指令 
│   └── Makefile
├── Makefile       启动指令  
├── LICENSE
└── readme.md
```


## 项目启动
需要先配置好对应的数据库
```bash
// 建立yourdb库
create database yourdb;

// 创建user表
USE yourdb;
CREATE TABLE user(
    username char(50) NULL,
    password char(50) NULL
)ENGINE=InnoDB;

// 添加数据
INSERT INTO user(username, password) VALUES('name', 'password');
```

```bash
make
./bin/WebServer_v1.0
```

## 压力测试
```bash
./webbench-1.5/webbench -c 100 -t 10 http://ip:port/
./webbench-1.5/webbench -c 1000 -t 10 http://ip:port/
./webbench-1.5/webbench -c 5000 -t 10 http://ip:port/
./webbench-1.5/webbench -c 10000 -t 10 http://ip:port/
```
* 测试环境: Ubuntu:19.10 cpu:i5-8400 内存:8G 
* QPS 10000+


