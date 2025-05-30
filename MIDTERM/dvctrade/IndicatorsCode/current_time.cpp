/**
   \file IndicatorsCode/current_time.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 351, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#include <fstream>
#include <strings.h>
#include <stdlib.h>
#include <sstream>

#include "dvccode/CDef/math_utils.hpp"

#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvctrade/Indicators/current_time.hpp"

namespace HFSAT {

void CurrentTime::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                    std::vector<std::string>& _ors_source_needed_vec_,
                                    const std::vector<const char*>& r_tokens_) {}

CurrentTime* CurrentTime::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                            const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_) {
  // INDICATOR  _this_weight_  _indicator_string_  _dep_market_view_  _indep_market_view_  _fractional_seconds_
  // _volume_measure_seconds_  _trend_weight_  _price_type_
  return GetUniqueInstance(t_dbglogger_, r_watch_);
}

CurrentTime* CurrentTime::GetUniqueInstance(DebugLogger& t_dbglogger, const Watch& r_watch) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName();
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, CurrentTime*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new CurrentTime(t_dbglogger, r_watch, concise_indicator_description_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

CurrentTime::CurrentTime(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                         const std::string& t_concise_indicator_description)
    : CommonIndicator(t_dbglogger_, r_watch_, t_concise_indicator_description) {
  watch_.subscribe_TimePeriod(this);
}
void CurrentTime::OnTimePeriodUpdate(const int num_pages_to_add_) {
  // Just returning time
  indicator_value_ = (double)watch_.tv().tv_sec + (double)watch_.tv().tv_usec / 1000000.0;

  NotifyIndicatorListeners(indicator_value_);
}

void CurrentTime::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}

void CurrentTime::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                               const MarketUpdateInfo& _market_update_info_) {}

void CurrentTime::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {}

void CurrentTime::OnMarketDataResumed(const unsigned int _security_id_) {}
}
