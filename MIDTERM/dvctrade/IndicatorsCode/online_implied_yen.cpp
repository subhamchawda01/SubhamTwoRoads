/**
    \file IndicatorsCode/online_implied_yen.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/online_implied_yen.hpp"

namespace HFSAT {

void OnlineImpliedYen::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                         std::vector<std::string>& _ors_source_needed_vec_,
                                         const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[5]);
}

OnlineImpliedYen* OnlineImpliedYen::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                      const std::vector<const char*>& r_tokens_,
                                                      PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dollar_market_view_ _yen_market_view_ currency_market_view
  // _fractional_seconds_ _price_type_
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[5])),
                           atof(r_tokens_[6]), StringToPriceType_t(r_tokens_[7]));
}

OnlineImpliedYen* OnlineImpliedYen::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                      SecurityMarketView& _dollar_market_view_,
                                                      SecurityMarketView& _yen_market_view_,
                                                      SecurityMarketView& _currency_market_view_,
                                                      double _fractional_seconds_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dollar_market_view_.secname() << ' ' << _yen_market_view_.secname() << ' '
              << _currency_market_view_.secname() << _fractional_seconds_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, OnlineImpliedYen*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new OnlineImpliedYen(t_dbglogger_, r_watch_, concise_indicator_description_, _dollar_market_view_,
                             _yen_market_view_, _currency_market_view_, _fractional_seconds_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

OnlineImpliedYen::OnlineImpliedYen(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                   const std::string& concise_indicator_description_,
                                   SecurityMarketView& _dollar_market_view_, SecurityMarketView& _yen_market_view_,
                                   SecurityMarketView& _currency_market_view_, double _fractional_seconds_,
                                   PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      IndicatorListener(),
      mkt_trend_(0.0),
      ipld_trend_(0.0) {
  market_trend_ = SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, _currency_market_view_.shortcode(), _fractional_seconds_,
                                                 _price_type_);
  implied_trend_ = OnlineRatioPairs::GetUniqueInstance(t_dbglogger_, r_watch_, _yen_market_view_, _dollar_market_view_,
                                                       _fractional_seconds_, _price_type_);

  market_trend_->add_indicator_listener(0, this, 0.000001);
  implied_trend_->add_unweighted_indicator_listener(1, this);
}

void OnlineImpliedYen::OnMarketUpdate(const unsigned int t_security_id_,
                                      const MarketUpdateInfo& cr_market_update_info_) {}

void OnlineImpliedYen::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (!is_ready_) {
    is_ready_ = true;
    NotifyIndicatorListeners(0);
  } else {
    if (_indicator_index_ == 0u) {
      mkt_trend_ = _new_value_;
    } else {
      ipld_trend_ = _new_value_;
    }
  }
  indicator_value_ = (mkt_trend_ - ipld_trend_);
  NotifyIndicatorListeners(indicator_value_);
}

void OnlineImpliedYen::InitializeValues() { indicator_value_ = 0; }

void OnlineImpliedYen::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {}

void OnlineImpliedYen::OnMarketDataResumed(const unsigned int _security_id_) {}
}
