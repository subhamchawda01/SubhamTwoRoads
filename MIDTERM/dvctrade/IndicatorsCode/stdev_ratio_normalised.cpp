/**
    \file IndicatorsCode/stdev_ratio_normalised.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/stdev_ratio_normalised.hpp"

namespace HFSAT {

void StdevRatioNormalised::CollectShortCodes(std::vector<std::string>& r_shortcodes_affecting_this_indicator_,
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

StdevRatioNormalised* StdevRatioNormalised::GetUniqueInstance(DebugLogger& r_dbglogger_, const Watch& r_watch_,
                                                              const std::vector<const char*>& r_tokens_,
                                                              PriceType_t _basepx_pxtype_) {
  if (r_tokens_.size() < 6) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, r_dbglogger_,
                "INDICATOR weight StdevRatioNormalised _dep_market_view_ _source_code_ _stdev_duration_ ");
  }
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);

  return GetUniqueInstance(r_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           (std::string)r_tokens_[4], atof(r_tokens_[5]));
}

StdevRatioNormalised* StdevRatioNormalised::GetUniqueInstance(DebugLogger& r_dbglogger_, const Watch& r_watch_,
                                                              SecurityMarketView& r_dep_market_view_,
                                                              std::string r_source_shortcode_,
                                                              double _stdev_duration_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << r_dep_market_view_.secname() << ' ' << r_source_shortcode_ << ' '
              << _stdev_duration_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, StdevRatioNormalised*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new StdevRatioNormalised(r_dbglogger_, r_watch_, concise_indicator_description_, r_dep_market_view_,
                                 r_source_shortcode_, _stdev_duration_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

StdevRatioNormalised::StdevRatioNormalised(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                           const std::string& concise_indicator_description_,
                                           SecurityMarketView& r_dep_market_view_, std::string r_source_shortcode_,
                                           double _stdev_duration_)
    : IndicatorListener(),
      CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(r_dep_market_view_),
      source_shortcode_(r_source_shortcode_),
      lrdb_(OfflineReturnsLRDB::GetUniqueInstance(t_dbglogger_, r_watch_, r_dep_market_view_.shortcode())),
      current_lrinfo_(0.0, 0.0),
      last_lrinfo_updated_msecs_(0),
      dep_stdev_(0.0),
      indep_stdev_(0.0),
      stdev_indep_dep_ratio_(1.0),
      is_ready_(false),
      lrdb_absent_(false),
      dep_updated_(false),
      indep_updated_(false) {
  watch_.subscribe_BigTimePeriod(this);  // for lrdb beta adjustment
  if (!(lrdb_.LRCoeffPresent(dep_market_view_.shortcode(), r_source_shortcode_))) {
    lrdb_absent_ = true;
  }
  if (lrdb_absent_) {
    indicator_value_ = 1;
  } else {
    // taking trend of 300 secs as we compute offline beta for 300 sec trend duration // similarly MktSizeWPrice
    // price subscription in done in simple trend itself
    dep_stdev_trend_calculator_ =
        SlowStdevTrendCalculator::GetUniqueInstance(t_dbglogger_, r_watch_, dep_market_view_.shortcode(),
                                                    _stdev_duration_, 300, StringToPriceType_t("MktSizeWPrice"));
    indep_stdev_trend_calculator_ = SlowStdevTrendCalculator::GetUniqueInstance(
        t_dbglogger_, r_watch_, r_source_shortcode_, _stdev_duration_, 300, StringToPriceType_t("MktSizeWPrice"));
    dep_stdev_trend_calculator_->add_unweighted_indicator_listener(0, this);
    indep_stdev_trend_calculator_->add_unweighted_indicator_listener(1, this);
  }
}

void StdevRatioNormalised::WhyNotReady() {
  if (!is_ready_) {
    if (!(dep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << dep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void StdevRatioNormalised::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (!is_ready_) {
    if (!lrdb_absent_) {
      InitializeValues();
      is_ready_ = true;
    }
  } else {
    if (_indicator_index_ == 0 && !dep_updated_) {
      dep_updated_ = true;
      return;
    } else if (_indicator_index_ == 1 && !indep_updated_) {
      indep_updated_ = true;
      return;
    } else if (dep_updated_ && indep_updated_) {
      if (_indicator_index_ == 0) {
        dep_stdev_ = _new_value_;
      } else if (_indicator_index_ == 1) {
        indep_stdev_ = _new_value_;
      }
      if (indep_stdev_ * OfflineStdevRatio() != 0) {
        stdev_indep_dep_ratio_ = dep_stdev_ / (indep_stdev_ * OfflineStdevRatio());
        indicator_value_ = stdev_indep_dep_ratio_;
        // std::cout << "stdev "<< dep_stdev_ << " " << indep_stdev_ << " " << OfflineStdevRatio() << " " <<
        // indicator_value_ << std::endl ;
        NotifyIndicatorListeners(indicator_value_);
      }
    }
  }
}

void StdevRatioNormalised::InitializeValues() {
  indicator_value_ = 1;
  UpdateLRInfo();
}

void StdevRatioNormalised::UpdateLRInfo() {
  if ((last_lrinfo_updated_msecs_ == 0) ||
      (watch_.msecs_from_midnight() - last_lrinfo_updated_msecs_ > TENMINUTESMSECS)) {
    current_lrinfo_ = lrdb_.GetLRCoeff(dep_market_view_.shortcode(), source_shortcode_);

    if (dbglogger_.CheckLoggingLevel(LRDB_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "lrinfo ( " << dep_market_view_.shortcode() << ", " << source_shortcode_ << " ) "
                             << current_lrinfo_.lr_coeff_ << ' ' << current_lrinfo_.lr_correlation_
                             << DBGLOG_ENDL_FLUSH;
    }
    last_lrinfo_updated_msecs_ = watch_.msecs_from_midnight() - (watch_.msecs_from_midnight() % TENMINUTESMSECS);
  }
}
void StdevRatioNormalised::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                   const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  indicator_value_ = 1;
  NotifyIndicatorListeners(indicator_value_);
}

void StdevRatioNormalised::OnMarketDataResumed(const unsigned int _security_id_) {
  InitializeValues();
  data_interrupted_ = false;
}
}
