/**
    \file dvccode/CommonTradeUtils/date_time.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_COMMONTRADEUTILS_DATE_TIME_H
#define BASE_COMMONTRADEUTILS_DATE_TIME_H

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/local_time/local_time.hpp>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/defines.hpp"

namespace HFSAT {

typedef struct {
  char yyyy_str_[5];
  char mm_str_[3];
  char dd_str_[3];
} YYYYMMDDStr_t;

/*! \brief Namespace with static utilities for extracting and operating on dates ( YYYYMMDD )
 *
 * TODO Lots of issues could come with using the functions of this class across dates where DST changes.
 * TODO Holiday Caledar support, specially for different markets
 */
namespace DateTime {

#define BOOST_ZONESPEC_FILE_PREFIX "/home/pengine/prod/live_configs/date_time_zonespec"

enum TZ { kUTC, kEST, kCST, kCET, kBRT, kBST, kIST, kJST, kHKT, kKST, kMSK, kLON, kPAR, kAMS, kAEST, kSGT };

class TZ_DB_Singleton {
 private:
  boost::local_time::tz_database* tz_db;
  TZ_DB_Singleton(int _yyyymmdd_) {
    tz_db = new boost::local_time::tz_database();
    std::string default_filename_ = std::string(BOOST_ZONESPEC_FILE_PREFIX) + ".csv";
    std::string file_to_use_ = default_filename_;
    if (_yyyymmdd_ != -1) {
      std::string yyyy_filename_ =
          std::string(BOOST_ZONESPEC_FILE_PREFIX) + "_" + std::to_string(_yyyymmdd_ / 10000) + ".csv";
      if (HFSAT::FileUtils::exists(yyyy_filename_)) {
        file_to_use_ = yyyy_filename_;
      }
    }
    try {
      tz_db->load_from_file(file_to_use_.c_str());
    } catch (boost::local_time::data_not_accessible& e) {
      std::cerr << "Error with time zone data file: " << e.what() << std::endl;
      tz_db = NULL;
    } catch (boost::local_time::bad_field_count& e) {
      std::cerr << "Error with time zone data file: " << e.what() << std::endl;
      tz_db = NULL;
    }
    // std::cout<<"intialized tz_db\n";
  }

 public:
  static boost::local_time::tz_database* get_tz_db(int _yyyymmdd_ = -1) {
    static TZ_DB_Singleton* instance = NULL;
    if (instance == NULL) instance = new TZ_DB_Singleton(_yyyymmdd_);
    return instance->tz_db;
  }
};

time_t GetTimeMidnightAEST(const int& t_yyyymmdd_);
time_t GetTimeMidnightAMS(const int& t_yyyymmdd_);
time_t GetTimeMidnightBRT(const int& t_yyyymmdd_);
time_t GetTimeMidnightBST(const int& t_yyyymmdd_);
time_t GetTimeMidnightCET(const int& t_yyyymmdd_);
time_t GetTimeMidnightCST(const int& t_yyyymmdd_);
time_t GetTimeMidnightEST(const int& t_yyyymmdd_);
time_t GetTimeMidnightHKT(const int& t_yyyymmdd_);
time_t GetTimeMidnightIST(const int& t_yyyymmdd_);
time_t GetTimeMidnightJST(const int& t_yyyymmdd_);
time_t GetTimeMidnightKST(const int& t_yyyymmdd_);
time_t GetTimeMidnightLocal(const int& t_yyyymmdd_);
time_t GetTimeMidnightLON(const int& t_yyyymmdd_);
time_t GetTimeMidnightMSK(const int& t_yyyymmdd_);
time_t GetTimeMidnightPAR(const int& t_yyyymmdd_);
time_t GetTimeMidnightSGT(const int& t_yyyymmdd_);
time_t GetTimeMidnightUTC(const int& t_yyyymmdd_);

time_t GetTimeUTC(const int& t_yyyymmdd_, const int t_hhmm_);

// somehow C/C++ wants every function defined in a header file to be inline
inline time_t GetTimeUTCFromMFMUTC(const int& t_yyyymmdd_, const unsigned int r_mfm_) {
  return GetTimeMidnightUTC(t_yyyymmdd_) + (r_mfm_ / 1000);
}

int Get_local_YYYYMMDD_from_ttime(const ttime_t& t_ttime_t_);
int Get_local_YYYYMMDD_from_time_t(const time_t& t_time_t_);

/// Returns the iso date of the UTC time corresponding to given ttime_t t_tv,
/// or unix time t_tv.tv_sec
int Get_UTC_YYYYMMDD_from_ttime(const ttime_t& t_tv);

/// returns YYYYMMDD corresponding to the last valid date
int CalcPrevDay(const int& r_current_yyyymmdd_);

/// returns YYYYMMDD corresponding to the next valid date
int CalcNextDay(const int& r_current_yyyymmdd_);
int CalcNextWeekDay(const int& r_current_yyyymmdd_);

/// Calls CalcPrevDay t_times_
int CalcPrevDay(const int t_times_, const int& r_current_yyyymmdd_);

/// Calls CalcNextDay t_times_
int CalcNextDay(const int t_times_, const int& r_current_yyyymmdd_);

/// returns YYYYMMDD corresponding to the last valid business (weekday) date
int CalcPrevWeekDay(const int& r_current_yyyymmdd_);
int CalcNextWeekDay(const int& r_current_yyyymmdd_);

bool IsWeekDay(const int& r_current_yyyymmdd_);
int GetWeekDay(const int& r_current_yyyymmdd_);

int CalcPrevWeekDay(const int t_times_, const int& current_yyyymmdd_);

int LastWorkingDay(const int& r_current_yyyymmdd_);

time_t RewindToMidnightLocal(const ttime_t& t_ttime_t_);
time_t RewindToMidnightLocal(const time_t& t_time_t_);

time_t AddOneDay(const time_t& _tvsec_);  ///< just add one day to the time_t (seconds since epoch) input

ttime_t AddOneDayDate(const int& r_current_yyyymmdd_);  ///< returns ttime_t of midnight of next day

int GetDiffDates(int first_yyyymmdd, int second_yyyymmdd);

int GetCurrentIsoDateUTC();
std::string GetCurrentIsoDateLocalAsString();
int GetCurrentIsoDateLocal();

YYYYMMDDStr_t BreakDateYYYYMMDD(const int& r_yyyymmdd_);

boost::local_time::local_date_time GetNYLocalTimeFromUTCTime(const time_t& t_utc_time_t);

boost::local_time::local_date_time GetIndLocalTimeFromUTCTime(const time_t& t_utc_time_t);

// Local Singapore Time
boost::local_time::local_date_time GetSPRLocalTimeFromUTCTime(const time_t& t_utc_time_t);

int GetHHMMSSTime(const char* time_string_);
int GetIsoDateFromString(std::string _date_string_);
/// returns the hours and minutes corresponding to a time_t
unsigned int GetUTCHHMMFromTime(const time_t& r_time_t);
unsigned int GetUTCHHMMSSFromTime(const time_t& r_time_t);

/// returns a time_t corresponding to a date, hhmm, and timezone
time_t GetTimeFromTZHHMM(const int& r_yyyymmdd_, unsigned int hhmm, const char* tzstr_);
time_t GetTimeFromTZHHMMSS(const int& r_yyyymmdd_, unsigned int hhmmss, const char* tzstr_);
time_t GetTimeFromTZHHMMStr(const int& _yyyymmdd_, const char* _tz_hhmm_str_);

/// returns the hhmm in UTC
/// Note the ( might be next or prev day )
unsigned int GetUTCHHMMFromTZHHMM(const int& r_yyyymmdd_, unsigned int hhmm, const char* tzstr_);
int GetUTCYYMMDDFromTZHHMMSS(const int& r_yyyymmdd_, unsigned int hhmmss, const char* tzstr_);
int GetUTCYYMMDDFromTZHHMM(const int& r_yyyymmdd_, unsigned int hhmm, const char* tzstr_);
unsigned int GetUTCHHMMSSFromTZHHMMSS(const int& r_yyyymmdd_, unsigned int hhmmss, const char* tzstr_);
int GetUTCSecondsFromMidnightFromTZHHMM(const int& r_yyyymmdd_, unsigned int hhmm, const char* tzstr_);
int GetESTSecondsFromMidnightFromTZHHMM(const int& r_yyyymmdd_, unsigned int hhmm, const char* tzstr_);
int GetUTCMsecsFromMidnightFromTZHHMM(const int& r_yyyymmdd_, unsigned int hhmm, const char* tzstr_);
}

/// Simple function to get minutes from midnight corresponding to specified HHMM time
inline unsigned int GetMinutesFromHHMM(const int _hhmm_) { return (((_hhmm_ / 100) * 60) + (_hhmm_ % 100)); }
inline unsigned int GetSecondsFromHHMM(const int _hhmm_) { return (GetMinutesFromHHMM(_hhmm_) * 60); }
inline unsigned int GetMsecsFromMidnightFromHHMM(const int _hhmm_) { return GetSecondsFromHHMM(_hhmm_) * 1000; }

inline unsigned int GetSecondsFromHHMMSS(const int _hhmmss_) {
  return ((_hhmmss_ / 10000) * 3600 + ((_hhmmss_ % 10000) / 100) * 60 + (_hhmmss_ % 100));
}
inline unsigned int GetMsecsFromMidnightFromHHMMSS(const int _hhmmss_) { return GetSecondsFromHHMMSS(_hhmmss_) * 1000; }

inline int GetHHMMSSFromMsecsFromMidnight(int r_mfm_, int period, int t) {
  int time_slot = ((r_mfm_ / 1000) / period + t) * period;
  int hh = time_slot / 3600;
  time_slot -= hh * 3600;
  int mm = time_slot / 60;
  time_slot -= mm * 60;
  int ss = time_slot;
  return 10000 * hh + 100 * mm + ss;
}

int DaysBetweenDates(int YYYYMMDD_start, int YYYYMMDD_end);
}

#endif  // BASE_COMMONTRADEUTILS_DATE_TIME_H
