// =============================================================================
// Created by yangb on 2021/4/7.
// =============================================================================

#ifndef WEBSERVERCPP11_SRC_HTTP_HTTP_CONN_H_
#define WEBSERVERCPP11_SRC_HTTP_HTTP_CONN_H_

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/uio.h>  // writev
#include "../log/log.h"
#include "../pool/sql_conn_raii.h"
#include "../buffer/buffer.h"
#include "http_request.h"
#include "http_response.h"

class HttpConn {
 public:
  HttpConn() : fd_(-1), addr_{0}, is_close_(true) {}

  ~HttpConn() { Close(); }

  void Init(int sock_fd, const sockaddr_in& addr);

  ssize_t Write(int* save_errno);

  bool Process();

  inline ssize_t Read(int* save_errno) {
    ssize_t len = -1;
    do {
      len = read_buff_.ReadFd(fd_, save_errno);
      if(len <= 0) { // 没有数据可读, 退出
        break;
      }
    } while (is_ET);
    return len;
  }

  inline void Close() {
    response_.UnmapFile();
    if (!is_close_) {
      is_close_ = true;
      --user_count;
      close(fd_);
      LOG_INFO("Client[%d](%s:%d) quit, user count: %d", GetFd(), GetIP(), GetPort(), static_cast<int>(user_count));
    }
  }

  inline int GetFd() const { return fd_; }

  /// @brief 获取端口号
  inline int GetPort() const { return addr_.sin_port; }

  /// @brief 获取IP地址
  inline const char* GetIP() const { return inet_ntoa(addr_.sin_addr); }

  inline sockaddr_in GetAddr() const { return addr_; }

  /// @brief 要写入的大小
  inline size_t ToWriteBytes() const { return iov_[0].iov_len + iov_[1].iov_len; }

  inline bool IsKeepAlive() const { return request_.IsKeeyAlive(); }

 public:
  static bool is_ET;
  static const char* kSrcDir;
  static std::atomic<int> user_count;

 private:
  int fd_;
  struct sockaddr_in addr_; // 请求端信息

  bool is_close_;

  int iov_cnt_{};
  struct iovec iov_[2]{};

  Buffer read_buff_;  // 读缓冲区
  Buffer write_buff_; // 写缓冲区

  HttpRequest request_;
  HttpResponse response_;
};

#endif //WEBSERVERCPP11_SRC_HTTP_HTTP_CONN_H_
