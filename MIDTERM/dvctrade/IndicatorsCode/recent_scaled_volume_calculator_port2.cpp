/**
    \file IndicatorsCode/volume_ratio_calculator_code.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include <fstream>
#include <strings.h>
#include <stdlib.h>
#include <sstream>

#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"

#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"

#include "dvctrade/Indicators/recent_scaled_volume_calculator_port2.hpp"

namespace HFSAT {

void RecentScaledVolumeCalculatorPort2::CollectShortCodes(std::vector<std::string>& shortcodes_affecting_this_indicator,
                                                          std::vector<std::string>& ors_source_needed_vec,
                                                          const std::vector<const char*>& r_tokens) {
  std::vector<std::string> shortcode_vec;
  IndicatorUtil::GetPortfolioShortCodeVec(r_tokens[3], shortcode_vec);
  VectorUtils::UniqueVectorAdd(shortcodes_affecting_this_indicator, shortcode_vec);
}

RecentScaledVolumeCalculatorPort2* RecentScaledVolumeCalculatorPort2::GetUniqueInstance(
    DebugLogger& t_dbglogger, const Watch& r_watch, const std::vector<const char*>& r_tokens,
    PriceType_t basepx_pxtype) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _duration_
  if (r_tokens.size() < 5) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger,
                "VolumeRatioCalculator incorrect syntax. Should be INDICATOR _this_weight_ _indicator_string_ "
                "_indep_market_view_ _fractional_seconds_ ");
  }
  return GetUniqueInstance(t_dbglogger, r_watch, r_tokens[3], atof(r_tokens[4]));
}

RecentScaledVolumeCalculatorPort2* RecentScaledVolumeCalculatorPort2::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::string& t_portfolio_descriptor_shortcode_,
    double _fractional_seconds_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << t_portfolio_descriptor_shortcode_ << ' ' << _fractional_seconds_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, RecentScaledVolumeCalculatorPort2*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new RecentScaledVolumeCalculatorPort2(t_dbglogger_, r_watch_, concise_indicator_description_,
                                              t_portfolio_descriptor_shortcode_, _fractional_seconds_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

RecentScaledVolumeCalculatorPort2::RecentScaledVolumeCalculatorPort2(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::string& t_concise_indicator_description_,
    const std::string& t_portfolio_descriptor_shortcode_, double _fractional_seconds_)
    : CommonIndicator(t_dbglogger_, r_watch_, t_concise_indicator_description_),
      indep_market_view_vec_(),
      volume_ratio_(1.0),
      volume_ratio_vec_(),
      volume_ratio_calculator_vec_(),
      volume_ratio_listener_vec_() {
  std::vector<std::string> shortcode_vec_;
  IndicatorUtil::GetPortfolioShortCodeVec(t_portfolio_descriptor_shortcode_, shortcode_vec_);

  (ShortcodeSecurityMarketViewMap::GetUniqueInstance())
      .GetSecurityMarketViewVec(shortcode_vec_, indep_market_view_vec_);

  for (unsigned int ii = 0u; ii < shortcode_vec_.size(); ii++) {
    SecurityMarketView* p_this_indep_market_view_ = indep_market_view_vec_[ii];
    if (p_this_indep_market_view_ == NULL) {
      std::cerr << "p_this_indep_market_view_ NULL in recent_volume_ratio_calculator_port.cpp constructor for "
                << shortcode_vec_[ii] << std::endl;
      ExitVerbose(kExitErrorCodeGeneral,
                  "p_this_indep_market_view_ NULL in recent_volume_ratio_calculator_port.cpp constructor");
    }

    auto this_array_index_ = volume_ratio_vec_.size();
    volume_ratio_vec_.push_back(1.0);  // default value

    volume_ratio_calculator_vec_.push_back(RecentScaledVolumeCalculator::GetUniqueInstance(
        t_dbglogger_, r_watch_, *p_this_indep_market_view_, _fractional_seconds_));
    if (volume_ratio_calculator_vec_.size() != this_array_index_ + 1) {
      std::cerr << "Some error in check volume_ratio_calculator_vec_.size() != this_array_index_+ 1 in "
                   "recent_volume_ratio_calculator_port.cpp constructor"
                << std::endl;
    }
    volume_ratio_calculator_vec_[this_array_index_]->AddRecentScaledVolumeListener(this_array_index_, this);
  }
  is_ready_ = false;
  NotifyListeners();
}

void RecentScaledVolumeCalculatorPort2::OnScaledVolumeUpdate(const unsigned int array_index,
                                                             const double& r_new_scaled_volume_value) {
  if (!is_ready_) {
    bool t_is_ready = true;
    for (auto smv : indep_market_view_vec_) {
      if (smv) {
        t_is_ready = t_is_ready && smv->is_ready();
      }
    }
    is_ready_ = t_is_ready;
  }

  if (array_index < volume_ratio_vec_.size())  // should be always true
  {
    volume_ratio_vec_[array_index] = r_new_scaled_volume_value;
    volume_ratio_ = 0;
    for (auto ii = 0u; ii < volume_ratio_vec_.size(); ii++) {
      volume_ratio_ += volume_ratio_vec_[ii];
    }
    volume_ratio_ /= volume_ratio_vec_.size();
    indicator_value_ = volume_ratio_;
  }

  if (is_ready_) {
    NotifyListeners();
    NotifyIndicatorListeners(indicator_value_);
  }
}
}
