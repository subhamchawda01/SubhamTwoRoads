/**
    \file IndicatorsCode/online_computed_pair_mktevents.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/online_computed_pair_mktevents.hpp"

namespace HFSAT {

void OnlineComputedPairMktEvents::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                    std::vector<std::string>& _ors_source_needed_vec_,
                                                    const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
}

OnlineComputedPairMktEvents* OnlineComputedPairMktEvents::GetUniqueInstance(DebugLogger& t_dbglogger_,
                                                                            const Watch& r_watch_,
                                                                            const std::vector<const char*>& r_tokens_,
                                                                            PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_shortcode_ _indep_market_view_ _num_events_halflife_ _price_type_

  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);

  if (r_tokens_.size() < 7) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "INDICATOR weight OnlineComputedPairMktEvents _dep_shortcode_ _indep_shortcode_ "
                "_num_events_/_fractional_seconds_ <T> _price_type_ ");
  } else if (r_tokens_.size() == 7) {
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                             (unsigned int)std::max(1, atoi(r_tokens_[5])), StringToPriceType_t(r_tokens_[6]));
  }
  if (std::string(r_tokens_[7]).compare("#") == 0) {
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                             (unsigned int)std::max(1, atoi(r_tokens_[5])), StringToPriceType_t(r_tokens_[6]));
  }

  if (std::string(r_tokens_[6]).compare("T") == 0) {
    double avg_l1_events_per_sec_ = HFSAT::SampleDataUtil::GetAvgForPeriod(
        std::string(r_tokens_[3]), r_watch_.YYYYMMDD(), 60, std::string("L1EVPerSec"));

    return GetUniqueInstance(t_dbglogger_, r_watch_,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                             (unsigned int)std::max(1.00, avg_l1_events_per_sec_ * atof(r_tokens_[5])),
                             StringToPriceType_t(r_tokens_[7]), true);
  } else {
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                             (unsigned int)std::max(1, atoi(r_tokens_[5])), StringToPriceType_t(r_tokens_[6]));
  }
}

OnlineComputedPairMktEvents* OnlineComputedPairMktEvents::GetUniqueInstance(DebugLogger& t_dbglogger_,
                                                                            const Watch& r_watch_,
                                                                            SecurityMarketView& _dep_market_view_,
                                                                            SecurityMarketView& _indep_market_view_,
                                                                            unsigned int _num_events_halflife_,
                                                                            PriceType_t _price_type_, bool _use_time_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _indep_market_view_.secname() << ' '
              << _num_events_halflife_ << ' ' << PriceType_t_To_String(_price_type_) << ' ' << _use_time_;

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, OnlineComputedPairMktEvents*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new OnlineComputedPairMktEvents(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_,
                                        _indep_market_view_, _num_events_halflife_, _price_type_, _use_time_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

OnlineComputedPairMktEvents::OnlineComputedPairMktEvents(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                         const std::string& concise_indicator_description_,
                                                         SecurityMarketView& _dep_market_view_,
                                                         SecurityMarketView& _indep_market_view_,
                                                         unsigned int _num_events_halflife_, PriceType_t _price_type_,
                                                         bool _use_time_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      indep_market_view_(_indep_market_view_),
      trend_history_num_events_halflife_(std::max((unsigned int)3, _num_events_halflife_)),
      price_type_(_price_type_),
      moving_avg_dep_price_(0),
      moving_avg_indep_price_(0),
      moving_avg_dep_indep_price_(0),
      moving_avg_indep_indep_price_(0),
      dep_decay_page_factor_(0.95),
      indep_decay_page_factor_(0.95),
      dep_indep_decay_page_factor_(0.95),
      current_dep_price_(0),
      current_indep_price_(0),
      use_time_(_use_time_) {
  if (dep_market_view_.security_id() == indep_market_view_.security_id()) {  // added this since for convenience one
                                                                             // could add a combo or portfolio as source
                                                                             // with a security
    // that is also the dependant
    indicator_value_ = 0;
    is_ready_ = true;
  } else {
    dep_decay_page_factor_ = MathUtils::CalcDecayFactor(trend_history_num_events_halflife_);
    dep_inv_decay_sum_ = (1 - dep_decay_page_factor_);

    if (_use_time_) {
      unsigned int indep_num_events_halflife_ = 1;
      HFSAT::IndicatorUtil::GetNormalizedL1EventsForIndep(indep_market_view_.shortcode(), dep_market_view_.shortcode(),
                                                          trend_history_num_events_halflife_,
                                                          indep_num_events_halflife_, r_watch_.YYYYMMDD());
      indep_decay_page_factor_ = MathUtils::CalcDecayFactor(indep_num_events_halflife_);
      indep_inv_decay_sum_ = (1 - indep_decay_page_factor_);

      unsigned dep_indep_num_events_halflife_ = trend_history_num_events_halflife_ + indep_num_events_halflife_;
      dep_indep_decay_page_factor_ = MathUtils::CalcDecayFactor(dep_indep_num_events_halflife_);
      dep_indep_inv_decay_sum_ = (1 - dep_indep_decay_page_factor_);
    } else {
      indep_decay_page_factor_ = dep_decay_page_factor_;
      indep_inv_decay_sum_ = dep_inv_decay_sum_;
      dep_indep_decay_page_factor_ = dep_decay_page_factor_;
      dep_indep_inv_decay_sum_ = dep_inv_decay_sum_;
    }

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

#if EQUITY_INDICATORS_ALWAYS_READY
    if (IndicatorUtil::IsEquityShortcode(dep_market_view_.shortcode()) &&
        IndicatorUtil::IsEquityShortcode(indep_market_view_.shortcode())) {
      is_ready_ = true;
      InitializeValues();
    }
#endif
  }
}

void OnlineComputedPairMktEvents::OnMarketUpdate(const unsigned int _security_id_,
                                                 const MarketUpdateInfo& cr_market_update_info_) {
  if (_security_id_ == dep_market_view_.security_id()) {
    current_dep_price_ = SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);
  } else {
    current_indep_price_ = SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);
  }

// the fllowing has been added since above we could be potentially be accessing the price when the
// SMV is not ready
#if EQUITY_INDICATORS_ALWAYS_READY
  if ((!IndicatorUtil::IsEquityShortcode(dep_market_view_.shortcode()) &&
       (dep_market_view_.IsBidBookEmpty() || dep_market_view_.IsAskBookEmpty())) ||
      (!IndicatorUtil::IsEquityShortcode(indep_market_view_.shortcode()) &&
       (indep_market_view_.IsBidBookEmpty() || indep_market_view_.IsAskBookEmpty())))
#else
  if (dep_market_view_.IsBidBookEmpty() || dep_market_view_.IsAskBookEmpty() || indep_market_view_.IsBidBookEmpty() ||
      indep_market_view_.IsAskBookEmpty())
#endif
  {
    return;
  }

  if (!is_ready_) {
#if EQUITY_INDICATORS_ALWAYS_READY
    if ((IndicatorUtil::IsEquityShortcode(dep_market_view_.shortcode()) || dep_market_view_.is_ready_complex(2)) &&
        (IndicatorUtil::IsEquityShortcode(indep_market_view_.shortcode()) || indep_market_view_.is_ready_complex(2)))
#else
    if (dep_market_view_.is_ready_complex(2) && indep_market_view_.is_ready_complex(2))
#endif
    {
      is_ready_ = true;
      InitializeValues();
    }
  } else {
    if (!use_time_ || _security_id_ == dep_market_view_.security_id()) {
      moving_avg_dep_price_ =
          (current_dep_price_ * dep_inv_decay_sum_) + (moving_avg_dep_price_ * dep_decay_page_factor_);
    }
    if (!use_time_ || _security_id_ == indep_market_view_.security_id()) {
      moving_avg_indep_price_ =
          (current_indep_price_ * indep_inv_decay_sum_) + (moving_avg_indep_price_ * indep_decay_page_factor_);
      moving_avg_indep_indep_price_ = (current_indep_price_ * current_indep_price_ * indep_inv_decay_sum_) +
                                      (moving_avg_indep_indep_price_ * indep_decay_page_factor_);
    }
    moving_avg_dep_indep_price_ = (current_dep_price_ * current_indep_price_ * dep_indep_inv_decay_sum_) +
                                  (moving_avg_dep_indep_price_ * dep_indep_decay_page_factor_);

    if (moving_avg_indep_indep_price_ > 1.00) {  // added to prevent nan

      // Main Difference from OnlineComputedPairsMktEvents
      double indep_to_proj_value_ = (current_indep_price_ - moving_avg_indep_price_);
      double dep_to_proj_value_ = (current_dep_price_ - moving_avg_dep_price_);

      indicator_value_ =
          ((moving_avg_dep_indep_price_ * indep_to_proj_value_) / moving_avg_indep_indep_price_) - dep_to_proj_value_;
    } else {
      indicator_value_ = 0;
    }

    // 	if ( isnan ( indicator_value_ ) )
    // 	  {
    // #ifdef EXIT_ON_NAN_IV
    // 	    std::cerr << __PRETTY_FUNCTION__ << " nan " << std::endl;
    // 	    exit ( 1 );
    // #else
    // 	    indicator_value_ = 0;
    // #endif
    // 	  }
  }

  if (data_interrupted_) indicator_value_ = 0;

  NotifyIndicatorListeners(indicator_value_);
}

void OnlineComputedPairMktEvents::InitializeValues() {
  moving_avg_dep_price_ = current_dep_price_;
  moving_avg_indep_price_ = current_indep_price_;
  moving_avg_dep_indep_price_ = current_dep_price_ * current_indep_price_;
  moving_avg_indep_indep_price_ = current_indep_price_ * current_indep_price_;

  indicator_value_ = 0;
}

void OnlineComputedPairMktEvents::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                          const int msecs_since_last_receive_) {
  // not comparing with dep shortcode as in that case trading has to stop anyways
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  } else
    return;
}

void OnlineComputedPairMktEvents::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    InitializeValues();
    data_interrupted_ = false;
  } else
    return;
}
}
