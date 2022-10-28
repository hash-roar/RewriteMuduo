#include "unix/ProcssInfo.h"

#include <dirent.h>
#include <pwd.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>

#include <cassert>
#include <string>
#include <string_view>

#include "file/File.h"
#include "unix/Thread.h"

namespace rnet::Unix {
thread_local int tNumOpenedFiles = 0;
int FdDirFilter(const struct dirent* d) {
  if (::isdigit(d->d_name[0])) {
    ++tNumOpenedFiles;
  }
  return 0;
}

thread_local std::vector<pid_t>* tPids = nullptr;
int TaskDirFilter(const struct dirent* d) {
  if (::isdigit(d->d_name[0])) {
    tPids->push_back(atoi(d->d_name));
  }
  return 0;
}

int ScanDir(const char* dirpath, int (*filter)(const struct dirent*)) {
  struct dirent** namelist = nullptr;
  int result = ::scandir(dirpath, &namelist, filter, alphasort);
  assert(namelist == nullptr);
  return result;
}

Timestamp globalStartTime = Timestamp::Now();
// assume those won't change during the life time of a process.
int globalClockTicks = static_cast<int>(::sysconf(_SC_CLK_TCK));
int globalPageSize = static_cast<int>(::sysconf(_SC_PAGE_SIZE));
}  // namespace rnet::Unix

using namespace rnet;

pid_t Unix::Pid() { return ::getpid(); }

std::string Unix::PidString() {
  char buf[32];
  snprintf(buf, sizeof buf, "%d", Pid());
  return buf;
}

uid_t Unix::Uid() { return ::getuid(); }

std::string Unix::Username() {
  struct passwd pwd;
  struct passwd* result = nullptr;
  char buf[8192];
  const char* name = "unknownUser";

  getpwuid_r(Uid(), &pwd, buf, sizeof buf, &result);
  if (result) {
    name = pwd.pw_name;
  }
  return name;
}

uid_t Unix::Euid() { return ::geteuid(); }

Unix::Timestamp Unix::StartTime() { return globalStartTime; }

int Unix::ClockTicksPerSecond() { return globalClockTicks; }

int Unix::PageSize() { return globalPageSize; }

bool Unix::IsDebugBuild() {
#ifdef NDEBUG
  return false;
#else
  return true;
#endif
}

std::string Unix::Hostname() {
  // HOST_NAME_MAX 64
  // _POSIX_HOST_NAME_MAX 255
  char buf[256];
  if (::gethostname(buf, sizeof buf) == 0) {
    buf[sizeof(buf) - 1] = '\0';
    return buf;
  } else {
    return "unknownhost";
  }
}

std::string Unix::Procname() { return std::string{Procname(ProcStat())}; }

std::string_view Unix::Procname(const std::string& stat) {
  std::string_view name{};
  size_t lp = stat.find('(');
  size_t rp = stat.rfind(')');
  if (lp != std::string::npos && rp != std::string::npos && lp < rp) {
    name =
        std::string_view(stat.data() + lp + 1, static_cast<int>(rp - lp - 1));
  }
  return name;
}

std::string Unix::ProcStatus() {
  std::string result;
  file::ReadFile("/proc/self/status", 65536, &result);
  return result;
}

std::string Unix::ProcStat() {
  std::string result;
  file::ReadFile("/proc/self/stat", 65536, &result);
  return result;
}

std::string Unix::ThreadStat() {
  char buf[64];
  snprintf(buf, sizeof buf, "/proc/self/task/%d/stat", thread::Tid());
  std::string result;
  file::ReadFile(buf, 65536, &result);
  return result;
}

std::string Unix::ExePath() {
  std::string result;
  char buf[1024];
  ssize_t n = ::readlink("/proc/self/exe", buf, sizeof buf);
  if (n > 0) {
    result.assign(buf, n);
  }
  return result;
}

int Unix::OpenedFiles() {
  tNumOpenedFiles = 0;
  ScanDir("/proc/self/fd", FdDirFilter);
  return tNumOpenedFiles;
}

int Unix::MaxOpenFiles() {
  struct rlimit rl;
  if (::getrlimit(RLIMIT_NOFILE, &rl)) {
    return OpenedFiles();
  } else {
    return static_cast<int>(rl.rlim_cur);
  }
}

// Unix::CpuTime Unix::cpuTime() {
//   Unix::CpuTime t;
//   struct tms tms;
//   if (::times(&tms) >= 0) {
//     const double hz = static_cast<double>(clockTicksPerSecond());
//     t.userSeconds = static_cast<double>(tms.tms_utime) / hz;
//     t.systemSeconds = static_cast<double>(tms.tms_stime) / hz;
//   }
//   return t;
// }

int Unix::NumThreads() {
  int result = 0;
  std::string status = ProcStatus();
  size_t pos = status.find("Threads:");
  if (pos != std::string::npos) {
    result = ::atoi(status.c_str() + pos + 8);
  }
  return result;
}

std::vector<pid_t> Unix::Threads() {
  std::vector<pid_t> result;
  tPids = &result;
  ScanDir("/proc/self/task", TaskDirFilter);
  tPids = nullptr;
  std::sort(result.begin(), result.end());
  return result;
}