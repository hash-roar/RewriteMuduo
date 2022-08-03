#pragma once
#include <asio.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
namespace http {
namespace ip = asio::ip;
class HttpServer {
 private:
  asio::io_context io_context_;
  ip::tcp::acceptor accepter_;
};
}  // namespace http