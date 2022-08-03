#pragma once

#include <asio/buffer.hpp>
#include <string>
#include <utility>
#include <vector>
namespace http {
struct HttpResponse {
  using Header = std::pair<std::string, std::string>;
  std::vector<Header> headers;
  std::string content;
  std::vector<asio::const_buffer> toBuffers();
};
}  // namespace http