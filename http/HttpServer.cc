#include "HttpServer.h"

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <cstdio>
#include <memory>
#include <system_error>
#include <utility>

#include "ConnectionManager.h"
#include "HttpConnection.h"

using namespace http;
namespace http {
HttpServer::HttpServer(const std::string& addr, const std::string port,
                       const std::string document_root)
    : io_context_(1),
      accepter_(io_context_),
      signal_(io_context_),
      connections_(),
      server_config_{document_root} {
  signal_.add(SIGINT);
  signal_.add(SIGTERM);

  handleSignal();

  // init acceptor
  asio::ip::tcp::resolver resolver(io_context_);
  asio::ip::tcp::endpoint endpoint = *resolver.resolve(addr, port).begin();
  accepter_.open(endpoint.protocol());
  accepter_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
  accepter_.bind(endpoint);
  accepter_.listen();

  accept();
}

void HttpServer::accept() {
  accepter_.async_accept([this](std::error_code ec, SocketType socket) {
    if (!accepter_.is_open()) {
      std::printf("acceptor is closed: %s\t",
                  accepter_.local_endpoint().address().to_string().c_str());
      return;
    }

    if (!ec) {
      connections_.start(
          std::make_shared<HttpConnection>(std::move(socket), connections_));
    }
    accept();
  });
}

void HttpServer::handleSignal() {
  signal_.async_wait([this](std::error_code ec) {
    accepter_.close();
    connections_.stopAllConnection();
  });
}

void HttpServer::run() { io_context_.run(); }

}  // namespace http