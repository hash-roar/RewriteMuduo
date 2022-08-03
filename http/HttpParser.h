#pragma once

#include <climits>
#include <string_view>
#include <utility>

#include "HttpRequest.h"
namespace http {
class HttpParser {
 public:
  enum Result { INTERMIDIATE, FINISH, ERROR };
  HttpParser() ;
  ~HttpParser();

  void reset();

  template <class Itr>
  std::pair<Result, Itr> parse(HttpRequest& req, Itr begin, Itr end);

 private:
  Result parseOneLine(HttpRequest& req, std::string_view line);
  enum ParseState {
    START,
    METHOD_LINE_OPEN,
    LINE_OPEN,
    EMPTY_LINE_OPEN,
    SUCCESS,
  };

  ParseState state_;
};
}  // namespace http