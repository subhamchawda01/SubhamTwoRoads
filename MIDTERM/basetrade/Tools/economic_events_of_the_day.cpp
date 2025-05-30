/**
    \file Tools/economic_events_of_the_day.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/

#include <iostream>
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/CommonTradeUtils/economic_events_manager.hpp"

int main(int argc, char** argv) {
  int tradingdate_ = 0;
  // expect :
  // 1. $exec date=TOMORROW

  if (argc < 2) {
    std::cerr << "expecting : $exec date=TOMORROW [local=NYC | IST]" << std::endl;
    exit(0);
  }
  if ((argc >= 2) && (strcmp(argv[1], "TODAY") != 0) && (strcmp(argv[1], "TOMORROW") != 0)) {
    tradingdate_ = atoi(argv[1]);
  } else {
    if (strcmp(argv[1], "TODAY") == 0)
      tradingdate_ = HFSAT::DateTime::GetCurrentIsoDateLocal();
    else
      tradingdate_ = HFSAT::DateTime::CalcNextDay(HFSAT::DateTime::GetCurrentIsoDateLocal());
  }

  std::string locale = "NYC";
  if (argc >= 3) {
    locale = argv[2];
  }

  HFSAT::DebugLogger dbglogger_(100, 1);  // dummy
  HFSAT::Watch watch_(dbglogger_, tradingdate_);
  HFSAT::EconomicEventsManager economic_events_manager_(dbglogger_, watch_);

  const std::vector<HFSAT::EventLine>& events_of_the_day_ = economic_events_manager_.events_of_the_day();

  std::cout << "EpochTime" << ' ' << "Zone" << ' ' << "Sev" << ' ' << "Desc" << ' ' << "StartTime" << ' ' << "StartSFM"
            << ' ' << "EventSFM" << ' ' << "EndSFM" << ' ' << "EndTime" << std::endl;

  for (auto i = 0u; i < events_of_the_day_.size(); i++) {
    std::cout << events_of_the_day_[i].event_time_;

    if (locale.compare("IST") == 0) {
      std::cout << ' ' << HFSAT::DateTime::GetIndLocalTimeFromUTCTime(events_of_the_day_[i].event_time_);

      std::cout << ' ' << HFSAT::GetStrFromEconomicZone(events_of_the_day_[i].ez_) << ' '
                << events_of_the_day_[i].severity_ << ' ' << events_of_the_day_[i].event_text_ << ' '
                << HFSAT::DateTime::GetIndLocalTimeFromUTCTime(
                       HFSAT::DateTime::GetTimeUTCFromMFMUTC(tradingdate_, events_of_the_day_[i].start_mfm_)) << ' '
                << events_of_the_day_[i].start_mfm_ / 1000 << ' ' << events_of_the_day_[i].event_mfm_ / 1000 << ' '
                << events_of_the_day_[i].end_mfm_ / 1000 << ' '
                << HFSAT::DateTime::GetIndLocalTimeFromUTCTime(HFSAT::DateTime::GetTimeUTCFromMFMUTC(
                       tradingdate_, events_of_the_day_[i].end_mfm_)) << std::endl;
    } else {
      std::cout << ' ' << HFSAT::DateTime::GetNYLocalTimeFromUTCTime(events_of_the_day_[i].event_time_);

      std::cout << ' ' << HFSAT::GetStrFromEconomicZone(events_of_the_day_[i].ez_) << ' '
                << events_of_the_day_[i].severity_ << ' ' << events_of_the_day_[i].event_text_ << ' '
                << HFSAT::DateTime::GetNYLocalTimeFromUTCTime(
                       HFSAT::DateTime::GetTimeUTCFromMFMUTC(tradingdate_, events_of_the_day_[i].start_mfm_)) << ' '
                << events_of_the_day_[i].start_mfm_ / 1000 << ' ' << events_of_the_day_[i].event_mfm_ / 1000 << ' '
                << events_of_the_day_[i].end_mfm_ / 1000 << ' '
                << HFSAT::DateTime::GetNYLocalTimeFromUTCTime(HFSAT::DateTime::GetTimeUTCFromMFMUTC(
                       tradingdate_, events_of_the_day_[i].end_mfm_)) << std::endl;
    }
  }

  return 0;
}
