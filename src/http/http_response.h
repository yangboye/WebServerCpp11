// =============================================================================
// Created by yangb on 2021/4/7.
// =============================================================================

#ifndef WEBSERVERCPP11_SRC_HTTP_HTTP_RESPONSE_H_
#define WEBSERVERCPP11_SRC_HTTP_HTTP_RESPONSE_H_

#include <unordered_map>
#include <fcntl.h>  // open
#include <unistd.h> // close
#include <sys/stat.h> // stat
#include <sys/mman.h> // mmap, munmap
#include "../buffer/buffer.h"
#include "../log/log.h"

///
/// @brief 服务器响应
/// Reference: https://developer.mozilla.org/zh-CN/docs/Web/HTTP/Status#%E6%9C%8D%E5%8A%A1%E7%AB%AF%E5%93%8D%E5%BA%94
/// Reference: https://www.cnblogs.com/open-yang/p/11182654.html
/// 响应格式如下(1行为状态行, 2~4行为响应头部, 5行为空行, 6行之后为响应正文)：
/// 1   HTTP/1.1 200 OK
/// 2   Date:Tue, 10 Jul 2012 06:50:15 GMT
/// 3   Content-Length:362
/// 4   Content-Type:text/html
/// 5
/// 6   <html>
/// 7   ...
///
class HttpResponse {
 public:
  HttpResponse();

  ~HttpResponse();

  void Init(const std::string& src_dir, std::string& path, bool is_keep_alive = false, int code = -1);

  void MakeResponse(Buffer& buff);

  inline void UnmapFile() {
    if (mm_file_) {
      munmap(mm_file_, mm_file_stat_.st_size);
      mm_file_ = nullptr;
    }
  }

  /// @brief 获取文件
  inline char* File() const { return mm_file_; }

  /// @brief 获取文件长度
  inline size_t FileLen() const { return mm_file_stat_.st_size; }

  void ErrorContent(Buffer& buff, const std::string& message) const;

  /// @brief 获取状态码
  inline int GetCode() const { return code_; }

 private:
  /// @brief 添加状态行
  void AddStateLine_(Buffer& buff);

  /// @brief 添加响应头部
  void AddHeader_(Buffer& buff);

  /// @brief 添加响应正文
  void AddContent_(Buffer& buff);

  /// @brief 请求出错的html网页
  void ErrorHtml_();

  /// @brief 请求文件对应的Content-Type类型
  std::string GetFileType_();

 private:
  int code_;  // 状态码
  bool is_keep_alive_;

  std::string path_;
  std::string src_dir_;

  char* mm_file_;
  struct stat mm_file_stat_{};

  static const std::unordered_map<std::string, std::string> kSuffixType_; // key: 文件扩展名   value: Content-Type
  static const std::unordered_map<int, std::string> kCodeStatus_;         // key: 状态码      value: 状态码对应的信息
  static const std::unordered_map<int, std::string> kCodePath_;           // key: 状态码      value: 对应网页的路径
};

#endif //WEBSERVERCPP11_SRC_HTTP_HTTP_RESPONSE_H_
