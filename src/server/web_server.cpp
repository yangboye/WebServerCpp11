// =============================================================================
// Created by yangb on 2021/4/12.
// =============================================================================

#include "web_server.h"

WebServer::WebServer(int port,
                     int trig_mode,
                     int timeout,
                     bool opt_linger,
                     int sql_port,
                     const char* sql_user,
                     const char* sql_passwd,
                     const char* db_name,
                     int conn_pool_num,
                     int thread_num,
                     bool open_log,
                     int log_level,
                     int log_que_size) : port_(port),
                                         open_linger_(opt_linger),
                                         timeout_(timeout),
                                         is_close_(false),
                                         timer_(new HeapTimer()),
                                         thread_pool_(new ThreadPool(thread_num)),
                                         epoller_(new Epoller()) {
  src_dir_ = getcwd(nullptr, 256);
  assert(src_dir_);
  strncat(src_dir_, "/resources/", 16);

  HttpConn::user_count = 0;
  HttpConn::kSrcDir = src_dir_;
  SqlConnPool::Instance()->Init("localhost", sql_port, sql_user, sql_passwd, db_name, conn_pool_num);

  InitEventMode_(trig_mode);

  if (!InitSocket_()) {
    is_close_ = true;
  }

  if (open_log) {
    Log::Instance()->Init(log_level, "./log", ".log", log_que_size);
    if (is_close_) {
      LOG_ERROR("============== Server init error! ==============");
    } else {
      LOG_INFO("============== Server init ==============");
      LOG_INFO("Port: %d\t\tOpenLinger: %s", port_, opt_linger ? "true" : "false");
      LOG_INFO("Listen Mode: %s\t\t OpenConn Mode: %s",
               (listen_event_ & EPOLLET ? "ET" : "LT"),
               (conn_event_ & EPOLLET ? "ET" : "LT"));
      LOG_INFO("Log level(0: debug, 1: info, 2: warn, 3: error): %d", log_level);
      LOG_INFO("Source Dir: %s", HttpConn::kSrcDir);
      LOG_INFO("SqlConnPool num: %d\t\tThreadPool num: %d", conn_pool_num, thread_num);
    }
  }

}

void WebServer::Start() {
  int time_ms = -1; // epoll wait timeout == -1 无事件阻塞
  if (!is_close_) {
    LOG_INFO("============== Server start ==============");
  }
  while (!is_close_) {
    if (timeout_ > 0) {
      time_ms = timer_->GetNextTick();
    }
    int event_cnt = epoller_->Wait(time_ms);
    for (int i = 0; i < event_cnt; ++i) { // 处理事件
      int fd = epoller_->GetEventFd(i);
      uint32_t events = epoller_->GetEvents(i);

      if (fd == listen_fd_) {  // 监听事件
        DealListen_();
      } else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) { // 关闭连接
        assert(users_.count(fd) > 0);
        CloseConn_(&users_[fd]);
      } else if (events & EPOLLIN) {  // 读
        assert(users_.count(fd) > 0);
        DealRead_(&users_[fd]);
      } else if (events & EPOLLOUT) { // 写
        assert(users_.count(fd) > 0);
        DealWrite_(&users_[fd]);
      } else {
        LOG_ERROR("Unexpected event!");
      } // if
    } // for
  } // while
}

bool WebServer::InitSocket_() {
  if (port_ < 1024 || port_ > 65535) {
    LOG_ERROR("Port: %d error!", port_);
    return false;
  }

  struct sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port_);

  struct linger opt_linger = {0};
  if (open_linger_) {
    // 打开优雅关闭 直到所剩数据发送完毕或超时
    opt_linger.l_linger = 1;
    opt_linger.l_onoff = 1;
  }

  listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd_ < 0) {
    LOG_ERROR("[Port:%d]Create socket error!", port_);
    return false;
  }

  int ret;
  ret = setsockopt(listen_fd_, SOL_SOCKET, SO_LINGER, &opt_linger, sizeof(opt_linger));
  if (ret < 0) {
    close(listen_fd_);
    LOG_ERROR("[Port:%d]Init linger error!", port_);
    return false;
  }

  int opt_val = 1;
  // 端口复用，只有最后一个套接字会正常接收数据
  ret = setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, (const void*)(&opt_val), sizeof(int));
  if (ret == -1) {
    LOG_ERROR("Set socket setsockopt error!");
    close(listen_fd_);
    return false;
  }

  ret = bind(listen_fd_, (struct sockaddr*)(&addr), sizeof(addr));
  if (ret < 0) {
    LOG_ERROR("Bind port:%d error!", port_);
    close(listen_fd_);
    return false;
  }

  ret = listen(listen_fd_, 6);
  if (ret < 0) {
    LOG_ERROR("Listen port:%d error!", port_);
    close(listen_fd_);
    return false;
  }

  ret = epoller_->AddFd(listen_fd_, listen_event_ | EPOLLIN);
  if (ret == 0) {
    LOG_ERROR("Add listen error!");
    close(listen_fd_);
    return false;
  }

  SetFdNonBlock_(listen_fd_);
  LOG_INFO("Server port:%d", port_);
  return true;
}

void WebServer::InitEventMode_(int trig_mode) {
  listen_event_ = EPOLLRDHUP;
  conn_event_ = EPOLLONESHOT | EPOLLRDHUP;

  switch (trig_mode) {
    case 0:
      break;
    case 1:
      conn_event_ |= EPOLLET;
      break;
    case 2:
      listen_event_ |= EPOLLET;
      break;
    case 3:
    default:
      listen_event_ |= EPOLLET;
      conn_event_ |= EPOLLET;
      break;
  }

  HttpConn::is_ET = (conn_event_ & EPOLLET);  // 连接事件是否被设置了ET模式
}

void WebServer::DealListen_() {
  struct sockaddr_in addr{};
  socklen_t len = sizeof(addr);
  do {
    int fd = accept(listen_fd_, reinterpret_cast<struct sockaddr*>(&addr), &len);
    if (fd <= 0) {
      return;
    } else if (HttpConn::user_count >= kMaxFd) {
      SendError_(fd, "Server busy!");
      LOG_WARN("Clients are full!");
      return;
    }
    AddClient_(fd, addr);
  } while (listen_event_ & EPOLLET);
}

