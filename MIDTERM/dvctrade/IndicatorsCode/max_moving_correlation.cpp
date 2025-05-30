/**
    \file IndicatorsCode/online_computed_pairs.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/max_moving_correlation.hpp"

namespace HFSAT {

void MaxMovingCorrelation::CollectShortCodes(std::vector<std::string> &_shortcodes_affecting_this_indicator_,
                                             std::vector<std::string> &_ors_source_needed_vec_,
                                             const std::vector<const char *> &r_tokens_) {
  for (unsigned int i = 3; i < r_tokens_.size() - 1; i++) {
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[i]);
  }
}

MaxMovingCorrelation *MaxMovingCorrelation::GetUniqueInstance(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                                                              const std::vector<const char *> &r_tokens_,
                                                              PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ _indep_market_view_ _fractional_seconds_ _price_type_
  std::vector<SecurityMarketView *> t_indep_market_view_vec_;
  for (unsigned int i = 4; i < r_tokens_.size() - 1; i++) {
    t_indep_market_view_vec_.push_back(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[i]));
  }

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           t_indep_market_view_vec_, atof(r_tokens_[r_tokens_.size() - 1]));
}

MaxMovingCorrelation *MaxMovingCorrelation::GetUniqueInstance(
    DebugLogger &t_dbglogger_, const Watch &r_watch_, SecurityMarketView &_dep_market_view_,
    std::vector<SecurityMarketView *> &_indep_market_view_vec_, double _fractional_seconds_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ';
  for (auto i = 0u; i < _indep_market_view_vec_.size(); i++) {
    t_temp_oss_ << _indep_market_view_vec_[i]->secname() << ' ';
  }
  t_temp_oss_ << _fractional_seconds_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, MaxMovingCorrelation *> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new MaxMovingCorrelation(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_,
                                 _indep_market_view_vec_, _fractional_seconds_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

MaxMovingCorrelation::MaxMovingCorrelation(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                                           const std::string &concise_indicator_description_,
                                           SecurityMarketView &_dep_market_view_,
                                           std::vector<SecurityMarketView *> &_indep_market_view_vec_,
                                           double _fractional_seconds_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      indep_market_view_vec_(),
      indep_corr_vec_(),
      max_indep_corr_idx_(0) {
  for (auto i = 0u; i < _indep_market_view_vec_.size(); i++) {
    indep_market_view_vec_.push_back(_indep_market_view_vec_[i]);
    SlowCorrCalculator *this_ind_ = SlowCorrCalculator::GetUniqueInstance(
        t_dbglogger_, r_watch_, dep_market_view_, *indep_market_view_vec_[i], _fractional_seconds_);
    slow_corr_calculator_vec_.push_back(this_ind_);
    slow_corr_calculator_vec_[i]->add_unweighted_indicator_listener(i, this);
    indep_corr_vec_.push_back(0);
  }
}

void MaxMovingCorrelation::OnIndicatorUpdate(const unsigned int &_indicator_index_, const double &_new_value_) {
  if (_indicator_index_ >= 0 && _indicator_index_ < indep_market_view_vec_.size()) {
    indep_corr_vec_[_indicator_index_] = _new_value_;
    if (_new_value_ > indep_corr_vec_[max_indep_corr_idx_]) {
      max_indep_corr_idx_ = _indicator_index_;
      indicator_value_ = max_indep_corr_idx_ + 1;
      NotifyIndicatorListeners(indicator_value_);
    }
  }
}

void MaxMovingCorrelation::InitializeValues() { indicator_value_ = 0; }

void MaxMovingCorrelation::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                   const int msecs_since_last_receive_) {
  // not comparing with dep shortcode as in that case trading has to stop anyways
  if (dep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  } else
    return;
}

void MaxMovingCorrelation::OnMarketDataResumed(const unsigned int _security_id_) {
  if (dep_market_view_.security_id() == _security_id_) {
    InitializeValues();
    data_interrupted_ = false;
  } else
    return;
}
}
