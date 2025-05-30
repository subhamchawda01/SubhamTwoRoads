/**
    \file IndicatorsCode/online_computed_negatively_correlated_pair_mktevents.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/online_computed_negatively_correlated_pair_mktevents.hpp"

namespace HFSAT {

void OnlineComputedNegativelyCorrelatedPairMktEvents::CollectShortCodes(
    std::vector<std::string>& _shortcodes_affecting_this_indicator_, std::vector<std::string>& _ors_source_needed_vec_,
    const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
}

OnlineComputedNegativelyCorrelatedPairMktEvents* OnlineComputedNegativelyCorrelatedPairMktEvents::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::vector<const char*>& r_tokens_,
    PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_shortcode_ _indep_market_view_ _num_events_halflife_ _price_type_
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           std::max(1, atoi(r_tokens_[5])), StringToPriceType_t(r_tokens_[6]));
}

OnlineComputedNegativelyCorrelatedPairMktEvents* OnlineComputedNegativelyCorrelatedPairMktEvents::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, SecurityMarketView& _dep_market_view_,
    SecurityMarketView& _indep_market_view_, unsigned int _num_events_halflife_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _indep_market_view_.secname() << ' '
              << _num_events_halflife_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, OnlineComputedNegativelyCorrelatedPairMktEvents*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new OnlineComputedNegativelyCorrelatedPairMktEvents(t_dbglogger_, r_watch_, concise_indicator_description_,
                                                            _dep_market_view_, _indep_market_view_,
                                                            _num_events_halflife_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

OnlineComputedNegativelyCorrelatedPairMktEvents::OnlineComputedNegativelyCorrelatedPairMktEvents(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::string& concise_indicator_description_,
    SecurityMarketView& _dep_market_view_, SecurityMarketView& _indep_market_view_, unsigned int _num_events_halflife_,
    PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      indep_market_view_(_indep_market_view_),
      trend_history_num_events_halflife_(std::max((unsigned int)3, _num_events_halflife_)),
      price_type_(_price_type_),
      twice_initial_indep_price_(0),
      moving_avg_dep_price_(0),
      moving_avg_indep_price_(0),
      moving_avg_dep_indep_price_(0),
      moving_avg_indep_indep_price_(0),
      decay_page_factor_(0.95),
      current_dep_price_(0),
      current_indep_price_(0) {
  if (dep_market_view_.security_id() == indep_market_view_.security_id()) {  // added this since for convenience one
                                                                             // could add a combo or portfolio as source
                                                                             // with a security
    // that is also the dependant
    indicator_value_ = 0;
    is_ready_ = true;
  } else {
    decay_page_factor_ = MathUtils::CalcDecayFactor(trend_history_num_events_halflife_);
    inv_decay_sum_ = (1 - decay_page_factor_);

    if (!dep_market_view_.subscribe_price_type(this, price_type_)) {
      PriceType_t t_error_price_type_ = price_type_;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed "
                << t_error_price_type_ << std::endl;
    }
    if (!indep_market_view_.subscribe_price_type(this, price_type_)) {
      PriceType_t t_error_price_type_ = price_type_;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed "
                << t_error_price_type_ << std::endl;
    }
  }
}

void OnlineComputedNegativelyCorrelatedPairMktEvents::OnMarketUpdate(const unsigned int t_security_id_,
                                                                     const MarketUpdateInfo& cr_market_update_info_) {
  if (!is_ready_) {
    if (t_security_id_ == dep_market_view_.security_id()) {
      current_dep_price_ = SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);
    } else {
      current_indep_price_ = SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);
      twice_initial_indep_price_ = current_indep_price_ * 2;
    }

    if (dep_market_view_.is_ready_complex(2) && indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else {
    if (t_security_id_ == dep_market_view_.security_id()) {
      current_dep_price_ = SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);
    } else {
      current_indep_price_ =
          twice_initial_indep_price_ - SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);
    }

    moving_avg_dep_price_ = (current_dep_price_ * inv_decay_sum_) + (moving_avg_dep_price_ * decay_page_factor_);
    moving_avg_indep_price_ = (current_indep_price_ * inv_decay_sum_) + (moving_avg_indep_price_ * decay_page_factor_);
    moving_avg_dep_indep_price_ = (current_dep_price_ * current_indep_price_ * inv_decay_sum_) +
                                  (moving_avg_dep_indep_price_ * decay_page_factor_);
    moving_avg_indep_indep_price_ = (current_indep_price_ * current_indep_price_ * inv_decay_sum_) +
                                    (moving_avg_indep_indep_price_ * decay_page_factor_);

    if (moving_avg_indep_indep_price_ > 1.00) {  // added to prevent nan

      // Main difference from OnlineComputedNegativelyCorrelatedPairsMktEvents
      double indep_to_proj_value_ = (current_indep_price_ - moving_avg_indep_price_);
      double dep_to_proj_value_ = (current_dep_price_ - moving_avg_dep_price_);

      indicator_value_ =
          ((moving_avg_dep_indep_price_ * indep_to_proj_value_) / moving_avg_indep_indep_price_) - dep_to_proj_value_;
    } else {
      indicator_value_ = 0;
    }
  }

  NotifyIndicatorListeners(indicator_value_);
}

void OnlineComputedNegativelyCorrelatedPairMktEvents::InitializeValues() {
  moving_avg_dep_price_ = current_dep_price_;
  moving_avg_indep_price_ = current_indep_price_;
  moving_avg_dep_indep_price_ = current_dep_price_ * current_indep_price_;
  moving_avg_indep_indep_price_ = current_indep_price_ * current_indep_price_;

  indicator_value_ = 0;
}

void OnlineComputedNegativelyCorrelatedPairMktEvents::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                                              const int msecs_since_last_receive_) {
  // not comparing with dep shortcode as in that case trading has to stop anyways
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  } else
    return;
}

void OnlineComputedNegativelyCorrelatedPairMktEvents::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    InitializeValues();
    data_interrupted_ = false;
  } else
    return;
}
}
