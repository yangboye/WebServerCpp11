// =============================================================================
// Created by yangb on 2021/4/7.
// =============================================================================


#include "http_conn.h"

bool HttpConn::is_ET{false};
const char* HttpConn::kSrcDir;
std::atomic<int> HttpConn::user_count{0};

void HttpConn::Init(int sock_fd, const sockaddr_in& addr) {
  assert(sock_fd > 0);
  ++user_count;
  addr_ = addr;
  fd_ = sock_fd;
  write_buff_.RetrieveAll();  // 清空
  read_buff_.RetrieveAll();   // 清空
  is_close_ = false;
  LOG_INFO("Client[%d](%s:%d) in, user count: %d\n", GetFd(), GetIP(), GetPort(), static_cast<int>(user_count));
}

ssize_t HttpConn::Write(int* save_errno) {
  ssize_t len = -1;
  do {
    len = writev(fd_, iov_, iov_cnt_);
    if (len <= 0) {
      *save_errno = errno;
      break;
    }

    if (ToWriteBytes() == 0) { // 传输结束
      break;
    } else if (static_cast<size_t>(len) > iov_[0].iov_len) {
      iov_[1].iov_base = static_cast<uint8_t*>(iov_[1].iov_base) + (len - iov_[0].iov_len);
      iov_[1].iov_len -= (len - iov_[0].iov_len);
      if (iov_[0].iov_len) {
        write_buff_.RetrieveAll();
        iov_[0].iov_len = 0;
      }
    } else {
      iov_[0].iov_base = static_cast<uint8_t*>(iov_[0].iov_base) + len;
      iov_[0].iov_len -= len;
      write_buff_.Retrieve(len);
    }

  } while (is_ET || ToWriteBytes() > 10240);

  return len;
}

bool HttpConn::Process() {
  request_.Init();
  if (read_buff_.ReadableBytes() <= 0) {
    return false;
  }

  if (request_.Parse(read_buff_)) { // 解析请求成功
    LOG_DEBUG("%s\n", request_.GetPath().c_str());
    response_.Init(kSrcDir, request_.GetPath(), request_.IsKeeyAlive(), 200);
  } else {  // 解析请求失败
    response_.Init(kSrcDir, request_.GetPath(), false, 400);
  }

  response_.MakeResponse(write_buff_);
  // 下面参考buffer中ReadFd的实现(iov_[0]指向状态行&响应头部&空行, iov_[1]指向响应正文（响应请求的文件）)
  // 响应头
  iov_[0].iov_base = const_cast<char*>(write_buff_.Peek());
  iov_[0].iov_len = write_buff_.ReadableBytes();
  iov_cnt_ = 1;
  // 文件
  if (response_.FileLen() > 0 && response_.File()) {
    iov_[1].iov_base = response_.File();
    iov_[1].iov_len = response_.FileLen();
    iov_cnt_ = 2;
  }
  LOG_DEBUG("File size: %d, iov count: %d, to write bytes: %d\n", response_.FileLen(), iov_cnt_, ToWriteBytes());
  return true;
}

