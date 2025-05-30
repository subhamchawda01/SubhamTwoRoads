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
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "baseinfra/VolatileTradingInfo/shortcode_ezone_vec.hpp"

void GetTimeAndMFMFromString(int _tradingdate_, time_t &_time_, int &_mfm_, const char *_tz_hhmm_str_);

int main(int argc, char **argv) {
  int tradingdate_ = 0;
  std::string shortcode_ = "";
  char *starttime_ = (char *)"0000";
  char *endtime_ = (char *)"2359";

  // expect :
  // 1. $exec date=TOMORROW

  if (argc < 5) {
    std::cerr << "expecting : $exec shortcode starttime=0000 endtime=2359 TOMORROW [local=NYC | IST]" << std::endl;
    exit(0);
  }

  if ((argc >= 2)) shortcode_ = argv[1];

  if ((argc >= 3)) {
    starttime_ = argv[2];
  }

  if ((argc >= 4)) {
    endtime_ = argv[3];
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

  time_t stime_, etime_;
  int smfm_, emfm_;

  GetTimeAndMFMFromString(tradingdate_, stime_, smfm_, starttime_);
  GetTimeAndMFMFromString(tradingdate_, etime_, emfm_, endtime_);

  if (strncmp(shortcode_.c_str(), "NSE_", 4) == 0) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadNSESecurityDefinitions();
  }
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);
  HFSAT::ExchSource_t exch_src_ = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, tradingdate_);

  HFSAT::DebugLogger dbglogger_(100, 1);  // dummy
  HFSAT::Watch watch_(dbglogger_, tradingdate_);
  HFSAT::EconomicEventsManager economic_events_manager_(dbglogger_, watch_);

  economic_events_manager_.AdjustSeverity(shortcode_, exch_src_);
  economic_events_manager_.AllowEconomicEventsFromList(shortcode_);
  // bool allowed_events_present_ = economic_events_manager_.AllowedEventsPresent();

  std::vector<HFSAT::EconomicZone_t> s_ezone_vec_, e_ezone_vec_;
  HFSAT::GetEZVecForShortcode(shortcode_, smfm_, s_ezone_vec_);
  HFSAT::GetEZVecForShortcode(shortcode_, emfm_, e_ezone_vec_);

  const std::vector<HFSAT::EventLine> &events_of_the_day_ = economic_events_manager_.events_of_the_day();

  // std::cout << "AllowedEventsPresent: " << allowed_events_present_ << std::endl;
  std::cout << "EpochTime" << ' ' << "Zone" << ' ' << "Sev" << ' ' << "Desc" << ' ' << "StartTime" << ' ' << "StartSFM"
            << ' ' << "EventSFM" << ' ' << "EndSFM" << ' ' << "EndTime" << std::endl;

  for (auto i = 0u; i < events_of_the_day_.size(); i++) {
    std::vector<HFSAT::EconomicZone_t>::iterator it1, it2;
    it1 = find(s_ezone_vec_.begin(), s_ezone_vec_.end(), events_of_the_day_[i].ez_);
    it2 = find(e_ezone_vec_.begin(), e_ezone_vec_.end(), events_of_the_day_[i].ez_);
    if (it1 == s_ezone_vec_.end() && it2 == e_ezone_vec_.end()) {
      continue;
    }

    time_t evtime_ = events_of_the_day_[i].event_time_;
    if (evtime_ < stime_ || evtime_ > etime_) {
      continue;
    }

    std::cout << evtime_;

    if (locale.compare("IST") == 0) {
      std::cout << ' ' << HFSAT::DateTime::GetIndLocalTimeFromUTCTime(evtime_);

      std::cout << ' ' << HFSAT::GetStrFromEconomicZone(events_of_the_day_[i].ez_) << ' '
                << events_of_the_day_[i].severity_ << ' ' << events_of_the_day_[i].event_text_ << ' '
                << HFSAT::DateTime::GetIndLocalTimeFromUTCTime(
                       HFSAT::DateTime::GetTimeUTCFromMFMUTC(tradingdate_, events_of_the_day_[i].start_mfm_)) << ' '
                << events_of_the_day_[i].start_mfm_ / 1000 << ' ' << events_of_the_day_[i].event_mfm_ / 1000 << ' '
                << events_of_the_day_[i].end_mfm_ / 1000 << ' '
                << HFSAT::DateTime::GetIndLocalTimeFromUTCTime(HFSAT::DateTime::GetTimeUTCFromMFMUTC(
                       tradingdate_, events_of_the_day_[i].end_mfm_)) << std::endl;
    } else {
      std::cout << ' ' << HFSAT::DateTime::GetNYLocalTimeFromUTCTime(evtime_);

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

void GetTimeAndMFMFromString(int _tradingdate_, time_t &_time_, int &_mfm_, const char *_tz_hhmm_str_) {
  if ((strncmp(_tz_hhmm_str_, "EST_", 4) == 0) || (strncmp(_tz_hhmm_str_, "CST_", 4) == 0) ||
      (strncmp(_tz_hhmm_str_, "CET_", 4) == 0) || (strncmp(_tz_hhmm_str_, "BRT_", 4) == 0) ||
      (strncmp(_tz_hhmm_str_, "UTC_", 4) == 0) || (strncmp(_tz_hhmm_str_, "KST_", 4) == 0) ||
      (strncmp(_tz_hhmm_str_, "HKT_", 4) == 0) || (strncmp(_tz_hhmm_str_, "MSK_", 4) == 0) ||
      (strncmp(_tz_hhmm_str_, "IST_", 4) == 0) || (strncmp(_tz_hhmm_str_, "JST_", 4) == 0) ||
      (strncmp(_tz_hhmm_str_, "BST_", 4) == 0) || (strncmp(_tz_hhmm_str_, "AST_", 4) == 0)) {
    _time_ = HFSAT::DateTime::GetTimeFromTZHHMMSS(_tradingdate_, HFSAT::DateTime::GetHHMMSSTime(_tz_hhmm_str_ + 4),
                                                  _tz_hhmm_str_);
    _mfm_ = HFSAT::GetMsecsFromMidnightFromHHMMSS(HFSAT::DateTime::GetUTCHHMMSSFromTZHHMMSS(
        _tradingdate_, HFSAT::DateTime::GetHHMMSSTime(_tz_hhmm_str_ + 4), _tz_hhmm_str_));
  } else {
    _time_ = HFSAT::DateTime::GetTimeFromTZHHMMSS(_tradingdate_, HFSAT::DateTime::GetHHMMSSTime(_tz_hhmm_str_), "UTC_");
    _mfm_ = HFSAT::GetMsecsFromMidnightFromHHMMSS(HFSAT::DateTime::GetHHMMSSTime(_tz_hhmm_str_));
  }
}
