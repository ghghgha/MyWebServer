/*
 * @Author: Wang
 * @Date: 2022-04-07 23:32:44
 * @Description: threadpool类的实现
 * @LastEditTime: 2022-05-19 16:22:51
 * @FilePath: \C++\pool\threadpool.cpp
 */

#ifndef WJ_THREADPOOL_H
#define  WJ_THREADPOOL_H

#include <mutex>// 互斥锁
#include <condition_variable>// 条件变量
#include <queue>
#include <thread>
#include <functional>// 调用对象
using namespace std;
// 线程池
class ThreadPool {
public:
    // 构造函数，初始化列表
    explicit ThreadPool(size_t threadCount = 8): pool_(std::make_shared<Pool>()) {
            assert(threadCount > 0);
            // 创建threadCount个线程对象
            for(size_t i = 0; i < threadCount; i++) {
                // 创建一个线程对象，传入lambda表达式
                std::thread(
                    // lambda表达式
                    [pool = pool_] {
                        std::unique_lock<std::mutex> locker(pool->mtx);//对以下声明块加锁
                        while(true) {
                            // 如果任务队列不为空
                            if(!pool->tasks.empty()) {
                                // 出队，获取一个任务
                                auto task = std::move(pool->tasks.front());
                                pool->tasks.pop();
                                // 临时解锁，让其他线程去竞争锁
                                locker.unlock();
                                // 执行任务
                                task();
                                // 继续加锁
                                locker.lock();
                            } 
                            else if(pool->isClosed) break;// 如果线程池关闭，跳出循环，结束线程
                            else pool->cond.wait(locker);// 此时任务队列为空，且线程池没有关闭，先阻塞当前线程，然后解锁locker
                        }
                    }
                ).detach();//不阻塞主线程，工作线程驻留后台运行
            }
    }

    ThreadPool() = default;// 默认构造函数
    ThreadPool(ThreadPool&&) = default; // 移动构造函数
    // 析构函数
    ~ThreadPool() {
        if(static_cast<bool>(pool_)) {
            {
                std::lock_guard<std::mutex> locker(pool_->mtx);
                pool_->isClosed = true;
            }
            pool_->cond.notify_all();
        }
    }

    template<class F>
    void AddTask(F&& task) {
        {
            std::lock_guard<std::mutex> locker(pool_->mtx);// 声明区加锁
            pool_->tasks.emplace(std::forward<F>(task));// 向任务队列中添加一个任务
        }
        pool_->cond.notify_one();// 唤醒等待队列中的一个阻塞线程
    }

private:
    struct Pool {
        std::mutex mtx;// 互斥锁
        std::condition_variable cond;// 条件变量
        bool isClosed;// 是否关闭
        std::queue<std::function<void()>> tasks;//任务队列，存储函数对象，存储需要放入线程池中的任务
    };
    std::shared_ptr<Pool> pool_;// 共享智能指针
};



#endif // MRL_THREADPOOL_H