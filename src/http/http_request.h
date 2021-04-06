// =============================================================================
// Created by yangb on 2021/4/5.
// =============================================================================

#ifndef WEBSERVERCPP11_SRC_HTTP_HTTP_REQUEST_H_
#define WEBSERVERCPP11_SRC_HTTP_HTTP_REQUEST_H_

#include <unordered_map>
#include <unordered_set>
#include <cassert>
#include "../buffer/buffer.h"

///
/// Reference: https://mp.weixin.qq.com/s/BfnNl-3jc_x5WPrWEJGdzQ
/// @brief Http请求的解析，主要的两种请求方法:
/// GET的请求报文格式如下(1行为请求行, 2~8行为请求头部, 9行为空行, 10行为请求数据)：
/// 1    GET /562f25980001b1b106000338.jpg HTTP/1.1
/// 2    Host:img.mukewang.com
/// 3    User-Agent:Mozilla/5.0 (Windows NT 10.0; WOW64)
/// 4    AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.106 Safari/537.36
/// 5    Accept:image/webp,image/*,*/*;q=0.8
/// 6    Referer:http://www.imooc.com/
/// 7    Accept-Encoding:gzip, deflate, sdch
/// 8    Accept-Language:zh-CN,zh;q=0.8
/// 9    空行
///10    请求数据为空
/// ---------------------------------------------------------
/// POST的请求报文格式如下(1行为请求行, 2~6行为请求头部, 7行为空行, 8行为请求数据)：
/// 1    POST / HTTP1.1
/// 2    Host:www.wrox.com
/// 3    User-Agent:Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 2.0.50727; .NET CLR 3.0.04506.648; .NET CLR 3.5.21022)
/// 4    Content-Type:application/x-www-form-urlencoded
/// 5    Content-Length:40
/// 6    Connection: Keep-Alive
/// 7    空行
/// 8    name=Professional%20Ajax&publisher=Wiley
///
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

  /// @brief get file path
  inline std::string GetPath() const { return path_; }

  /// @brief get file path
  inline std::string& GetPath() { return path_; }

  /// @brief GET or POST
  inline std::string GetMethod() const { return method_; }

  /// @brief version of HTTP, e.g. 1.1
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
  /// @brief 十六进制 大于10部分将字符转换为数字, e.g. B = 11, c= 12
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

  /// @brief 解析 POST 方法的请求数据
  void ParsePost_();

  /// @brief 解析路径 在路径名(path_)后加上.html
  void ParsePath_();

  /// @brief 解析Content-Type:application/x-www-form-urlencoded
  void ParseFromUrlencoded_();

 private:
  PaserState state_{};
  std::string method_{};  // GET or POST
  std::string path_{};    // file path
  std::string version_{}; // HTTP version e.g. 1.1
  std::string body_{};

  std::unordered_map<std::string, std::string> header_{}; // 请求头部中的值
  std::unordered_map<std::string, std::string> post_{};

  static const std::unordered_set<std::string> kDefaultHtml;  // 默认网页
  static const std::unordered_map<std::string, int> kDefaultHtmlTag;
};

#endif //WEBSERVERCPP11_SRC_HTTP_HTTP_REQUEST_H_
