/**
    \file IndicatorsCode/regime_slow_stdev_2.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#include "dvccode/CommonDataStructures/vector_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/regime_slow_stdev_2.hpp"

namespace HFSAT {

void RegimeSlowStdev2::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                         std::vector<std::string>& _ors_source_needed_vec_,
                                         const std::vector<const char*>& r_tokens_) {
  if (IndicatorUtil::IsPortfolioShortcode(r_tokens_[3])) {
    IndicatorUtil::AddPortfolioShortCodeVec((std::string)r_tokens_[3], _shortcodes_affecting_this_indicator_);
  } else {
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  }
}

RegimeSlowStdev2* RegimeSlowStdev2::GetUniqueInstance(DebugLogger& t_dbglogger, const Watch& r_watch,
                                                      const std::vector<const char*>& r_tokens_,
                                                      PriceType_t _basepx_pxtype_) {
  if (r_tokens_.size() >= 8) {
    if (r_tokens_.size() >= 9) {
      return GetUniqueInstance(t_dbglogger, r_watch, r_tokens_[3], atof(r_tokens_[4]), atof(r_tokens_[5]),
                               atof(r_tokens_[6]), atof(r_tokens_[7]), atoi(r_tokens_[8]) != 0);
    } else {
      return GetUniqueInstance(t_dbglogger, r_watch, r_tokens_[3], atof(r_tokens_[4]), atof(r_tokens_[5]),
                               atof(r_tokens_[6]), atof(r_tokens_[7]), false);
    }

  } else {
    ExitVerbose(
        kExitErrorCodeGeneral,
        "insufficient inputs to RegimeSlowStdev2 : INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ "
        "_short_duration_ _long_duration_ tolerance normalize_hist_stdev \n");
    return nullptr;  // wont reach here , just to remove warning
  }
}

RegimeSlowStdev2* RegimeSlowStdev2::GetUniqueInstance(DebugLogger& t_dbglogger, const Watch& r_watch,
                                                      std::string indep_name, double st_fractional_secs,
                                                      double lt_fractional_secs, double switch_threshold,
                                                      double tolerance, bool normalize_stdev) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << indep_name << ' ' << st_fractional_secs << ' ' << lt_fractional_secs << ' '
              << switch_threshold << ' ' << tolerance << ' ' << normalize_stdev;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, RegimeSlowStdev2*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new RegimeSlowStdev2(t_dbglogger, r_watch, concise_indicator_description_, indep_name, st_fractional_secs,
                             lt_fractional_secs, switch_threshold, tolerance, normalize_stdev);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

RegimeSlowStdev2::RegimeSlowStdev2(DebugLogger& t_dbglogger, const Watch& r_watch,
                                   const std::string& t_concise_indicator_description_, std::string indep_name,
                                   double st_fractional_secs, double lt_fractional_secs, double switch_threshold,
                                   double tolerance, bool normalize_stdev)
    : IndicatorListener(),
      CommonIndicator(t_dbglogger, r_watch, t_concise_indicator_description_),
      indep_market_view_(nullptr),
      p_st_slow_stdev_calculator_(nullptr),
      p_lt_slow_stdev_calculator_(nullptr),
      p_st_slow_stdev_calculator_port_(nullptr),
      p_lt_slow_stdev_calculator_port_(nullptr),
      indep_name_(indep_name),
      switch_threshold_(switch_threshold),
      lt_st_factor_(1.00),
      st_volatile_(true),
      lt_volatile_(false),
      avg_stdev_(0.0),
      tolerance_(tolerance) {
  ///
  ///
  if (IndicatorUtil::IsPortfolioShortcode(indep_name)) {
    // if input is portfolio
    avg_stdev_ = 0.0;

    // compute stdev of the portfolio
    const EigenConstituentsVec& eigen_constituent_vec =
        (PcaWeightsManager::GetUniqueInstance()).GetEigenConstituentVec(indep_name_);

    // get the constituent vector and get stdev from sample data
    std::vector<std::string> constituent_vec;
    IndicatorUtil::AddPortfolioShortCodeVec(indep_name_, constituent_vec);

    std::vector<double> stdev_vector(constituent_vec.size());
    for (auto i = 0u; i < constituent_vec.size(); i++) {
      stdev_vector[i] = SampleDataUtil::GetAvgForPeriod(constituent_vec[i], watch_.YYYYMMDD(), 60, "STDEV");
    }
    // std::vector<double> stdev_vector = PcaWeightsManager::GetUniqueInstance().GetPortfolioStdevs(indep_name_);

    for (auto id = 0u; id < stdev_vector.size(); id++) {
      DBGLOG_TIME_CLASS_FUNC << " prod: " << constituent_vec[id] << " stdev: " << stdev_vector[id]
                             << " wt: " << eigen_constituent_vec[0].eigenvec_components_[id] << DBGLOG_ENDL_FLUSH;
      avg_stdev_ += eigen_constituent_vec[0].eigenvec_components_[id] * stdev_vector[id];
    }

    p_st_slow_stdev_calculator_port_ =
        SlowStdevCalculatorPort::GetUniqueInstance(t_dbglogger, r_watch, indep_name_, st_fractional_secs * 1000u, 0);
    p_st_slow_stdev_calculator_port_->add_unweighted_indicator_listener(0, this);

    p_lt_slow_stdev_calculator_port_ =
        SlowStdevCalculatorPort::GetUniqueInstance(t_dbglogger, r_watch, indep_name_, lt_fractional_secs * 1000u, 0);
    p_lt_slow_stdev_calculator_port_->add_unweighted_indicator_listener(1, this);
  } else {
    // individual security

    ShortcodeSecurityMarketViewMap::StaticCheckValid(indep_name_);
    avg_stdev_ = SampleDataUtil::GetAvgForPeriod(indep_name_, watch_.YYYYMMDD(), 60, "STDEV");

    indep_market_view_ = ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(indep_name_);
    SecurityMarketView& smv = *indep_market_view_;

    p_st_slow_stdev_calculator_ =
        SlowStdevCalculator::GetUniqueInstance(t_dbglogger, r_watch, smv.shortcode(), st_fractional_secs * 1000u, 0);
    p_st_slow_stdev_calculator_->add_unweighted_indicator_listener(0, this);

    p_lt_slow_stdev_calculator_ =
        SlowStdevCalculator::GetUniqueInstance(t_dbglogger, r_watch, smv.shortcode(), lt_fractional_secs * 1000u, 0);
    p_lt_slow_stdev_calculator_->add_unweighted_indicator_listener(1, this);
  }

  if (normalize_stdev) {
    double factor = sqrt(st_fractional_secs / 300.0);
    avg_stdev_ *= factor;
  }

  DBGLOG_TIME_CLASS_FUNC << " indep: " << indep_name_ << " stdev: " << avg_stdev_ << DBGLOG_ENDL_FLUSH;

  lt_st_factor_ = sqrt(lt_fractional_secs / st_fractional_secs);
}

void RegimeSlowStdev2::WhyNotReady() {
  if (!is_ready_) {
    if (p_st_slow_stdev_calculator_ != nullptr && !p_st_slow_stdev_calculator_->IsIndicatorReady()) {
      DBGLOG_TIME_CLASS << p_lt_slow_stdev_calculator_->concise_indicator_description() << " Not Ready "
                        << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    } else if (p_st_slow_stdev_calculator_port_ != nullptr && !p_st_slow_stdev_calculator_port_->IsIndicatorReady()) {
      DBGLOG_TIME_CLASS << p_lt_slow_stdev_calculator_port_->concise_indicator_description() << " Not Ready "
                        << DBGLOG_ENDL_FLUSH;
    }
  }
}

void RegimeSlowStdev2::OnIndicatorUpdate(const unsigned int& indicator_index, const double& new_value) {
  if (!is_ready_) {
    is_ready_ = true;
    indicator_value_ = 1;
    // NotifyIndicatorListeners ( 1 ) ;
  } else {
    if (indicator_index == 0) {
      if (!st_volatile_ && std::abs(new_value) > (1 + tolerance_) * switch_threshold_ * avg_stdev_) {
        st_volatile_ = true;
      } else if (st_volatile_ && std::abs(new_value) < (1 - tolerance_) * switch_threshold_ * avg_stdev_) {
        st_volatile_ = false;
      }
    } else if (indicator_index == 1) {
      if (!lt_volatile_ && std::abs(new_value) > (1 + tolerance_) * switch_threshold_ * lt_st_factor_ * avg_stdev_) {
        lt_volatile_ = true;
      } else if (lt_volatile_ &&
                 std::abs(new_value) < (1 - tolerance_) * switch_threshold_ * lt_st_factor_ * avg_stdev_) {
        lt_volatile_ = false;
      }
    }

    indicator_value_ = 1;

    if (lt_volatile_ || st_volatile_) {
      indicator_value_ = 2;
    }

    NotifyIndicatorListeners(indicator_value_);
  }
}

void RegimeSlowStdev2::OnMarketDataInterrupted(const unsigned int security_id, const int msecs_since_last_receive) {
  if (indep_market_view_ != nullptr && indep_market_view_->security_id() == security_id) {
    data_interrupted_ = true;
    indicator_value_ = 1;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void RegimeSlowStdev2::OnMarketDataResumed(const unsigned int security_id) {
  if (indep_market_view_ != nullptr && indep_market_view_->security_id() == security_id) {
    data_interrupted_ = false;
  } else
    return;
}
}
