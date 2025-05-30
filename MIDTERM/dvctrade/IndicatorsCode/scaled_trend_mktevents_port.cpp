/**
    \file IndicatorsCode/scaled_trend_mktevents_port.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/scaled_trend_mktevents_port.hpp"

namespace HFSAT {

void ScaledTrendMktEventsPort::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                 std::vector<std::string>& _ors_source_needed_vec_,
                                                 const std::vector<const char*>& r_tokens_) {
  IndicatorUtil::AddPortfolioShortCodeVec((std::string)r_tokens_[3], _shortcodes_affecting_this_indicator_);
}

ScaledTrendMktEventsPort* ScaledTrendMktEventsPort::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                      const std::vector<const char*>& r_tokens_,
                                                                      PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _portfolio_descriptor_shortcode_ _num_events_halflife_ _price_type_
  if (r_tokens_.size() < 6) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "INDICATOR weight ScaledTrendMktEventsPort _indep_market_view_ _num_events_/_fractional_seconds_ <T> "
                "_price_type_ ");
  } else if (r_tokens_.size() == 6) {
    return GetUniqueInstance(t_dbglogger_, r_watch_, std::string(r_tokens_[3]),
                             (unsigned int)std::max(1, atoi(r_tokens_[4])), StringToPriceType_t(r_tokens_[5]));
  }
  if (std::string(r_tokens_[6]).compare("#") == 0) {
    return GetUniqueInstance(t_dbglogger_, r_watch_, std::string(r_tokens_[3]),
                             (unsigned int)std::max(1, atoi(r_tokens_[4])), StringToPriceType_t(r_tokens_[5]));
  }

  if (std::string(r_tokens_[5]).compare("T") == 0) {
    unsigned int indep_num_events_halflife_ = 1;
    HFSAT::IndicatorUtil::GetNormalizedL1EventsForPort(std::string(r_tokens_[3]), indep_num_events_halflife_,
                                                       atof(r_tokens_[4]), r_watch_.YYYYMMDD());

    return GetUniqueInstance(t_dbglogger_, r_watch_, std::string(r_tokens_[3]), indep_num_events_halflife_,
                             StringToPriceType_t(r_tokens_[6]), true);
  } else {
    return GetUniqueInstance(t_dbglogger_, r_watch_, std::string(r_tokens_[3]),
                             (unsigned int)std::max(1, atoi(r_tokens_[4])), StringToPriceType_t(r_tokens_[5]));
  }
}

ScaledTrendMktEventsPort* ScaledTrendMktEventsPort::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                      std::string _portfolio_descriptor_shortcode_,
                                                                      int _num_events_halflife_,
                                                                      PriceType_t _price_type_, bool _use_time_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _portfolio_descriptor_shortcode_ << ' ' << _num_events_halflife_ << ' '
              << PriceType_t_To_String(_price_type_) << ' ' << _use_time_;

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, ScaledTrendMktEventsPort*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new ScaledTrendMktEventsPort(t_dbglogger_, r_watch_, concise_indicator_description_,
                                     _portfolio_descriptor_shortcode_, _num_events_halflife_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

ScaledTrendMktEventsPort::ScaledTrendMktEventsPort(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                   const std::string& concise_indicator_description_,
                                                   std::string _portfolio_descriptor_shortcode_,
                                                   int _num_events_halflife_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_portfolio_price__(
          PCAPortPrice::GetUniqueInstance(t_dbglogger_, r_watch_, _portfolio_descriptor_shortcode_, _price_type_)),
      trend_history_num_events_halflife_(std::max(3, _num_events_halflife_)),
      price_type_(_price_type_),
      moving_avg_price_(0),
      moving_avg_squared_price_(0),
      decay_page_factor_(0.5),
      inv_decay_sum_(1),
      stdev_value_(0),
      current_indep_price_(0),
      min_unbiased_l2_norm_(1) {
  min_unbiased_l2_norm_ =
      indep_portfolio_price__->min_price_increment() * indep_portfolio_price__->min_price_increment();

  SetTimeDecayWeights();

  indep_portfolio_price__->AddPriceChangeListener(this);

#if EQUITY_INDICATORS_ALWAYS_READY
  if (indep_portfolio_price__->is_ready()) {
    is_ready_ = true;
    InitializeValues();
  }
#endif
}

void ScaledTrendMktEventsPort::OnPortfolioPriceChange(double _new_price_) {
  current_indep_price_ = _new_price_;

  if (!is_ready_) {
    is_ready_ = true;
    InitializeValues();
  } else {
    moving_avg_price_ = (current_indep_price_ * inv_decay_sum_) + (moving_avg_price_ * decay_page_factor_);
    moving_avg_squared_price_ = (current_indep_price_ * current_indep_price_ * inv_decay_sum_) +
                                (moving_avg_squared_price_ * decay_page_factor_);

    double unadjusted_trend_ = current_indep_price_ - moving_avg_price_;
    double unbiased_l2_norm_ =
        std::max(min_unbiased_l2_norm_, (moving_avg_squared_price_ - (moving_avg_price_ * moving_avg_price_)));
    stdev_value_ = sqrt(unbiased_l2_norm_);
    indicator_value_ = (unadjusted_trend_ / stdev_value_);
  }
  if (data_interrupted_) indicator_value_ = 0;

  NotifyIndicatorListeners(indicator_value_);
}

void ScaledTrendMktEventsPort::OnPortfolioPriceReset(double t_new_price_, double t_old_price_,
                                                     unsigned int is_data_interrupted_) {
  if (is_data_interrupted_ == 1u) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  } else if (is_data_interrupted_ == 2u) {
    InitializeValues();
    data_interrupted_ = false;
  } else if (is_data_interrupted_ == 0u) {
    current_indep_price_ = t_new_price_;  // = current_indep_price_ + ( t_new_price_ - t_old_price_ )

    double delta_ = (t_old_price_ - t_new_price_);
    moving_avg_squared_price_ =
        moving_avg_squared_price_ + (delta_ * delta_) -
        (2 * delta_ * moving_avg_price_);  // avg(sq(xi-delta_)) = avg(sq(xi)) + sq(delta_) - 2 * delta_ * avg(xi); - RC
    moving_avg_price_ -= delta_;

    double unadjusted_trend_ = current_indep_price_ - moving_avg_price_;
    double unbiased_l2_norm_ =
        std::max(min_unbiased_l2_norm_, (moving_avg_squared_price_ - (moving_avg_price_ * moving_avg_price_)));
    stdev_value_ = sqrt(unbiased_l2_norm_);
    indicator_value_ = (unadjusted_trend_ / stdev_value_);
    // last_price_recorded_ = current_indep_price_ ;
  }
}

void ScaledTrendMktEventsPort::InitializeValues() {
  moving_avg_price_ = current_indep_price_;
  moving_avg_squared_price_ = current_indep_price_ * current_indep_price_;
  indicator_value_ = 0;
}

void ScaledTrendMktEventsPort::SetTimeDecayWeights() {
  decay_page_factor_ = MathUtils::CalcDecayFactor(trend_history_num_events_halflife_);
  inv_decay_sum_ = (1 - decay_page_factor_);
}
void ScaledTrendMktEventsPort::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                       const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  indicator_value_ = 0;
  NotifyIndicatorListeners(indicator_value_);
}

void ScaledTrendMktEventsPort::OnMarketDataResumed(const unsigned int _security_id_) {
  InitializeValues();
  data_interrupted_ = false;
}
}
