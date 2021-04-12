// =============================================================================
// Created by yangb on 2021/4/7.
// =============================================================================


#include <cassert>
#include "http_response.h"

///
/// @brief: HTTP Content-Type
/// Reference: https://tool.oschina.net/commons
///
const std::unordered_map<std::string, std::string> HttpResponse::kSuffixType_ = { // NOLINT
    {".html", "text/html"},
    {".xml", "text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/nsword"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".mpg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},
    {".tar", "application/x-tar"},
    {".css", "text/css "},
    {".js", "text/javascript "},
};

///
/// @brief Http信息响应
/// Reference: https://developer.mozilla.org/zh-CN/docs/Web/HTTP/Status#%E6%9C%8D%E5%8A%A1%E7%AB%AF%E5%93%8D%E5%BA%94
///
const std::unordered_map<int, std::string> HttpResponse::kCodeStatus_ = { // NOLINT
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
};

const std::unordered_map<int, std::string> HttpResponse::kCodePath_ = { // NOLINT
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"},
};

HttpResponse::HttpResponse() : code_(-1), is_keep_alive_(false), mm_file_(nullptr) {
  mm_file_stat_ = {0};
}

HttpResponse::~HttpResponse() {
  UnmapFile();
}

void HttpResponse::Init(const std::string& src_dir, std::string& path, bool is_keep_alive, int code) {
  assert(!src_dir.empty());

  if (this->mm_file_) {
    UnmapFile();
  }

  this->mm_file_ = nullptr;
  this->code_ = code;
  this->is_keep_alive_ = is_keep_alive;
  this->path_ = path;
  this->src_dir_ = src_dir;
  this->mm_file_stat_ = {0};
}

void HttpResponse::MakeResponse(Buffer& buff) {
  // 判断请求的资源文件
  if(stat((src_dir_ + path_).data(), &mm_file_stat_) < 0 || S_ISDIR(mm_file_stat_.st_mode)) { // 文件不存在 or 是文件夹
    code_ = 404;  // File not found
  } else if(!(mm_file_stat_.st_mode & S_IROTH)) { // 没有读的权限
    code_ = 403;  // Forbidden
  } else if(code_ == 1) {
    code_ = 200;
  }

  ErrorHtml_();
  AddStateLine_(buff);
  AddHeader_(buff);
  AddContent_(buff);
}

void HttpResponse::ErrorHtml_() {
  if (kCodePath_.count(code_) == 1) {
    path_ = kCodeStatus_.at(code_);
    stat((src_dir_ + path_).data(), &mm_file_stat_);
  }
}

void HttpResponse::AddStateLine_(Buffer& buff) {
  std::string status;
  if(kCodeStatus_.count(code_) == 1) {
    status = kCodeStatus_.at(code_);
  } else {  // Bad request
    code_ = 400;
    status = kCodeStatus_.at(code_);
  }
  // generate response status line, e.g. HTTP/1.1 200 OK
  buff.Append("HTTP/1.1 " + std::to_string(code_) + " " + status + "\r\n");
}

void HttpResponse::AddHeader_(Buffer& buff) {
  buff.Append("Connection: ");
  if(is_keep_alive_) {
    buff.Append("keep-alive\r\n");
    buff.Append("keep-alive: max=6, timeout=120\r\n");
  } else {
    buff.Append("close\r\n");
  }
  buff.Append("Content-Type: " + GetFileType_() + "\r\n");
}

void HttpResponse::AddContent_(Buffer& buff) {
  int src_fd = open((src_dir_ + path_).data(), O_RDONLY);
  if(src_fd < 0) {  // 打开文件失败
    ErrorContent(buff, "File NotFound!");
    return;
  }

  LOG_DEBUG("file path: %s", (src_dir_+path_).data());
  // 将文件映射到内存提高文件的访问速度
  void* mm_ret = (int*)mmap(nullptr, mm_file_stat_.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
  if(mm_ret == MAP_FAILED) {
    ErrorContent(buff, "File NoteFound!");
    return;
  }
  mm_file_ = (char*)mm_ret;
  close(src_fd);
  // Content-Length 是加到响应头部的
  buff.Append("Content-Length:" + std::to_string(mm_file_stat_.st_size) + "\r\n\r\n");
}

std::string HttpResponse::GetFileType_() {
  std::string::size_type idx = path_.find_last_of('.');
  if(idx == std::string::npos) {  // 如果没有类型, 则为纯文本
    return "text/plain";
  }
  std::string suffix = path_.substr(idx); // 获取后缀(文件类型)
  if(kSuffixType_.count(suffix) == 1) {
    return kSuffixType_.at(suffix);
  }
  return "text/plain";
}

void HttpResponse::ErrorContent(Buffer& buff, const std::string& message) const {
  std::string body;
  std::string status;

  body += R"(<html><title>Error</title>)";
  body += R"(<body bgcolor="ffffff">)";

  if (kCodeStatus_.count(code_) == 1) {
    status = kCodeStatus_.at(code_);
  } else {
    status = "Bad Request";
  }

  body += std::to_string(code_) + " : " + status + "\n";
  body += "<p>" + message + "</p>";
  body += R"(<hr><em>TinyWebServer</em></body></html>)";

  buff.Append("Content-Length:" + std::to_string(body.size()) + "\r\n\r\n");
  buff.Append(body);
}


