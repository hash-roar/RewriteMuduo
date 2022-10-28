#include "log/Logger.h"

#include <bits/types/time_t.h>

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstring>

#include "unix/Thread.h"

using namespace rnet;

namespace rnet::log {
// 线程内缓存
thread_local std::array< char, 512 > tErrnoBuf;
thread_local std::array< char, 512 > tTimeBuf;
thread_local time_t                  tLastSecond;

const char* GetErrnoMessage( int savedErrno ) {
  return strerror_r( savedErrno, tErrnoBuf.data(), tErrnoBuf.size() );
}

enum Logger::LogLevel InitLogLevel() {
  if ( ::getenv( "LOG_TRACE" ) ) {
    return Logger::trace;
  }
  else if ( ::getenv( "LOG_DEBUG" ) ) {
    return Logger::debug;
  }
  else {
    return Logger::info;
  }
}

enum Logger::LogLevel globalLogLevel                       = InitLogLevel();
constexpr const char* logLevelName[ Logger::numLogLevels ] = {
  "TRACE ", "DEBUG ", "INFO  ", "WARN  ", "ERROR ", "FATAL ",
};

class T {
public:
  T( const char* str, unsigned len ) : str_( str ), len( len ) {
    assert( strlen( str ) == len );
  }

  const char*    str_;
  const unsigned len;
};

inline LogStream& operator<<( LogStream& s, T v ) {
  s.Append( v.str_, v.len );
  return s;
}

inline LogStream& operator<<( LogStream& s, const Logger::SourceFile& v ) {
  s.Append( v.data_, v.size_ );
  return s;
}

void WriteStdout( const char* buf, size_t len ) {
  size_t writeLen = fwrite( buf, 1, len, stdout );
}

void FlushStdout() {
  fflush( stdout );
}

Logger::OutputFunc globalOutput = WriteStdout;
Logger::FlushFunc  globalFlush  = FlushStdout;
// TimeZone GlobalLogTimeZone;

}  // namespace rnet::log

using namespace rnet::log;

Logger::Impl::Impl( LogLevel level, int savedErrno, const SourceFile& file, int line ) : time_( Timestamp::Now() ), stream_(), level_( level ), line_( line ), basename_( file ) {
  FormatTime();
  thread::Tid();
  stream_ << T( thread::TidString(), thread::TidStringLength() );
  stream_ << T( logLevelName[ level ], 6 );
  if ( savedErrno != 0 ) {
    stream_ << GetErrnoMessage( savedErrno ) << " (errno=" << savedErrno << ") ";
  }
}

void Logger::Impl::FormatTime() {
  int64_t microSecondsSinceEpoch = time_.MicroSecondsSinceEpoch();
  time_t  seconds                = static_cast< time_t >( microSecondsSinceEpoch / Timestamp::kMicroSecondsPerSecond );
  int     microseconds           = static_cast< int >( microSecondsSinceEpoch % Timestamp::kMicroSecondsPerSecond );
  if ( seconds != tLastSecond ) {
    tLastSecond = seconds;
    struct tm tmTime;
    ::gmtime_r( &seconds, &tmTime );  // FIXME TimeZone::fromUtcTime

    int len = snprintf( tTimeBuf.data(), tTimeBuf.size(), "%4d%02d%02d %02d:%02d:%02d", tmTime.tm_year + 1900, tmTime.tm_mon + 1, tmTime.tm_mday, tmTime.tm_hour, tmTime.tm_min, tmTime.tm_sec );
    assert( len == 17 );
    ( void )len;
  }

  Fmt us( ".%06dZ ", microseconds );
  assert( us.Length() == 9 );
  stream_ << T( tTimeBuf.data(), 17 ) << T( us.Data(), 9 );
}

void Logger::Impl::Finish() {
  stream_ << " - " << basename_ << ':' << line_ << '\n';
}

Logger::Logger( SourceFile file, int line ) : impl_( info, 0, file, line ) {}

Logger::Logger( SourceFile file, int line, enum LogLevel level, const char* func ) : impl_( level, 0, file, line ) {
  impl_.stream_ << func << ' ';
}

Logger::Logger( SourceFile file, int line, enum LogLevel level ) : impl_( level, 0, file, line ) {}

Logger::Logger( SourceFile file, int line, bool toAbort ) : impl_( toAbort ? fatal : error, errno, file, line ) {}

Logger::~Logger() {
  impl_.Finish();
  const LogStream::Buffer& buf( Stream().GetBuffer() );
  globalOutput( buf.Data(), buf.Length() );
  if ( impl_.level_ == fatal ) {
    globalFlush();
    abort();
  }
}
void Logger::SetLogLevel( enum Logger::LogLevel level ) {
  globalLogLevel = level;
}

void Logger::SetOutput( OutputFunc out ) {
  globalOutput = out;
}

void Logger::SetFlush( FlushFunc flush ) {
  globalFlush = flush;
}

// void Logger::setTimeZone(const TimeZone& tz) { GlobalLogTimeZone = tz; }
