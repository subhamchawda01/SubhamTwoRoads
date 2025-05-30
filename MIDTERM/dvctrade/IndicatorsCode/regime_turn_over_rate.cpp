/**
   \file IndicatorsCode/regime_turn_over_rate.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#include "dvctrade/Indicators/regime_turn_over_rate.hpp"

namespace HFSAT {

void RegimeTurnOverRate::CollectShortCodes(std::vector<std::string>& t_shortcodes_affecting_this_indicator,
                                           std::vector<std::string>& _ors_source_needed_vec_,
                                           const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(t_shortcodes_affecting_this_indicator, (std::string)r_tokens_[3]);
}

RegimeTurnOverRate* RegimeTurnOverRate::GetUniqueInstance(DebugLogger& t_dbglogger, const Watch& r_watch,
                                                          const std::vector<const char*>& r_tokens,
                                                          PriceType_t basepx_pxtype) {
  // INDICATOR  _this_weight_  _indicator_string_  _dep_market_view_ fractional_seconds tolerance
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens[3]);
  if (r_tokens.size() >= 7) {
    return GetUniqueInstance(t_dbglogger, r_watch,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens[3])),
                             atof(r_tokens[4]), atof(r_tokens[5]), StringToPriceType_t(r_tokens[6]));
  } else {
    return NULL;
  }
}

RegimeTurnOverRate* RegimeTurnOverRate::GetUniqueInstance(DebugLogger& t_dbglogger, const Watch& r_watch,
                                                          const SecurityMarketView& dep_market_view,
                                                          double fractional_seconds, double threshold,
                                                          PriceType_t price_type) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << dep_market_view.secname() << ' ' << fractional_seconds << ' ' << threshold << ' '
              << PriceType_t_To_String(price_type);
  std::string t_concise_indicator_description(t_temp_oss_.str());

  static std::map<std::string, RegimeTurnOverRate*> t_concise_indicator_descriptionmap_;

  if (t_concise_indicator_descriptionmap_.find(t_concise_indicator_description) ==
      t_concise_indicator_descriptionmap_.end()) {
    t_concise_indicator_descriptionmap_[t_concise_indicator_description] =
        new RegimeTurnOverRate(t_dbglogger, r_watch, t_concise_indicator_description, dep_market_view,
                               fractional_seconds, threshold, price_type);
  }
  return t_concise_indicator_descriptionmap_[t_concise_indicator_description];
}

RegimeTurnOverRate::RegimeTurnOverRate(DebugLogger& t_dbglogger, const Watch& r_watch,
                                       const std::string& t_concise_indicator_description,
                                       const SecurityMarketView& t_dep_market_view, double fractional_seconds,
                                       double threshold, PriceType_t price_type)
    : CommonIndicator(t_dbglogger, r_watch, t_concise_indicator_description),
      dep_market_view_(&t_dep_market_view),
      p_dep_indicator_(nullptr),
      tolerance_(threshold),
      avg_periodic_tor_(0.0),
      current_slot_(0) {
  p_dep_indicator_ = TurnOverRate::GetUniqueInstance(t_dbglogger, r_watch, t_dep_market_view, fractional_seconds);
  if (p_dep_indicator_ == NULL) {
    indicator_value_ = 0;
    is_ready_ = true;
    return;
  }
  p_dep_indicator_->add_unweighted_indicator_listener(1u, this);
  watch_.subscribe_FifteenSecondPeriod(this);
}

void RegimeTurnOverRate::OnTimePeriodUpdate(const int num_pages_to_add_) {
  int t_current_slot_ = SampleDataUtil::GetSlotFromMfm(watch_.msecs_from_midnight());
  if (current_slot_ != t_current_slot_) {
    current_slot_ = t_current_slot_;
    if (periodic_tors_.find(current_slot_) != periodic_tors_.end()) {
      avg_periodic_tor_ = periodic_tors_[current_slot_];
    }
  }
}

void RegimeTurnOverRate::OnIndicatorUpdate(const unsigned int& t_indicator_index_,
                                           const double& t_new_indicator_value_) {
  if (!is_ready_) {
    InitializeValues();
    is_ready_ = true;
  } else {
    if ((t_new_indicator_value_ > avg_periodic_tor_ * (1 - tolerance_) && indicator_value_ == 1) ||
        (t_new_indicator_value_ < -1 * (1 - tolerance_) * avg_periodic_tor_ && indicator_value_ == 2) ||
        (-avg_periodic_tor_ * (1 + tolerance_) < t_new_indicator_value_ &&
         t_new_indicator_value_ < avg_periodic_tor_ * (1 + tolerance_) && indicator_value_ == 3)) {
    } else {
      if (t_new_indicator_value_ > avg_periodic_tor_) {
        indicator_value_ = 1;
      } else if (t_new_indicator_value_ < -avg_periodic_tor_) {
        indicator_value_ = 2;
      } else {
        indicator_value_ = 3;
      }
      NotifyIndicatorListeners(indicator_value_);
    }
  }
}

void RegimeTurnOverRate::WhyNotReady() {}

void RegimeTurnOverRate::OnMarketDataInterrupted(const unsigned int security_id, const int msecs_since_last_receive) {}

void RegimeTurnOverRate::OnMarketDataResumed(const unsigned int security_id) {}

void RegimeTurnOverRate::InitializeValues() {
  avg_periodic_tor_ =
      HFSAT::SampleDataUtil::GetAvgForPeriod(dep_market_view_->shortcode(), watch_.YYYYMMDD(), 60, trading_start_mfm_,
                                             trading_end_mfm_, "TOR", periodic_tors_);
  current_slot_ = SampleDataUtil::GetSlotFromMfm(watch_.msecs_from_midnight());
  if (periodic_tors_.find(current_slot_) != periodic_tors_.end()) {
    avg_periodic_tor_ = periodic_tors_[current_slot_];
  }
  indicator_value_ = 3;
}
}
