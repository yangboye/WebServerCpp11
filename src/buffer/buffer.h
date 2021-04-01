// =============================================================================
// Created by yangb on 2021/4/1.
// Reference: https://github.com/chenshuo/muduo/blob/master/muduo/net/Buffer.h
// 输入输出缓冲区
// =============================================================================

#ifndef WEBSERVERCPP11_SRC_BUFFER_BUFFER_H_
#define WEBSERVERCPP11_SRC_BUFFER_BUFFER_H_

#include <vector>
#include <atomic>
#include <string>

/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode
class Buffer {
 public:
  explicit Buffer(int _buffer_size=1024);
  ~Buffer() = default;

  /// @brief 可写字节数 = len(buffer) - write_pos
  size_t WritableBytes() const;

  /// @brief 可读字节数 = write_pos - read_pos
  size_t ReadableBytes() const;

  /// @brief 前向字节数 = read_pos
  size_t PrependableBytes() const;

  /// @brief 指向readable bytes开头的指针
  const char* Peek() const;

  /// @brief 确保writable bytes可以装下新来的len个字节, 如果不够则扩容
  /// @param len 将要写入的字节数
  void EnsureWritable(size_t len);

  /// @brief 已经写入len个字节到writable bytes中, 即更新writer_pos的位置
  /// @param len 已写入的字节数
  void HasWritten(size_t len);

  /// @brief 读取len个字节, 即更新read_pos的位置
  /// @param len 读取的字节数
  void Retrieve(size_t len);

  /// @brief 读取到end指针所指向的位置
  /// @param end 读取的结束位置
  void RetrieveUntil(const char* end);

  /// @brief 读取所有的内容
  void RetrieveAll();

  /// @brief 读取所有的内容到字符串中
  /// @return 读取的内容
  std::string RetrieveAllToStr();

  /// @brief begin iterator of `writable bytes`
  const char* BeginWriteConst() const;

  /// @brief begin iterator of `writable bytes`
  char* BeginWrite();

  void Append(const std::string& str);
  void Append(const char* str, size_t len);
  void Append(const void* data, size_t len);
  void Append(const Buffer& buff);

  ssize_t ReadFd(int fd, int* Errno);

  /// @brief 将readable bytes中的内容写到句柄fd中
  /// @param fd 句柄(文件描述符)
  /// @param Errno 错误信息
  /// @return 写入的字节数
  ssize_t WriteFd(int fd, int* Errno);

 private:
  /// @brief 获取开始的指针
  char* BeginPtr_();
  const char* BeginPtr_() const;
  /// @brief 扩容
  void MakeSpace_(size_t len);

 private:
  std::vector<char> buffer_;
  std::atomic<std::size_t> read_pos_;
  std::atomic<std::size_t> write_pos_;
};

#endif //WEBSERVERCPP11_SRC_BUFFER_BUFFER_H_
