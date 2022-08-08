#include "HttpResponse.h"

#include <asio/buffer.hpp>
#include <map>
#include <string>
#include <vector>

namespace {
using namespace http;
// using namespace HttpResponse;
const std::map<HttpResponse::Status, std::string> status_map{
    {HttpResponse::OK, "OK"}};
}  // namespace
namespace http {
HttpResponse HttpResponse::buildResponse(Status) {
  HttpResponse rep{};
  return rep;
}

std::vector<asio::const_buffer> HttpResponse::toBuffers() {
  rep_header_.append(head_line_);
  for (const auto& [key, value] : headers) {
    rep_header_.append(key);
    rep_header_.append(":");
    rep_header_.append(value);
    rep_header_.append("\r\n");
  }
  rep_header_.append("Content-Length:" + std::to_string(content.size()) +
                     "\r\n\r\n");
  return {{rep_header_.data(), rep_header_.length()},
          {content.data(), content.length()}};
}
std::string HttpResponse::statusToString(HttpResponse::Status status) const {
  return status_map.at(status);  // throw at not_found
}
}  // namespace http