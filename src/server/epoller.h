// =============================================================================
// Created by yangb on 2021/4/12.
// =============================================================================

#ifndef WEBSERVERCPP11_SRC_SERVER_EPOLLER_H_
#define WEBSERVERCPP11_SRC_SERVER_EPOLLER_H_

#include <vector>
#include <sys/epoll.h>  // epoll_ctl
#include <fcntl.h>  // fcntl
#include <unistd.h> // close
#include <cassert>

class Epoller {
 public:
  explicit Epoller(int max_event = 1024) : epoll_fd_(epoll_create(512)), events_(max_event) {
    assert(epoll_fd_ >= 0 && !events_.empty());
  }

  ~Epoller() {
    close(epoll_fd_);
  }

  /// @brief 添加
  inline bool AddFd(int fd, uint32_t events) { // NOLINT
    if (fd < 0) {
      return false;
    }

    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return (0 == epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev));
  }

  /// @brief 修改
  inline bool ModFd(int fd, uint32_t events) {  // NOLINT
    if (fd < 0) {
      return false;
    }

    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return (0 == epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev));
  }

  /// @brief 删除
  inline bool DelFd(int fd) { // NOLINT
    if (fd < 0) {
      return false;
    }

    epoll_event ev = {0};
    return (0 == epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, &ev));
  }

  inline int Wait(int timeout/*ms*/ = -1) {
    return epoll_wait(epoll_fd_, &events_[0], static_cast<int>(events_.size()), timeout);
  }

  inline int GetEventFd(size_t i) const {
    assert(i < events_.size() && i >= 0);
    return events_[i].data.fd;
  }

  inline uint32_t GetEvents(size_t i) const {
    assert(i < events_.size() && i >= 0);
    return events_[i].events;
  }

 private:
  int epoll_fd_;
  std::vector<struct epoll_event> events_;
};

#endif //WEBSERVERCPP11_SRC_SERVER_EPOLLER_H_
