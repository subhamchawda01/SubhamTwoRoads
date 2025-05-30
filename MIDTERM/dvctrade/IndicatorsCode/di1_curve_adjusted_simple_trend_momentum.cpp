/**
    \file IndicatorsCode/di1_curve_adjusted_simple_trend_momentum.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
//#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/di1_curve_adjusted_simple_trend_momentum.hpp"

namespace HFSAT {

void DI1CurveAdjustedSimpleTrendMomentum::CollectShortCodes(
    std::vector<std::string>& _shortcodes_affecting_this_indicator_, std::vector<std::string>& _ors_source_needed_vec_,
    const std::vector<const char*>& r_tokens_) {
  if (r_tokens_.size() > 7u)  // doesnt mean indicator string syntax is valid .
  {
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);  // dep
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);  // indep1
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[5]);  // indep2
  }
}

DI1CurveAdjustedSimpleTrendMomentum* DI1CurveAdjustedSimpleTrendMomentum::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::vector<const char*>& r_tokens_,
    PriceType_t _base_price_type_) {
  if (r_tokens_.size() < 8u) {
    ExitVerbose(kExitErrorCodeGeneral, "DI1CurveAdjustedSimpleTrendMomentum needs 8 tokens");
    return NULL;
  }

  // INDICATOR 1.00 DI1CurveAdjustedSimpleTrendMomentum DEP INDEP1 INDEP2 HALFLIFE PRICE_TYPE
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);  // dep
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);  // indep1
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[5]);  // indep2

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(
                               r_tokens_[3])),  // dep is not necessary to hold for now but just in case...
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[5])),
                           atoi(r_tokens_[6]), StringToPriceType_t(r_tokens_[7]));
}

DI1CurveAdjustedSimpleTrendMomentum* DI1CurveAdjustedSimpleTrendMomentum::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const SecurityMarketView& _dep_market_view_,
    const SecurityMarketView& _indep1_market_view_, const SecurityMarketView& _indep2_market_view_,
    double _fractional_secs_, PriceType_t t_price_type_) {
  std::ostringstream t_temp_oss_;

  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _indep1_market_view_.secname() << ' '
              << _indep2_market_view_.secname() << ' ' << _fractional_secs_ << ' ' << t_price_type_;

  std::string concise_indicator_description_(t_temp_oss_.str());
  static std::map<std::string, DI1CurveAdjustedSimpleTrendMomentum*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new DI1CurveAdjustedSimpleTrendMomentum(
        t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_, _indep1_market_view_,
        _indep2_market_view_, _fractional_secs_, t_price_type_);
  }

  return concise_indicator_description_map_[concise_indicator_description_];
}

DI1CurveAdjustedSimpleTrendMomentum::DI1CurveAdjustedSimpleTrendMomentum(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::string& concise_indicator_description_,
    const SecurityMarketView& _dep_market_view_, const SecurityMarketView& _indep1_market_view_,
    const SecurityMarketView& _indep2_market_view_, double _fractional_secs_, PriceType_t _price_type_)
    : IndicatorListener(),
      CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      indep1_market_view_(_indep1_market_view_),
      indep2_market_view_(_indep2_market_view_),
      dep_term_(CurveUtils::_get_term_(r_watch_.YYYYMMDD(), _dep_market_view_.secname())),
      indep1_term_(CurveUtils::_get_term_(r_watch_.YYYYMMDD(), _indep1_market_view_.secname())),
      indep2_term_(CurveUtils::_get_term_(r_watch_.YYYYMMDD(), _indep2_market_view_.secname())) {
  double alpha1_ = ((indep2_term_ - dep_term_) / (indep2_term_ - indep1_term_) * (indep1_term_ / dep_term_));
  double alpha2_ = (-(indep1_term_ - dep_term_) / (indep2_term_ - indep1_term_) * (indep2_term_ / dep_term_));

  indep1_trendI_ =
      SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, indep1_market_view_.shortcode(), _fractional_secs_, _price_type_);
  indep2_trendI_ =
      SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, indep2_market_view_.shortcode(), _fractional_secs_, _price_type_);

  indep1_trendI_->add_indicator_listener(0, this, alpha1_);
  indep2_trendI_->add_indicator_listener(1, this, alpha2_);

  is_ready_vec_.push_back(false);
  is_ready_vec_.push_back(false);

  prev_value_vec_.push_back(0);
  prev_value_vec_.push_back(0);

  indep1_interrupted_ = false;
  indep2_interrupted_ = false;
}

void DI1CurveAdjustedSimpleTrendMomentum::OnIndicatorUpdate(const unsigned int& indicator_index_,
                                                            const double& new_value_) {
  if (!is_ready_) {
    is_ready_vec_[indicator_index_] = true;
    is_ready_ = AreAllReady();
  } else if (!data_interrupted_) {
    indicator_value_ += (new_value_ - prev_value_vec_[indicator_index_]);
    prev_value_vec_[indicator_index_] = new_value_;
    NotifyIndicatorListeners(indicator_value_);
  }
}

// market_interrupt_listener interface
void DI1CurveAdjustedSimpleTrendMomentum::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                                  const int msecs_since_last_receive_) {
  if (indep1_market_view_.security_id() == _security_id_) {
    indep1_interrupted_ = true;
  } else if (indep2_market_view_.security_id() == _security_id_) {
    indep2_interrupted_ = true;
  } else {
    return;
  }

  data_interrupted_ = true;
  InitializeValues();
  NotifyIndicatorListeners(indicator_value_);
}

void DI1CurveAdjustedSimpleTrendMomentum::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep1_market_view_.security_id() == _security_id_) {
    indep1_interrupted_ = false;
  } else if (indep2_market_view_.security_id() == _security_id_) {
    indep2_interrupted_ = false;
  } else {
    return;
  }

  if ((!indep1_interrupted_) && (!indep2_interrupted_)) {
    InitializeValues();
    data_interrupted_ = false;
  }
}

bool DI1CurveAdjustedSimpleTrendMomentum::AreAllReady() { return VectorUtils::CheckAllForValue(is_ready_vec_, true); }

void DI1CurveAdjustedSimpleTrendMomentum::InitializeValues() { indicator_value_ = 0; }
}
