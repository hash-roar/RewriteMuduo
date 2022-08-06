#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <tuple>
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
  void addHeader(Header header) {
    // perfect forward three times?
    headers_.insert(std::move(header));
  }

  std::optional<std::string> getHeader(const std::string& key) {
    auto itr = headers_.find(key);
    return itr == headers_.end() ? std::nullopt
                                 : std::make_optional(itr->second);
  }

  void addBuffer(std::string_view buffer) { read_buffer_.append(buffer); }

 private:
  std::string version_;
  std::string method_;
  std::string uri_;
  std::string read_buffer_;
  HeaderMap headers_;
};
}  // namespace http