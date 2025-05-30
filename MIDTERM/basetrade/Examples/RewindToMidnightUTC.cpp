
#include <iostream>
#include <boost/date_time/local_time/local_time.hpp>
#include "dvccode/CommonTradeUtils/date_time.hpp"

using namespace boost::gregorian;
using namespace boost::local_time;
using namespace boost::posix_time;

// following inline functions taken from date_time.cpp
inline time_t to_time_t(const boost::posix_time::ptime& t_ptime_) {
  boost::posix_time::ptime _epoch_ptime_(boost::gregorian::date(1970, 1, 1));
  boost::posix_time::time_duration::sec_type _sec_type_ = (t_ptime_ - _epoch_ptime_).total_seconds();
  return time_t(_sec_type_);
}

inline boost::gregorian::date GetBoostDateFromIsoDate(const int& t_yyyymmdd_) {
  boost::gregorian::date d1((int)(t_yyyymmdd_ / 10000), (int)((t_yyyymmdd_ / 100) % 100), (int)(t_yyyymmdd_ % 100));
  return d1;
}

time_t RewindToMidnight(const time_t& t_time_t_) {
  boost::local_time::tz_database tz_db;
  tz_db.load_from_file("/apps/boost/root/libs/date_time/data/date_time_zonespec.csv");
  boost::local_time::time_zone_ptr est_tz = tz_db.time_zone_from_region("America/New_York");
  boost::local_time::time_zone_ptr cet_tz = tz_db.time_zone_from_region("Europe/Berlin");
  boost::local_time::time_zone_ptr brt_tz = tz_db.time_zone_from_region("America/Sao_Paulo");

  std::cout << "------------------" << std::endl;
  boost::local_time::local_date_time EST_midnight(GetBoostDateFromIsoDate(tradingdate_), boost::posix_time::hours(0),
                                                  est_tz, boost::local_time::local_date_time::EXCEPTION_ON_ERROR);
  boost::posix_time::ptime EST_midnight_utc_ptime = EST_midnight.utc_time();
  time_t EST_midnight_utc_time_t = to_time_t(EST_midnight_utc_ptime);

  std::cout << "As is: " << EST_midnight << " UTC: " << EST_midnight_utc_ptime << " time_t " << EST_midnight_utc_time_t
            << std::endl;

  boost::local_time::local_date_time CET_midnight(GetBoostDateFromIsoDate(tradingdate_), boost::posix_time::hours(0),
                                                  cet_tz, boost::local_time::local_date_time::EXCEPTION_ON_ERROR);
  boost::posix_time::ptime CET_midnight_utc_ptime = CET_midnight.utc_time();
  time_t CET_midnight_utc_time_t = to_time_t(CET_midnight_utc_ptime);

  std::cout << "As is: " << CET_midnight << " UTC: " << CET_midnight.utc_time() << " time_t " << CET_midnight_utc_time_t
            << std::endl;

  boost::local_time::local_date_time BRT_midnight(GetBoostDateFromIsoDate(tradingdate_), boost::posix_time::hours(0),
                                                  brt_tz, boost::local_time::local_date_time::EXCEPTION_ON_ERROR);
  boost::posix_time::ptime BRT_midnight_utc_ptime = BRT_midnight.utc_time();
  time_t BRT_midnight_utc_time_t = to_time_t(BRT_midnight_utc_ptime);

  std::cout << "As is: " << BRT_midnight << " UTC: " << BRT_midnight.utc_time() << " time_t " << BRT_midnight_utc_time_t
            << std::endl;
}

int main(int argc, char** argv) {
  PrintMidNight(20101130);
  PrintMidNight(20110325);
  PrintMidNight(20110501);

  return 0;
}
