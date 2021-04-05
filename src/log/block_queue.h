// =============================================================================
// Created by yangb on 2021/4/3.
// =============================================================================

#ifndef WEBSERVERCPP11_SRC_LOG_BLOCK_QUEUE_H_
#define WEBSERVERCPP11_SRC_LOG_BLOCK_QUEUE_H_

#include <mutex>
#include <deque>
#include <condition_variable>
#include <ctime>
#include <cassert>

/// @brief 生产者-消费者模式
template <typename T>
class BlockQueue {
 public:
  explicit BlockQueue(size_t max_capacity = 1000);

  ~BlockQueue();

  void Close();
  void Flush();

  void clear();
  size_t size() const;
  size_t capacity() const;
  bool empty() const;
  bool full() const;
  T front();
  T back();
  void push_back(const T& item);
  void push_front(const T& item);
  bool pop(T& item);
  bool pop(T& item, int timeout);

 private:
  std::deque<T> deq_; // 竞争资源
  size_t capacity_;
  bool is_close_;

  mutable std::mutex mtx_;
  std::condition_variable cond_consumer_;
  std::condition_variable cond_producer_;
};

template <typename T>
BlockQueue<T>::BlockQueue(size_t max_capacity):capacity_(max_capacity), is_close_(false) {
  assert(max_capacity > 0);
}

template <typename T>
BlockQueue<T>::~BlockQueue() {
  Close();
}

template <typename T>
void BlockQueue<T>::Close() {
  {
    std::lock_guard<std::mutex> locker(mtx_);
    deq_.clear();
    is_close_ = true;
  }
  cond_producer_.notify_all();
  cond_consumer_.notify_all();
}

template <typename T>
void BlockQueue<T>::Flush() {
  cond_consumer_.notify_one();
}

template <typename T>
void BlockQueue<T>::clear() {
  std::lock_guard<std::mutex> locker(mtx_);
  deq_.clear();
}

template <typename T>
size_t BlockQueue<T>::size() const {
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.size();
}

template <typename T>
size_t BlockQueue<T>::capacity() const {
  std::lock_guard<std::mutex> locker(mtx_);
  return capacity_;
}

template <typename T>
bool BlockQueue<T>::empty() const {
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.empty();
}
template <typename T>
bool BlockQueue<T>::full() const {
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.size() >= capacity_;
}

template <typename T>
T BlockQueue<T>::front() {
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.front();
}

template <typename T>
T BlockQueue<T>::back() {
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.back();
}

template <typename T>
void BlockQueue<T>::push_back(const T& item) {
  std::unique_lock<std::mutex> locker(mtx_);
  while (deq_.size() >= capacity_) {  // 当前队列已满, 需先等待消费者处理一部分
    cond_producer_.wait(locker);
  }
  deq_.push_back(item);
  cond_consumer_.notify_one();
}

template <typename T>
void BlockQueue<T>::push_front(const T& item) {
  std::unique_lock<std::mutex> locker(mtx_);
  while (deq_.size() >= capacity_) {  // 当前队列已满, 需先等待消费者处理一部分
    cond_producer_.wait(locker);
  }
  deq_.push_front(item);
  cond_consumer_.notify_one();
}

template <typename T>
bool BlockQueue<T>::pop(T& item) {
  std::unique_lock<std::mutex> locker(mtx_);
  while (deq_.empty()) {
    cond_consumer_.wait(locker);
    if (is_close_) {
      return false;
    }
  }
  item = deq_.front();
  deq_.pop_front();
  cond_producer_.notify_one();
  return true;
}

template <typename T>
bool BlockQueue<T>::pop(T& item, int timeout) {
  std::unique_lock<std::mutex> locker(mtx_);
  while (deq_.empty()) {
    if (cond_consumer_.wait_for(locker, std::chrono::seconds(timeout)) == std::cv_status::timeout) {
      return false;
    }
    if (is_close_) {
      return false;
    }
  }
  item = deq_.front();
  deq_.pop_front();
  cond_producer_.notify_one();
  return true;
}

#endif //WEBSERVERCPP11_SRC_LOG_BLOCK_QUEUE_H_
