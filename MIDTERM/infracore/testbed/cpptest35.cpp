#include <time.h>
#include "dvccode/CommonTradeUtils/date_time.hpp"

void CheckForDate(time_t& t_time_t_) {
  int yyyymmdd = HFSAT::DateTime::Get_local_YYYYMMDD_from_time_t(t_time_t_);
  std::cout << t_time_t_ << " -> LocalDate " << yyyymmdd << " local midnight time_t "
            << HFSAT::DateTime::GetTimeMidnightLocal(yyyymmdd) << " -> UTCDate "
            << HFSAT::DateTime::Get_UTC_YYYYMMDD_from_ttime(HFSAT::ttime_t(t_time_t_, 0)) << " UTC midnight time_t "
            << HFSAT::DateTime::GetTimeMidnightUTC(
                   HFSAT::DateTime::Get_UTC_YYYYMMDD_from_ttime(HFSAT::ttime_t(t_time_t_, 0)))
            << std::endl;

  for (int i = 0; i < 24; i++) {
    t_time_t_ += 3600;
    yyyymmdd = HFSAT::DateTime::Get_local_YYYYMMDD_from_time_t(t_time_t_);
    std::cout << t_time_t_ << " -> LocalDate " << yyyymmdd << " local midnight time_t "
              << HFSAT::DateTime::GetTimeMidnightLocal(yyyymmdd) << " -> UTCDate "
              << HFSAT::DateTime::Get_UTC_YYYYMMDD_from_ttime(HFSAT::ttime_t(t_time_t_, 0)) << " UTC midnight time_t "
              << HFSAT::DateTime::GetTimeMidnightUTC(
                     HFSAT::DateTime::Get_UTC_YYYYMMDD_from_ttime(HFSAT::ttime_t(t_time_t_, 0)))
              << std::endl;
  }
  std::cout << "---------------------------------" << std::endl;
}

int main(int argc, char** argv) {
  time_t t_time_t_ = 0;

  t_time_t_ = 1291093200;  // Tue Nov 30 00:00:00 2010 EST
  CheckForDate(t_time_t_);

  t_time_t_ = 1275811222;  // Sun Jun  6 04:00:22 2010 EST
  CheckForDate(t_time_t_);

  t_time_t_ = 1275796800;  // Sun Jun  6 00:00:00 2010 EST
  CheckForDate(t_time_t_);

  return 0;
}
