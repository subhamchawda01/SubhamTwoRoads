/**
    \file IndicatorsCode/positioning.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/positioning.hpp"

namespace HFSAT {

void Positioning::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                    std::vector<std::string>& _ors_source_needed_vec_,
                                    const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

Positioning* Positioning::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                            const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _num_trades_halflife_
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atoi(r_tokens_[4]), _basepx_pxtype_);
}

Positioning* Positioning::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                            SecurityMarketView& _indep_market_view_, int _bucket_size_,
                                            PriceType_t _basepx_pxtype_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _bucket_size_ << " "
              << PriceType_t_To_String(_basepx_pxtype_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, Positioning*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new Positioning(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, _bucket_size_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

Positioning::Positioning(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                         const std::string& concise_indicator_description_, SecurityMarketView& _indep_market_view_,
                         int _bucket_size_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      bucket_size_(_bucket_size_),
      alpha_(1.0 / _bucket_size_) {
  _indep_market_view_.subscribe_tradeprints(this);
  _indep_market_view_.ComputeIntTradeType();
}

void Positioning::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                               const MarketUpdateInfo& _market_update_info_) {
  if (_trade_print_info_.size_traded_ > bucket_size_) {
    indicator_value_ = _trade_print_info_.int_trade_type_;
  } else {
    indicator_value_ = indicator_value_ * (1 - _trade_print_info_.size_traded_ * alpha_) +
                       _trade_print_info_.size_traded_ * alpha_ * _trade_print_info_.int_trade_type_;
  }

  indicator_value_ = indicator_value_ > 1.0 ? 1.0 : indicator_value_;
  indicator_value_ = indicator_value_ < -1.0 ? -1.0 : indicator_value_;

  if (data_interrupted_) indicator_value_ = 0;

  NotifyIndicatorListeners(indicator_value_);
}

void Positioning::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  } else
    return;
}

void Positioning::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
  } else
    return;
}
}
