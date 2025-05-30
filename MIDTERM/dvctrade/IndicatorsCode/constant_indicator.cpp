/**
    \file IndicatorsCode/constant_indicator.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/constant_indicator.hpp"

namespace HFSAT {

void ConstantIndicator::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                          std::vector<std::string>& _ors_source_needed_vec_,
                                          const std::vector<const char*>& r_tokens_) {}

ConstantIndicator* ConstantIndicator::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                        const std::vector<const char*>& r_tokens_,
                                                        PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _sec_market_view_ _price_type_
  if (r_tokens_.size() < 4) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_, "INDICATOR weight ConstantIndicator _value_ ");
  }
  return GetUniqueInstance(t_dbglogger_, r_watch_, atof(r_tokens_[3]));
}

ConstantIndicator* ConstantIndicator::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                        double _value_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _value_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, ConstantIndicator*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new ConstantIndicator(t_dbglogger_, r_watch_, concise_indicator_description_, _value_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

ConstantIndicator::ConstantIndicator(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                     const std::string& concise_indicator_description_, double _value_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_), value_(_value_) {
  InitializeValues();
  watch_.subscribe_BigTimePeriod(this);
}

void ConstantIndicator::OnTimePeriodUpdate(const int num_pages_to_add_) { NotifyIndicatorListeners(indicator_value_); }

void ConstantIndicator::WhyNotReady() {
  if (!is_ready_) {
    is_ready_ = true;
  }
}

void ConstantIndicator::InitializeValues() {
  is_ready_ = true;
  indicator_value_ = value_;
}
}
