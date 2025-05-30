/**
    \file IndicatorsCode/recent_simple_trades_measure.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvctrade/Indicators/indicator_util.hpp"

#include "dvctrade/Indicators/recent_simple_trades_measure.hpp"

namespace HFSAT {

void RecentSimpleTradesMeasure::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                  std::vector<std::string>& _ors_source_needed_vec_,
                                                  const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

RecentSimpleTradesMeasure* RecentSimpleTradesMeasure::GetUniqueInstance(DebugLogger& r_dbglogger_,
                                                                        const Watch& r_watch_,
                                                                        const std::vector<const char*>& r_tokens_,
                                                                        PriceType_t _basepx_pxtype_) {
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(r_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]));
}

RecentSimpleTradesMeasure* RecentSimpleTradesMeasure::GetUniqueInstance(DebugLogger& t_dbglogger_,
                                                                        const Watch& r_watch_,
                                                                        const SecurityMarketView& _indep_market_view_,
                                                                        double _fractional_seconds_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _fractional_seconds_;

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, RecentSimpleTradesMeasure*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new RecentSimpleTradesMeasure(
        t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, _fractional_seconds_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

RecentSimpleTradesMeasure::RecentSimpleTradesMeasure(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                     const std::string& concise_indicator_description_,
                                                     const SecurityMarketView& _indep_market_view_,
                                                     double _fractional_seconds_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      pages_since_last_broadcast_(0),
      num_pages_to_recall_(std::max(10, (int)ceil(_fractional_seconds_))),
      new_page_value_(0),
      past_tradess_(),
      read_first_secs_(false) {
  r_watch_.subscribe_BigTimePeriod(this);
  _indep_market_view_.subscribe_tradeprints(this);
}

void RecentSimpleTradesMeasure::OnTimePeriodUpdate(const int num_pages_to_add_) {
  if (!read_first_secs_) {
    if (watch_.tv().tv_sec % 60 != 0) {
      return;
    } else {
      read_first_secs_ = true;
    }
  }

  for (int i = 0; i < (num_pages_to_add_ - 1); i++) {
    past_tradess_.push_back(0);
  }
  while (past_tradess_.size() >= num_pages_to_recall_) {
    indicator_value_ -= past_tradess_[0];
    past_tradess_.erase(past_tradess_.begin());
  }

  past_tradess_.push_back(new_page_value_);
  indicator_value_ += new_page_value_;
  new_page_value_ = 0;

  pages_since_last_broadcast_ += num_pages_to_add_;
  if (pages_since_last_broadcast_ >= 60) {
    // broadcast to listeners
    NotifyIndicatorListeners(indicator_value_);
    pages_since_last_broadcast_ = 0;
  }
}

void RecentSimpleTradesMeasure::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                                             const MarketUpdateInfo& _market_update_info_) {
  ++new_page_value_;
}
void RecentSimpleTradesMeasure::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                        const int msecs_since_last_receive_) {
  data_interrupted_ = true;
}

void RecentSimpleTradesMeasure::OnMarketDataResumed(const unsigned int _security_id_) { data_interrupted_ = false; }
}
