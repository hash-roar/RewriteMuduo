#include "log/LogFile.h"

#include <bits/types/FILE.h>

#include <cassert>
#include <mutex>
#include <optional>

#include "file/File.h"
#include "unix/ProcssInfo.h"

namespace rnet::log {

LogFile::LogFile( const std::string& basename, off_t rollSize, bool threadSafe, int flushInterval, int checkEveryN )
  : basename( basename ), rollSize( rollSize ), flushInterval( flushInterval ), checkEveryN( checkEveryN ), count_( 0 ), mutex_( threadSafe ? std::make_optional< std::mutex >() : std::nullopt ),
    startOfPeriod_( 0 ), lastRoll_( 0 ), lastFlush_( 0 ) {
  assert( basename.find( '/' ) == std::string::npos );
  RollFile();
}

LogFile::~LogFile() = default;

void LogFile::Append( const char* logline, int len ) {
  if ( mutex_.has_value() ) {
    std::lock_guard lock( mutex_.value() );
    AppendUnlocked( logline, len );
  }
  else {
    AppendUnlocked( logline, len );
  }
}

void LogFile::Flush() {
  if ( mutex_.has_value() ) {
    std::lock_guard lock( mutex_.value() );
    file_->Flush();
  }
  else {
    file_->Flush();
  }
}

void LogFile::AppendUnlocked( const char* logline, int len ) {
  file_->Append( logline, len );

  if ( file_->WrittenBytes() > rollSize ) {
    RollFile();
  }
  else {
    ++count_;
    if ( count_ >= checkEveryN ) {
      count_            = 0;
      time_t now        = ::time( nullptr );
      time_t thisPeriod = now / kRollPerSeconds * kRollPerSeconds;
      if ( thisPeriod != startOfPeriod_ ) {
        RollFile();
      }
      else if ( now - lastFlush_ > flushInterval ) {
        lastFlush_ = now;
        file_->Flush();
      }
    }
  }
}

bool LogFile::RollFile() {
  time_t      now      = 0;
  std::string filename = GetLogFileName( basename, &now );
  time_t      start    = now / kRollPerSeconds * kRollPerSeconds;

  if ( now > lastRoll_ ) {
    lastRoll_      = now;
    lastFlush_     = now;
    startOfPeriod_ = start;
    file_.reset( new file::AppendFile( filename ) );
    return true;
  }
  return false;
}

std::string LogFile::GetLogFileName( const std::string& basename, time_t* now ) {
  std::string filename;
  filename.reserve( basename.size() + 64 );
  filename = basename;

  char      timebuf[ 32 ];
  struct tm tm;
  *now = time( nullptr );
  gmtime_r( now, &tm );  // FIXME: localtime_r ?
  strftime( timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm );
  filename += timebuf;

  filename += Unix::Hostname();

  char pidbuf[ 32 ];
  snprintf( pidbuf, sizeof pidbuf, ".%d", Unix::Pid() );
  filename += pidbuf;

  filename += ".log";

  return filename;
}
}  // namespace rnet::log
