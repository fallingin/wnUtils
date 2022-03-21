#ifndef THREADPOOL_H
#define THREADPOOL_H


#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

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
  typedef std::function<void()> Task;

  explicit ThreadPool(const std::string& nameArg = std::string("ThreadPool"))
      : mutex_(), maxQueueSize_(0), running_(false) {}
  ~ThreadPool();

  // Must be called before start().
  void setMaxQueueSize(int maxSize) { maxQueueSize_ = maxSize; }
  void setThreadInitCallback(const Task& cb) { threadInitCallback_ = cb; }
  //

  void start(int numThreads);

  size_t queueSize() const;

  // Could block if maxQueueSize > 0
  void run(Task f);

 private:
  bool isFull() const;
  void runInThread();
  Task take();
  void stop();

  mutable std::mutex mutex_;
  std::condition_variable notEmpty_;
  std::condition_variable notFull_;
  Task threadInitCallback_;
  std::vector<std::unique_ptr<std::thread>> threads_;
  std::deque<Task> queue_;
  size_t maxQueueSize_;
  bool running_;
};
#endif  // THREADPOOL_H