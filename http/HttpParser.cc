#include "HttpParser.h"

#include <cassert>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace {
inline bool isChar(int c) { return c >= 0 && c <= 127; }

inline bool isCtl(int c) { return (c >= 0 && c <= 31) || (c == 127); }

inline bool isDigit(int c) { return c >= '0' && c <= '9'; }

std::vector<std::string_view> splitStringView(std::string_view str_view,
                                              char delimeter) {
  std::vector<std::string_view> result{};
  auto last_pos = str_view.begin();
  auto pos = str_view.find_first_of(delimeter);
  while (pos != std::string::npos) {
    result.emplace_back(last_pos, last_pos + pos);
    last_pos = last_pos + pos;
    str_view.find(delimeter, pos);
    pos = str_view.find_first_of(delimeter, pos);
  }
  return result;
}

}  // namespace

namespace http {
constexpr size_t kTempLineSize = 64;
HttpParser::HttpParser() : state_(START) { temp_line_.reserve(kTempLineSize); }

template <class Itr>
std::pair<HttpParser::Result, Itr> HttpParser::parse(HttpRequest& req,
                                                     Itr begin, Itr end) {
  std::string_view str_view(begin, end);

  auto pos = str_view.find("\r\n");
  auto last_pos = pos;
  while (pos != std::string::npos) {
    temp_line_.append(str_view.begin(), pos);
    last_pos = pos;
    auto parse_result = parseOneLine(req, temp_line_);
    temp_line_.clear();

    switch (parse_result) {
      case INTERMIDIATE:
        break;
      case FINISH:
        return {FINISH, end};
      case ERROR:
        return {ERROR, end};
    }
  }
  if (last_pos != std::string::npos) {
    temp_line_.append(str_view.begin() + last_pos + 2, str_view.end());
  } else {
    temp_line_.append(str_view.begin(), str_view.begin());
  }
  return {INTERMIDIATE, end};
}

HttpParser::Result HttpParser::parseOneLine(HttpRequest& req,
                                            std::string_view line) {
  switch (state_) {
    case START: {
      auto str_vec = splitStringView(line, ' ');
      // TODO validation
      req.method() = str_vec[0];
      req.uri() = str_vec[1];
      req.version() = str_vec[2];
      state_ = LINE_OPEN;
      return INTERMIDIATE;
    }
    case LINE_OPEN: {
      if (line.empty()) {
        // TODO get content length
        return FINISH;
      }

      auto str_vec = splitStringView(line, ':');
      // TODO validation
      req.addHeader({std::string{str_vec[0]}, std::string{str_vec[1]}});
    }
  }
  return INTERMIDIATE;
}

}  // namespace http