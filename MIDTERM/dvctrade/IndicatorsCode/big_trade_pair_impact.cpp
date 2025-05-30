/**
    \file IndicatorsCode/moving_avg_trade_size.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/big_trade_pair_impact.hpp"

namespace HFSAT {

void BigTradePairImpact::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                           std::vector<std::string>& _ors_source_needed_vec_,
                                           const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

BigTradePairImpact* BigTradePairImpact::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                          const std::vector<const char*>& r_tokens_,
                                                          PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _window_msecs_ buysell_(0:buy:1:sell)
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atoi(r_tokens_[4]), atoi(r_tokens_[5]));
}

BigTradePairImpact* BigTradePairImpact::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                          SecurityMarketView& _indep_market_view_, int _window_msecs_,
                                                          int buysell_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _window_msecs_ << ' ' << buysell_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, BigTradePairImpact*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new BigTradePairImpact(
        t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, _window_msecs_, buysell_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

BigTradePairImpact::BigTradePairImpact(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                       const std::string& concise_indicator_description_,
                                       SecurityMarketView& _indep_market_view_, int _window_msecs_, int _buysell_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      page_width_msecs_(_window_msecs_),
      buysell_(_buysell_),
      current_trade_impact_(0) {
  indep_market_view_.ComputeTradeImpact();
  indep_market_view_.subscribe_tradeprints(this);
}

void BigTradePairImpact::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void BigTradePairImpact::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                                      const MarketUpdateInfo& _market_update_info_) {
  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
    }
  } else {
    if ((int)_trade_print_info_.buysell_ == buysell_) {
      current_trade_impact_ = _trade_print_info_.size_traded_;

      while (!trades_queue_.empty()) {
        TradeElem* t_trade_elem_ = trades_queue_.front();
        if (watch_.msecs_from_midnight() - t_trade_elem_->time_msecs_ < page_width_msecs_) {
          break;
        }
        indicator_value_ = indicator_value_ - t_trade_elem_->impact_;
        trades_queue_.pop_front();
      }
      trades_queue_.push_back(new TradeElem(watch_.msecs_from_midnight(), current_trade_impact_));
      indicator_value_ += current_trade_impact_;

      if (data_interrupted_) {
        indicator_value_ = 0;
      }

      NotifyIndicatorListeners(indicator_value_);
    }
  }
}

void BigTradePairImpact::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                 const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void BigTradePairImpact::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
  }
}
}
