/**
    \file IndicatorsCode/self_position.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/risk_indicator_util.hpp"
#include "dvctrade/Indicators/self_position.hpp"

namespace HFSAT {

void SelfPosition::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                     std::vector<std::string>& _ors_source_needed_vec_,
                                     const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_ors_source_needed_vec_, (std::string)r_tokens_[3]);
}

SelfPosition* SelfPosition::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                              const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_prom_order_manager_shortcode_
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodePromOrderManagerMap::StaticGetPromOrderManager(r_tokens_[3])), _basepx_pxtype_);
}

SelfPosition* SelfPosition::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                              PromOrderManager& _indep_prom_order_manager_,
                                              PriceType_t _basepx_pxtype_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_prom_order_manager_.secname() << " "
              << PriceType_t_To_String(_basepx_pxtype_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, SelfPosition*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new SelfPosition(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_prom_order_manager_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

SelfPosition::SelfPosition(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                           const std::string& concise_indicator_description_,
                           PromOrderManager& _indep_prom_order_manager_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_) {
  _indep_prom_order_manager_.AddGlobalPositionChangeListener(this);
}

void SelfPosition::OnGlobalPositionChange(const unsigned int _security_id_, int _new_global_position_) {
  if (!is_ready_) {
    is_ready_ = true;
    indicator_value_ = 0;
  } else {
    indicator_value_ = _new_global_position_;

    NotifyIndicatorListeners(indicator_value_);
  }
}
}
