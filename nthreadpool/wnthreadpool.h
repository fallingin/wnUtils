#ifndef WNTHREADPOOL_H
#define WNTHREADPOOL_H

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

// 单例类
class noncopyable {
public:
    noncopyable(const noncopyable&) = delete;
    void operator=(const noncopyable&) = delete;

protected:
    noncopyable() = default;
    ~noncopyable() = default;
};

class ThreadPool : noncopyable {
public:
    // 万能function模板
    typedef std::function<void()> Task;

    explicit ThreadPool(const std::string& nameArg = std::string("ThreadPool"))
        : mutex_()
        , maxQueueSize_(0)
        , running_(false)
    {
    }
    ~ThreadPool();

    // 必须在start()前调用
    void setMaxQueueSize(int maxSize) { maxQueueSize_ = maxSize; }
    // 设置线程初始化要调用的函数
    void setThreadInitCallback(const Task& cb) { threadInitCallback_ = cb; }
    // 启动线程池
    void start(int numThreads);

    size_t queueSize() const;
    // 将任务放入任务队列
    void run(Task f);

private:
    bool isFull() const;
    void runInThread();
    Task take();
    void stop();

    // 用于队列的锁
    mutable std::mutex mutex_;
    // 唤醒 / 阻塞子线程 :队列空
    std::condition_variable notEmpty_;
    // 唤醒 / 阻塞主线程 :队列满
    std::condition_variable notFull_;
    Task threadInitCallback_;
    std::vector<std::unique_ptr<std::thread>> threads_;
    std::deque<Task> queue_;
    size_t maxQueueSize_;
    bool running_;
};
#endif // WNTHREADPOOL_H