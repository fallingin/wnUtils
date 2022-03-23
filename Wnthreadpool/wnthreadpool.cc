#include "wnthreadpool.h"

#include <cassert>
#include <cstdio>

ThreadPool::~ThreadPool()
{
    if (running_) {
        stop();
    }
}
void ThreadPool::start(int numThreads)
{
    assert(threads_.empty());
	running_ = true;
	if (numThreads == 0 && threadInitCallback_) {
        threadInitCallback_();
		return ;
    }
    threads_.reserve(numThreads);
    for (int i = 0; i < numThreads; i++) {
        threads_.emplace_back(new std::thread(&ThreadPool::runInThread, this));
    }
}

void ThreadPool::stop()
{
    {
        std::lock_guard<std::mutex> lock { mutex_ };
        running_ = false;
        notEmpty_.notify_all();
    }
    for (auto& thr : threads_) {
        thr->join();
    }
}

size_t ThreadPool::queueSize() const
{
    std::lock_guard<std::mutex> lock { mutex_ };
    return queue_.size();
}

void ThreadPool::run(ThreadPool::Task task)
{
    if (threads_.empty()) {
        task();
    } else {
        std::unique_lock<std::mutex> lock { this->mutex_ };

		// 如果任务队列满了，就阻塞主线程
        while (isFull()) {
            notFull_.wait(lock);
        }
        assert(!isFull());

        queue_.emplace_back(std::move(task));
		// 当新任务到来就唤醒一个子线程
        notEmpty_.notify_one();
    }
}

ThreadPool::Task ThreadPool::take()
{
    std::unique_lock<std::mutex> lock { this->mutex_ };

	// 如果队列空了就阻塞当前子线程
    while (queue_.empty() && running_) {
        notEmpty_.wait(lock);
    }
    Task task;
    if (!queue_.empty()) {
        task = queue_.front();
        queue_.pop_front();
		// 如果任务队列中被取走一个任务就唤醒主线程
        if (maxQueueSize_ > 0) {
            notFull_.notify_one();
        }
    }
    return task;
}

bool ThreadPool::isFull() const
{
    return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
}

void ThreadPool::runInThread()
{
    try {
        if (threadInitCallback_) {
            threadInitCallback_();
        }
        while (running_) {
            Task task(take());
            if (task) {
                task();
            }
        }
    } catch (const std::exception& ex) {
        fprintf(stderr, "exception caught in ThreadPool\n");
        fprintf(stderr, "reason: %s\n", ex.what());
    } catch (...) {
        fprintf(stderr, "unknown exception caught in ThreadPool\n");
        throw; // rethrow
    }
}