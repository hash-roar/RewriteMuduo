#pragma once

#include <string>
#include <utility>
namespace http {
class HttpRequest {
 public:
  using header = std::pair<std::string, std::string>;
  HttpRequest();
  HttpRequest(HttpRequest &&) = default;
  HttpRequest(const HttpRequest &) = default;
  HttpRequest &operator=(HttpRequest &&) = default;
  HttpRequest &operator=(const HttpRequest &) = default;
  ~HttpRequest();

 private:
};
}  // namespace http