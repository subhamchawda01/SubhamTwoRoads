/**
    \file IndicatorsCode/online_offline_corr_diff_base_regime.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CommonDataStructures/vector_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/online_offline_corr_diff_base_regime.hpp"

namespace HFSAT {

void OnlineOfflineCorrDiffBaseRegime::CollectShortCodes(
    std::vector<std::string>& r_shortcodes_affecting_this_indicator_,
    std::vector<std::string>& r_ors_source_needed_vec_, const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(r_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(r_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
}

OnlineOfflineCorrDiffBaseRegime* OnlineOfflineCorrDiffBaseRegime::GetUniqueInstance(
    DebugLogger& r_dbglogger_, const Watch& r_watch_, const std::vector<const char*>& r_tokens_,
    PriceType_t _basepx_pxtype_) {
  if (r_tokens_.size() < 7) {
    ExitVerbose(
        kModelCreationIndicatorLineLessArgs, r_dbglogger_,
        "INDICATOR weight OnlineOfflineCorrDiffBaseRegime _dep_market_view_ _indep_market_view_ _seconds_ _epsilon_ ");
  }

  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);

  return GetUniqueInstance(r_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           atof(r_tokens_[5]), atof(r_tokens_[6]));
}

OnlineOfflineCorrDiffBaseRegime* OnlineOfflineCorrDiffBaseRegime::GetUniqueInstance(
    DebugLogger& r_dbglogger_, const Watch& r_watch_, SecurityMarketView& r_dep_market_view_,
    SecurityMarketView& r_indep_market_view_, double t_secs_, double t_epsilon_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << r_dep_market_view_.secname() << ' ' << r_indep_market_view_.secname() << ' '
              << t_secs_ << ' ' << t_epsilon_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, OnlineOfflineCorrDiffBaseRegime*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new OnlineOfflineCorrDiffBaseRegime(r_dbglogger_, r_watch_, concise_indicator_description_, r_dep_market_view_,
                                            r_indep_market_view_, t_secs_, t_epsilon_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

OnlineOfflineCorrDiffBaseRegime::OnlineOfflineCorrDiffBaseRegime(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                 const std::string& concise_indicator_description_,
                                                                 SecurityMarketView& r_dep_market_view_,
                                                                 SecurityMarketView& r_indep_market_view_,
                                                                 double t_secs_, double t_epsilon_)
    : IndicatorListener(),
      CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(r_dep_market_view_),
      indep_market_view_(r_indep_market_view_),
      lrdb_(OfflineReturnsLRDB::GetUniqueInstance(t_dbglogger_, r_watch_, r_dep_market_view_.shortcode())),
      current_lrinfo_(0.0, 0.0),
      last_lrinfo_updated_msecs_(0),
      epsilon_(1 + t_epsilon_),  // pre compute
      offpsign_(true)            // not sure
{
  watch_.subscribe_BigTimePeriod(this);  // for lrdb corr adjustment
  bool lrdb_absent_ = false;

  if (dep_market_view_.security_id() == indep_market_view_.security_id()) {
    lrdb_absent_ = true;
  }
  if (!(lrdb_.LRCoeffPresent(dep_market_view_.shortcode(), indep_market_view_.shortcode()))) {
    lrdb_absent_ = true;
  }
  if (lrdb_absent_) {
    indicator_value_ = 1;
    is_ready_ = true;
  } else {
    p_slow_corr_calculator_ =
        SlowCorrCalculator::GetUniqueInstance(t_dbglogger_, r_watch_, dep_market_view_, indep_market_view_, t_secs_);
    // when did we go from OnCorrUpdate to OnIndicatorUpdate ?
    p_slow_corr_calculator_->add_unweighted_indicator_listener(0, this);
  }
}

void OnlineOfflineCorrDiffBaseRegime::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
    if (!(dep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << dep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void OnlineOfflineCorrDiffBaseRegime::OnIndicatorUpdate(const unsigned int& _indicator_index_,
                                                        const double& _new_value_) {
  // 0 we want to use model file with which has less/no indep's indicators
  // 1 we want to use model file with decent/high indep's indicators
  if (!is_ready_) {
    InitializeValues();
    is_ready_ = true;
  } else {
    double abs_online_corr_ = std::fabs(_new_value_);
    if (epsilon_ * abs_online_corr_ - FabsCorrelation() >= 0 && (offpsign_ ? (_new_value_ > 0) : (_new_value_ < 0))) {
      indicator_value_ = 2;
    } else {
      indicator_value_ = 1;
    }
    NotifyIndicatorListeners(indicator_value_);
  }
}

void OnlineOfflineCorrDiffBaseRegime::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                              const int msecs_since_last_receive_) {
  // well if dep is interrupted, hope we stopped the trading.
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 1;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void OnlineOfflineCorrDiffBaseRegime::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
  } else
    return;
}

void OnlineOfflineCorrDiffBaseRegime::InitializeValues() {
  indicator_value_ = 1;
  UpdateLRInfo();
}

void OnlineOfflineCorrDiffBaseRegime::UpdateLRInfo() {
  if ((last_lrinfo_updated_msecs_ == 0) ||
      (watch_.msecs_from_midnight() - last_lrinfo_updated_msecs_ > TENMINUTESMSECS)) {
    current_lrinfo_ = lrdb_.GetLRCoeff(dep_market_view_.shortcode(), indep_market_view_.shortcode());
    offpsign_ = (current_lrinfo_.lr_correlation_ > 0);

    if (dbglogger_.CheckLoggingLevel(LRDB_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "lrinfo ( " << dep_market_view_.shortcode() << ", " << indep_market_view_.shortcode()
                             << " ) " << current_lrinfo_.lr_coeff_ << ' ' << current_lrinfo_.lr_correlation_
                             << DBGLOG_ENDL_FLUSH;
    }
    last_lrinfo_updated_msecs_ = watch_.msecs_from_midnight() - (watch_.msecs_from_midnight() % TENMINUTESMSECS);
  }
}
}
