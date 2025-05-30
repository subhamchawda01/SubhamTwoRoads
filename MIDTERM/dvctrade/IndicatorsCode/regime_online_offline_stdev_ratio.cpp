/**
    \file IndicatorsCode/regime_online_offline_stedv_ratio.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/regime_online_offline_stdev_ratio.hpp"

namespace HFSAT {

void RegimeOnlineOfflineStdevRatio::CollectShortCodes(std::vector<std::string>& r_shortcodes_affecting_this_indicator_,
                                                      std::vector<std::string>& r_ors_source_needed_vec_,
                                                      const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(r_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  std::string t_source_shortcode_ = (std::string)r_tokens_[4];
  if (IndicatorUtil::IsPortfolioShortcode(t_source_shortcode_)) {
    IndicatorUtil::AddPortfolioShortCodeVec(t_source_shortcode_, r_shortcodes_affecting_this_indicator_);
  } else {
    VectorUtils::UniqueVectorAdd(r_shortcodes_affecting_this_indicator_, t_source_shortcode_);
  }
}

RegimeOnlineOfflineStdevRatio* RegimeOnlineOfflineStdevRatio::GetUniqueInstance(
    DebugLogger& r_dbglogger_, const Watch& r_watch_, const std::vector<const char*>& r_tokens_,
    PriceType_t _basepx_pxtype_) {
  if (r_tokens_.size() < 8) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, r_dbglogger_,
                "INDICATOR weight RegimeOnlineOfflineStdevRatio _dep_market_view_ _source_code_ _seconds_ _threshold_ "
                "_tolerance_ ");
  }

  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(r_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           (std::string)r_tokens_[4], atof(r_tokens_[5]), atof(r_tokens_[6]), atof(r_tokens_[7]));
}

RegimeOnlineOfflineStdevRatio* RegimeOnlineOfflineStdevRatio::GetUniqueInstance(
    DebugLogger& r_dbglogger_, const Watch& r_watch_, SecurityMarketView& r_dep_market_view_,
    std::string r_source_shortcode_, double t_secs_, double t_threshold_, double t_tolerance_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << r_dep_market_view_.secname() << ' ' << r_source_shortcode_ << ' ' << t_secs_ << ' '
              << t_threshold_ << ' ' << t_tolerance_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, RegimeOnlineOfflineStdevRatio*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new RegimeOnlineOfflineStdevRatio(r_dbglogger_, r_watch_, concise_indicator_description_, r_dep_market_view_,
                                          r_source_shortcode_, t_secs_, t_threshold_, t_tolerance_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

RegimeOnlineOfflineStdevRatio::RegimeOnlineOfflineStdevRatio(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                             const std::string& concise_indicator_description_,
                                                             SecurityMarketView& r_dep_market_view_,
                                                             std::string r_source_shortcode_, double t_secs_,
                                                             double t_threshold_, double t_tolerance_)
    : IndicatorListener(),
      CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(r_dep_market_view_),
      source_shortcode_(r_source_shortcode_),
      threshold_(t_threshold_),
      tolerance_(t_tolerance_),
      is_ready_(false) {
  p_stdev_ratio_calculator_ =
      StdevRatioNormalised::GetUniqueInstance(t_dbglogger_, r_watch_, dep_market_view_, r_source_shortcode_, t_secs_);
  p_stdev_ratio_calculator_->add_unweighted_indicator_listener(0, this);
}

void RegimeOnlineOfflineStdevRatio::WhyNotReady() {
  if (!is_ready_) {
    if (!(dep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << dep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void RegimeOnlineOfflineStdevRatio::OnIndicatorUpdate(const unsigned int& _indicator_index_,
                                                      const double& _new_value_) {
  // 1 we want to use model file with which has less/no indep's indicators
  // 2 we want to use model file with decent/high indep's indicators
  if (!is_ready_) {
    InitializeValues();
    is_ready_ = true;
  } else {
    if (indicator_value_ == 1 && _new_value_ >= threshold_ + tolerance_) {
      indicator_value_ = 2;
    } else if (indicator_value_ == 2 && _new_value_ <= threshold_ - tolerance_) {
      indicator_value_ = 1;
    }
    NotifyIndicatorListeners(indicator_value_);
  }
}

void RegimeOnlineOfflineStdevRatio::InitializeValues() { indicator_value_ = 1; }
}
