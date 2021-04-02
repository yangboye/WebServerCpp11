// =============================================================================
// Created by yangb on 2021/4/2.
// 线程池
// =============================================================================

#ifndef WEBSERVERCPP11_SRC_POOL_THREAD_POOL_H_
#define WEBSERVERCPP11_SRC_POOL_THREAD_POOL_H_

#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <memory>
#include <cassert>
#include <thread>

class ThreadPool {
 public:
  ThreadPool() = default;

  // 默认 move constructor
  ThreadPool(ThreadPool&&) = default;

  ~ThreadPool() {
    if (static_cast<bool>(pool_)) {
      std::lock_guard<std::mutex> locker(pool_->mtx);
      pool_->is_closed = true;
    }
    pool_->cond.notify_all();
  }

  /// @brief 构造函数
  /// @param thread_count 线程池中线程的数量
  explicit ThreadPool(size_t thread_count = 8) : pool_(std::make_shared<Pool>()) {
    assert(thread_count > 0);
    for (size_t i = 0; i < thread_count; ++i) {
      std::thread([pool = pool_] {
        std::unique_lock<std::mutex> locker(pool->mtx);
        while (true) {
          if (!pool->tasks.empty()) {
            auto task = std::move(pool->tasks.front());
            pool->tasks.pop();
            locker.unlock();
            task(); // 执行函数
            locker.lock();
          } else if (pool->is_closed) {
            break;
          } else {
            pool->cond.wait(locker);
          }
        } // while
      }).detach();
    } // for
  } // ThreadPool

  template <typename F>
  void AddTask(F&& task) {
    {
      std::lock_guard<std::mutex> locker(pool_->mtx);
      pool_->tasks.emplace(std::forward<F>(task));
    }
    pool_->cond.notify_one();
  }

 private:
  struct Pool {
    std::mutex mtx;
    std::condition_variable cond;
    bool is_closed;
    std::queue<std::function<void()>> tasks;
  };

 private:
  std::shared_ptr<Pool> pool_;
};

#endif //WEBSERVERCPP11_SRC_POOL_THREAD_POOL_H_
