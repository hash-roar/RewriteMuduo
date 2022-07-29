#include "ProcssInfo.h"

#include <dirent.h>
#include <pwd.h>
#include <unistd.h>

#include <cassert>
#include <string>
#include <string_view>

namespace rnet::Unix {
thread_local int tNumOpenedFiles = 0;
int fdDirFilter(const struct dirent* d) {
  if (::isdigit(d->d_name[0])) {
    ++tNumOpenedFiles;
  }
  return 0;
}

thread_local std::vector<pid_t>* tPids = nullptr;
int taskDirFilter(const struct dirent* d) {
  if (::isdigit(d->d_name[0])) {
    tPids->push_back(atoi(d->d_name));
  }
  return 0;
}

int scanDir(const char* dirpath, int (*filter)(const struct dirent*)) {
  struct dirent** namelist = nullptr;
  int result = ::scandir(dirpath, &namelist, filter, alphasort);
  assert(namelist == nullptr);
  return result;
}

Timestamp GlobalStartTime = Timestamp::now();
// assume those won't change during the life time of a process.
int GlobalClockTicks = static_cast<int>(::sysconf(_SC_CLK_TCK));
int GlobalPageSize = static_cast<int>(::sysconf(_SC_PAGE_SIZE));
}  // namespace rnet::Unix

using namespace rnet;

pid_t Unix::pid() { return ::getpid(); }

std::string Unix::pidString() {
  char buf[32];
  snprintf(buf, sizeof buf, "%d", pid());
  return buf;
}

uid_t Unix::uid() { return ::getuid(); }

std::string Unix::username() {
  struct passwd pwd;
  struct passwd* result = nullptr;
  char buf[8192];
  const char* name = "unknownUser";

  getpwuid_r(uid(), &pwd, buf, sizeof buf, &result);
  if (result) {
    name = pwd.pw_name;
  }
  return name;
}

uid_t Unix::euid() { return ::geteuid(); }

Unix::Timestamp Unix::startTime() { return GlobalStartTime; }

int Unix::clockTicksPerSecond() { return GlobalClockTicks; }

int Unix::pageSize() { return GlobalPageSize; }

bool Unix::isDebugBuild() {
#ifdef NDEBUG
  return false;
#else
  return true;
#endif
}

std::string Unix::hostname() {
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

std::string Unix::procname() { return std::string{procname(procStat())}; }

std::string_view Unix::procname(const std::string& stat) {
  std::string_view name{};
  size_t lp = stat.find('(');
  size_t rp = stat.rfind(')');
  if (lp != std::string::npos && rp != std::string::npos && lp < rp) {
    name =
        std::string_view(stat.data() + lp + 1, static_cast<int>(rp - lp - 1));
  }
  return name;
}

std::string Unix::procStatus() {
  std::string result;
  FileUtil::readFile("/proc/self/status", 65536, &result);
  return result;
}

std::string Unix::procStat() {
  std::string result;
  FileUtil::readFile("/proc/self/stat", 65536, &result);
  return result;
}

std::string Unix::threadStat() {
  char buf[64];
  snprintf(buf, sizeof buf, "/proc/self/task/%d/stat", CurrentThread::tid());
  std::string result;
  FileUtil::readFile(buf, 65536, &result);
  return result;
}

std::string Unix::exePath() {
  std::string result;
  char buf[1024];
  ssize_t n = ::readlink("/proc/self/exe", buf, sizeof buf);
  if (n > 0) {
    result.assign(buf, n);
  }
  return result;
}

int Unix::openedFiles() {
  t_numOpenedFiles = 0;
  scanDir("/proc/self/fd", fdDirFilter);
  return t_numOpenedFiles;
}

int Unix::maxOpenFiles() {
  struct rlimit rl;
  if (::getrlimit(RLIMIT_NOFILE, &rl)) {
    return openedFiles();
  } else {
    return static_cast<int>(rl.rlim_cur);
  }
}

Unix::CpuTime Unix::cpuTime() {
  Unix::CpuTime t;
  struct tms tms;
  if (::times(&tms) >= 0) {
    const double hz = static_cast<double>(clockTicksPerSecond());
    t.userSeconds = static_cast<double>(tms.tms_utime) / hz;
    t.systemSeconds = static_cast<double>(tms.tms_stime) / hz;
  }
  return t;
}

int Unix::numThreads() {
  int result = 0;
  std::string status = procStatus();
  size_t pos = status.find("Threads:");
  if (pos != std::string::npos) {
    result = ::atoi(status.c_str() + pos + 8);
  }
  return result;
}

std::vector<pid_t> Unix::threads() {
  std::vector<pid_t> result;
  t_pids = &result;
  scanDir("/proc/self/task", taskDirFilter);
  t_pids = NULL;
  std::sort(result.begin(), result.end());
  return result;
}