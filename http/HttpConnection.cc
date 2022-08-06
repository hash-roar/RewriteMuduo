#include "HttpConnection.h"

#include <strings.h>

#include <asio/buffer.hpp>
#include <asio/error.hpp>
#include <asio/error_code.hpp>
#include <asio/ip/tcp.hpp>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include "ConnectionManager.h"
#include "HttpParser.h"
#include "HttpResponse.h"
#include "HttpServer.h"
#include "file/File.h"

namespace fs = std::filesystem;
namespace {
constexpr size_t kMaxFileBufferSize = 1024 * 126;

enum class FsError {
  NOT_FOUND,
  NONE,
  PERMISSION_OUT,
};

bool isValidatePath(std::string_view path) {
  if (path.empty() || path[0] != '/' || path.find("..") != std::string::npos) {
    return false;
  }
  return true;
}

std::optional<std::string> getFileExtension(std::string_view path) {
  return {};
}

std::pair<std::size_t, FsError> fileSize(std::string_view path) {
  fs::path fs_path{path};
  if (!fs::exists(path)) {
    return {0, FsError::NOT_FOUND};
  }
  return {fs::file_size(fs_path), FsError::NONE};
}
}  // namespace
namespace http {
HttpConnection::HttpConnection(SocketType socket, ConnectionManager& manager,
                               ServerConfig& config)
    : socket_(std::move(socket)),
      connection_manager_(manager),
      server_config_(config),
      conn_state_(PARSE) {
  bzero(rbuffer_.data(), rbuffer_.size());
}

void HttpConnection::start() { read(); }

void HttpConnection::stop() { socket_.close(); }

void HttpConnection::read() {
  auto selfptr(shared_from_this());
  socket_.async_read_some(asio::buffer(rbuffer_), [this, selfptr](
                                                      std::error_code ec,
                                                      std::size_t read_bytes) {
    // TODO handle methods which carrying loads of bytes
    size_t content_offset = 0;
    if (!ec) {
      switch (conn_state_) {
        case PARSE: {
          auto [result, itr] = request_parser_.parse(
              request_, rbuffer_.begin(), rbuffer_.begin() + read_bytes);
          switch (result) {
            case HttpParser::FINISH: {
              auto content_len = request_.getHeader("Content-Length");
              if (!content_len) {
                conn_state_ = ERROR;
                response_ =
                    HttpResponse::buildResponse(HttpResponse::BAD_REQUEST);
                write();
                break;
              }
              need_read_ = std::stoi(content_len.value());
              // itr =
              // static_cast<std::string::const_iterator::iterator_type::>(itr);
              // handleRequest();

              content_offset = itr - rbuffer_.begin();
              request_.addBuffer({itr, read_bytes - content_offset});
              conn_state_ = READ_BUFFER;
              break;
            }
            case HttpParser::INTERMIDIATE: {
              read();
              break;
            }
            case HttpParser::ERROR: {
              response_ =
                  HttpResponse::buildResponse(HttpResponse::BAD_REQUEST);
              conn_state_ = ERROR;
              write();
              break;
            }
          }

          // no break here intentional
        }
        case READ_BUFFER: {
          need_read_ -= static_cast<int32_t>(read_bytes - content_offset);
          if (need_read_ < 0) {
            // TODO incorrect connection
          }
          request_.addBuffer({rbuffer_.begin() + content_offset, read_bytes});
          if (need_read_ == 0) {
            conn_state_ = READ_FINISH;
          }
        }
        case READ_FINISH: {
          handleRequest();
          write();
        }
        case ERROR: {
          // do nothing
        }
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

// handle get method
void HttpConnection::handleRequest() {
  std::string path;
  if (!decodeUri(request_.uri(), path) || !isValidatePath(path)) {
    response_ = HttpResponse::buildResponse(HttpResponse::BAD_REQUEST);
    return;
  }

  if (path[path.size() - 1] == '/') {
    path += "index.html";
  }

  path = server_config_.document_root_ + path;

  auto [file_size, result] = fileSize(path);
  if (result != FsError::NONE) {
    // TODO
    switch (result) {
      case FsError::NOT_FOUND: {
        response_ = HttpResponse::buildResponse(HttpResponse::NOT_FOUND);
        return;
      }
      default: {
        response_ =
            HttpResponse::buildResponse(HttpResponse::INTERNAL_SERVER_ERROR);
        return;
      }
    }
  }
  response_.content.reserve(file_size);
  rnet::File::readFile(path, response_.content.size(),
                       response_.content.data());
  response_.headers.emplace_back("Content-Length", std::to_string(file_size));
  response_.headers.emplace_back("Content-Type", "txt/html");
}

bool HttpConnection::decodeUri(const std::string& uri, std::string& out) {
  // TODO
  return true;
}

}  // namespace http