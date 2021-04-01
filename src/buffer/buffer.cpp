// =============================================================================
// Created by yangb on 2021/4/1.
// =============================================================================

#include <cassert>
#include <cstring>  // bzero
#include <unistd.h> // write
#include <sys/uio.h>  // iovec, readv
#include "buffer.h"

Buffer::Buffer(int _buffer_size) : buffer_(_buffer_size), read_pos_(0), write_pos_(0) {
  assert(ReadableBytes() == 0);
  assert(WritableBytes() == _buffer_size);
}

size_t Buffer::WritableBytes() const {
  return buffer_.size() - write_pos_;
}

size_t Buffer::ReadableBytes() const {
  return write_pos_ - read_pos_;
}

size_t Buffer::PrependableBytes() const {
  return read_pos_;
}

const char* Buffer::Peek() const {
  return BeginPtr_() + read_pos_;
}

void Buffer::EnsureWritable(size_t len) {
  if (WritableBytes() < len) {
    // 空间不够, 扩容
    MakeSpace_(len);
  }
  assert(WritableBytes() >= len);
}

void Buffer::HasWritten(size_t len) {
  EnsureWritable(len);
  write_pos_ += len;
}

void Buffer::Retrieve(size_t len) {
  assert(ReadableBytes() >= len);
  read_pos_ += len;
}

void Buffer::RetrieveUntil(const char* end) {
  assert(Peek() <= end);
  Retrieve(end - Peek());
}

void Buffer::RetrieveAll() {
  // 获取全部之后, 重置到初始状态
  bzero(&buffer_[0], buffer_.size());
  read_pos_ = 0;
  write_pos_ = 0;
}

std::string Buffer::RetrieveAllToStr() {
  std::string str(Peek(), ReadableBytes());
  RetrieveAll();
  return str;
}

const char* Buffer::BeginWriteConst() const {
  return BeginPtr_() + write_pos_;
}

char* Buffer::BeginWrite() {
  return BeginPtr_() + write_pos_;
}

void Buffer::Append(const std::string& str) {
  Append(str.data(), str.length());
}

void Buffer::Append(const void* data, size_t len) {
  assert(data);
  Append(static_cast<const char*>(data), len);
}

void Buffer::Append(const char* str, size_t len) {
  assert(str);
  EnsureWritable(len);
  std::copy(str, str + len, BeginWrite());
  HasWritten(len);
}

void Buffer::Append(const Buffer& buff) {
  Append(buff.Peek(), buff.ReadableBytes());
}

ssize_t Buffer::ReadFd(int fd, int* Errno) {
  char buff[65536]; // 栈上分配 64KB
  struct iovec iov[2];
  const size_t writable = WritableBytes();

  // 分散读, 保证数据全部读完
  iov[0].iov_base = BeginPtr_() + write_pos_;
  iov[0].iov_len = writable;
  iov[1].iov_base = buff;
  iov[1].iov_len = sizeof(buff);

  const ssize_t len = readv(fd, iov, 2);
  if (len < 0) {
    *Errno = errno;
  } else if (static_cast<size_t>(len) <= writable) {
    // 如果buffer空间够, 直接放到buffer中
    write_pos_ += len;
  } else {
    // 空间不够, 先放满buffer, 剩下的放到栈中, 然后再追加到buffer尾部
    write_pos_ = buffer_.size();
    Append(buff, len - writable);
  }

  return len;
}

ssize_t Buffer::WriteFd(int fd, int* Errno) {
  size_t readable_size = ReadableBytes();
  ssize_t len = write(fd, Peek(), readable_size);
  if (len < 0) {
    *Errno = errno;
    return len;
  }
  read_pos_ += len;
  return len;
}

char* Buffer::BeginPtr_() {
  return &*buffer_.begin();
}

const char* Buffer::BeginPtr_() const {
  return &*buffer_.begin();
}

void Buffer::MakeSpace_(size_t len) {
  if (WritableBytes() + PrependableBytes() < len) {
    // 总的长度不够了, 扩容
    buffer_.resize(write_pos_ + len + 1);
  } else {
    // 总长度够, 则将内容移动到最前面
    size_t readable = ReadableBytes();
    // 将readable bytes移动到最前面
    std::copy(BeginPtr_() + read_pos_, BeginPtr_() + write_pos_, BeginPtr_());
    read_pos_ = 0;
    write_pos_ = read_pos_ + readable;
    assert(readable == ReadableBytes());
  }
}