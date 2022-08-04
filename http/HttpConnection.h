#pragma once

#include <array>
#include <asio/ip/tcp.hpp>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

#include "HttpParser.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
namespace http {
class ServerConfig;
class ConnectionManager;
constexpr size_t kBufferSize = static_cast<const size_t>(1024) * 8;
using SocketType = asio::ip::tcp::socket;
class HttpConnection : std::enable_shared_from_this<HttpConnection> {
 public:
  HttpConnection(const HttpConnection&) = delete;
  HttpConnection& operator=(const HttpConnection&) = delete;

  explicit HttpConnection(SocketType socket, ConnectionManager& manager,
                          ServerConfig& config);
  void start();
  void stop();

 private:
  void read();
  void write();

  void handleRequest();
  bool decodeUri(const std::string& uri, std::string& out);

  ServerConfig& server_config_;
  ConnectionManager& connection_manager_;
  SocketType socket_;
  std::array<char, kBufferSize> rbuffer_;
  HttpRequest request_;
  HttpParser request_parser_;
  HttpResponse response_;
};

using ConnectionPtr = std::shared_ptr<HttpConnection>;

}  // namespace http