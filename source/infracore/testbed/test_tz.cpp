#include <iostream>
#include <ctime>
#include "dvccode/CommonTradeUtils/date_time.hpp"

int main() {
  time_t t = HFSAT::DateTime::GetTimeMidnightBRT(20110906);
  std::cout << t << " : BRT\n";

  t = HFSAT::DateTime::GetTimeMidnightCET(20110906);
  std::cout << t << " : CET\n";

  t = HFSAT::DateTime::GetTimeMidnightCST(20110906);
  std::cout << t << " : CST\n";

  t = HFSAT::DateTime::GetTimeMidnightEST(20110906);
  std::cout << t << " : EST\n";

  t = HFSAT::DateTime::GetTimeMidnightUTC(20110906);
  std::cout << t << " : UTC\n";

  boost::local_time::local_date_time t1 = HFSAT::DateTime::GetIndLocalTimeFromUTCTime(t);
  std::cout << t1 << " : india local\n";

  t1 = HFSAT::DateTime::GetNYLocalTimeFromUTCTime(t);
  std::cout << t1 << " : NY local\n";

  return 0;
}
