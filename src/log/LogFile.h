#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include "base/Common.h"
#include "file/File.h"

namespace rnet::log {
class LogFile : Noncopyable {
public:
  LogFile( const std::string& basename, off_t rollSize, bool threadSafe = true, int flushInterval = 3, int checkEveryN = kbSize );
  ~LogFile();

  void Append( const char* logline, int len );
  void Flush();
  bool RollFile();

private:
  void AppendUnlocked( const char* logline, int len );

  static std::string GetLogFileName( const std::string& basename, time_t* now );

  const std::string basename;
  const off_t       rollSize;
  const int         flushInterval;
  const int         checkEveryN;

  int count_;

  std::optional< std::mutex >         mutex_;
  time_t                              startOfPeriod_;
  time_t                              lastRoll_;
  time_t                              lastFlush_;
  std::unique_ptr< file::AppendFile > file_;

  constexpr static int kRollPerSeconds = 60 * 60 * 24;
};

}  // namespace rnet::log