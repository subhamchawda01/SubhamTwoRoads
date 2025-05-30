/**
    \file IndicatorsCode/td_sizeavg_ttype_st_trend.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/td_sizeavg_ttype_st_trend.hpp"

namespace HFSAT {

void TDSizeAvgTTypeStTrend::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                              std::vector<std::string>& _ors_source_needed_vec_,
                                              const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

TDSizeAvgTTypeStTrend* TDSizeAvgTTypeStTrend::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                const std::vector<const char*>& r_tokens_,
                                                                PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ lt_fractional_seconds_ st_factor_
  // st_fractional_seconds_ st_trade_seconds_ price_type_
  if (r_tokens_.size() < 9) {
    std::cerr << "Expected format: INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ "
                 "lt_fractional_seconds_ st_factor_ st_fractional_seconds_ st_trade_seconds_ price_type_"
              << std::endl;
    return NULL;
  }
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), atof(r_tokens_[5]), atof(r_tokens_[6]), atof(r_tokens_[7]),
                           StringToPriceType_t(r_tokens_[8]));
}

TDSizeAvgTTypeStTrend* TDSizeAvgTTypeStTrend::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                SecurityMarketView& _indep_market_view_,
                                                                double lt_fractional_seconds_, double st_factor_,
                                                                double st_fractional_seconds_, double st_trade_seconds_,
                                                                PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << lt_fractional_seconds_ << ' ' << st_factor_
              << ' ' << st_fractional_seconds_ << ' ' << st_trade_seconds_ << ' '
              << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, TDSizeAvgTTypeStTrend*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new TDSizeAvgTTypeStTrend(
        t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, lt_fractional_seconds_, st_factor_,
        st_fractional_seconds_, st_trade_seconds_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

TDSizeAvgTTypeStTrend::TDSizeAvgTTypeStTrend(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                             const std::string& concise_indicator_description_,
                                             SecurityMarketView& _indep_market_view_, double t_lt_fractional_seconds_,
                                             double t_st_factor_, double t_st_fractional_seconds_,
                                             double t_st_trade_seconds_, PriceType_t t_price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      lt_fractional_seconds_(t_lt_fractional_seconds_),
      st_factor_(t_st_factor_),
      st_fractional_seconds_(t_st_fractional_seconds_),
      st_trade_seconds_(t_st_trade_seconds_),
      recent_simple_range_measure_(*(RecentSimpleRangeMeasure::GetUniqueInstance(
          t_dbglogger_, r_watch_, _indep_market_view_, std::max(1u, 2u * (unsigned int)t_lt_fractional_seconds_)))),
      lt_time_decayed_trade_info_manager_(*(TimeDecayedTradeInfoManager::GetUniqueInstance(
          t_dbglogger_, r_watch_, _indep_market_view_, t_lt_fractional_seconds_))),
      st_trade_adjusted_simple_trend_(*(TradeAdjustedSimpleTrend::GetUniqueInstance(
          t_dbglogger_, r_watch_, _indep_market_view_, t_st_fractional_seconds_, t_st_trade_seconds_, t_price_type_))),
      lt_range_(_indep_market_view_.min_price_increment()),
      lt_bias_(0),
      st_trend_(0) {
  _indep_market_view_.subscribe_price_type(this, t_price_type_);
  lt_time_decayed_trade_info_manager_.compute_sumtypesz();
  lt_time_decayed_trade_info_manager_.compute_sumsz();
  recent_simple_range_measure_.AddRecentSimpleRangeMeasureListener(this);
  st_trade_adjusted_simple_trend_.add_indicator_listener(1u, this, st_factor_);
}

void TDSizeAvgTTypeStTrend::OnMarketUpdate(const unsigned int _security_id_,
                                           const MarketUpdateInfo& _market_update_info_) {
  /// LT indicator = ( Sum of trade type and trade size divided by sum of trade size ) * ( ( lt_fractional_seconds_ /
  /// recent_simple_range_measure.range_calc_seconds() ) * recent_simple_range_measure.recent_range() )
  if (lt_time_decayed_trade_info_manager_.sumsz_ <= 0.1) {
    lt_bias_ = 0;
  } else {
    lt_bias_ = -lt_time_decayed_trade_info_manager_.sumtypesz_ /
               lt_time_decayed_trade_info_manager_.sumsz_;  // should have added a -ve sign since we expect the long
                                                            // term positioning to be a contrarian signal ... negatively
                                                            // correlated
  }

  if (is_ready_) {
    indicator_value_ = (lt_bias_ * lt_range_) + st_trend_;
    if (data_interrupted_) indicator_value_ = 0;

    NotifyIndicatorListeners(indicator_value_);
  }
}

void TDSizeAvgTTypeStTrend::OnRangeUpdate(const unsigned int r_security_id_, const double& _new_range_value_) {
  // ( ( lt_fractional_seconds_ / recent_simple_range_measure.range_calc_seconds() ) *
  // recent_simple_range_measure.recent_range() )
  lt_range_ = (lt_fractional_seconds_ * _new_range_value_) / recent_simple_range_measure_.range_calc_seconds();
  indicator_value_ = (lt_bias_ * lt_range_) + st_trend_;

  // printout to plot values
  // DBGLOG_TIME_CLASS_FUNC << " " << indep_market_view_.secname() << " " << _new_range_value_ << DBGLOG_ENDL_FLUSH ;
}

void TDSizeAvgTTypeStTrend::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  /// ST indicator = TradeAdjustedSimpleTrend ( st_fractional_seconds_, st_trade_seconds_ )
  indicator_value_ += (_new_value_ - st_trend_);
  st_trend_ = _new_value_;
  if (!is_ready_) {
    is_ready_ = true;
    indicator_value_ = (lt_bias_ * lt_range_) + st_trend_;
    DBGLOG_TIME_CLASS_FUNC << " ready" << DBGLOG_ENDL_FLUSH;
  }
  NotifyIndicatorListeners(indicator_value_);
}
void TDSizeAvgTTypeStTrend::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                    const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  indicator_value_ = 0;
  NotifyIndicatorListeners(indicator_value_);
}

void TDSizeAvgTTypeStTrend::OnMarketDataResumed(const unsigned int _security_id_) { data_interrupted_ = false; }
}
