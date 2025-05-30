/**
    \file IndicatorsCode/di1_levered_simple_trend_mktevents.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
//#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/di1_levered_simple_trend_mktevents.hpp"

namespace HFSAT {

void DI1LeveredSimpleTrendMktEvents::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                       std::vector<std::string>& _ors_source_needed_vec_,
                                                       const std::vector<const char*>& r_tokens_) {
  if (r_tokens_.size() > 6u)  // doesnt mean indicator string syntax is valid .
  {
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);  // dep
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);  // indep
  }
}

DI1LeveredSimpleTrendMktEvents* DI1LeveredSimpleTrendMktEvents::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::vector<const char*>& r_tokens_,
    PriceType_t _base_price_type_) {
  if (r_tokens_.size() < 7u) {
    ExitVerbose(kExitErrorCodeGeneral, "DI1LeveredSimpleTrendMktEvents needs 8 tokens");
    return NULL;
  }

  // INDICATOR 1.00 DI1LeveredSimpleTrendMktEvents DEP INDEP HALFLIFE PRICE_TYPE
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);  // dep
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);  // indep

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(
                               r_tokens_[3])),  // dep is not necessary to hold for now but just in case...
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           atoi(r_tokens_[5]), StringToPriceType_t(r_tokens_[6]));
}

DI1LeveredSimpleTrendMktEvents* DI1LeveredSimpleTrendMktEvents::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const SecurityMarketView& _dep_market_view_,
    const SecurityMarketView& _indep_market_view_, double _num_events_halflife_, PriceType_t t_price_type_) {
  std::ostringstream t_temp_oss_;

  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _indep_market_view_.secname() << ' '
              << _num_events_halflife_ << ' ' << t_price_type_;

  std::string concise_indicator_description_(t_temp_oss_.str());
  static std::map<std::string, DI1LeveredSimpleTrendMktEvents*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new DI1LeveredSimpleTrendMktEvents(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_,
                                           _indep_market_view_, _num_events_halflife_, t_price_type_);
  }

  return concise_indicator_description_map_[concise_indicator_description_];
}

DI1LeveredSimpleTrendMktEvents::DI1LeveredSimpleTrendMktEvents(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                               const std::string& concise_indicator_description_,
                                                               const SecurityMarketView& _dep_market_view_,
                                                               const SecurityMarketView& _indep_market_view_,
                                                               double _num_events_halflife_, PriceType_t _price_type_)
    : IndicatorListener(),
      CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      indep_market_view_(_indep_market_view_),
      dep_term_(CurveUtils::_get_term_(r_watch_.YYYYMMDD(), _dep_market_view_.secname())),
      indep_term_(CurveUtils::_get_term_(r_watch_.YYYYMMDD(), _indep_market_view_.secname())) {
  double alpha_ = 1;  // now it is a simple simple_trend
  // double alpha_ =  dep_term_ / indep_term_ ;

  dep_trendI_ = SimpleTrendMktEvents::GetUniqueInstance(t_dbglogger_, r_watch_, dep_market_view_, _num_events_halflife_,
                                                        _price_type_);
  indep_trendI_ = SimpleTrendMktEvents::GetUniqueInstance(t_dbglogger_, r_watch_, indep_market_view_,
                                                          _num_events_halflife_, _price_type_);

  dep_trendI_->add_indicator_listener(0, this, -1.0);
  indep_trendI_->add_indicator_listener(1, this, alpha_);

  is_ready_vec_.push_back(false);
  is_ready_vec_.push_back(false);

  prev_value_vec_.push_back(0);
  prev_value_vec_.push_back(0);

  indep_interrupted_ = false;
  dep_interrupted_ = false;
}

void DI1LeveredSimpleTrendMktEvents::OnIndicatorUpdate(const unsigned int& indicator_index_, const double& new_value_) {
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
void DI1LeveredSimpleTrendMktEvents::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                             const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    indep_interrupted_ = true;
  } else if (dep_market_view_.security_id() == _security_id_) {
    dep_interrupted_ = true;
  } else {
    return;
  }

  data_interrupted_ = true;
  InitializeValues();
  NotifyIndicatorListeners(indicator_value_);
}

void DI1LeveredSimpleTrendMktEvents::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    indep_interrupted_ = false;
  } else if (dep_market_view_.security_id() == _security_id_) {
    dep_interrupted_ = false;
  } else {
    return;
  }
  if ((!indep_interrupted_) && (!dep_interrupted_)) {
    InitializeValues();
    data_interrupted_ = false;
  }
}

bool DI1LeveredSimpleTrendMktEvents::AreAllReady() { return VectorUtils::CheckAllForValue(is_ready_vec_, true); }

void DI1LeveredSimpleTrendMktEvents::InitializeValues() { indicator_value_ = 0; }
}
