#pragma once

#include <array>
#include <asio/ip/tcp.hpp>
#include <cstddef>
#include <memory>

#include "HttpParser.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
namespace http {
constexpr size_t kBufferSize = static_cast<const size_t>(1024) * 8;
class HttpConnection : std::enable_shared_from_this<HttpConnection> {
 public:
  HttpConnection(const HttpConnection&) = delete;
  HttpConnection& operator=(const HttpConnection&) = delete;

  void start();
  void stop();

 private:
  void read();
  void write();

  asio::ip::tcp::socket socket_;
  std::array<char, kBufferSize> rbuffer_;
  HttpRequest request_;
  HttpParser request_parser_;
  HttpResponse response_;
};

using ConnectionPtr = std::shared_ptr<HttpConnection>;
}  // namespace http