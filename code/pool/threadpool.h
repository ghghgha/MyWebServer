/*
 * @Author: Wang
 * @Date: 2022-04-07 23:32:44
 * @Description: threadpool类的实现
 * @LastEditTime: 2022-05-19 16:25:30
 * @FilePath: \C++\pool\threadpool.h
 */

#ifndef WJ_THREADPOOL_H
#define  WJ_THREADPOOL_H

#include <mutex>// 互斥锁
#include <condition_variable>// 条件变量
#include <queue>
#include <thread>
#include<atomic>// 原子性
#include <functional>// 调用对象
#include<assert.h>


// 线程池
class ThreadPool {
public:
    using Task= std::function<void()>;// C++11允许使用using为模板定义别名，typedef可以给普通变量定义别名
    // 构造函数，explicit表明必须显示调用
    explicit ThreadPool(size_t  threadCount):
    // 初始化列表
    poolPtr(std::make_shared<Pool>()) {
        this->poolPtr->isClosed=false;
        this->AddThreads( threadCount);
    }
    // 析构函数
    ~ThreadPool() {
        // 判断该智能指针是否存在
        if(static_cast<bool>(this->poolPtr)) {
            {
                std::lock_guard<std::mutex> locker(this->poolPtr->queue_mutex);// 声明区加锁
                this->poolPtr->isClosed = true;// 设置线程池状态为关闭
            }
            this->poolPtr->condition.notify_all();//唤醒所有阻塞线程
            for(std::thread& t: this->poolPtr->threadsArr){
                if(t.joinable()){
                    t.join();// 如果有线程没有执行完，等待执行完毕
                }
            }
        }
    }
    // 禁用如下默认函数
    ThreadPool() = delete;// 默认构造函数
    ThreadPool(ThreadPool&&) = delete; // 移动构造函数
    ThreadPool(const ThreadPool&)=delete;// 复制构造函数
    ThreadPool & operator=(const ThreadPool& other)=delete;//=运算符重载函数
public:
    // 添加若干个线程
    void AddThreads(size_t  threadCount){
        assert(threadCount > 0);
        //创建threadCount个线程对象
        for(size_t i = 0; i < threadCount; i++) {
            // 创建一个线程对象，传入lambda表达式
            this->poolPtr->threadsArr.emplace_back(
                std::thread(
                        // lambda表达式
                        [pool = this->poolPtr] {
                            std::unique_lock<std::mutex> locker(pool->queue_mutex);//对以下声明块加锁
                            while(true) {
                                // 如果任务队列不为空
                                if(!pool->taskQue.empty()) {
                                    // 出队，获取一个任务,使用了移动语义
                                    auto task = std::move(pool->taskQue.front());
                                    pool->taskQue.pop();
                                    // 临时解锁，让其他线程去竞争锁
                                    locker.unlock();
                                    // 执行任务
                                    task();
                                    // 继续加锁
                                    locker.lock();
                                } 
                                else if(pool->isClosed) break;// 如果线程池关闭，跳出循环，结束线程
                                // 此时任务队列为空，且线程池没有关闭，
                                else pool->condition.wait(locker);//先阻塞当前线程，然后解锁locker,等待唤醒
                            }// 自动解锁
                       }
                )
            );
            
        }
    }
    // 函数模板，添加一个任务
    template<class F>
    void AddTask(F&& task) {
        if(!this->poolPtr->isClosed){
            std::lock_guard<std::mutex> locker(poolPtr->queue_mutex);// 声明区加锁
            this->poolPtr->taskQue.emplace(std::forward<F>(task));// 向任务队列中添加一个任务
            this->poolPtr->condition.notify_one();// 唤醒等待队列中的一个阻塞线程
        }
        else{
            throw std::runtime_error("theadpool is closed but have task adding");
        }
    }
private:
    struct Pool {
        // 数据结构
        std::queue<Task> taskQue;//任务队列，每个元素为函数对象类型
        std::vector<std::thread> threadsArr;//工作线程数组，每个元素为线程对象类型
        // 多线程同步
        std::mutex queue_mutex;// 互斥锁,   保证每一刻只有一个线程可以从任务队列中取出任务
        std::condition_variable condition;// 条件变量，控制阻塞线程的唤醒与阻塞
        // 标志变量
        std::atomic_bool  isClosed;// 原子布尔变量，是否关闭，可以像正常bool一样使用，但是该变量具有原子性
    };
    std::shared_ptr<Pool>  poolPtr;// 共享智能指针
};

// void test_pool(){
//     ThreadPool* pool=new  ThreadPool(8);
//     pool->AddTask([]{
//             //std::cout<<"hello,world!!"<<endl;
//     });
//     getchar();
// }
#endif // MRL_THREADPOOL_H