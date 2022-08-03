#pragma once

#include <asio/buffer.hpp>
#include <string>
#include <utility>
#include <vector>
namespace http {
struct HttpResponse {
  enum Status {
    OK = 200,
    CREATED = 201,
    ACCEPTED = 202,
    NO_CONTENT = 204,
    MULTIPLE_CHOICES = 300,
    MOVED_PERMANENTLY = 301,
    MOVED_TEMPORARILY = 302,
    NOT_MODIFIED = 304,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    BAD_GATEWAY = 502,
    SERVICE_UNAVAILABLE = 503
  };
  using Header = std::pair<std::string, std::string>;
  Status status_;
  std::vector<Header> headers;
  std::string content;
  std::vector<asio::const_buffer> toBuffers();

  HttpResponse static buildResponse(Status);
};
}  // namespace http