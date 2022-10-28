#pragma once
#include <string>
#include <string_view>
#include <vector>

#include "Time.h"
namespace rnet::Unix {
pid_t Pid();
std::string PidString();
uid_t Uid();
std::string Username();
uid_t Euid();
Timestamp StartTime();
int ClockTicksPerSecond();
int PageSize();
bool IsDebugBuild();  // constexpr

std::string Hostname();
std::string Procname();
std::string_view Procname(const std::string& stat);

/// read /proc/self/status
std::string ProcStatus();

/// read /proc/self/stat
std::string ProcStat();

/// read /proc/self/task/tid/stat
std::string ThreadStat();

/// readlink /proc/self/exe
std::string ExePath();

int OpenedFiles();
int MaxOpenFiles();

struct CpuTime {
  double userSeconds;
  double systemSeconds;

  CpuTime() : userSeconds(0.0), systemSeconds(0.0) {}

  double Total() const { return userSeconds + systemSeconds; }
};
CpuTime CpuTime();

int NumThreads();
std::vector<pid_t> Threads();
}  // namespace rnet::Unix