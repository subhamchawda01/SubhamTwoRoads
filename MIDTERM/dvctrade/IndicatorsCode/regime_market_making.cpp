/**
    \file IndicatorsCode/regime_market_making.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CommonDataStructures/vector_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/regime_market_making.hpp"

namespace HFSAT {

void RegimeMarketMaking::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                           std::vector<std::string>& _ors_source_needed_vec_,
                                           const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

RegimeMarketMaking* RegimeMarketMaking::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                          const std::vector<const char*>& r_tokens_,
                                                          PriceType_t _basepx_pxtype_) {
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), atof(r_tokens_[5]), atof(r_tokens_[6]), atof(r_tokens_[7]));
}

RegimeMarketMaking* RegimeMarketMaking::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                          SecurityMarketView& _indep_market_view_,
                                                          double fractional_sec_, double l1sz_switch_threshold_,
                                                          double stdev_threshold_, double tolerance_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << fractional_sec_ << ' '
              << l1sz_switch_threshold_ << ' ' << stdev_threshold_ << ' ' << tolerance_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, RegimeMarketMaking*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new RegimeMarketMaking(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_,
                               fractional_sec_, l1sz_switch_threshold_, stdev_threshold_, tolerance_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

RegimeMarketMaking::RegimeMarketMaking(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                       const std::string& concise_indicator_description_,
                                       SecurityMarketView& _indep_market_view_, double fractional_sec_,
                                       double l1sz_switch_threshold, double stdev_switch_threshold, double tolerance)
    : IndicatorListener(),
      CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      l1sz_switch_threshold_(l1sz_switch_threshold),
      stdev_switch_threshold_(stdev_switch_threshold),
      tolerance_(tolerance),
      current_l1sz_ratio_(0),
      current_stdev_ratio_(0) {
  p_slow_stdev_calculator = SlowStdevCalculator::GetUniqueInstance(
      t_dbglogger_, r_watch_, indep_market_view_.shortcode(), fractional_sec_ * 1000u, 0);
  p_slow_stdev_calculator->add_unweighted_indicator_listener(0u, this);
  l1_trend_var_ = L1SizeTrend::GetUniqueInstance(t_dbglogger_, r_watch_, indep_market_view_, fractional_sec_,
                                                 HFSAT::kPriceTypeMidprice);
  l1_trend_var_->add_unweighted_indicator_listener(1u, this);
  avg_l1sz_ = HFSAT::SampleDataUtil::GetAvgForPeriod(indep_market_view_.shortcode(), r_watch_.YYYYMMDD(), 60,
                                                     trading_start_mfm_, trading_end_mfm_, std::string("L1SZ"));
  avg_stdev_ = HFSAT::SampleDataUtil::GetAvgForPeriod(indep_market_view_.shortcode(), r_watch_.YYYYMMDD(), 60,
                                                      trading_start_mfm_, trading_end_mfm_, std::string("STDEV"));
}

void RegimeMarketMaking::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void RegimeMarketMaking::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (!is_ready_) {
    is_ready_ = true;
    indicator_value_ = 2;
    // NotifyIndicatorListeners ( 1 ) ;
  } else {
    if (_indicator_index_ == 0) {
      current_stdev_ratio_ = _new_value_ / avg_stdev_;
    } else if (_indicator_index_ == 1) {
      current_l1sz_ratio_ = _new_value_ / avg_l1sz_;
    }

    if (indicator_value_ == 2) {
      if ((current_l1sz_ratio_ > l1sz_switch_threshold_ * (1 + tolerance_)) &&
          (current_stdev_ratio_ < stdev_switch_threshold_ * (1 - tolerance_))) {
        indicator_value_ = 1;
      }
    } else if (indicator_value_ == 1) {
      if ((current_l1sz_ratio_ < l1sz_switch_threshold_ * (1 - tolerance_)) ||
          (current_stdev_ratio_ > stdev_switch_threshold_ * (1 + tolerance_))) {
        indicator_value_ = 2;
      }
    }
    NotifyIndicatorListeners(indicator_value_);
  }
}

void RegimeMarketMaking::InitializeValues() {
  indicator_value_ = 2;
  NotifyIndicatorListeners(indicator_value_);
  is_ready_ = true;
}

void RegimeMarketMaking::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                 const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 2;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void RegimeMarketMaking::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
  } else
    return;
}
}
