// =============================================================================
// Created by yangb on 2021/4/1.
// 时间堆(最小堆实现, 底层数据结构为vector)
// =============================================================================

#ifndef WEBSERVERCPP11_SRC_TIMER_HEAP_TIMER_H_
#define WEBSERVERCPP11_SRC_TIMER_HEAP_TIMER_H_

#include <functional>
#include <chrono>
#include <vector>
#include <unordered_map>

using TimeoutCallBack = std::function<void()>;  // 回调函数指针
using Clock = std::chrono::high_resolution_clock; // 时钟
using Ms = std::chrono::milliseconds; // 毫秒
using TimeStamp = Clock::time_point;  // 时间戳

/// @brief 定时器结构体
struct TimerNode {
  int id;             // 句柄(文件描述符)
  TimeStamp expires;  // 生效时间
  TimeoutCallBack cb; // 回调函数

  TimerNode(int _id, TimeStamp _expires, const TimeoutCallBack& _cb)
      : id(_id), expires(_expires), cb(_cb) {}

  bool operator<(const TimerNode& rhs) const {
    return expires < rhs.expires;
  }
};

/// @brief 时间堆
class HeapTimer {
 public:
  explicit HeapTimer(int _n = 64) { heap_.reserve(_n); }
  ~HeapTimer() { clear(); }

  // 调整句柄id的生效时间
  void Adjust(int id, int new_expires);
  // 添加节点
  void Add(int id, int timeout, const TimeoutCallBack& cb);
  // 执行回调函数
  void DoWork(int id);
  // 心搏函数
  void Tick();
  // 删除堆顶节点
  void Pop();
  // 获取下一次心搏
  int GetNextTick();

  // 清空
  inline void clear() {
    this->heap_.clear();
    this->ref_.clear();
  }

 private:
  // 删除第i个节点
  void Del_(size_t i);
  // 上滤操作, 将第i个节点上滤
  void SiftUp_(size_t i);
  // 下滤操作, 将第j个节点下滤
  bool SiftDown_(size_t index, size_t n);
  // 交换两节点
  void SwapNode_(size_t i, size_t j);

 private:
  // 时间堆的底层结构为数组
  std::vector<TimerNode> heap_;
  // key: 句柄  value: 节点位置(在heap_中的位置)
  std::unordered_map<int, size_t> ref_;
};

#endif //WEBSERVERCPP11_SRC_TIMER_HEAP_TIMER_H_
