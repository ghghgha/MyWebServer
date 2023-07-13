/*
 * @Author: Wang
 * @Date: 2022-03-21 22:46:06
 * @Description: SqlConnPool的头文件
 * @LastEditTime: 2022-03-23 17:41:33
 * @FilePath: \WebServer\pool\sqlconnpool.h
 */
#ifndef WJ_SQL_CONN_POOL_H_
#define WJ_SQL_CONN_POOL_H_

#include <mysql/mysql.h>//MSQL数据库
#include <queue>//队列容器
#include <mutex>//互斥锁
#include <semaphore.h>//信号量
#include <thread>
#include<assert.h>
#include <string>
//#include "../log/log.h"
#include<iostream>
using namespace std;

// MySQL数据库连接池类
class SqlConnPool
{
private:
    int MAX_CONN_;// 连接池最大连接数
    int USED_CONN_;//连接池已用连接数
    int FREE_CONN_;//连接池空闲连接数
    queue<MYSQL *> connQue_;// MySQl连接的队列容器
    sem_t  semId_;//信号量
    mutex mtx_;//互斥量
private:
    SqlConnPool();// 构造函数
    ~SqlConnPool();// 析构函数
public:
    static SqlConnPool *Instance();// 单例函数
    // 初始连接池
    void InitPool(const char* host, int port,
              const char* user,const char* pwd, 
              const char* dbName, int connSize);
    // 关闭连接池
    void ClosePool();
    // 获取一个连接
    MYSQL *GetConn();
    // 释放一个连接
    void FreeConn(MYSQL * conn);
    // 获取空闲的连接数
    int GetFreeConnCount();
    

};



#endif //MRL_SQL_CONN_POOL_H_