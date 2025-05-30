/**
    \file IndicatorsCode/stable_scaled_trend.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/stable_scaled_trend_version2_mktevents_port.hpp"

namespace HFSAT {

void StableScaledTrendVersion2MktEventsPort::CollectShortCodes(
    std::vector<std::string>& t_shortcodes_affecting_this_indicator_, std::vector<std::string>& _ors_source_needed_vec_,
    const std::vector<const char*>& r_tokens_) {
  IndicatorUtil::AddPortfolioShortCodeVec((std::string)r_tokens_[3], t_shortcodes_affecting_this_indicator_);
}

StableScaledTrendVersion2MktEventsPort* StableScaledTrendVersion2MktEventsPort::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::vector<const char*>& r_tokens_,
    PriceType_t _basepx_pxtype_) {
  if (r_tokens_.size() < 7) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "INDICATOR weight StableScaledTrendVersion2MktEventsPort _portfolio_ _fractional_seconds_ "
                "_stdev_duration_ _price_type_ ");
  }
  // INDICATOR _this_weight_ _indicator_string_ _portfolio_ _num_events_halflife_ _stdev_duration_ _price_type_
  return GetUniqueInstance(t_dbglogger_, r_watch_, (std::string)r_tokens_[3], atof(r_tokens_[4]), atof(r_tokens_[5]),
                           StringToPriceType_t(r_tokens_[6]));
}

StableScaledTrendVersion2MktEventsPort* StableScaledTrendVersion2MktEventsPort::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::string& _portfolio_descriptor_shortcode_,
    unsigned int _num_events_halflife_, double _stdev_duration_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _portfolio_descriptor_shortcode_ << ' ' << _num_events_halflife_ << ' '
              << _stdev_duration_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, StableScaledTrendVersion2MktEventsPort*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new StableScaledTrendVersion2MktEventsPort(
        t_dbglogger_, r_watch_, concise_indicator_description_, _portfolio_descriptor_shortcode_, _num_events_halflife_,
        _stdev_duration_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

StableScaledTrendVersion2MktEventsPort::StableScaledTrendVersion2MktEventsPort(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::string& concise_indicator_description_,
    const std::string& _portfolio_descriptor_shortcode_, unsigned int _num_events_halflife_, double _stdev_duration_,
    PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_portfolio_price__(
          PCAPortPrice::GetUniqueInstance(t_dbglogger_, r_watch_, _portfolio_descriptor_shortcode_, _price_type_)),
      trend_history_num_events_halflife_(std::max((unsigned int)3, _num_events_halflife_)),
      price_type_(_price_type_),
      stable_stdev_value_(1),
      fast_math_multiplier_(1),
      moving_avg_price_(0),
      inv_decay_sum_(1),
      current_indep_price_(0) {
  decay_page_factor_ = MathUtils::CalcDecayFactor(trend_history_num_events_halflife_);
  inv_decay_sum_ = (1 - decay_page_factor_);

  slow_stdev_trend_calculator_ = SlowStdevTrendCalculator::GetUniqueInstance(
      t_dbglogger_, r_watch_, _portfolio_descriptor_shortcode_, _stdev_duration_, _num_events_halflife_, _price_type_);
  slow_stdev_trend_calculator_->add_unweighted_indicator_listener(1u, this);

  indep_portfolio_price__->AddPriceChangeListener(this);
}

void StableScaledTrendVersion2MktEventsPort::OnIndicatorUpdate(const unsigned int& _indicator_index_,
                                                               const double& _new_stdev_value_) {
  if (!is_ready_) {
    is_ready_ = true;
    InitializeValues();
  }
  stable_stdev_value_ = _new_stdev_value_;
  fast_math_multiplier_ = ((1 - inv_decay_sum_) / stable_stdev_value_);
}

void StableScaledTrendVersion2MktEventsPort::WhyNotReady() {
  if (!is_ready_) {
    if (!indep_portfolio_price__->is_ready()) {
      indep_portfolio_price__->WhyNotReady();
    }
  }
}

void StableScaledTrendVersion2MktEventsPort::OnPortfolioPriceChange(double _new_price_) {
  current_indep_price_ = _new_price_;

  if (is_ready_ && (!data_interrupted_)) {
    moving_avg_price_ = (current_indep_price_ * inv_decay_sum_) + (moving_avg_price_ * decay_page_factor_);
    indicator_value_ = (current_indep_price_ - moving_avg_price_) / stable_stdev_value_;

    NotifyIndicatorListeners(indicator_value_);
  }
}

void StableScaledTrendVersion2MktEventsPort::OnPortfolioPriceReset(double t_new_price_, double t_old_price_,
                                                                   unsigned int is_data_interrupted_) {
  if (is_data_interrupted_ == 1u) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  } else if (is_data_interrupted_ == 2u) {
    InitializeValues();
    data_interrupted_ = false;
  } else if (is_data_interrupted_ == 0u) {
    // TODO

    // current_indep_price_ = t_new_price_ ; // = current_indep_price_ + ( t_new_price_ - t_old_price_ )
    // double delta_ = t_old_price_ - t_new_price_ ;
    // moving_avg_price_         -= delta_ ;
    // moving_avg_squared_price_ -= delta_ * ( delta_ + 2 * moving_avg_price_ );

    // double unadjusted_trend_ = current_indep_price_ - moving_avg_price_ ;
    // double unbiased_l2_norm_ = std::max ( min_unbiased_l2_norm_ , ( moving_avg_squared_price_ - ( moving_avg_price_ *
    // moving_avg_price_ ) ) ) ;
    // stdev_value_ = sqrt ( unbiased_l2_norm_ ) ;
    // indicator_value_ = ( unadjusted_trend_ / stdev_value_ );
    // last_price_recorded_ = current_indep_price_ ;
  }
}

void StableScaledTrendVersion2MktEventsPort::InitializeValues() {
  moving_avg_price_ = current_indep_price_;
  indicator_value_ = 0;
}
}
