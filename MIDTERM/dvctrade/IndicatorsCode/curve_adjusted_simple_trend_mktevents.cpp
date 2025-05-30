/**
    \file IndicatorsCode/curve_adjusted_simple_trend_mktevents.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
//#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/curve_adjusted_simple_trend_mktevents.hpp"

namespace HFSAT {

void CurveAdjustedSimpleTrendMktEvents::CollectShortCodes(
    std::vector<std::string>& _shortcodes_affecting_this_indicator_, std::vector<std::string>& _ors_source_needed_vec_,
    const std::vector<const char*>& r_tokens_) {
  if (r_tokens_.size() > 7u)  // doesnt mean indicator string syntax is valid .
  {
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);  // dep
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);  // indep1
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[5]);  // indep2
  }
}

CurveAdjustedSimpleTrendMktEvents* CurveAdjustedSimpleTrendMktEvents::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::vector<const char*>& r_tokens_,
    PriceType_t _base_price_type_) {

  // INDICATOR 1.00 CurveAdjustedSimpleTrendMktEvents DEP INDEP1 INDEP2 HALFLIFE <T> PRICE_TYPE
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);  // dep
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);  // indep1
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[5]);  // indep2

  if (r_tokens_.size() < 8) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "INDICATOR weight CurveAdjustedSimpleTrendMktEvents DEP INDEP1 INDEP2 _num_events_/_fractional_seconds_ <T> "
                "_price_type_ ");
    return NULL;
  } else if(r_tokens_.size() == 8) {
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(
                               r_tokens_[3])),  // dep is not necessary to hold for now but just in case...
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[5])),
                           atoi(r_tokens_[6]), StringToPriceType_t(r_tokens_[7]));
  }

  if (std::string(r_tokens_[8]).compare("#") == 0) {
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(
                               r_tokens_[3])),  // dep is not necessary to hold for now but just in case...
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[5])),
                           atoi(r_tokens_[6]), StringToPriceType_t(r_tokens_[7]));
  }

  if (std::string(r_tokens_[7]).compare("T") == 0) {

    return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(
                               r_tokens_[3])),  // dep is not necessary to hold for now but just in case...
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[5])),
                           atoi(r_tokens_[6]), StringToPriceType_t(r_tokens_[8]), true);
  } else {
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(
                               r_tokens_[3])),  // dep is not necessary to hold for now but just in case...
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[5])),
                           atoi(r_tokens_[6]), StringToPriceType_t(r_tokens_[7]));
  }    
}

CurveAdjustedSimpleTrendMktEvents* CurveAdjustedSimpleTrendMktEvents::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const SecurityMarketView& _dep_market_view_,
    const SecurityMarketView& _indep1_market_view_, const SecurityMarketView& _indep2_market_view_,
    double _num_events_halflife_, PriceType_t t_price_type_, bool _use_time_) {
  std::ostringstream t_temp_oss_;

  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _indep1_market_view_.secname() << ' '
              << _indep2_market_view_.secname() << ' ' << _num_events_halflife_ << ' '  << t_price_type_ << ' ' << _use_time_;

  std::string concise_indicator_description_(t_temp_oss_.str());
  static std::map<std::string, CurveAdjustedSimpleTrendMktEvents*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new CurveAdjustedSimpleTrendMktEvents(
        t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_, _indep1_market_view_,
        _indep2_market_view_, _num_events_halflife_, t_price_type_, _use_time_);
  }

  return concise_indicator_description_map_[concise_indicator_description_];
}

CurveAdjustedSimpleTrendMktEvents::CurveAdjustedSimpleTrendMktEvents(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::string& concise_indicator_description_,
    const SecurityMarketView& _dep_market_view_, const SecurityMarketView& _indep1_market_view_,
    const SecurityMarketView& _indep2_market_view_, double _num_events_halflife_, 
    PriceType_t _price_type_, bool _use_time_)
    : IndicatorListener(),
      CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      indep1_market_view_(_indep1_market_view_),
      indep2_market_view_(_indep2_market_view_),
      dep_term_(CurveUtils::_get_term_(r_watch_.YYYYMMDD(), _dep_market_view_.secname())),
      indep1_term_(CurveUtils::_get_term_(r_watch_.YYYYMMDD(), _indep1_market_view_.secname())),
      indep2_term_(CurveUtils::_get_term_(r_watch_.YYYYMMDD(), _indep2_market_view_.secname())) {
  // double alpha1_ =  ( ( indep2_term_ - dep_term_ )  / ( indep2_term_ - indep1_term_ ) * ( indep1_term_ / dep_term_ )
  // ) ;
  // double alpha2_ =  ( - ( indep1_term_ - dep_term_ ) / ( indep2_term_ - indep1_term_ ) * ( indep2_term_ / dep_term_ )
  // ) ;

  double _num_events_halflife_dep_ = _num_events_halflife_;
  double _num_events_halflife_indep1_ = _num_events_halflife_;
  double _num_events_halflife_indep2_ = _num_events_halflife_;
  if ( _use_time_) {
    double avg_l1_events_per_sec_dep_ = HFSAT::SampleDataUtil::GetAvgForPeriod(
        _dep_market_view_.shortcode(), r_watch_.YYYYMMDD(), 60, std::string("L1EVPerSec"));
    double avg_l1_events_per_sec_indep1_ = HFSAT::SampleDataUtil::GetAvgForPeriod(
        _indep1_market_view_.shortcode(), r_watch_.YYYYMMDD(), 60, std::string("L1EVPerSec"));
    double avg_l1_events_per_sec_indep2_ = HFSAT::SampleDataUtil::GetAvgForPeriod(
        _indep2_market_view_.shortcode(), r_watch_.YYYYMMDD(), 60, std::string("L1EVPerSec"));

    _num_events_halflife_dep_ = (unsigned int)std::max(1.00, avg_l1_events_per_sec_dep_ * _num_events_halflife_);
    _num_events_halflife_indep1_ = (unsigned int)std::max(1.00, avg_l1_events_per_sec_indep1_ * _num_events_halflife_);
    _num_events_halflife_indep2_ = (unsigned int)std::max(1.00, avg_l1_events_per_sec_indep2_ * _num_events_halflife_);
  }

  std::string t_dep_secname_ = std::string(_dep_market_view_.secname());
  double alpha1_ = 0.0;
  double alpha2_ = 0.0;
  CurveUtils::get_alphas(t_dep_secname_, indep1_term_, indep2_term_, dep_term_, alpha1_, alpha2_, watch_.YYYYMMDD());

  dep_trendI_ = SimpleTrendMktEvents::GetUniqueInstance(t_dbglogger_, r_watch_, dep_market_view_, _num_events_halflife_dep_,
                                                        _price_type_);
  indep1_trendI_ = SimpleTrendMktEvents::GetUniqueInstance(t_dbglogger_, r_watch_, indep1_market_view_,
                                                           _num_events_halflife_indep1_, _price_type_);
  indep2_trendI_ = SimpleTrendMktEvents::GetUniqueInstance(t_dbglogger_, r_watch_, indep2_market_view_,
                                                           _num_events_halflife_indep2_, _price_type_);

  dep_trendI_->add_indicator_listener(0, this, -1.0);
  indep1_trendI_->add_indicator_listener(1, this, alpha1_);
  indep2_trendI_->add_indicator_listener(2, this, alpha2_);

  is_ready_vec_.push_back(false);
  is_ready_vec_.push_back(false);
  is_ready_vec_.push_back(false);

  prev_value_vec_.push_back(0);
  prev_value_vec_.push_back(0);
  prev_value_vec_.push_back(0);

  indep1_interrupted_ = false;
  indep2_interrupted_ = false;
  dep_interrupted_ = false;
}

void CurveAdjustedSimpleTrendMktEvents::OnIndicatorUpdate(const unsigned int& indicator_index_,
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
void CurveAdjustedSimpleTrendMktEvents::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                                const int msecs_since_last_receive_) {
  if (indep1_market_view_.security_id() == _security_id_) {
    indep1_interrupted_ = true;
  } else if (indep2_market_view_.security_id() == _security_id_) {
    indep2_interrupted_ = true;
  } else if (dep_market_view_.security_id() == _security_id_) {
    dep_interrupted_ = true;
  } else {
    return;
  }

  data_interrupted_ = true;
  InitializeValues();
  NotifyIndicatorListeners(indicator_value_);
}

void CurveAdjustedSimpleTrendMktEvents::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep1_market_view_.security_id() == _security_id_) {
    indep1_interrupted_ = false;
  } else if (indep2_market_view_.security_id() == _security_id_) {
    indep2_interrupted_ = false;
  } else if (dep_market_view_.security_id() == _security_id_) {
    dep_interrupted_ = false;
  } else {
    return;
  }

  if ((!indep1_interrupted_) && (!indep2_interrupted_) && (!dep_interrupted_)) {
    InitializeValues();
    data_interrupted_ = false;
  }
}

bool CurveAdjustedSimpleTrendMktEvents::AreAllReady() { return VectorUtils::CheckAllForValue(is_ready_vec_, true); }

void CurveAdjustedSimpleTrendMktEvents::InitializeValues() { indicator_value_ = 0; }
}
