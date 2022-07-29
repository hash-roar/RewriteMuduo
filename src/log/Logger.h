#pragma once
#include <cassert>
#include <cstddef>
#include <cstring>

#include "Logger.h"
#include "file/Buffer.h"
#include "log/LogStream.h"
#include "unix/Time.h"
namespace rnet {
namespace log {

// 每条日志创建一个logger对象然后丢弃,在析构函数中写入异步日志后端
// 意味着写入后端操作不能有异常
class Logger {
  using Timestamp = Unix::Timestamp;

 public:
  enum LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
    NUM_LOG_LEVELS,
  };

  // compile time calculation of basename of source file
  class SourceFile {
   public:
    template <int N>
    SourceFile(char (&arr)[N]) : data_(arr), size_(N - 1) {
      const char* slash = strrchr(data_, '/');  // builtin function
      if (slash) {
        data_ = slash + 1;
        size_ -= static_cast<int>(data_ - arr);
      }
    }

    SourceFile(const char* filename) : data_(filename) {
      const char* slash = strrchr(filename, '/');
      if (slash) {
        data_ = slash + 1;
      }
      size_ = static_cast<int>(strlen(data_));
    }

    const char* data_;
    int size_;
  };

  Logger(SourceFile file, int line);
  Logger(SourceFile file, int line, LogLevel level);
  Logger(SourceFile file, int line, LogLevel level, const char* func);
  Logger(SourceFile file, int line, bool toAbort);
  ~Logger();

  LogStream& stream() { return impl_.stream_; }

  static LogLevel logLevel();
  static void setLogLevel(LogLevel level);

  using OutputFunc = void (*)(const char*, size_t);
  using FlushFunc = void (*)();

  static void setOutput(OutputFunc);
  static void setFlush(FlushFunc);
  // static void setTimeZone(const detail::TimeZone& tz);

 private:
  class Impl {
   public:
    using LogLevel = Logger::LogLevel;
    Impl(LogLevel level, int old_errno, const SourceFile& file, int line);
    void formatTime();
    void finish();

    Timestamp time_;
    LogStream stream_;
    LogLevel level_;
    int line_;
    SourceFile basename_;
  };

  Impl impl_;
};

extern Logger::LogLevel GlobalLogLevel;

inline Logger::LogLevel Logger::logLevel() { return GlobalLogLevel; }

class Fmt  // : noncopyable
{
 public:
  template <typename T>
  Fmt(const char* fmt, T val) {
    static_assert(std::is_arithmetic<T>::value == true,
                  "Must be arithmetic type");

    length_ = snprintf(buf_, sizeof buf_, fmt, val);
    assert(static_cast<size_t>(length_) < sizeof buf_);
  }

  const char* data() const { return buf_; }
  int length() const { return length_; }

 private:
  char buf_[32];
  int length_;
};

}  // namespace log
//
// CAUTION: do not write:
//
// if (good)
//   LOG_INFO << "Good news";
// else
//   LOG_WARN << "Bad news";
//
// this expends to
//
// if (good)
//   if (logging_INFO)
//     logInfoStream << "Good news";
//   else
//     logWarnStream << "Bad news";
//

#define LOG_TRACE                                    \
  if (log::Logger::logLevel() <= log::Logger::TRACE) \
  log::Logger(__FILE__, __LINE__, log::Logger::TRACE, __func__).stream()
#define LOG_DEBUG                                    \
  if (log::Logger::logLevel() <= log::Logger::DEBUG) \
  log::Logger(__FILE__, __LINE__, log::Logger::DEBUG, __func__).stream()
#define LOG_INFO                                    \
  if (log::Logger::logLevel() <= log::Logger::INFO) \
  log::Logger(__FILE__, __LINE__).stream()
#define LOG_WARN log::Logger(__FILE__, __LINE__, log::Logger::WARN).stream()
#define LOG_ERROR log::Logger(__FILE__, __LINE__, log::Logger::ERROR).stream()
#define LOG_FATAL log::Logger(__FILE__, __LINE__, log::Logger::FATAL).stream()
#define LOG_SYSERR log::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL log::Logger(__FILE__, __LINE__, true).stream()

// const char* getErrnoMessage(int savedErrno);

// Taken from glog/logging.h
//
// Check that the input is non NULL.  This very useful in constructor
// initializer lists.

#define CHECK_NOTNULL(val) \
  ::log::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

// A small helper for CHECK_NOTNULL().
template <typename T>
T* CheckNotNull(log::Logger::SourceFile file, int line, const char* names,
                T* ptr) {
  if (ptr == NULL) {
    log::Logger(file, line, log::Logger::FATAL).stream() << names;
  }
  return ptr;
}

}  // namespace rnet