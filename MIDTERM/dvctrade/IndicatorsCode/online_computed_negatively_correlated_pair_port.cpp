/**
    \file IndicatorsCode/online_computed_negatively_correlated_pair_port.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/online_computed_negatively_correlated_pair_port.hpp"

namespace HFSAT {

void OnlineComputedNegativelyCorrelatedPairPort::CollectShortCodes(
    std::vector<std::string>& _shortcodes_affecting_this_indicator_, std::vector<std::string>& _ors_source_needed_vec_,
    const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  IndicatorUtil::AddPortfolioShortCodeVec((std::string)r_tokens_[4], _shortcodes_affecting_this_indicator_);
}

OnlineComputedNegativelyCorrelatedPairPort* OnlineComputedNegativelyCorrelatedPairPort::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::vector<const char*>& r_tokens_,
    PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ _portfolio_descriptor_shortcode_ _fractional_seconds_
  // _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           (std::string)(r_tokens_[4]), atof(r_tokens_[5]), StringToPriceType_t(r_tokens_[6]));
}

OnlineComputedNegativelyCorrelatedPairPort* OnlineComputedNegativelyCorrelatedPairPort::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, SecurityMarketView& _dep_market_view_,
    std::string _portfolio_descriptor_shortcode_, double _fractional_seconds_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _portfolio_descriptor_shortcode_ << ' '
              << _fractional_seconds_ << ' ' << PriceType_t_To_String(_price_type_);

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, OnlineComputedNegativelyCorrelatedPairPort*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new OnlineComputedNegativelyCorrelatedPairPort(
        t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_, _portfolio_descriptor_shortcode_,
        _fractional_seconds_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

OnlineComputedNegativelyCorrelatedPairPort::OnlineComputedNegativelyCorrelatedPairPort(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::string& concise_indicator_description_,
    SecurityMarketView& _dep_market_view_, std::string _portfolio_descriptor_shortcode_, double _fractional_seconds_,
    PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      indep_portfolio_price__(
          PCAPortPrice::GetUniqueInstance(t_dbglogger_, r_watch_, _portfolio_descriptor_shortcode_, _price_type_)),
      price_type_(_price_type_),
      twice_initial_indep_price_(0),
      moving_avg_dep_price_(0),
      moving_avg_indep_price_(0),
      moving_avg_dep_indep_price_(0),
      moving_avg_indep_indep_price_(0),
      last_dep_price_recorded_(0),
      last_indep_price_recorded_(0),
      current_dep_price_(0),
      current_indep_price_(0) {
  trend_history_msecs_ = std::max(MIN_MSEC_HISTORY_INDICATORS, (int)round(1000 * _fractional_seconds_));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();

  if (!dep_market_view_.subscribe_price_type(this, price_type_)) {
    PriceType_t t_error_price_type_ = price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed " << t_error_price_type_
              << std::endl;
  }
  indep_portfolio_price__->AddPriceChangeListener(this);
}

void OnlineComputedNegativelyCorrelatedPairPort::WhyNotReady() {
  if (!is_ready_) {
    if (!(dep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << dep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
    if (!indep_portfolio_price__->is_ready()) {
      indep_portfolio_price__->WhyNotReady();
    }
  }
}

void OnlineComputedNegativelyCorrelatedPairPort::OnPortfolioPriceChange(double _new_price_) {
  current_indep_price_ = twice_initial_indep_price_ - _new_price_;

  if (!is_ready_) {
    if (dep_market_view_.is_ready_complex(2) && indep_portfolio_price__->is_ready()) {
      is_ready_ = true;
      twice_initial_indep_price_ = 2 * _new_price_;
      current_indep_price_ = _new_price_;
      InitializeValues();
    }
  } else {
    UpdateComputedVariables();
  }
}

void OnlineComputedNegativelyCorrelatedPairPort::OnPortfolioPriceReset(double t_new_portfolio_price_,
                                                                       double t_old_portfolio_price_,
                                                                       unsigned int is_data_interrupted_) {
  if (is_data_interrupted_ == 1u) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  } else if (is_data_interrupted_ == 2u) {
    InitializeValues();
    data_interrupted_ = false;
  } else if (is_data_interrupted_ == 0u) {
    // AIM :: set variables such that earlier signals are retained in the indicator value
    // new_twice_initial_indep_price_ - t_new_portfolio_price_ = twice_initial_indep_price_ - t_old_portfolio_price_
    // twice_initial_indep_price_ = twice_initial_indep_price_ + ( t_new_portfolio_price_ - t_old_portfolio_price_ )
    // current_indep_price_ = twice_initial_indep_price_ - t_new_portfolio_price_

    twice_initial_indep_price_ = twice_initial_indep_price_ + (t_new_portfolio_price_ - t_old_portfolio_price_);
    current_indep_price_ = twice_initial_indep_price_ - t_new_portfolio_price_;
    UpdateComputedVariables();
  }
}

void OnlineComputedNegativelyCorrelatedPairPort::OnMarketUpdate(
    const unsigned int _security_id_,
    const MarketUpdateInfo& cr_market_update_info_) {  // this must be an update of dependant
  // hence using GetPriceFromType and not dep_market_view_.price_from_type
  current_dep_price_ = SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);

  if (!is_ready_) {
    if (dep_market_view_.is_ready_complex(2) && indep_portfolio_price__->is_ready()) {
      is_ready_ = true;
      twice_initial_indep_price_ = 2 * indep_portfolio_price__->current_price();
      current_indep_price_ = twice_initial_indep_price_ - indep_portfolio_price__->current_price();
      InitializeValues();
    }
  } else {
    UpdateComputedVariables();
  }
}

void OnlineComputedNegativelyCorrelatedPairPort::UpdateComputedVariables() {
  if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
    moving_avg_dep_price_ += inv_decay_sum_ * (current_dep_price_ - last_dep_price_recorded_);
    moving_avg_indep_price_ += inv_decay_sum_ * (current_indep_price_ - last_indep_price_recorded_);
    moving_avg_dep_indep_price_ += inv_decay_sum_ * ((current_dep_price_ * current_indep_price_) -
                                                     (last_dep_price_recorded_ * last_indep_price_recorded_));
    moving_avg_indep_indep_price_ += inv_decay_sum_ * ((current_indep_price_ * current_indep_price_) -
                                                       (last_indep_price_recorded_ * last_indep_price_recorded_));

  } else {
    int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
    if (num_pages_to_add_ >= (int)decay_vector_.size()) {
      InitializeValues();
    } else {
      if (num_pages_to_add_ == 1) {
        moving_avg_dep_price_ = (current_dep_price_ * inv_decay_sum_) + (moving_avg_dep_price_ * decay_page_factor_);
        moving_avg_indep_price_ =
            (current_indep_price_ * inv_decay_sum_) + (moving_avg_indep_price_ * decay_page_factor_);
        moving_avg_dep_indep_price_ = (current_dep_price_ * current_indep_price_ * inv_decay_sum_) +
                                      (moving_avg_dep_indep_price_ * decay_page_factor_);
        moving_avg_indep_indep_price_ = (current_indep_price_ * current_indep_price_ * inv_decay_sum_) +
                                        (moving_avg_indep_indep_price_ * decay_page_factor_);
      } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
        moving_avg_dep_price_ = (current_dep_price_ * inv_decay_sum_) + (last_dep_price_recorded_ * inv_decay_sum_ *
                                                                         decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                                (moving_avg_dep_price_ * decay_vector_[num_pages_to_add_]);
        moving_avg_indep_price_ =
            (current_indep_price_ * inv_decay_sum_) +
            (last_indep_price_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
            (moving_avg_indep_price_ * decay_vector_[num_pages_to_add_]);
        moving_avg_dep_indep_price_ = (current_dep_price_ * current_indep_price_ * inv_decay_sum_) +
                                      (last_dep_price_recorded_ * last_indep_price_recorded_ * inv_decay_sum_ *
                                       decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                                      (moving_avg_dep_indep_price_ * decay_vector_[num_pages_to_add_]);
        moving_avg_indep_indep_price_ = (current_indep_price_ * current_indep_price_ * inv_decay_sum_) +
                                        (last_indep_price_recorded_ * last_indep_price_recorded_ * inv_decay_sum_ *
                                         decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                                        (moving_avg_indep_indep_price_ * decay_vector_[num_pages_to_add_]);
      }
      last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
    }
  }

  if (moving_avg_indep_indep_price_ > 1.00) {  // added to prevent nan

    // Main difference from OnlineComputedNegativelyCorrelatedPairsPort
    double indep_to_proj_value_ = (current_indep_price_ - moving_avg_indep_price_);
    double dep_to_proj_value_ = (current_dep_price_ - moving_avg_dep_price_);

    indicator_value_ =
        ((moving_avg_dep_indep_price_ * indep_to_proj_value_) / moving_avg_indep_indep_price_) - dep_to_proj_value_;
  } else {
    indicator_value_ = 0;
  }
  if (data_interrupted_) indicator_value_ = 0;

  NotifyIndicatorListeners(indicator_value_);

  last_dep_price_recorded_ = current_dep_price_;
  last_indep_price_recorded_ = current_indep_price_;
}

void OnlineComputedNegativelyCorrelatedPairPort::InitializeValues() {
  moving_avg_dep_price_ = current_dep_price_;
  moving_avg_indep_price_ = current_indep_price_;
  moving_avg_dep_indep_price_ = current_dep_price_ * current_indep_price_;
  moving_avg_indep_indep_price_ = current_indep_price_ * current_indep_price_;

  last_dep_price_recorded_ = current_dep_price_;
  last_indep_price_recorded_ = current_indep_price_;

  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = 0;
}
void OnlineComputedNegativelyCorrelatedPairPort::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                                         const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  indicator_value_ = 0;
  NotifyIndicatorListeners(indicator_value_);
}

void OnlineComputedNegativelyCorrelatedPairPort::OnMarketDataResumed(const unsigned int _security_id_) {
  InitializeValues();
  data_interrupted_ = false;
}
}
