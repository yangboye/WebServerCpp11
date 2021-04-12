// =============================================================================
// Created by yangb on 2021/4/12.
// =============================================================================

#ifndef WEBSERVERCPP11_SRC_SERVER_WEB_SERVER_H_
#define WEBSERVERCPP11_SRC_SERVER_WEB_SERVER_H_

#include <fcntl.h>  // fcntl
#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <cstring>
#include "../timer/heap_timer.h"
#include "../pool/thread_pool.h"
#include "epoller.h"
#include "../http/http_conn.h"

class WebServer {
 public:
  ///
  /// @brief
  /// @param port 端口号
  ///     取值范围[1024, 65536)
  /// @param trig_mode 事件触发模式
  ///     0: Listen(LT), Conn(LT); 1: Listen(LT), Conn(ET)
  ///     2: Listen(ET), Conn(LT); 3: Listen(ET), Conn(ET)
  /// @param timeout  超时时间
  ///     单位: 毫秒(ms)
  /// @param opt_linger 是否开启优雅关闭
  /// @param sql_port 数据库连接端口
  /// @param sql_user 数据库用户名
  /// @param sql_passwd 数据库密码
  /// @param db_name 数据库名称
  /// @param conn_pool_num 据库连接池中的连接资源数量
  /// @param thread_num 线程数量
  /// @param open_log 是否开启日志
  /// @param log_level 日志等级
  ///     0: debug, 1: info, 2: warn, 3: error
  /// @param log_que_size 日志同步队列长度
  ///     <=0: 同步日志;   >0: 异步日志
  ///
  WebServer(int port, int trig_mode, int timeout, bool opt_linger,
            int sql_port, const char* sql_user, const char* sql_passwd,
            const char* db_name, int conn_pool_num, int thread_num,
            bool open_log, int log_level, int log_que_size);

  ~WebServer() {
    close(listen_fd_);
    is_close_ = true;
    free(src_dir_);
    SqlConnPool::Instance()->ClosePool();
  }

  void Start();

 private:
  ///
  /// @brief 初始化socket
  /// 步骤：(1) 创建socket, 并设置socket属性;
  ///      (2) 绑定端口;
  ///      (3) 监听;
  ///
  bool InitSocket_();

  ///
  /// @brief 初始化事件模式
  /// @param trig_mode    Listen      Conn
  ///           0          LT          LT
  ///           1          LT          ET
  ///           2          ET          LT
  ///           3          ET          ET
  ///        default       ET          ET
  ///
  void InitEventMode_(int trig_mode);

  /// @brief 处理监听事件
  void DealListen_();

  /// @brief 处理写事件
  inline void DealWrite_(HttpConn* client) {
    assert(client);
    ExtentTime_(client);
    thread_pool_->AddTask(std::bind(&WebServer::OnWrite_, this, client));
  }

  /// @brief 处理读事件
  inline void DealRead_(HttpConn* client) {
    assert(client);
    ExtentTime_(client);
    thread_pool_->AddTask(std::bind(&WebServer::OnRead_, this, client));
  }

  inline void SendError_(int fd, const char* info) {
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if (ret < 0) {
      LOG_WARN("Send error to client[%d] error!", fd);
    }
    close(fd);
  }

  inline void ExtentTime_(HttpConn* client) {
    assert(client);
    if (timeout_ > 0) {
      timer_->Adjust(client->GetFd(), timeout_);
    }
  }

  /// @brief 关闭连接
  inline void CloseConn_(HttpConn* client) {
    assert(client);
    LOG_INFO("Client[%d] quit!", client->GetFd());
    epoller_->DelFd(client->GetFd());
    client->Close();
  }

  inline void OnRead_(HttpConn* client) {
    assert(client);
    int ret = -1;
    int read_errno = 0;
    ret = client->Read(&read_errno);
    if (ret <= 0 && read_errno != EAGAIN) {
      CloseConn_(client);
      return;
    }
    OnProcess_(client);
  }

  inline void OnWrite_(HttpConn* client) {
    assert(client);
    int ret = -1;
    int write_errno = 0;
    ret = client->Write(&write_errno);
    if (client->ToWriteBytes() == 0) { // 传输完成
      if (client->IsKeepAlive()) {
        OnProcess_(client);
        return;
      }
    } else if (ret < 0) {
      if (write_errno == EAGAIN) { // 继续传输
        epoller_->ModFd(client->GetFd(), conn_event_ | EPOLLOUT);
        return;
      }
    }
    CloseConn_(client);
  }

  inline void OnProcess_(HttpConn* client) {
    if (client->Process()) {
      epoller_->ModFd(client->GetFd(), conn_event_ | EPOLLOUT);
    } else {
      epoller_->ModFd(client->GetFd(), conn_event_ | EPOLLIN);
    }
  }

  /// 添加客户连接
  inline void AddClient_(int fd, sockaddr_in addr) {
    assert(fd > 0);
    users_[fd].Init(fd, addr);
    if (timeout_ > 0) {
      timer_->Add(fd, timeout_, std::bind(&WebServer::CloseConn_, this, &users_[fd]));
    }

    epoller_->AddFd(fd, EPOLLIN || conn_event_);
    SetFdNonBlock_(fd);
    LOG_INFO("Client[%d] in!", users_[fd].GetFd());
  }

  /// @brief 设置句柄非阻塞IO
  inline static int SetFdNonBlock_(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
  }

 private:
  static const int kMaxFd = 65536;

  int port_;
  bool open_linger_;  // 是否开启优雅关闭
  int timeout_; // 单位：ms
  bool is_close_;
  int listen_fd_;
  char* src_dir_; // 资源路径

  uint32_t listen_event_; // 监听事件
  uint32_t conn_event_;   // 连接事件

  std::unique_ptr<HeapTimer> timer_;  // 时间堆
  std::unique_ptr<ThreadPool> thread_pool_; // 线程池
  std::unique_ptr<Epoller> epoller_;
  std::unordered_map<int/*句柄*/, HttpConn> users_;
};

#endif //WEBSERVERCPP11_SRC_SERVER_WEB_SERVER_H_
