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
      trace,
      debug,
      info,
      warn,
      error,
      fatal,
      numLogLevels,
    };

    // compile time calculation of basename of source file
    class SourceFile {
    public:
      template < int N > SourceFile( char ( &arr )[ N ] ) : data_( arr ), size_( N - 1 ) {
        const char* slash = strrchr( data_, '/' );  // builtin function
        if ( slash ) {
          data_ = slash + 1;
          size_ -= static_cast< int >( data_ - arr );
        }
      }

      SourceFile( const char* filename ) : data_( filename ) {
        const char* slash = strrchr( filename, '/' );
        if ( slash ) {
          data_ = slash + 1;
        }
        size_ = static_cast< int >( strlen( data_ ) );
      }

      const char* data_;
      int         size_;
    };

    Logger( SourceFile file, int line );
    Logger( SourceFile file, int line, LogLevel level );
    Logger( SourceFile file, int line, LogLevel level, const char* func );
    Logger( SourceFile file, int line, bool toAbort );
    ~Logger();

    LogStream& Stream() {
      return impl_.stream_;
    }

    static LogLevel LogLevel();
    static void     SetLogLevel( enum LogLevel level );

    using OutputFunc = void ( * )( const char*, size_t );
    using FlushFunc  = void ( * )();

    static void SetOutput( OutputFunc );
    static void SetFlush( FlushFunc );
    // static void setTimeZone(const detail::TimeZone& tz);

  private:
    class Impl {
    public:
      using LogLevel = enum Logger::LogLevel;
      Impl( LogLevel level, int old_errno, const SourceFile& file, int line );
      void FormatTime();
      void Finish();

      Timestamp  time_;
      LogStream  stream_;
      LogLevel   level_;
      int        line_;
      SourceFile basename_;
    };

    Impl impl_;
  };

  extern enum Logger::LogLevel globalLogLevel;

  inline enum Logger::LogLevel Logger::LogLevel() {
    return globalLogLevel;
  }

  class Fmt  // : noncopyable
  {
  public:
    template < typename T > Fmt( const char* fmt, T val ) {
      static_assert( std::is_arithmetic< T >::value == true, "Must be arithmetic type" );

      length_ = snprintf( buf_, sizeof buf_, fmt, val );
      assert( static_cast< size_t >( length_ ) < sizeof buf_ );
    }

    const char* Data() const {
      return buf_;
    }
    int Length() const {
      return length_;
    }

  private:
    char buf_[ 32 ];
    int  length_;
  };

}  // namespace log

#define LOG_TRACE                                      \
  if ( log::Logger::LogLevel() <= log::Logger::trace ) \
  log::Logger( __FILE__, __LINE__, log::Logger::trace, __func__ ).Stream()
#define LOG_DEBUG                                      \
  if ( log::Logger::LogLevel() <= log::Logger::debug ) \
  log::Logger( __FILE__, __LINE__, log::Logger::debug, __func__ ).Stream()
#define LOG_INFO                                      \
  if ( log::Logger::LogLevel() <= log::Logger::info ) \
  log::Logger( __FILE__, __LINE__ ).Stream()
#define LOG_WARN log::Logger( __FILE__, __LINE__, log::Logger::warn ).Stream()
#define LOG_ERROR log::Logger( __FILE__, __LINE__, log::Logger::error ).Stream()
#define LOG_FATAL log::Logger( __FILE__, __LINE__, log::Logger::fatal ).Stream()
#define LOG_SYSERR log::Logger( __FILE__, __LINE__, false ).Stream()
#define LOG_SYSFATAL log::Logger( __FILE__, __LINE__, true ).Stream()

// Check that the input is non NULL.  This very useful in constructor
// initializer lists.

#define CHECK_NOTNULL( val ) ::CheckNotNull( __FILE__, __LINE__, "'" #val "' Must be non NULL", ( val ) )

// A small helper for CHECK_NOTNULL().
template < typename T > T* CheckNotNull( log::Logger::SourceFile file, int line, const char* names, T* ptr ) {
  if ( ptr == NULL ) {
    log::Logger( file, line, log::Logger::fatal ).Stream() << names;
  }
  return ptr;
}

}  // namespace rnet