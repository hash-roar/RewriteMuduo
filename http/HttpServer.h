#pragma once
#include <asio.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/signal_set.hpp>
#include <string>

#include "ConnectionManager.h"
namespace http {

struct ServerConfig {
  std::string document_root_;
};
class HttpServer {
 public:
  HttpServer(const HttpServer&) = delete;
  HttpServer& operator=(const HttpServer&) = delete;

  explicit HttpServer(const std::string& addr, const std::string port,
                      const std::string document_root);

  void run();

 private:
  void accept();

  // void handleRequest();
  void handleSignal();
  ServerConfig server_config_;
  asio::io_context io_context_;
  asio::ip::tcp::acceptor accepter_;
  asio::signal_set signal_;
  ConnectionManager connections_;
};
}  // namespace http