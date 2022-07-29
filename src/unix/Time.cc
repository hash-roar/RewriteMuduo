#include "Time.h"

#include <sys/time.h>

#include <cassert>
#include <cinttypes>
#include <vector>

using namespace rnet::Unix;
using namespace std;

static_assert(sizeof(Timestamp) == sizeof(int64_t),
              "Timestamp should be same size as int64_t");

namespace rnet::detail {}  // namespace rnet::detail

std::string Timestamp::toString() const {
  char buf[32] = {0};
  int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
  int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
  snprintf(buf, sizeof(buf), "%" PRId64 ".%06" PRId64 "", seconds,
           microseconds);
  return buf;
}

std::string Timestamp::toFormattedString(bool showMicroseconds) const {
  char buf[64] = {0};
  auto seconds =
      static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
  struct tm tm_time;
  gmtime_r(&seconds, &tm_time);

  if (showMicroseconds) {
    int microseconds =
        static_cast<int>(microSecondsSinceEpoch_ % kMicroSecondsPerSecond);
    snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, microseconds);
  } else {
    snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d",
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
  }
  return buf;
}

Timestamp Timestamp::now() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  int64_t seconds = tv.tv_sec;
  return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
}

// struct Transition {
//   time_t gmttime;
//   time_t localtime;
//   int localtimeIdx;

//   Transition(time_t t, time_t l, int localIdx)
//       : gmttime(t), localtime(l), localtimeIdx(localIdx) {}
// };

// struct Comp {
//   bool compareGmt;

//   Comp(bool gmt) : compareGmt(gmt) {}

//   bool operator()(const Transition& lhs, const Transition& rhs) const {
//     if (compareGmt)
//       return lhs.gmttime < rhs.gmttime;
//     else
//       return lhs.localtime < rhs.localtime;
//   }

//   bool equal(const Transition& lhs, const Transition& rhs) const {
//     if (compareGmt)
//       return lhs.gmttime == rhs.gmttime;
//     else
//       return lhs.localtime == rhs.localtime;
//   }
// };

// struct Localtime {
//   time_t gmtOffset;
//   bool isDst;
//   int arrbIdx;

//   Localtime(time_t offset, bool dst, int arrb)
//       : gmtOffset(offset), isDst(dst), arrbIdx(arrb) {}
// };

// inline void fillHMS(unsigned seconds, struct tm* utc) {
//   utc->tm_sec = seconds % 60;
//   unsigned minutes = seconds / 60;
//   utc->tm_min = minutes % 60;
//   utc->tm_hour = minutes / 60;
// }

// struct TimeZone::Data {
//   vector<detail::Transition> transitions;
//   vector<detail::Localtime> localtimes;
//   vector<string> names;
//   string abbreviation;
// };

// class File : noncopyable {
//  public:
//   File(const char* file) : fp_(::fopen(file, "rb")) {}

//   ~File() {
//     if (fp_) {
//       ::fclose(fp_);
//     }
//   }

//   bool valid() const { return fp_; }

//   string readBytes(int n) {
//     char buf[n];
//     ssize_t nr = ::fread(buf, 1, n, fp_);
//     if (nr != n) throw logic_error("no enough data");
//     return string(buf, n);
//   }

//   int32_t readInt32() {
//     int32_t x = 0;
//     ssize_t nr = ::fread(&x, 1, sizeof(int32_t), fp_);
//     if (nr != sizeof(int32_t)) throw logic_error("bad int32_t data");
//     return be32toh(x);
//   }

//   uint8_t readUInt8() {
//     uint8_t x = 0;
//     ssize_t nr = ::fread(&x, 1, sizeof(uint8_t), fp_);
//     if (nr != sizeof(uint8_t)) throw logic_error("bad uint8_t data");
//     return x;
//   }

//  private:
//   FILE* fp_;
// };

// bool readTimeZoneFile(const char* zonefile, struct TimeZone::Data* data) {
//   File f(zonefile);
//   if (f.valid()) {
//     try {
//       string head = f.readBytes(4);
//       if (head != "TZif") throw logic_error("bad head");
//       string version = f.readBytes(1);
//       f.readBytes(15);

//       int32_t isgmtcnt = f.readInt32();
//       int32_t isstdcnt = f.readInt32();
//       int32_t leapcnt = f.readInt32();
//       int32_t timecnt = f.readInt32();
//       int32_t typecnt = f.readInt32();
//       int32_t charcnt = f.readInt32();

//       vector<int32_t> trans;
//       vector<int> localtimes;
//       trans.reserve(timecnt);
//       for (int i = 0; i < timecnt; ++i) {
//         trans.push_back(f.readInt32());
//       }

//       for (int i = 0; i < timecnt; ++i) {
//         uint8_t local = f.readUInt8();
//         localtimes.push_back(local);
//       }

//       for (int i = 0; i < typecnt; ++i) {
//         int32_t gmtoff = f.readInt32();
//         uint8_t isdst = f.readUInt8();
//         uint8_t abbrind = f.readUInt8();

//         data->localtimes.push_back(Localtime(gmtoff, isdst, abbrind));
//       }

//       for (int i = 0; i < timecnt; ++i) {
//         int localIdx = localtimes[i];
//         time_t localtime = trans[i] + data->localtimes[localIdx].gmtOffset;
//         data->transitions.push_back(Transition(trans[i], localtime,
//         localIdx));
//       }

//       data->abbreviation = f.readBytes(charcnt);

//       // leapcnt
//       for (int i = 0; i < leapcnt; ++i) {
//         // int32_t leaptime = f.readInt32();
//         // int32_t cumleap = f.readInt32();
//       }
//       // FIXME
//       (void)isstdcnt;
//       (void)isgmtcnt;
//     } catch (logic_error& e) {
//       fprintf(stderr, "%s\n", e.what());
//     }
//   }
//   return true;
// }

// const Localtime* findLocaltime(const TimeZone::Data& data, Transition sentry,
//                                Comp comp) {
//   const Localtime* local = NULL;

//   if (data.transitions.empty() || comp(sentry, data.transitions.front())) {
//     // FIXME: should be first non dst time zone
//     local = &data.localtimes.front();
//   } else {
//     vector<Transition>::const_iterator transI = lower_bound(
//         data.transitions.begin(), data.transitions.end(), sentry, comp);
//     if (transI != data.transitions.end()) {
//       if (!comp.equal(sentry, *transI)) {
//         assert(transI != data.transitions.begin());
//         --transI;
//       }
//       local = &data.localtimes[transI->localtimeIdx];
//     } else {
//       // FIXME: use TZ-env
//       local = &data.localtimes[data.transitions.back().localtimeIdx];
//     }
//   }

//   return local;
// }

// TimeZone::TimeZone(const char* zonefile) : data_(new TimeZone::Data) {
//   if (!detail::readTimeZoneFile(zonefile, data_.get())) {
//     data_.reset();
//   }
// }

// TimeZone::TimeZone(int eastOfUtc, const char* name)
//     : data_(new TimeZone::Data) {
//   data_->localtimes.push_back(detail::Localtime(eastOfUtc, false, 0));
//   data_->abbreviation = name;
// }

// struct tm TimeZone::toLocalTime(time_t seconds) const {
//   struct tm localTime;
//   memZero(&localTime, sizeof(localTime));
//   assert(data_ != NULL);
//   const Data& data(*data_);

//   detail::Transition sentry(seconds, 0, 0);
//   const detail::Localtime* local =
//       findLocaltime(data, sentry, detail::Comp(true));

//   if (local) {
//     time_t localSeconds = seconds + local->gmtOffset;
//     ::gmtime_r(&localSeconds, &localTime);  // FIXME: fromUtcTime
//     localTime.tm_isdst = local->isDst;
//     localTime.tm_gmtoff = local->gmtOffset;
//     localTime.tm_zone = &data.abbreviation[local->arrbIdx];
//   }

//   return localTime;
// }

// time_t TimeZone::fromLocalTime(const struct tm& localTm) const {
//   assert(data_ != NULL);
//   const Data& data(*data_);

//   struct tm tmp = localTm;
//   time_t seconds = ::timegm(&tmp);  // FIXME: toUtcTime
//   detail::Transition sentry(0, seconds, 0);
//   const detail::Localtime* local =
//       findLocaltime(data, sentry, detail::Comp(false));
//   if (localTm.tm_isdst) {
//     struct tm tryTm = toLocalTime(seconds - local->gmtOffset);
//     if (!tryTm.tm_isdst && tryTm.tm_hour == localTm.tm_hour &&
//         tryTm.tm_min == localTm.tm_min) {
//       // FIXME: HACK
//       seconds -= 3600;
//     }
//   }
//   return seconds - local->gmtOffset;
// }

// struct tm TimeZone::toUtcTime(time_t secondsSinceEpoch, bool yday) {
//   struct tm utc;
//   memZero(&utc, sizeof(utc));
//   utc.tm_zone = "GMT";
//   int seconds = static_cast<int>(secondsSinceEpoch % kSecondsPerDay);
//   int days = static_cast<int>(secondsSinceEpoch / kSecondsPerDay);
//   if (seconds < 0) {
//     seconds += kSecondsPerDay;
//     --days;
//   }
//   detail::fillHMS(seconds, &utc);
//   Date date(days + Date::kJulianDayOf1970_01_01);
//   Date::YearMonthDay ymd = date.yearMonthDay();
//   utc.tm_year = ymd.year - 1900;
//   utc.tm_mon = ymd.month - 1;
//   utc.tm_mday = ymd.day;
//   utc.tm_wday = date.weekDay();

//   if (yday) {
//     Date startOfYear(ymd.year, 1, 1);
//     utc.tm_yday = date.julianDayNumber() - startOfYear.julianDayNumber();
//   }
//   return utc;
// }

// time_t TimeZone::fromUtcTime(const struct tm& utc) {
//   return fromUtcTime(utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday,
//                      utc.tm_hour, utc.tm_min, utc.tm_sec);
// }

// time_t TimeZone::fromUtcTime(int year, int month, int day, int hour, int
// minute,
//                              int seconds) {
//   Date date(year, month, day);
//   int secondsInDay = hour * 3600 + minute * 60 + seconds;
//   time_t days = date.julianDayNumber() - Date::kJulianDayOf1970_01_01;
//   return days * kSecondsPerDay + secondsInDay;
// }