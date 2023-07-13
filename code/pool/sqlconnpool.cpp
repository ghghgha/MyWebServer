/*
 * @Author: Wang
 * @Date: 2022-03-21 22:46:32
 * @Description: SqlConnPool的实现
 * @LastEditTime: 2022-03-28 15:27:37
 * @FilePath: \WebServer\pool\sqlconnpool.cpp
 */
#include"sqlconnpool.h"


// 构造函数
SqlConnPool::SqlConnPool(){
    USED_CONN_ = 0;
    FREE_CONN_ = 0;
}
// 析构函数
SqlConnPool::~SqlConnPool(){
    ClosePool();
}

// 单例函数
SqlConnPool* SqlConnPool::Instance() {
    static SqlConnPool connPool;
    return &connPool;
}
// 初始连接池
void SqlConnPool::InitPool(const char* host, int port,
            const char* user,const char* pwd, const char* dbName,
            int connSize = 10) {
    assert(connSize > 0);
    MAX_CONN_ = connSize;
    // 初始化若干个连接
    for (int i = 0; i < MAX_CONN_; i++) {
        MYSQL *conn = nullptr;
        // 初始化连接
        conn = mysql_init(conn);
        if (!conn) {
            cout<<"MySql初始化出错"<<endl;
            //LOG_ERROR("MySql init error!");
            assert(conn);
        }
        // 建立数据库连接
        conn = mysql_real_connect(conn, host, user, pwd,dbName, port, NULL, 0);
        if (!conn) {
            cout<<"MySql连接出错"<<endl;
            //LOG_ERROR("MySql Connect error!");
        }
        // 添加到容器
        connQue_.push(conn);//添加到队列
    }
    // 初始化信号量
    sem_init(&semId_, 0, MAX_CONN_);
    cout<<"成功初始化连接池"<<endl;
}
// 关闭连接池
void SqlConnPool::ClosePool() {
    lock_guard<mutex> locker(mtx_);//在声明周期内对互斥量上锁
    // 遍历队列，关闭所有连接
    while(!connQue_.empty()) {
        // 元素出队
        auto item = connQue_.front();
        connQue_.pop();
        // 关闭连接
        mysql_close(item);
    }
    mysql_library_end();  // 关闭MySQL API库
    cout<<"成功关闭连接池"<<endl;
}
// 获取一个连接
MYSQL* SqlConnPool::GetConn() {
    MYSQL *conn = nullptr;
    if(connQue_.empty()){
        cout<<"SqlConnPool busy!"<<endl;
        return conn;
    }
    // // 具备信号量的线程才能执行
    sem_wait(&semId_);
    {
        lock_guard<mutex> locker(mtx_);//在声明周期内对互斥量上锁
        //一个连接出队
        conn = connQue_.front();
        connQue_.pop();
    }
    return conn;
}
// 释放一个连接
void SqlConnPool::FreeConn(MYSQL* sql) {
    assert(sql);
    lock_guard<mutex> locker(mtx_);//在声明周期内对互斥量上锁
    connQue_.push(sql);//入队
    sem_post(&semId_);//释放一个信号量
}

// 获取空闲的连接数量
int SqlConnPool::GetFreeConnCount() {
    lock_guard<mutex> locker(mtx_);//在声明周期内对互斥量上锁
    return connQue_.size();//此时连接池的数量
}