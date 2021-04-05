// =============================================================================
// Created by yangb on 2021/4/3.
// =============================================================================

#ifndef WEBSERVERCPP11_SRC_LOG_LOG_H_
#define WEBSERVERCPP11_SRC_LOG_LOG_H_

#include <memory>
#include <thread>
#include "block_queue.h"
#include "../buffer/buffer.h"

/// @brief 单例模式
class Log {
 public:
  static Log* Instance();
  static void FlushLogThread();

  ~Log();

  void Init(int level=1, const char* path = "./log", const char* suffix = ".log", int max_queue_capacity = 1024);
  void Write(int level, const char* format, ...);
  void Flush();

  int GetLevel() const;
  void SetLevel(int level);
  inline bool IsOpen() const {return is_open_;}

 private:
  Log();
  void AppendLogLevelTitle_(int level);
  void AsyncWrite_(); // 异步写

 private:
  static const int kLogPathLen = 256;
  static const int kLogNameLen = 256;
  static const int kMaxLines = 50'000;

  const char* path_;
  const char* suffix_;

  int max_lens_{};
  int line_count_{};
  int today_{};
  bool is_open_{};

  Buffer buff_;
  int level_{};
  bool is_async_{};

  FILE* fp_;
  std::unique_ptr<BlockQueue<std::string>> dque_;
  std::unique_ptr<std::thread> write_thread_;
  mutable std::mutex mtx_;

};

#endif //WEBSERVERCPP11_SRC_LOG_LOG_H_
