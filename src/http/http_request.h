// =============================================================================
// Created by yangb on 2021/4/5.
// =============================================================================

#ifndef WEBSERVERCPP11_SRC_HTTP_HTTP_REQUEST_H_
#define WEBSERVERCPP11_SRC_HTTP_HTTP_REQUEST_H_

#include <unordered_map>
#include <unordered_set>
#include <cassert>
#include "../buffer/buffer.h"

class HttpRequest {
 public:
  enum PaserState { // 解析的状态
    REQUEST_LINE, // 请求行
    HEADERS,      // 请求头部
    BODY,         // 请求数据
    FINISH,
  };

  enum HttpCode {
    NO_REQUEST = 0,
    GET_REQUEST,
    BAD_REQUEST,
    NO_RESOURCE,
    FORBIDDENT_REQUEST,
    FILE_REQUEST,
    INTERNAL_ERROR,
    CLOSED_CONNECTION,
  };

  HttpRequest() { Init(); }
  ~HttpRequest() = default;

  void Init();

  /// @brief 解析
  bool Parse(Buffer& buff);

  inline std::string GetPath() const { return path_; }

  inline std::string& GetPaht() { return path_; }

  inline std::string GetMethod() const { return method_; }

  inline std::string GetVersion() const { return version_; }

  inline std::string GetPost(const char* key) const {
    assert(key != "");
    if (post_.count(key) == 1) {
      return post_.find(key)->second;
    }
    return "";
  }

  inline std::string GetPost(const std::string& key) const {
    return GetPost(key.c_str());
  }

  inline bool IsKeeyAlive() const {
    if (header_.count("Connection") == 1) {
      return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
  }

 private:
  inline static int ConverHex_(char ch) {
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return ch;
  }

  static bool UserVerify_(const std::string& name, const std::string& passwd, bool is_login);

  /// @brief 解析 请求行
  bool ParseRequestLine_(const std::string& line);

  /// @brief 解析 请求头部
  void ParseHeader_(const std::string& line);

  /// @brief 解析 请求数据
  void ParseBody_(const std::string& line);

  /// @brief 解析路径 在路径名(path_)后加上.html
  void ParsePath_();

  void ParsePost_();

  void ParseFromUrlencoded_();

 private:
  PaserState state_{};
  std::string method_{};  // GET or POST
  std::string path_{};
  std::string version_{}; // HTTP version e.g. 1.1
  std::string body_{};

  std::unordered_map<std::string, std::string> header_{}; // 请求头部中的值
  std::unordered_map<std::string, std::string> post_{};

  static const std::unordered_set<std::string> kDefaultHtml;  // 默认网页
  static const std::unordered_map<std::string, int> kDefaultHtmlTag;
};

#endif //WEBSERVERCPP11_SRC_HTTP_HTTP_REQUEST_H_
