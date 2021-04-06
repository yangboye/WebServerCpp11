// =============================================================================
// Created by yangb on 2021/4/5.
// =============================================================================

#include <algorithm>
#include <regex>
#include "../log/log.h"
#include "http_request.h"

const std::unordered_set<std::string> HttpRequest::kDefaultHtml{
    "/index", "/register", "/login",
    "/welcome", "/video", "/picture",
};

const std::unordered_map<std::string, int> HttpRequest::kDefaultHtmlTag{
    {"/register.html", 0},
    {"/login.html", 1},
};

void HttpRequest::Init() {
  state_ = REQUEST_LINE;
  header_.clear();
  post_.clear();
}

bool HttpRequest::Parse(Buffer& buff) {
  const char kCRLF[] = "\r\n";
  if (buff.ReadableBytes() <= 0) {
    return false;
  }

  while (buff.ReadableBytes() && state_ != FINISH) {
    // 寻找到空行, 一行一行解析
    const char* line_end = std::search(buff.Peek(), buff.BeginWriteConst(), kCRLF, kCRLF + 2);
    std::string line(buff.Peek(), line_end);
    switch (state_) {
      case REQUEST_LINE:
        if (!ParseRequestLine_(line)) {
          return false;
        }
        ParsePath_();
        break;
      case HEADERS:
        ParseHeader_(line);
        if (buff.ReadableBytes() <= 2) { // GET请求
          state_ = FINISH;
        }
        break;
      case BODY:
        ParseBody_(line);
        break;
      default:
        break;
    } // switch
    if (line_end == buff.BeginWriteConst()) {
      break;
    }
    buff.RetrieveUntil(line_end + 2); // 2: \r\n的长度
  }// while
//  LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
  return true;
}

void HttpRequest::ParsePath_() {
  if (path_ == "/") {
    path_ = "/index.html";
  } else {
    for (const auto& item : kDefaultHtml) {
      if (item == path_) {
        path_ += ".html";
        break;
      }
    }
  }
}

bool HttpRequest::ParseRequestLine_(const std::string& line) {
  // e.g GET /562f25980001b1b106000338.jpg HTTP/1.1
  std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
  std::smatch sub_match;
  if (std::regex_match(line, sub_match, patten)) {
    method_ = sub_match[1];   // 请求方法, e.g. GET
    path_ = sub_match[2];     // 请求路径, e.g. /562f25980001b1b106000338.jpg
    version_ = sub_match[3];  // http版本号, e.g. 1.1
    // 转移到下一个状态：解析请求头部
    state_ = HEADERS;
    return true;
  }
  std::ostringstream ostr;
  ostr << "File: " << __FILE__ << "\tFunction: " << __FUNCTION__ << std::endl;
//  LOG_DEBUG("In %s, requestLine error", ostr.str().c_str());
  return false;
}

void HttpRequest::ParseHeader_(const std::string& line) {
  std::regex patten("^([^:]*): ?(.*)$");
  std::smatch sub_match;
  if (std::regex_match(line, sub_match, patten)) {
    header_[sub_match[1]] = sub_match[2];
  } else {
    state_ = BODY;  // 处理完了，转换到下一状态
  }
}

void HttpRequest::ParseBody_(const std::string& line) {
  body_ = line;
  ParsePost_();
  state_ = FINISH;
//  LOG_DEBUG("Body: %s, len: %d", line.c_str(), line.size());
}

void HttpRequest::ParsePost_() {
  // e.g.
  // POST http://192.168.2.12/index HTTP/1.1
  // Content-Type: application/x-www-form-urlencoded;charset=utf-8
  // title=test&sub%5B%5D=1&sub%5B%5D=2&sub%5B%5D=3
  //
  if (method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
    ParseFromUrlencoded_();
    if (kDefaultHtmlTag.count(path_)) {
      int tag = kDefaultHtmlTag.find(path_)->second;
//      LOG_DEBUG("Tag: %d", tag);
      if (tag == 0 || tag == 1) {
        bool is_login = (tag == 1);
        if (UserVerify_(post_["username"], post_["password"], is_login)) {
          path_ = "/welcome.html";
        } else {
          path_ = "/error.html";
        }
      } // if
    } // if
  } // if
}

void HttpRequest::ParseFromUrlencoded_() {
  if (body_.empty()) {
    return;
  }

  std::string key;
  std::string value;
  int num = 0;
  int n = body_.size();
  int i = 0;
  int j = 0;

  for (; i < n; ++i) {
    char ch = body_[i];
    switch (ch) {
      case '=':
        key = body_.substr(j, i - j);
        j = i + 1;
        break;
      case '+':
        body_[i] = ' ';
        break;
      case '%':
        num = ConverHex_(body_[i + 1]) * 16 + ConverHex_(body_[i + 2]);
        body_[i + 2] = num % 10 + '0';
        body_[i + 2] = num / 10 + '0';
        i += 2;
        break;
      case '&':
        value = body_.substr(j, i - j);
        j = i + 1;
        post_[key] = value;
//        LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
        break;
      default:
        break;
    } // switch
  } // for
  assert(j <= i);
  if (post_.count(key) == 0 && j < i) {
    value = body_.substr(j, i - j);
    post_[key] = value;
  }
}




