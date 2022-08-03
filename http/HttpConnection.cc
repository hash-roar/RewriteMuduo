#include "HttpConnection.h"

#include <asio/buffer.hpp>
#include <asio/error.hpp>
#include <asio/error_code.hpp>
#include <asio/ip/tcp.hpp>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <system_error>
#include <utility>

#include "ConnectionManager.h"
#include "HttpParser.h"
#include "HttpServer.h"

namespace http {
HttpConnection::HttpConnection(SocketType socket, ConnectionManager& manager,
                               ServerConfig& config)
    : socket_(std::move(socket)),
      connection_manager_(manager),
      server_config_(config) {}

void HttpConnection::start() { read(); }

void HttpConnection::stop() { socket_.close(); }

void HttpConnection::read() {
  auto selfptr(shared_from_this());
  socket_.async_read_some(
      asio::buffer(rbuffer_),
      [this, selfptr](std::error_code ec, std::size_t read_bytes) {
        if (!ec) {
          auto [result, itr] = request_parser_.parse(
              request_, rbuffer_.data(), rbuffer_.data() + read_bytes);
          switch (result) {
            case HttpParser::FINISH:
              handleRequest();
              write();
              break;
            case HttpParser::INTERMIDIATE:
              read();
              break;
            case HttpParser::ERROR:
              write();
              break;
          }
        } else if (ec != asio::error::operation_aborted) {
          connection_manager_.stop(shared_from_this());
        }
      });
}

void HttpConnection::write() {
  auto selfptr(shared_from_this());
  socket_.async_write_some(
      asio::buffer(response_.toBuffers()),
      [this, selfptr](std::error_code ec, std::size_t write_bytes) {
        if (!ec) {
          asio::error_code error;
          socket_.shutdown(asio::ip::tcp::socket::shutdown_both, error);
          if (!error) {
            std::cout << error.message() << "\n";
          }
          if (ec != asio::error::operation_aborted) {
            connection_manager_.stop(shared_from_this());
          }
        }
      });
}

void HttpConnection::handleRequest() {}

}  // namespace http