// =============================================================================
// Created by yangb on 2021/4/1.
// =============================================================================

#include <cassert>
#include "heap_timer.h"

void HeapTimer::Adjust(int id, int new_expires) {
  assert(!heap_.empty() && ref_.count(id) != 0);
  heap_[ref_[id]].expires = Clock::now() + static_cast<Ms>(new_expires);
  // 更新生效时间后, 一定比之前的生效时间大, 所以只需要执行下滤即可
  SiftDown_(ref_[id], heap_.size());
}

void HeapTimer::Add(int id, int timeout, const TimeoutCallBack& cb) {
  assert(id >= 0);
  size_t i;

  if (ref_.count(id) == 0) { // 新的节点, 先插入堆尾, 然后再调整
    i = heap_.size();
    ref_[id] = i;
    heap_.push_back({id, Clock ::now() + static_cast<Ms>(timeout), cb});
//    heap_.emplace_back(TimerNode(id, Clock::now() + static_cast<Ms>(timeout), cb));
    SiftUp_(i);
  } else {  // 已有节点, 更新后调整堆
    i = ref_[id];
    heap_[i].expires = Clock::now() + static_cast<Ms>(timeout);
    heap_[i].cb = cb;
    // 调整节点
    if (!SiftDown_(i, heap_.size())) {
      SiftUp_(i);
    }
  }
}

void HeapTimer::DoWork(int id) {
  // 删除指定id节点, 并触发回调函数
  if (heap_.empty() || ref_.count(id) == 0) {
    return;
  }
  size_t i = ref_[id];
  auto node = heap_[i];
  node.cb();  // 执行回调函数
  Del_(i);    // 删除该节点
}

void HeapTimer::Tick() {
  // 清楚超时节点
  if (heap_.empty()) {
    return;
  }
  while (!heap_.empty()) {
    TimerNode node = heap_.front();
    if (std::chrono::duration_cast<Ms>(node.expires - Clock::now()).count() > 0) {
      break;
    }
    node.cb();
    Pop();
  }
}

void HeapTimer::Pop() {
  // 删除堆顶节点
  assert(!heap_.empty());
  Del_(0);
}

int HeapTimer::GetNextTick() {
  Tick();
  size_t res = -1;
  if (!heap_.empty()) {
    res = std::chrono::duration_cast<Ms>(heap_.front().expires - Clock::now()).count();
    if (res < 0) {
      res = 0;
    }
  }

  return res;
}

void HeapTimer::Del_(size_t i) {
  assert(!heap_.empty() && i >= 0 && i < heap_.size());
  // 将要删除的节点换到队尾, 然后调整堆
  size_t n = heap_.size() - 1;

  if (i < n) {
    SwapNode_(i, n);
    if (!SiftDown_(i, n)) {  // 下滤失败, 尝试上滤
      SiftUp_(i);
    }
  }

  // 删除队尾元素
  ref_.erase(heap_.back().id);
  heap_.pop_back();
}

void HeapTimer::SiftUp_(size_t i) {
  assert(i >= 0 && i < heap_.size());
  size_t j = (i - 1) / 2; // 父节点
  while (j >= 0) {
    // 如果父节点已经比子节点小
    if (heap_[j] < heap_[i]) {
      break;
    }
    SwapNode_(i, j);
    i = j;
    j = (i - 1) / 2;
  }
}

bool HeapTimer::SiftDown_(size_t index, size_t n) {
  assert(index >= 0 && index < heap_.size());
  assert(n >= 0 && n <= heap_.size());

  int i = index;
  int j = 2 * i + 1;
  while (j < n) {
    // 取左右节点中生效时间最小的
    if (j + 1 < n && heap_[j + 1].expires < heap_[j].expires) {
      ++j;
    }
    // 如果根节点的生效时间已经比左右节点的生效时间小
    if (heap_[i] < heap_[j]) {
      break;
    }
    // 生效时间小的向上移动
    SwapNode_(i, j);
    i = j;
    j = i * 2 + 1;
  }

  return i > index;
}

void HeapTimer::SwapNode_(size_t i, size_t j) {
  assert(i >= 0 && i < heap_.size());
  assert(j >= 0 && j < heap_.size());

  std::swap(heap_[i], heap_[j]);
  ref_[heap_[i].id] = i;
  ref_[heap_[j].id] = j;
}
