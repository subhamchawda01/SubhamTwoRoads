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
  std::string shortcode_ = "";
  int starttime_ = -1;
  int endtime_ = -1;

  // expect :
  // 1. $exec date=TOMORROW

  if (argc < 5) {
    std::cerr << "expecting : $exec shortcode starttime=0000 endtime=2359 TOMORROW [local=NYC | IST]" << std::endl;
    exit(0);
  }

  if ((argc >= 2)) shortcode_ = argv[1];

  if ((argc >= 3)) {
    starttime_ = atoi(argv[2]);

    if (starttime_ < 0 || starttime_ > 2359) {
      std::cerr << "starttime argument invalid..!" << std::endl;
      exit(0);
    }
  }

  if ((argc >= 4)) {
    endtime_ = atoi(argv[3]);

    if (endtime_ < 0 || endtime_ > 2359) {
      std::cerr << "endtime argument invalid..!" << std::endl;
      exit(0);
    }
  }

  if ((argc >= 5) && (strcmp(argv[4], "TODAY") != 0) && (strcmp(argv[4], "TOMORROW") != 0)) {
    tradingdate_ = atoi(argv[4]);
  } else {
    if (strcmp(argv[4], "TODAY") == 0)
      tradingdate_ = HFSAT::DateTime::GetCurrentIsoDateLocal();
    else
      tradingdate_ = HFSAT::DateTime::CalcNextDay(HFSAT::DateTime::GetCurrentIsoDateLocal());
  }

  std::string locale = "NYC";
  if (argc >= 6) {
    locale = argv[5];
  }

  HFSAT::DebugLogger dbglogger_(100, 1);  // dummy
  HFSAT::Watch watch_(dbglogger_, tradingdate_);
  HFSAT::EconomicEventsManager economic_events_manager_(dbglogger_, watch_, shortcode_, starttime_, endtime_);

  const std::vector<HFSAT::EventLine>& events_of_the_day_ = economic_events_manager_.events_of_the_day();

  std::cout << "EpochTime" << ' ' << "Zone" << ' ' << "Sev" << ' ' << "Desc" << ' ' << "StartTime" << ' ' << "StartSFM"
            << ' ' << "EventSFM" << ' ' << "EndSFM" << ' ' << "EndTime" << std::endl;

  for (unsigned int i = 0; i < events_of_the_day_.size(); i++) {
    std::cout << events_of_the_day_[i].event_time_;

    if (locale.compare("IST") == 0) {
      std::cout << ' ' << HFSAT::DateTime::GetIndLocalTimeFromUTCTime(events_of_the_day_[i].event_time_);

      std::cout << ' ' << HFSAT::GetStrFromEconomicZone(events_of_the_day_[i].ez_) << ' '
                << events_of_the_day_[i].severity_ << ' ' << events_of_the_day_[i].event_text_ << ' '
                << HFSAT::DateTime::GetIndLocalTimeFromUTCTime(
                       HFSAT::DateTime::GetTimeUTCFromMFMUTC(tradingdate_, events_of_the_day_[i].start_mfm_))
                << ' ' << events_of_the_day_[i].start_mfm_ / 1000 << ' ' << events_of_the_day_[i].event_mfm_ / 1000
                << ' ' << events_of_the_day_[i].end_mfm_ / 1000 << ' '
                << HFSAT::DateTime::GetIndLocalTimeFromUTCTime(
                       HFSAT::DateTime::GetTimeUTCFromMFMUTC(tradingdate_, events_of_the_day_[i].end_mfm_))
                << std::endl;
    } else {
      std::cout << ' ' << HFSAT::DateTime::GetNYLocalTimeFromUTCTime(events_of_the_day_[i].event_time_);

      std::cout << ' ' << HFSAT::GetStrFromEconomicZone(events_of_the_day_[i].ez_) << ' '
                << events_of_the_day_[i].severity_ << ' ' << events_of_the_day_[i].event_text_ << ' '
                << HFSAT::DateTime::GetNYLocalTimeFromUTCTime(
                       HFSAT::DateTime::GetTimeUTCFromMFMUTC(tradingdate_, events_of_the_day_[i].start_mfm_))
                << ' ' << events_of_the_day_[i].start_mfm_ / 1000 << ' ' << events_of_the_day_[i].event_mfm_ / 1000
                << ' ' << events_of_the_day_[i].end_mfm_ / 1000 << ' '
                << HFSAT::DateTime::GetNYLocalTimeFromUTCTime(
                       HFSAT::DateTime::GetTimeUTCFromMFMUTC(tradingdate_, events_of_the_day_[i].end_mfm_))
                << std::endl;
    }
  }

  return 0;
}
