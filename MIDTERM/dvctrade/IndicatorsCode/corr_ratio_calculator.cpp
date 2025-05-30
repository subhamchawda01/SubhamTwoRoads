/**
   \file IndicatorsCode/corr_ratio_calculator.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
    Address:
        Suite No 353, Evoma, #14, Bhattarhalli,
        Old Madras Road, Near Garden City College,
        KR Puram, Bangalore 560049, India
        +91 80 4190 3551
*/
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/corr_ratio_calculator.hpp"

namespace HFSAT {

void CorrRatioCalculator::CollectShortCodes(std::vector<std::string>& r_shortcodes_affecting_this_indicator_,
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

CorrRatioCalculator* CorrRatioCalculator::GetUniqueInstance(DebugLogger& r_dbglogger_, const Watch& r_watch_,
                                                            const std::vector<const char*>& r_tokens_,
                                                            PriceType_t _basepx_pxtype_) {
  if (r_tokens_.size() < 6) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, r_dbglogger_,
                "INDICATOR weight CorrRatioCalculator _dep_market_view_ _source_code_ _beta_secs_  ");
  }

  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  // ShortcodeSecurityMarketViewMap::StaticCheckValid ( r_tokens_[ 4 ] );

  return GetUniqueInstance(r_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           (std::string)r_tokens_[4], atof(r_tokens_[5]));
}

CorrRatioCalculator* CorrRatioCalculator::GetUniqueInstance(DebugLogger& r_dbglogger_, const Watch& r_watch_,
                                                            SecurityMarketView& r_dep_market_view_,
                                                            std::string r_source_shortcode_,
                                                            const unsigned int _beta_secs_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << r_dep_market_view_.secname() << ' ' << r_source_shortcode_ << ' ' << _beta_secs_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, CorrRatioCalculator*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new CorrRatioCalculator(
        r_dbglogger_, r_watch_, concise_indicator_description_, r_dep_market_view_, r_source_shortcode_, _beta_secs_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

CorrRatioCalculator::CorrRatioCalculator(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                         const std::string& concise_indicator_description_,
                                         SecurityMarketView& r_dep_market_view_, std::string r_source_shortcode_,
                                         const unsigned int _beta_secs_)
    : IndicatorListener(),
      CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(r_dep_market_view_),
      source_shortcode_(r_source_shortcode_),
      lrdb_(OfflineReturnsLRDB::GetUniqueInstance(t_dbglogger_, r_watch_, r_dep_market_view_.shortcode())),
      current_lrinfo_(0.0, 0.0),
      last_lrinfo_updated_msecs_(0),
      online_beta_(0.0),
      online_offline_stdev_ratio_(1.0),
      beta_updated_(false),
      stdev_ratio_updated_(false) {
  watch_.subscribe_BigTimePeriod(this);  // for lrdb beta adjustment
  if (!(lrdb_.LRCoeffPresent(dep_market_view_.shortcode(), r_source_shortcode_))) {
    lrdb_absent_ = true;
  }
  if (lrdb_absent_) {
    indicator_value_ = 1;
    is_ready_ = true;
  } else {
    // taking trend of 300 secs as we compute offline beta for 300 sec trend duration // similarly MktSizeWPrice
    // price subscription in done in simple trend itself
    p_online_beta_calculator_ =
        OnlineBetaTrend::GetUniqueInstance(t_dbglogger_, r_watch_, dep_market_view_, r_source_shortcode_, 300,
                                           _beta_secs_, StringToPriceType_t("MktSizeWPrice"));
    p_online_beta_calculator_->add_unweighted_indicator_listener(0, this);

    p_online_offline_stdev_ratio_ = StdevRatioNormalised::GetUniqueInstance(t_dbglogger_, r_watch_, dep_market_view_,
                                                                            r_source_shortcode_, _beta_secs_);
    p_online_offline_stdev_ratio_->add_unweighted_indicator_listener(1, this);
  }
}

void CorrRatioCalculator::WhyNotReady() {
  if (!is_ready_) {
    if (!(dep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << dep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void CorrRatioCalculator::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  // 1 we want to use model file with which has less/no indep's indicators
  // 2 we want to use model file with decent/high indep's indicators
  if (_indicator_index_ == 0) {
    online_beta_ = _new_value_;
    beta_updated_ = true;
  } else {
    online_offline_stdev_ratio_ = _new_value_;
    stdev_ratio_updated_ = true;
  }

  if (beta_updated_ && stdev_ratio_updated_) {
    if (!is_ready_) {
      InitializeValues();
      is_ready_ = true;
    } else if (!lrdb_absent_) {
      double _beta_ratio_ = online_beta_ / OfflineBeta();
      double corr_ratio_ = _beta_ratio_ / online_offline_stdev_ratio_;
      indicator_value_ = corr_ratio_;
      NotifyIndicatorListeners(indicator_value_);
    }
  }
}

void CorrRatioCalculator::InitializeValues() {
  indicator_value_ = 1;
  UpdateLRInfo();
  online_beta_ = OfflineBeta();
  online_offline_stdev_ratio_ = 1.0;
}

void CorrRatioCalculator::UpdateLRInfo() {
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
}
