/**
   \file CommonTradeUtilsCode/date_time.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#include <time.h>

#include "dvccode/CommonTradeUtils/date_time.hpp"

// #define BOOST_ZONESPEC_FILE "/apps/boost/root/libs/date_time/data/date_time_zonespec.csv" // removed dependancy on
// file in absolute location, put in BASESYSINFODIR

namespace HFSAT {
namespace DateTime {

// commented since now in boost/date_time/posix_time/conversion.hpp
// /// Simply converts from posix time to time_t.
// /// Probably assumes ptime is utc time ?
// inline time_t to_time_t ( const boost::posix_time::ptime & t_utc_ptime_ )
// {
//   boost::posix_time::ptime _epoch_ptime_(boost::gregorian::date(1970,1,1));
//   boost::posix_time::time_duration::sec_type _sec_type_ = ( t_utc_ptime_ - _epoch_ptime_ ).total_seconds();
//   return time_t ( _sec_type_ );
// }

/// meant to set other variables of _temp_struct_tm_
/// currently implemented by calling localtime as well
inline void SlowInitStructTm(tm& this_tm) {
  time_t tvsec_ = time(NULL);
  localtime_r(&tvsec_, &this_tm);
}

/// Finds time_t using the info in tm, including dst info
inline time_t time_t_from_struct_tm(tm& this_tm) { return mktime(&this_tm); }

/// ignoring all other information, purely based on tm_year, tm_mon, tm_mday returns iso_date
inline int Get_local_YYYYMMDD_from_struct_tm(const tm& this_tm) {
  return ((((1900 + this_tm.tm_year) * 100 + (1 + this_tm.tm_mon)) * 100) + this_tm.tm_mday);
}

inline boost::gregorian::date GetBoostDateFromIsoDate(const int& t_yyyymmdd_) {
  boost::gregorian::date d1((int)(t_yyyymmdd_ / 10000), (int)((t_yyyymmdd_ / 100) % 100), (int)(t_yyyymmdd_ % 100));
  return d1;
}

inline int YYYYMMDD_from_date(const boost::gregorian::date& d1) {
  boost::gregorian::date::ymd_type ymd = d1.year_month_day();
  return (((ymd.year * 100 + ymd.month) * 100) + ymd.day);
}

time_t GetTimeMidnightLocal(const int& t_yyyymmdd_) {
  boost::gregorian::date d1 = GetBoostDateFromIsoDate(t_yyyymmdd_);
  std::tm this_tm = to_tm(d1);
  return (mktime(&this_tm));
}

time_t GetTimeMidnightUTC(const int& t_yyyymmdd_) {
  boost::gregorian::date d1 = GetBoostDateFromIsoDate(t_yyyymmdd_);
  boost::posix_time::ptime t_ptime_midnight_(d1, boost::posix_time::time_duration(0, 0, 0));
  return to_time_t(t_ptime_midnight_);
}

inline time_t GetTimeMidnight_TZ(const int& t_yyyymmdd_, TZ tz) {
  boost::local_time::tz_database* tz_db = TZ_DB_Singleton::get_tz_db(t_yyyymmdd_);

  if (tz_db == NULL) return GetTimeMidnightUTC(t_yyyymmdd_);

  boost::local_time::time_zone_ptr tz_ptr;
  switch (tz) {
    case kEST:
      tz_ptr = tz_db->time_zone_from_region("America/New_York");
      break;
    case kCST:
      tz_ptr = tz_db->time_zone_from_region("America/Chicago");
      break;
    case kCET:
      tz_ptr = tz_db->time_zone_from_region("Europe/Berlin");
      break;
    case kBRT:
      tz_ptr = tz_db->time_zone_from_region("America/Sao_Paulo");
      break;
    case kHKT:
      tz_ptr = tz_db->time_zone_from_region("Asia/Hong_Kong");
      break;
    case kKST:
      tz_ptr = tz_db->time_zone_from_region("Asia/Seoul");
      break;
    case kJST:
      tz_ptr = tz_db->time_zone_from_region("Asia/Tokyo");
      break;
    case kBST:
      tz_ptr = tz_db->time_zone_from_region("Europe/London");
      break;
    case kIST:
      tz_ptr = tz_db->time_zone_from_region("Asia/Calcutta");
      break;
    case kMSK:
      tz_ptr = tz_db->time_zone_from_region("Europe/Moscow");
      break;
    case kLON:
      tz_ptr = tz_db->time_zone_from_region("Europe/London");
      break;
    case kPAR:
      tz_ptr = tz_db->time_zone_from_region("Europe/Paris");
      break;
    case kAMS:
      tz_ptr = tz_db->time_zone_from_region("Europe/Amsterdam");
      break;
    case kAEST:
      tz_ptr = tz_db->time_zone_from_region("Australia/Sydney");
      break;
    case kSGT:
      tz_ptr = tz_db->time_zone_from_region("Asia/Singapore");
      break;
    default:
      return GetTimeMidnightUTC(t_yyyymmdd_);
  }

  boost::local_time::local_date_time tz_midnight(GetBoostDateFromIsoDate(t_yyyymmdd_), boost::posix_time::hours(0),
                                                 tz_ptr, boost::local_time::local_date_time::EXCEPTION_ON_ERROR);

  boost::posix_time::ptime tz_midnight_utc_ptime = tz_midnight.utc_time();
  return to_time_t(tz_midnight_utc_ptime);
}

time_t GetTimeMidnightEST(const int& t_yyyymmdd_) { return GetTimeMidnight_TZ(t_yyyymmdd_, kEST); }
time_t GetTimeMidnightCST(const int& t_yyyymmdd_) { return GetTimeMidnight_TZ(t_yyyymmdd_, kCST); }
time_t GetTimeMidnightCET(const int& t_yyyymmdd_) { return GetTimeMidnight_TZ(t_yyyymmdd_, kCET); }
time_t GetTimeMidnightBRT(const int& t_yyyymmdd_) { return GetTimeMidnight_TZ(t_yyyymmdd_, kBRT); }
time_t GetTimeMidnightBST(const int& t_yyyymmdd_) { return GetTimeMidnight_TZ(t_yyyymmdd_, kBST); }
time_t GetTimeMidnightIST(const int& t_yyyymmdd_) { return GetTimeMidnight_TZ(t_yyyymmdd_, kIST); }
time_t GetTimeMidnightKST(const int& t_yyyymmdd_) { return GetTimeMidnight_TZ(t_yyyymmdd_, kKST); }
time_t GetTimeMidnightJST(const int& t_yyyymmdd_) { return GetTimeMidnight_TZ(t_yyyymmdd_, kJST); }
time_t GetTimeMidnightHKT(const int& t_yyyymmdd_) { return GetTimeMidnight_TZ(t_yyyymmdd_, kHKT); }
time_t GetTimeMidnightMSK(const int& t_yyyymmdd_) { return GetTimeMidnight_TZ(t_yyyymmdd_, kMSK); }
time_t GetTimeMidnightLON(const int& t_yyyymmdd_) { return GetTimeMidnight_TZ(t_yyyymmdd_, kLON); }
time_t GetTimeMidnightPAR(const int& t_yyyymmdd_) { return GetTimeMidnight_TZ(t_yyyymmdd_, kPAR); }
time_t GetTimeMidnightAMS(const int& t_yyyymmdd_) { return GetTimeMidnight_TZ(t_yyyymmdd_, kAMS); }
time_t GetTimeMidnightAEST(const int& t_yyyymmdd_) { return GetTimeMidnight_TZ(t_yyyymmdd_, kAEST); }
time_t GetTimeMidnightSGT(const int& t_yyyymmdd_) { return GetTimeMidnight_TZ(t_yyyymmdd_, kSGT); }
time_t GetTimeUTC(const int& t_yyyymmdd_, const int t_hhmm_) {
  time_t t1 = GetTimeMidnightUTC(t_yyyymmdd_);
  t1 += (GetMinutesFromHHMM(t_hhmm_) * 60);
  return t1;
}
int Get_local_YYYYMMDD_from_ttime(const ttime_t& t_ttime_t_) {
  return Get_local_YYYYMMDD_from_time_t(t_ttime_t_.tv_sec);
}

inline void Get_local_struct_tm_from_time_t(const time_t& t_tvsec_, tm& this_tm) { localtime_r(&t_tvsec_, &this_tm); }

int Get_local_YYYYMMDD_from_time_t(const time_t& t_time_t_) {
  tm this_tm;
  SlowInitStructTm(this_tm);
  Get_local_struct_tm_from_time_t(t_time_t_, this_tm);
  return Get_local_YYYYMMDD_from_struct_tm(this_tm);
}

int Get_UTC_YYYYMMDD_from_ttime(const ttime_t& t_tv) {
  boost::posix_time::ptime t_ptime_ = boost::posix_time::from_time_t(t_tv.tv_sec);
  boost::gregorian::date t_date_ = t_ptime_.date();
  return YYYYMMDD_from_date(t_date_);
}

int CalcPrevDay(const int& current_yyyymmdd_) {
  boost::gregorian::date d1(current_yyyymmdd_ / 10000, ((current_yyyymmdd_ / 100) % 100), (current_yyyymmdd_ % 100));
  d1 = d1 - boost::gregorian::date_duration(1);
  return YYYYMMDD_from_date(d1);
}

int CalcNextDay(const int& current_yyyymmdd_) {
  boost::gregorian::date d1(current_yyyymmdd_ / 10000, ((current_yyyymmdd_ / 100) % 100), (current_yyyymmdd_ % 100));
  d1 = d1 + boost::gregorian::date_duration(1);
  return YYYYMMDD_from_date(d1);
}

int CalcNextWeekDay(const int& current_yyyymmdd_) {
  boost::gregorian::date d1(current_yyyymmdd_ / 10000, ((current_yyyymmdd_ / 100) % 100), (current_yyyymmdd_ % 100));
  d1 = d1 + boost::gregorian::date_duration(1);
  while (d1.day_of_week() == boost::gregorian::Saturday || d1.day_of_week() == boost::gregorian::Sunday) {
    d1 = d1 + boost::gregorian::date_duration(1);
  }
  return YYYYMMDD_from_date(d1);
}

bool IsWeekDay(const int& current_yyyymmdd_) {
  boost::gregorian::date d1(current_yyyymmdd_ / 10000, ((current_yyyymmdd_ / 100) % 100), (current_yyyymmdd_ % 100));
  if (d1.day_of_week() == boost::gregorian::Saturday || d1.day_of_week() == boost::gregorian::Sunday) {
    return false;
  }
  return true;
}

int GetWeekDay(const int& current_yyyymmdd_) {
  boost::gregorian::date d1(current_yyyymmdd_ / 10000, ((current_yyyymmdd_ / 100) % 100), (current_yyyymmdd_ % 100));
  return d1.day_of_week(); // 0: Sunday 
}

int CalcPrevDay(const int t_times_, const int& r_current_yyyymmdd_) {
  int t_date_ = r_current_yyyymmdd_;
  for (int i = 0; i < t_times_; i++) {
    t_date_ = CalcPrevDay(t_date_);
  }
  return t_date_;
}

int CalcNextDay(const int t_times_, const int& r_current_yyyymmdd_) {
  int t_date_ = r_current_yyyymmdd_;
  for (int i = 0; i < t_times_; i++) {
    t_date_ = CalcNextDay(t_date_);
  }
  return t_date_;
}

int CalcPrevWeekDay(const int& current_yyyymmdd_) {
  boost::gregorian::date d1(current_yyyymmdd_ / 10000, ((current_yyyymmdd_ / 100) % 100), (current_yyyymmdd_ % 100));

  d1 = d1 - boost::gregorian::date_duration(1);
  while (d1.day_of_week() == boost::gregorian::Saturday || d1.day_of_week() == boost::gregorian::Sunday) {
    d1 = d1 - boost::gregorian::date_duration(1);
  }
  return YYYYMMDD_from_date(d1);
}

int CalcPrevWeekDay(const int t_times_, const int& current_yyyymmdd_) {
  boost::gregorian::date d1(current_yyyymmdd_ / 10000, ((current_yyyymmdd_ / 100) % 100), (current_yyyymmdd_ % 100));

  int times_ = t_times_;
  while (times_--) {
    d1 = d1 - boost::gregorian::date_duration(1);
    while (d1.day_of_week() == boost::gregorian::Saturday || d1.day_of_week() == boost::gregorian::Sunday) {
      d1 = d1 - boost::gregorian::date_duration(1);
    }
  }
  return YYYYMMDD_from_date(d1);
}

int LastWorkingDay(const int& current_yyyymmdd_) {
  return current_yyyymmdd_;  // TODO .. last valid date that was a working day
}

inline void ZeroOutIntraDayTimes(tm& this_tm) {
  this_tm.tm_sec = 0;  // zero out intraday times
  this_tm.tm_min = 0;
  this_tm.tm_hour = 0;
}

time_t RewindToMidnightLocal(const ttime_t& t_ttime_t_) {
  time_t t_time_t = t_ttime_t_.tv_sec;
  return RewindToMidnightLocal(t_time_t);
}

time_t RewindToMidnightLocal(const time_t& t_time_t_) {
  int t_yyyymmdd_ = Get_local_YYYYMMDD_from_time_t(t_time_t_);
  return GetTimeMidnightLocal(t_yyyymmdd_);

  // // faulty solution since it does not handle dst
  // return ( (5*3600 ) + ((( t_time_t_ - 5*3600 )/86400 ) * 86400 ) );

  // // one solution, problem it returns time_t of midnight::UTC
  // boost::posix_time::ptime t_ptime_ = boost::posix_time::from_time_t ( t_time_t_ );
  // boost::gregorian::date _date_ = t_ptime_.date();
  // boost::posix_time::ptime t_ptime_midnight_ ( _date_, boost::posix_time::time_duration ( 0, 0, 0 ) );
  // return to_time_t ( t_ptime_midnight_ );
}

time_t AddOneDay(const time_t& t_tvsec_) {
  return (t_tvsec_ + 86400);  // TODO ... should be much more complex ... with DST support
}

/// Given a date it returns ttime_t corresponding to midnight of the next day
ttime_t AddOneDayDate(const int& current_yyyymmdd_) {
  int next_day_yyyymmdd_ = CalcNextDay(current_yyyymmdd_);
  time_t time_t_of_next_day_midnight = GetTimeMidnightLocal(next_day_yyyymmdd_);
  return ttime_t(time_t_of_next_day_midnight, 0);
}

/**
 *
 * @param first_yyyymmdd
 * @param second_yyyymmdd
 * @return Number of days between two dates
 */
int GetDiffDates(int first_yyyymmdd, int second_yyyymmdd) {
  boost::gregorian::date d1 = GetBoostDateFromIsoDate(first_yyyymmdd);
  boost::gregorian::date d2 = GetBoostDateFromIsoDate(second_yyyymmdd);
  return (d2 - d1).days();
}

int GetCurrentIsoDateLocal() {
  time_t tvsec_ = time(NULL);
  return Get_local_YYYYMMDD_from_time_t(tvsec_);
}

std::string GetCurrentIsoDateLocalAsString() {
  std::stringstream ss;
  ss << GetCurrentIsoDateLocal();
  return ss.str();
}

int GetCurrentIsoDateUTC() { return YYYYMMDD_from_date(boost::gregorian::day_clock::universal_day()); }

YYYYMMDDStr_t BreakDateYYYYMMDD(const int& t_yyyymmdd_) {
  YYYYMMDDStr_t retval_;
  snprintf(retval_.yyyy_str_, 5, "%04d", (int)(t_yyyymmdd_ / 10000));
  snprintf(retval_.mm_str_, 3, "%02d", (int)((t_yyyymmdd_ / 100) % 100));
  snprintf(retval_.dd_str_, 3, "%02d", (int)(t_yyyymmdd_ % 100));
  return retval_;
}

boost::local_time::local_date_time GetNYLocalTimeFromUTCTime(const time_t& t_utc_time_t) {
  boost::posix_time::ptime pt_ = boost::posix_time::from_time_t(t_utc_time_t);

  boost::local_time::tz_database* tz_db = TZ_DB_Singleton::get_tz_db();
  boost::local_time::time_zone_ptr _tz = tz_db->time_zone_from_region("America/New_York");

  boost::local_time::local_date_time est_local_time_(pt_, _tz);
  return est_local_time_;
}

boost::local_time::local_date_time GetIndLocalTimeFromUTCTime(const time_t& t_utc_time_t) {
  boost::posix_time::ptime pt_ = boost::posix_time::from_time_t(t_utc_time_t);

  boost::local_time::tz_database* tz_db = TZ_DB_Singleton::get_tz_db();
  boost::local_time::time_zone_ptr _tz = tz_db->time_zone_from_region("Asia/Calcutta");

  boost::local_time::local_date_time ist_local_time_(pt_, _tz);
  return ist_local_time_;
}

boost::local_time::local_date_time GetSPRLocalTimeFromUTCTime(const time_t& t_utc_time_t) {
  boost::posix_time::ptime pt_ = boost::posix_time::from_time_t(t_utc_time_t);

  boost::local_time::tz_database* tz_db = TZ_DB_Singleton::get_tz_db();
  boost::local_time::time_zone_ptr _tz = tz_db->time_zone_from_region("Asia/Singapore");

  boost::local_time::local_date_time ist_local_time_(pt_, _tz);
  return ist_local_time_;
}

int GetHHMMSSTime(const char* time_string_) {
  std::string this_time_str_ = std::string(time_string_);
  if (this_time_str_.length() >= 5) {
    return atoi(time_string_);
  } else {
    if (this_time_str_.length() >= 0) {
      return atoi(time_string_) * 100;
    } else {
      return 0;
    }
  }
}

unsigned int GetUTCHHMMFromTime(const time_t& r_time_t) {
  int utc_tradingdate_ = Get_UTC_YYYYMMDD_from_ttime(ttime_t(r_time_t, 0));
  time_t midnight_utc_time_t = GetTimeMidnightUTC(utc_tradingdate_);
  unsigned int minutes_ = (r_time_t - midnight_utc_time_t) / 60;
  return (minutes_ % 60) + (((minutes_ / 60) % 24) * 100);
}

unsigned int GetUTCHHMMSSFromTime(const time_t& r_time_t) {
  int utc_tradingdate_ = Get_UTC_YYYYMMDD_from_ttime(ttime_t(r_time_t, 0));
  time_t midnight_utc_time_t = GetTimeMidnightUTC(utc_tradingdate_);
  unsigned int seconds_ = (r_time_t - midnight_utc_time_t);
  unsigned int minutes_ = seconds_ / 60;
  return (seconds_ % 60) + (minutes_ % 60) * 100 + (((minutes_ / 60) % 24) * 10000);
}

time_t GetTimeFromTZHHMM(const int& r_yyyymmdd_, unsigned int hhmm, const char* tzstr_) {
  unsigned int add_seconds_ = GetSecondsFromHHMM(hhmm);
  if (strncmp(tzstr_, "EST", 3) == 0) {
    return GetTimeMidnightEST(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "CST", 3) == 0) {
    return GetTimeMidnightCST(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "CET", 3) == 0) {
    return GetTimeMidnightCET(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "BRT", 3) == 0) {
    return GetTimeMidnightBRT(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "BST", 3) == 0) {
    return GetTimeMidnightBST(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "HKT", 3) == 0) {
    return GetTimeMidnightHKT(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "IST", 3) == 0) {
    return GetTimeMidnightIST(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "KST", 3) == 0) {
    return GetTimeMidnightKST(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "JST", 3) == 0) {
    return GetTimeMidnightJST(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "MSK", 3) == 0) {
    return GetTimeMidnightMSK(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "UTC", 3) == 0) {
    return GetTimeMidnightUTC(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "GMT", 3) == 0) {
    return GetTimeMidnightUTC(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "LON", 3) == 0) {
    return GetTimeMidnightLON(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "PAR", 3) == 0) {
    return GetTimeMidnightPAR(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "AMS", 3) == 0) {
    return GetTimeMidnightAMS(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "AST", 3) == 0) {
    return GetTimeMidnightAEST(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "SGT", 3) == 0) {
    return GetTimeMidnightSGT(r_yyyymmdd_) + add_seconds_;
  } else {
    return GetTimeMidnightUTC(r_yyyymmdd_) + add_seconds_;
  }
}

time_t GetTimeFromTZHHMMStr(const int& _yyyymmdd_, const char* _tz_hhmm_str_) {
  if (strncmp(_tz_hhmm_str_, "PREV_", 5) == 0) {
    return GetTimeFromTZHHMM(CalcPrevDay(_yyyymmdd_), atoi(_tz_hhmm_str_ + 9), _tz_hhmm_str_ + 5);
  } else {
    return GetTimeFromTZHHMM(_yyyymmdd_, atoi(_tz_hhmm_str_ + 4), _tz_hhmm_str_);
  }
}

time_t GetTimeFromTZHHMMSS(const int& r_yyyymmdd_, unsigned int _hhmmss_, const char* tzstr_) {
  unsigned int add_seconds_ = GetSecondsFromHHMMSS(_hhmmss_);
  if (strncmp(tzstr_, "EST", 3) == 0) {
    return GetTimeMidnightEST(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "CST", 3) == 0) {
    return GetTimeMidnightCST(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "CET", 3) == 0) {
    return GetTimeMidnightCET(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "BRT", 3) == 0) {
    return GetTimeMidnightBRT(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "BST", 3) == 0) {
    return GetTimeMidnightBST(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "HKT", 3) == 0) {
    return GetTimeMidnightHKT(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "IST", 3) == 0) {
    return GetTimeMidnightIST(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "KST", 3) == 0) {
    return GetTimeMidnightKST(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "JST", 3) == 0) {
    return GetTimeMidnightJST(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "MSK", 3) == 0) {
    return GetTimeMidnightMSK(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "UTC", 3) == 0) {
    return GetTimeMidnightUTC(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "GMT", 3) == 0) {
    return GetTimeMidnightUTC(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "LON", 3) == 0) {
    return GetTimeMidnightLON(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "PAR", 3) == 0) {
    return GetTimeMidnightPAR(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "AMS", 3) == 0) {
    return GetTimeMidnightAMS(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "AST", 3) == 0) {
    return GetTimeMidnightAEST(r_yyyymmdd_) + add_seconds_;
  } else if (strncmp(tzstr_, "SGT", 3) == 0) {
    return GetTimeMidnightSGT(r_yyyymmdd_) + add_seconds_;
  } else {
    return GetTimeMidnightUTC(r_yyyymmdd_) + add_seconds_;
  }
}

int GetIsoDateFromString(std::string _date_string_) {
  int date_ = std::max(20110101, atoi(_date_string_.c_str()));
  if (_date_string_.compare("TODAY") == 0) {
    return GetCurrentIsoDateLocal();
  } else if (_date_string_.substr(0, 6).compare("TODAY-") == 0) {
    int num_prev_ = atoi(_date_string_.substr(6).c_str());
    date_ = CalcPrevWeekDay(num_prev_, GetCurrentIsoDateLocal());
  }
  return date_;
}

unsigned int GetUTCHHMMFromTZHHMM(const int& r_yyyymmdd_, unsigned int hhmm, const char* tzstr_) {
  return GetUTCHHMMFromTime(GetTimeFromTZHHMM(r_yyyymmdd_, hhmm, tzstr_));
}

int GetUTCYYMMDDFromTZHHMMSS(const int& r_yyyymmdd_, unsigned int hhmmss, const char* tzstr_) {
  if (hhmmss < 10000) {
    hhmmss *= 100;
  }  // so that this works for HHMM as well as HHMMSS
  return Get_UTC_YYYYMMDD_from_ttime(ttime_t(GetTimeFromTZHHMMSS(r_yyyymmdd_, hhmmss, tzstr_), 0));
}

int GetUTCYYMMDDFromTZHHMM(const int& r_yyyymmdd_, unsigned int hhmm, const char* tzstr_) {
  return Get_UTC_YYYYMMDD_from_ttime(ttime_t(GetTimeFromTZHHMM(r_yyyymmdd_, hhmm, tzstr_), 0));
}

unsigned int GetUTCHHMMSSFromTZHHMMSS(const int& r_yyyymmdd_, unsigned int hhmmss, const char* tzstr_) {
  return GetUTCHHMMSSFromTime(GetTimeFromTZHHMMSS(r_yyyymmdd_, hhmmss, tzstr_));
}

int GetUTCSecondsFromMidnightFromTZHHMM(const int& r_yyyymmdd_, unsigned int hhmm, const char* tzstr_) {
  return (GetTimeFromTZHHMM(r_yyyymmdd_, hhmm, tzstr_) - GetTimeMidnightUTC(r_yyyymmdd_));
}

int GetUTCMsecsFromMidnightFromTZHHMM(const int& r_yyyymmdd_, unsigned int hhmm, const char* tzstr_) {
  return (1000 * GetUTCSecondsFromMidnightFromTZHHMM(r_yyyymmdd_, hhmm, tzstr_));
}
}

int DaysBetweenDates(int YYYYMMDD_start, int YYYYMMDD_end) {
  if (YYYYMMDD_start > YYYYMMDD_end) {
    return 0;
  }

  boost::gregorian::date d1((int)(YYYYMMDD_start / 10000), (int)((YYYYMMDD_start / 100) % 100),
                            (int)(YYYYMMDD_start % 100));
  boost::gregorian::date d2((int)(YYYYMMDD_end / 10000), (int)((YYYYMMDD_end / 100) % 100), (int)(YYYYMMDD_end % 100));
  return (d2 - d1).days();
}
}
