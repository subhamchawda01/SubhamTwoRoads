/**
    \file IndicatorsCode/simple_trend_indep_mktevents.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/simple_trend_indep_mktevents.hpp"

namespace HFSAT {

void SimpleTrendIndepMktEvents::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                  std::vector<std::string>& _ors_source_needed_vec_,
                                                  const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
}

SimpleTrendIndepMktEvents* SimpleTrendIndepMktEvents::GetUniqueInstance(DebugLogger& t_dbglogger_,
                                                                        const Watch& r_watch_,
                                                                        const std::vector<const char*>& r_tokens_,
                                                                        PriceType_t _basepx_pxtype_) {
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);

  if (r_tokens_.size() < 7) {
    ExitVerbose(
        kModelCreationIndicatorLineLessArgs, t_dbglogger_,
        "INDICATOR weight SimpleTrendIndepMktEvents _dep_market_view_ _indep_market_view_ _num_events_ _price_type_ ");
    return NULL;
  }

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           (unsigned int)std::max(1, atoi(r_tokens_[5])), StringToPriceType_t(r_tokens_[6]));
}

SimpleTrendIndepMktEvents* SimpleTrendIndepMktEvents::GetUniqueInstance(DebugLogger& t_dbglogger_,
                                                                        const Watch& r_watch_,
                                                                        const SecurityMarketView& _dep_market_view_,
                                                                        const SecurityMarketView& _indep_market_view_,
                                                                        unsigned int _num_events_halflife_,
                                                                        PriceType_t _price_type_, bool _use_time_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _indep_market_view_.secname() << ' '
              << _num_events_halflife_ << ' ' << PriceType_t_To_String(_price_type_) << ' ' << _use_time_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, SimpleTrendIndepMktEvents*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new SimpleTrendIndepMktEvents(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_,
                                      _indep_market_view_, _num_events_halflife_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

SimpleTrendIndepMktEvents::SimpleTrendIndepMktEvents(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                     const std::string& concise_indicator_description_,
                                                     const SecurityMarketView& _dep_market_view_,
                                                     const SecurityMarketView& _indep_market_view_,
                                                     unsigned int _num_events_halflife_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      indep_market_view_(_indep_market_view_),
      trend_history_num_events_halflife_(std::max((unsigned int)3, _num_events_halflife_)),
      price_type_(_price_type_),
      inv_decay_sum_(1),
      moving_avg_price_(0),
      current_dep_price_(0) {
  p_dep_indicator_ = RecentSimpleEventsMeasure::GetUniqueInstance(t_dbglogger_, r_watch_, _dep_market_view_, 600);
  p_dep_indicator_->add_unweighted_indicator_listener(1u, this);

  p_indep_indicator_ = RecentSimpleEventsMeasure::GetUniqueInstance(t_dbglogger_, r_watch_, _indep_market_view_, 600);
  p_indep_indicator_->add_unweighted_indicator_listener(2u, this);

  offline_events_ratio_ = 1;
  SetTimeDecayWeights();

  if (!dep_market_view_.subscribe_price_type(this, price_type_)) {
    PriceType_t t_error_price_type_ = price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }

  indc_ready_ = false;
  indep_interrupted_ = false;
  dep_interrupted_ = false;
}

void SimpleTrendIndepMktEvents::SetTimeDecayWeights() {
  double l1ev_dep_ =
      HFSAT::SampleDataUtil::GetAvgForPeriod(dep_market_view_.shortcode(), watch_.YYYYMMDD(), 60, trading_start_mfm_,
                                             trading_end_mfm_, std::string("L1EVPerSec"));
  double l1ev_indep_ =
      HFSAT::SampleDataUtil::GetAvgForPeriod(indep_market_view_.shortcode(), watch_.YYYYMMDD(), 60, trading_start_mfm_,
                                             trading_end_mfm_, std::string("L1EVPerSec"));

  offline_events_ratio_ = l1ev_indep_ / l1ev_dep_;

  kRatioFactor_ = 5;
  double decay_factor_ =
      MathUtils::CalcDecayFactor(kRatioFactor_ * trend_history_num_events_halflife_ / offline_events_ratio_);

  decay_vector_.resize(kRatioFactor_ * kRatioFactor_);

  decay_vector_[0] = decay_factor_;
  for (unsigned int i = 1; i < decay_vector_.size(); i++) {
    decay_vector_[i] = decay_vector_[i - 1] * decay_factor_;
  }
}

void SimpleTrendIndepMktEvents::OnIndicatorUpdate(const unsigned int& t_indicator_index_,
                                                  const double& t_new_indicator_value_) {
  if (!indc_ready_) {
    if (dep_market_view_.is_ready_complex(2) && indep_market_view_.is_ready_complex(2)) {
      indc_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    double dep_events_ = p_dep_indicator_->indicator_value(indc_ready_);
    double indep_events_ = p_indep_indicator_->indicator_value(indc_ready_);

    if (dep_events_ > 0) {
      scaled_event_ratio_ = (indep_events_ / dep_events_) / offline_events_ratio_;
    } else {
      scaled_event_ratio_ = 1;
    }
    int decay_index_ =
        std::max(1, std::min((int)decay_vector_.size(), (int)round(kRatioFactor_ * scaled_event_ratio_)));
    // std::cout << "decay_index: " << decay_index_ << std::endl;

    decay_page_factor_ = decay_vector_[decay_index_ - 1];
    inv_decay_sum_ = (1 - decay_page_factor_);
  }
}

void SimpleTrendIndepMktEvents::OnMarketUpdate(const unsigned int _security_id_,
                                               const MarketUpdateInfo& cr_market_update_info_) {
  if (dep_market_view_.security_id() != _security_id_) {
    std::cout << "OnMarketUpdate for Indep" << std::endl;
    return;
  }

  current_dep_price_ = SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);

  if (!is_ready_) {
    if (dep_market_view_.is_ready_complex(2) && indc_ready_) {
      is_ready_ = true;

      current_dep_price_ = SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    current_dep_price_ = SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);
    moving_avg_price_ = (current_dep_price_ * inv_decay_sum_) + (moving_avg_price_ * decay_page_factor_);

    indicator_value_ = current_dep_price_ - moving_avg_price_;
  }

  NotifyIndicatorListeners(indicator_value_);
}

void SimpleTrendIndepMktEvents::InitializeValues() {
  indicator_value_ = 0;
  moving_avg_price_ = current_dep_price_;

  scaled_event_ratio_ = 1;
  int decay_index_ = std::max(1, std::min((int)decay_vector_.size(), (int)round(kRatioFactor_ * scaled_event_ratio_)));

  decay_page_factor_ = decay_vector_[decay_index_];
  inv_decay_sum_ = (1 - decay_page_factor_);
}
// market_interrupt_listener interface
void SimpleTrendIndepMktEvents::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                        const int msecs_since_last_receive_) {
  if (dep_market_view_.security_id() == _security_id_) {
    dep_interrupted_ = true;
  } else if (indep_market_view_.security_id() == _security_id_) {
    indep_interrupted_ = true;
  } else {
    return;
  }

  data_interrupted_ = true;
  indicator_value_ = 0;
  NotifyIndicatorListeners(indicator_value_);
}

void SimpleTrendIndepMktEvents::OnMarketDataResumed(const unsigned int _security_id_) {
  if (dep_market_view_.security_id() == _security_id_) {
    dep_interrupted_ = false;
  } else if (indep_market_view_.security_id() == _security_id_) {
    indep_interrupted_ = false;
  } else {
    return;
  }

  if ((!dep_interrupted_) && (!indep_interrupted_)) {
    InitializeValues();
    data_interrupted_ = false;
  }
}
}
