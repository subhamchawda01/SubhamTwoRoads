#include <locale>
#include <string>
#include <iostream>
#include <sstream>
#include <boost/date_time/gregorian/gregorian.hpp>

inline int YYYYMMDD_from_date(const boost::gregorian::date& d1) {
  boost::gregorian::date::ymd_type ymd = d1.year_month_day();
  return (((ymd.year * 100 + ymd.month) * 100) + ymd.day);
}

int main(int argc, char** argv) {
  boost::gregorian::date dloc = boost::gregorian::day_clock::local_day();
  boost::gregorian::date dutc = boost::gregorian::day_clock::universal_day();

  std::cout << "LOC: " << YYYYMMDD_from_date(dloc) << " UTC: " << YYYYMMDD_from_date(dutc) << std::endl;
  return 0;
}
