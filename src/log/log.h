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

  void Init(int level = 1, const char* path = "./log", const char* suffix = ".log", int max_queue_capacity = 1024);
  void Write(int level, const char* format, ...);
  void Flush();

  int GetLevel() const;
  void SetLevel(int level);
  inline bool IsOpen() const { return is_open_; }

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

// 只记录比level_等级高的log
#define LOG_BASE(level, format, ...) \
  do {\
    Log* log = Log::Instance();\
    if (log->IsOpen() && log->GetLevel() <= level) {\
      log->Write(level, format, ##__VA_ARGS__); \
      log->Flush();\
    }\
  } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0)
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0)
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0)
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0)

#endif //WEBSERVERCPP11_SRC_LOG_LOG_H_
