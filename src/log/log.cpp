// =============================================================================
// Created by yangb on 2021/4/3.
// =============================================================================

#include <sys/stat.h>
#include <sys/time.h>
#include <cstdarg>
#include "log.h"

Log::Log() : write_thread_(nullptr), dque_(nullptr), fp_(nullptr) {}

Log::~Log() {
  if (write_thread_ && write_thread_->joinable()) {
    while (!dque_->empty()) {
      dque_->Flush();
    }
    dque_->Close();
    write_thread_->join();
  }
  if (fp_) {
    std::lock_guard<std::mutex> locker(mtx_);
    Flush();
    fclose(fp_);
  }
}

Log* Log::Instance() {
  static Log log;
  return &log;
}

void Log::FlushLogThread() {
  Log::Instance()->AsyncWrite_();
}

void Log::Init(int level, const char* path, const char* suffix, int max_queue_capacity) {
  assert(level >= 0);
  assert(path);
  assert(suffix);

  is_open_ = true;
  level_ = level;
  path_ = path;
  suffix_ = suffix;

  if (max_queue_capacity > 0) { // 使用同步队列，则为异步日志
    is_async_ = true;
    if (!dque_) {
      dque_ = std::make_unique<BlockQueue<std::string>>(max_queue_capacity);
      write_thread_ = std::make_unique<std::thread>(FlushLogThread);
    } else {
      is_async_ = false;
    }
  }

  line_count_ = 0;

  time_t timer = time(nullptr);
  struct tm* sys_time = std::localtime(&timer);
  struct tm t = *sys_time;

  char file_name[kLogNameLen] = {0};
  /// e.g. ./log/2021_04_04.log
  snprintf(file_name,
           sizeof(file_name),
           "%s/%04d_%02d_%02d%s",
           path_,
           t.tm_year + 1900,
           t.tm_mon + 1,
           t.tm_mday,
           suffix_);
  today_ = t.tm_mday;

  {
    std::lock_guard<std::mutex> locker(mtx_);
    buff_.RetrieveAll();
    if (fp_) {
      Flush();
      fclose(fp_);
    }

    fp_ = fopen(file_name, "a");
    if (fp_ == nullptr) {
      mkdir(path_, 0777);
      fp_ = fopen(file_name, "a");
    }
    assert(fp_);
  }
}

void Log::Write(int level, const char* format, ...) {
  struct timeval now = {0, 0};
  gettimeofday(&now, nullptr);
  time_t t_sec = now.tv_sec;
  struct tm* sys_time = localtime(&t_sec);
  struct tm t = *sys_time;
  va_list v_list;

  // 日志日期 日志行数
  if (today_ != t.tm_mday || (line_count_ && (line_count_ % kMaxLines == 0))) {
    std::unique_lock<std::mutex> locker(mtx_);
    locker.unlock();

    char new_file[kLogNameLen];
    char tail[36] = {0};
    snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

    if (today_ != t.tm_mday) {
      snprintf(new_file, kLogNameLen - 72, "%s/%s%s", path_, tail, suffix_);
      today_ = t.tm_mday;
      line_count_ = 0;
    } else {
      snprintf(new_file, kLogNameLen - 72, "%s/%s-%d%s", path_, tail, (line_count_ / kMaxLines), suffix_);
    }

    locker.lock();
    Flush();
    fclose(fp_);
    fp_ = fopen(new_file, "a");
    assert(fp_);
  }

  {
    std::lock_guard<std::mutex> locker(mtx_);
    ++line_count_;
    int n = snprintf(buff_.BeginWrite(), 128, "%d-%02d%02d %02d:%02d:%02d.%06ld ",
                     t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
    assert(n >= 0 && n < 128);
    buff_.HasWritten(n);
    AppendLogLevelTitle_(level);

    va_start(v_list, format);
    int size = buff_.WritableBytes();
    int m = vsnprintf(buff_.BeginWrite(), size, format, v_list);
    va_end(v_list);
    assert(n >= 0 && n < size);
    buff_.HasWritten(m);
    buff_.Append("\n\0", 2);

    if (is_async_ && dque_ && !dque_->full()) {
      dque_->push_back(buff_.RetrieveAllToStr());
    } else {
      fputs(buff_.Peek(), fp_);
    }
    buff_.RetrieveAll();
  }
}

void Log::Flush() {
  if (is_async_) {
    dque_->Flush();
  }
  fflush(fp_);
}

int Log::GetLevel() const {
  std::lock_guard<std::mutex> locker(mtx_);
  return level_;
}

void Log::SetLevel(int level) {
  std::lock_guard<std::mutex> locker(mtx_);
  level_ = level;
}

void Log::AppendLogLevelTitle_(int level) {
  switch (level) {
    case 0:buff_.Append("[debug]: ", 9);
      break;
    case 1:buff_.Append("[info] : ", 9);
      break;
    case 2:buff_.Append("[warn] : ", 9);
      break;
    case 3:buff_.Append("[error]: ", 9);
      break;
    default:buff_.Append("[info] : ", 9);
      break;
  }
}

void Log::AsyncWrite_() {
  std::string str;
  while (dque_->pop(str)) {
    std::lock_guard<std::mutex> locker(mtx_);
    fputs(str.c_str(), fp_);
  }
}

