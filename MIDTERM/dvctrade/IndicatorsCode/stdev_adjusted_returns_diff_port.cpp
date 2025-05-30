/**
    \file IndicatorsCode/stdev_adjusted_returns_diff_port.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
 */
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/stdev_adjusted_returns_diff_port.hpp"

namespace HFSAT {

void StdevAdjustedReturnsDiffPort::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                     std::vector<std::string>& _ors_source_needed_vec_,
                                                     const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  IndicatorUtil::AddPortfolioShortCodeVec((std::string)r_tokens_[4], _shortcodes_affecting_this_indicator_);
}

StdevAdjustedReturnsDiffPort* StdevAdjustedReturnsDiffPort::GetUniqueInstance(DebugLogger& t_dbglogger_,
                                                                              const Watch& r_watch_,
                                                                              const std::vector<const char*>& r_tokens_,
                                                                              PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ _indep_market_view_ _fractional_seconds_
  // _stdev_duration_ _include_lrdb_sign_ _price_type_
  if (r_tokens_.size() < 8) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "INDICATOR weight StdevAdjustedReturnsDiffPort _dep_market_view_ _portfolio_ _fractional_seconds_ "
                "_stdev_duration_ _price_type_ ");
  }
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(
      t_dbglogger_, r_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
      (const std::string)(r_tokens_[4]), atof(r_tokens_[5]), atof(r_tokens_[6]), StringToPriceType_t(r_tokens_[7]));
}

StdevAdjustedReturnsDiffPort* StdevAdjustedReturnsDiffPort::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const SecurityMarketView& _dep_market_view_,
    const std::string& _portfolio_descriptor_shortcode_, double _fractional_seconds_, double _stdev_duration_,
    PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _portfolio_descriptor_shortcode_ << ' '
              << _fractional_seconds_ << ' ' << _stdev_duration_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, StdevAdjustedReturnsDiffPort*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new StdevAdjustedReturnsDiffPort(
        t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_, _portfolio_descriptor_shortcode_,
        _fractional_seconds_, _stdev_duration_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

StdevAdjustedReturnsDiffPort::StdevAdjustedReturnsDiffPort(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                           const std::string& concise_indicator_description_,
                                                           const SecurityMarketView& _dep_market_view_,
                                                           const std::string& _portfolio_descriptor_shortcode_,
                                                           double _fractional_seconds_, double _stdev_duration_,
                                                           PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      indep_portfolio_price_(
          PCAPortPrice::GetUniqueInstance(t_dbglogger_, r_watch_, _portfolio_descriptor_shortcode_, _price_type_)),
      price_type_(_price_type_),
      moving_avg_dep_(0),
      last_dep_price_(0),
      current_dep_price_(0),
      moving_avg_indep_(0),
      last_indep_price_(0),
      current_indep_price_(0),
      stdev_dep_(-1),
      stdev_indep_(-1),
      returns_dep_(0),
      returns_indep_(0),
      dep_interrupted_(false),
      indep_interrupted_(false),
      lrdb_sign_(1),
      stdev_dep_updated_(false),
      stdev_indep_updated_(false) {
  trend_history_msecs_ = std::max(20, (int)round(1000 * _fractional_seconds_));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();
  if (!dep_market_view_.subscribe_price_type(this, _price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
  indep_portfolio_price_->AddPriceChangeListener(this);

  double t_stdev_duration_ = std::max(100.0, _stdev_duration_);
  SlowStdevReturnsCalculator* std_1 = SlowStdevReturnsCalculator::GetUniqueInstance(
      t_dbglogger_, r_watch_, dep_market_view_.shortcode(), t_stdev_duration_, _fractional_seconds_, _price_type_);
  if (std_1 != NULL) {
    std_1->add_unweighted_indicator_listener(1u, this);
  }
  SlowStdevReturnsCalculator* std_2 = SlowStdevReturnsCalculator::GetUniqueInstance(
      t_dbglogger_, r_watch_, _portfolio_descriptor_shortcode_, t_stdev_duration_, _fractional_seconds_, _price_type_);
  if (std_2 != NULL) {
    std_2->add_unweighted_indicator_listener(2u, this);
  }

  if (OfflineReturnsLRDB::GetUniqueInstance(t_dbglogger_, r_watch_, dep_market_view_.shortcode())
          .GetLRCoeff(dep_market_view_.shortcode(), _portfolio_descriptor_shortcode_)
          .lr_correlation_ < 0) {
    lrdb_sign_ = -1;
  }
}

void StdevAdjustedReturnsDiffPort::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (_indicator_index_ == 1u) {
    stdev_dep_ = _new_value_;
    stdev_dep_updated_ = true;
  } else if (_indicator_index_ == 2u) {
    stdev_indep_ = _new_value_;
    stdev_indep_updated_ = true;
  }
}

void StdevAdjustedReturnsDiffPort::WhyNotReady() {
  if (!is_ready_) {
    if (!(dep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << dep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
    if (!(indep_portfolio_price_->is_ready())) {
      DBGLOG_TIME_CLASS << indep_portfolio_price_->shortcode() << " is_ready_ = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void StdevAdjustedReturnsDiffPort::OnMarketUpdate(const unsigned int _security_id_,
                                                  const MarketUpdateInfo& _market_update_info_) {
  current_dep_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);

  if (!is_ready_) {
    if (indep_portfolio_price_->is_ready() && dep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
      moving_avg_dep_ += inv_decay_sum_ * (current_dep_price_ - last_dep_price_);
    } else {
      int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
      if (num_pages_to_add_ >= (int)decay_vector_.size()) {
        InitializeValues();
      } else {
        if (num_pages_to_add_ == 1) {
          moving_avg_dep_ = (current_dep_price_ * inv_decay_sum_) + (moving_avg_dep_ * decay_vector_[1]);
        } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
          moving_avg_dep_ = (current_dep_price_ * inv_decay_sum_) +
                            (last_dep_price_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                            (moving_avg_dep_ * decay_vector_[num_pages_to_add_]);
        }
        last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
      }
    }

    last_dep_price_ = current_dep_price_;
    // indicator_value_ = ( current_indep_price_ - moving_avg_indep_ ) / stdev_indep_  - ( current_dep_price_ -
    // moving_avg_dep_ ) / stdev_dep_ ;
    // NotifyIndicatorListeners ( indicator_value_ ) ;
  }
}

void StdevAdjustedReturnsDiffPort::OnPortfolioPriceChange(double _new_price_) {
  current_indep_price_ = _new_price_;

  if (!is_ready_) {
    if (dep_market_view_.is_ready_complex(2) && indep_portfolio_price_->is_ready()) {
      is_ready_ = true;
      InitializeValues();
    }
  } else {
    UpdateComputedVariables();
  }
}

void StdevAdjustedReturnsDiffPort::UpdateComputedVariables() {
  if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
    moving_avg_indep_ += inv_decay_sum_ * (current_indep_price_ - last_indep_price_);
  } else {
    int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
    if (num_pages_to_add_ >= (int)decay_vector_.size()) {
      InitializeValues();
    } else {
      if (num_pages_to_add_ == 1) {
        moving_avg_indep_ = (current_indep_price_ * inv_decay_sum_) + (moving_avg_indep_ * decay_page_factor_);
      } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
        moving_avg_indep_ = (current_indep_price_ * inv_decay_sum_) +
                            (last_indep_price_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                            (moving_avg_indep_ * decay_vector_[num_pages_to_add_]);
      }
      last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
    }
  }

  last_indep_price_ = current_indep_price_;
  if (stdev_dep_updated_ && stdev_indep_updated_) {
    if (moving_avg_indep_ != 0 && moving_avg_dep_ != 0) {
      returns_indep_ = (current_indep_price_ - moving_avg_indep_) / moving_avg_indep_;
      returns_dep_ = (current_dep_price_ - moving_avg_dep_) / moving_avg_dep_;

      indicator_value_ = (lrdb_sign_ * stdev_dep_ * returns_indep_ / stdev_indep_ - returns_dep_) * current_dep_price_;
    }
  } else {
    indicator_value_ = 0;
  }

  if (data_interrupted_) indicator_value_ = 0;

  NotifyIndicatorListeners(indicator_value_);
}

void StdevAdjustedReturnsDiffPort::OnPortfolioPriceReset(double t_new_portfolio_price_, double t_old_portfolio_price_,
                                                         unsigned int is_data_interrupted_) {
  if (is_data_interrupted_ == 1u) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  } else if (is_data_interrupted_ == 2u) {
    InitializeValues();
    data_interrupted_ = false;
  } else if (is_data_interrupted_ == 0u) {
    current_indep_price_ = t_new_portfolio_price_;
    double jump_ = t_new_portfolio_price_ - t_old_portfolio_price_;
    moving_avg_indep_ = moving_avg_indep_ + jump_;
    last_indep_price_ = current_indep_price_;
  }
}

void StdevAdjustedReturnsDiffPort::InitializeValues() {
  moving_avg_dep_ = current_dep_price_;
  moving_avg_indep_ = current_indep_price_;

  last_indep_price_ = current_indep_price_;
  last_dep_price_ = current_dep_price_;

  returns_dep_ = 0;
  returns_indep_ = 0;

  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = 0;
}

// market_interrupt_listener interface
void StdevAdjustedReturnsDiffPort::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                           const int msecs_since_last_receive_) {
  if (dep_market_view_.security_id() == _security_id_) {
    dep_interrupted_ = true;
    stdev_dep_updated_ = false;
  }
  if (indep_interrupted_ || dep_interrupted_) {
    data_interrupted_ = true;
    stdev_indep_updated_ = false;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void StdevAdjustedReturnsDiffPort::OnMarketDataResumed(const unsigned int _security_id_) {
  if (data_interrupted_) {
    if (dep_market_view_.security_id() == _security_id_) {
      dep_interrupted_ = false;
    }
    if (!(dep_interrupted_ || indep_interrupted_)) {
      InitializeValues();
      data_interrupted_ = false;
    }
  }
}
}
