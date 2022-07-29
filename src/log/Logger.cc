#include "log/Logger.h"

#include <cassert>
#include <bits/types/time_t.h>

#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>

#include "unix/Thread.h"

using namespace rnet;

namespace rnet::log {
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

Logger::LogLevel GlobalLogLevel = initLogLevel();
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
// TimeZone GlobalLogTimeZone;

} // namespace rnet::log

using namespace rnet::log;

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
    stream_ << getErrnoMessage(savedErrno) << " (errno=" << savedErrno << ") ";
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
    // if (GlobalLogTimeZone.valid()) {
    //   tm_time = GlobalLogTimeZone.toLocalTime(seconds);
    // } else {
    //   ::gmtime_r(&seconds, &tm_time);  // FIXME TimeZone::fromUtcTime
    // }
    ::gmtime_r(&seconds, &tm_time);  // FIXME TimeZone::fromUtcTime

    int len =
        snprintf(tTimeBuf.data(), tTimeBuf.size(), "%4d%02d%02d %02d:%02d:%02d",
                 tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    assert(len == 17);
    (void)len;
  }

  // if (GlobalLogTimeZone.valid()) {
  //   Fmt us(".%06d ", microseconds);
  //   assert(us.length() == 8);
  //   stream_ << T(tTimeBuf.data(), 17) << T(us.data(), 8);
  // } else {
  //   Fmt us(".%06dZ ", microseconds);
  //   assert(us.length() == 9);
  //   stream_ << T(tTimeBuf.data(), 17) << T(us.data(), 9);
  // }
  Fmt us(".%06dZ ", microseconds);
  assert(us.length() == 9);
  stream_ << T(tTimeBuf.data(), 17) << T(us.data(), 9);
}

void Logger::Impl::finish() {
  stream_ << " - " << basename_ << ':' << line_ << '\n';
}

Logger::Logger(SourceFile file, int line) : impl_(INFO, 0, file, line) {}

Logger::Logger(SourceFile file, int line, LogLevel level, const char* func)
    : impl_(level, 0, file, line) {
  impl_.stream_ << func << ' ';
}

Logger::Logger(SourceFile file, int line, LogLevel level)
    : impl_(level, 0, file, line) {}

Logger::Logger(SourceFile file, int line, bool toAbort)
    : impl_(toAbort ? FATAL : ERROR, errno, file, line) {}

Logger::~Logger() {
  impl_.finish();
  const LogStream::Buffer& buf(stream().buffer());
  GlobalOutput(buf.data(), buf.length());
  if (impl_.level_ == FATAL) {
    GlobalFlush();
    abort();
  }
}
void Logger::setLogLevel(Logger::LogLevel level) { GlobalLogLevel = level; }

void Logger::setOutput(OutputFunc out) { GlobalOutput = out; }

void Logger::setFlush(FlushFunc flush) { GlobalFlush = flush; }

// void Logger::setTimeZone(const TimeZone& tz) { GlobalLogTimeZone = tz; }
