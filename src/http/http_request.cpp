// =============================================================================
// Created by yangb on 2021/4/5.
// =============================================================================

#include <algorithm>
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
  if(buff.ReadableBytes() <= 0) {
    return false;
  }

  while (buff.ReadableBytes() && state_ != FINISH) {
    // 寻找到空行
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
        if(buff.ReadableBytes() <= 2) { // GET请求
          state_ = FINISH;
        }
        break;
      case BODY:
        ParseBody_(line);
        break;
      default:
        break;
    } // switch
    if(line_end == buff.BeginWriteConst()) {
      break;
    }
    buff.RetrieveUntil(line_end + 2); // 2: \r\n的长度
  }// while
  LOG_DEBUG()
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




