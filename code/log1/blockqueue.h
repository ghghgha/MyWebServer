/*
 * @Author: Wang
 * @Date:  2022-03-28 15:39:15
 * @Description: blockqueue类的头文件
 * @LastEditTime: 2022-03-28 23:23:13
 * @FilePath:  /WebServer/log/blockqueue.h
 */
#ifndef WJ_LBLOCKQUEUE_H_
#define WJ_LBLOCKQUEUE_H_
#include <mutex>//互斥锁
#include <deque>//双端队列
#include <condition_variable>//条件变量
#include <sys/time.h>

// 阻塞队列的模板类
template<class T>
class BlockDeque {
public:
    // 构造函数
    explicit BlockDeque(size_t MaxCapacity = 1000);//explicit限制其隐式调用
    // 析构函数
    ~BlockDeque();
    // 向队列中添加一个元素
    void push_back(const T &item);
    void push_front(const T &item);
    // 从队列中取出一个元素
    bool pop(T &item);
    bool pop(T &item, int timeout);
public:
    void flush();
    void Close();
    // 附加操作
    void clear();
    bool empty();
    bool full();
    size_t size();
    size_t capacity();
    T front();// 获取队列的队首元素
    T back();// 获取队列的队尾元素
private:
    bool isClose_;//队列是否关闭
    size_t capacity_;//阻塞队列的最大容量
    std::deque<T> deq_;// 双端队列容器
    //互斥锁
    std::mutex mtx_;
    // 条件变量，生产者和消费者是互斥关系，对缓存区的访问互斥，同时只有生产者生产后，消费者才能消费
    std::condition_variable condConsumer_;//消费者
    std::condition_variable condProducer_;//生产者
};

// 构造函数
template<class T>
BlockDeque<T>::BlockDeque(size_t MaxCapacity) :capacity_(MaxCapacity) {
    assert(MaxCapacity > 0);//如果输入的最大容量为0则报错
    isClose_ = false;//
}
// 析构函数
template<class T>
BlockDeque<T>::~BlockDeque() {
    Close();
};
// 刷新队列
template<class T>
void BlockDeque<T>::flush() {
    condConsumer_.notify_one();
};

// 在队列的队尾添加元素，当前操作线程相当于生产者线程，生产了一个元素
// 它将元素加入队列时，先查看队列是否已满，如果满了则阻塞等待
// 如果没有满，则将元素加入队列，并唤醒因消费者变量而阻塞的线程
template<class T>
void BlockDeque<T>::push_back(const T &item) {
    std::unique_lock<std::mutex> locker(mtx_);//对接下来的操作加锁
    // 当队列已满时，当前
    while(deq_.size() >= capacity_) {
        condProducer_.wait(locker);//当前生产者线程阻塞等待锁释放
    }
    // 将元素加入队列
    deq_.push_back(item);
    condConsumer_.notify_one();
}
// 在队列的队首添加元素
template<class T>
void BlockDeque<T>::push_front(const T &item) {
    std::unique_lock<std::mutex> locker(mtx_);
    // 当前队列已满时，当前操作线程会被挂起
    while(deq_.size() >= capacity_) {
        condProducer_.wait(locker);
    }
    deq_.push_front(item);
    condConsumer_.notify_one();
}
// 从队列的队首取出一个元素，相当于
template<class T>
bool BlockDeque<T>::pop(T &item) {
    std::unique_lock<std::mutex> locker(mtx_);//对以下操作加锁
    // 当队列为空时，
    while(deq_.empty()){
        condConsumer_.wait(locker);//消费者线程阻塞等待
        if(isClose_){
            return false;
        }
    }
    //出队
    item = deq_.front();
    deq_.pop_front();
    // 唤醒在生产者变量阻塞的生产者线程
    condProducer_.notify_one();
    return true;
}

template<class T>
bool BlockDeque<T>::pop(T &item, int timeout) {
    std::unique_lock<std::mutex> locker(mtx_);
    // 当队列为空时，当前操作线程将会被挂起
    while(deq_.empty()){
        if(condConsumer_.wait_for(locker, std::chrono::seconds(timeout)) 
                == std::cv_status::timeout){
            return false;
        }
        if(isClose_){
            return false;
        }
    }
    item = deq_.front();
    deq_.pop_front();
    condProducer_.notify_one();
    return true;
}





// 获取队列的队首元素
template<class T>
T BlockDeque<T>::front() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.front();
}
// 获取队列的队尾元素
template<class T>
T BlockDeque<T>::back() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.back();
}

// 关闭函数
template<class T>
void BlockDeque<T>::Close() {
    {   
        std::lock_guard<std::mutex> locker(mtx_);//上锁
        deq_.clear();//清空队列
        isClose_ = true;
    }
    condProducer_.notify_all();
    condConsumer_.notify_all();
};


// 判断队列是否为空
template<class T>
bool BlockDeque<T>::empty() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.empty();
}
// 判断队列是否为满
template<class T>
bool BlockDeque<T>::full(){
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size() >= capacity_;
}
// 获取队列的元素个数
template<class T>
size_t BlockDeque<T>::size() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size();
}
// 获取队列的容量
template<class T>
size_t BlockDeque<T>::capacity() {
    std::lock_guard<std::mutex> locker(mtx_);
    return capacity_;
}
// 清空队列
template<class T>
void BlockDeque<T>::clear() {
    std::lock_guard<std::mutex> locker(mtx_);
    deq_.clear();
}
#endif /*MRL_LBLOCKQUEUE_H_*/