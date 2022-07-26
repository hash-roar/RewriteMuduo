#include "Logger.h"

#include <assert.h>
#include <bits/types/time_t.h>

#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>

#include "detail/Thread.h"

using namespace rnet;
using namespace rnet::detail;

namespace rnet {
// 线程内缓存
thread_local std::array<char, 512> tErrnoBuf;
thread_local std::array<char, 512> tTimeBuf;
thread_local time_t tLastSecond;

const char* getErrnoMessage(int savedErrno) {
  return strerror_r(savedErrno, tErrnoBuf.data(), tErrnoBuf.size());
}

Logger::LogLevel initLogLevel() {
  if (::getenv("LOG_TRACE"))
    return Logger::TRACE;
  else if (::getenv("LOG_DEBUG"))
    return Logger::DEBUG;
  else
    return Logger::INFO;
}

Logger::LogLevel g_logLevel = initLogLevel();
constexpr const char* LogLevelName[Logger::NUM_LOG_LEVELS] = {
    "TRACE ", "DEBUG ", "INFO  ", "WARN  ", "ERROR ", "FATAL ",
};

class T {
 public:
  T(const char* str, unsigned len) : str_(str), len_(len) {
    assert(strlen(str) == len_);
  }

  const char* str_;
  const unsigned len_;
};

inline LogStream& operator<<(LogStream& s, T v) {
  s.append(v.str_, v.len_);
  return s;
}

inline LogStream& operator<<(LogStream& s, const Logger::SourceFile& v) {
  s.append(v.data_, v.size_);
  return s;
}

void WriteStdout(const char* buf, size_t len) {
  size_t write_len = fwrite(buf, 1, len, stdout);
}

void FlushStdout() { fflush(stdout); }

Logger::OutputFunc GlobalOutput = WriteStdout;
Logger::FlushFunc GlobalFlush = FlushStdout;
TimeZone GlobalLogTimeZone;

}  // namespace rnet

Logger::Impl::Impl(LogLevel level, int savedErrno, const SourceFile& file,
                   int line)
    : time_(Timestamp::now()),
      stream_(),
      level_(level),
      line_(line),
      basename_(file) {
  formatTime();
  Thread::tid();
  stream_ << T(Thread::tidString(), Thread::tidStringLength());
  stream_ << T(LogLevelName[level], 6);
  if (savedErrno != 0) {
    stream_ << strerror_tl(savedErrno) << " (errno=" << savedErrno << ") ";
  }
}

void Logger::Impl::formatTime() {
  int64_t microSecondsSinceEpoch = time_.microSecondsSinceEpoch();
  time_t seconds = static_cast<time_t>(microSecondsSinceEpoch /
                                       Timestamp::kMicroSecondsPerSecond);
  int microseconds = static_cast<int>(microSecondsSinceEpoch %
                                      Timestamp::kMicroSecondsPerSecond);
  if (seconds != tLastSecond) {
    tLastSecond = seconds;
    struct tm tm_time;
    if (GlobalLogTimeZone.valid()) {
      tm_time = GlobalLogTimeZone.toLocalTime(seconds);
    } else {
      ::gmtime_r(&seconds, &tm_time);  // FIXME TimeZone::fromUtcTime
    }

    int len =
        snprintf(tTimeBuf.data(), tTimeBuf.size(), "%4d%02d%02d %02d:%02d:%02d",
                 tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    assert(len == 17);
    (void)len;
  }

  if (GlobalLogTimeZone.valid()) {
    Fmt us(".%06d ", microseconds);
    assert(us.length() == 8);
    stream_ << T(tTimeBuf.data(), 17) << T(us.data(), 8);
  } else {
    Fmt us(".%06dZ ", microseconds);
    assert(us.length() == 9);
    stream_ << T(tTimeBuf.data(), 17) << T(us.data(), 9);
  }
}