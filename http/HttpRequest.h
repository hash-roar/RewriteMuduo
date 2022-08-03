#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
namespace http {
class HttpRequest {
 public:
  using Header = std::pair<std::string, std::string>;
  using HeaderMap = std::unordered_map<std::string, std::string>;

  HttpRequest() = default;
  ~HttpRequest() = default;
  auto& version() { return version_; }
  auto& method() { return method_; }
  auto& uri() { return uri_; }
  void addHeader(const Header& header) {
    headers_[header.first] = header.second;
  }
 private:
  std::string version_;
  std::string method_;
  std::string uri_;
  HeaderMap headers_;
};
}  // namespace http