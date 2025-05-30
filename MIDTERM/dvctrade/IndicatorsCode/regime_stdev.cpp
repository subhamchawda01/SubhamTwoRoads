/**
    \file IndicatorsCode/regime_stdev.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CommonDataStructures/vector_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/regime_stdev.hpp"

namespace HFSAT {

void RegimeStdev::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                    std::vector<std::string>& _ors_source_needed_vec_,
                                    const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

RegimeStdev* RegimeStdev::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                            const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_) {
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), atof(r_tokens_[5]), atof(r_tokens_[6]));
}

RegimeStdev* RegimeStdev::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                            SecurityMarketView& _indep_market_view_, double fractional_sec_,
                                            double avg_val, double tolerance) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << fractional_sec_ << ' ' << avg_val << ' '
              << tolerance;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, RegimeStdev*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new RegimeStdev(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, fractional_sec_,
                        avg_val, tolerance);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

RegimeStdev::RegimeStdev(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                         const std::string& concise_indicator_description_, SecurityMarketView& _indep_market_view_,
                         double fractional_sec_, double avg_val, double tolerance)
    : IndicatorListener(),
      CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      avg_val_(avg_val),
      tolerance_(tolerance) {
  // price stdev
  p_stdev_calculator = SlowStdevCalculator::GetUniqueInstance(t_dbglogger_, r_watch_, indep_market_view_.shortcode(),
                                                              fractional_sec_ * 1000u, 0);
  p_stdev_calculator->add_unweighted_indicator_listener(0, this);
}

void RegimeStdev::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void RegimeStdev::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (!is_ready_) {
    is_ready_ = true;
    indicator_value_ = 1;
    NotifyIndicatorListeners(1);
    // low avg high
  } else {
    if (_new_value_ > avg_val_ * (1 + tolerance_)) {
      indicator_value_ = 3;
    } else if (_new_value_ < avg_val_ * (1 - tolerance_)) {
      indicator_value_ = 2;
    } else {
      indicator_value_ = 1;
    }
    NotifyIndicatorListeners(indicator_value_);
  }
}

void RegimeStdev::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 1;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void RegimeStdev::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
  } else
    return;
}
}
