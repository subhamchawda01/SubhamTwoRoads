/**
    \file IndicatorsCode/stud_price_diff.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
 */
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/stud_price_diff.hpp"

namespace HFSAT {

void StudPriceDiff::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                      std::vector<std::string>& _ors_source_needed_vec_,
                                      const std::vector<const char*>& r_tokens_) {
  IndicatorUtil::CollectShortcodeOrPortfolio(_shortcodes_affecting_this_indicator_, _ors_source_needed_vec_, r_tokens_);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
}

StudPriceDiff* StudPriceDiff::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                const std::vector<const char*>& r_tokens_,
                                                PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ _indep_market_view_ _fractional_seconds_
  // _include_lrdb_sign_ _price_type_
  if (r_tokens_.size() < 7) {
    ExitVerbose(
        kModelCreationIndicatorLineLessArgs, t_dbglogger_,
        "INDICATOR weight StudPriceDiff _dep_market_view_ _indep_market_view_ _fractional_seconds_ _price_type_ ");
  } else if (r_tokens_.size() == 7) {
    ShortcodeSecurityMarketViewMap::StaticCheckValidShortCodeOrPortWithExit(r_tokens_[3]);
    ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);
    return GetUniqueInstance(t_dbglogger_, r_watch_, r_tokens_[3],
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                             atof(r_tokens_[5]), -1, StringToPriceType_t(r_tokens_[6]));
  }
  if (std::string(r_tokens_[7]).compare("#") == 0) {
    ShortcodeSecurityMarketViewMap::StaticCheckValidShortCodeOrPortWithExit(r_tokens_[3]);
    ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);
    return GetUniqueInstance(t_dbglogger_, r_watch_, r_tokens_[3],
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                             atof(r_tokens_[5]), -1, StringToPriceType_t(r_tokens_[6]));
  }
  ShortcodeSecurityMarketViewMap::StaticCheckValidShortCodeOrPortWithExit(r_tokens_[3]);
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);
  return GetUniqueInstance(t_dbglogger_, r_watch_, r_tokens_[3],
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           atof(r_tokens_[5]), atof(r_tokens_[6]), StringToPriceType_t(r_tokens_[7]));
}

StudPriceDiff* StudPriceDiff::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                std::string _shortcode_, const SecurityMarketView& _indep_market_view_,
                                                double _fractional_seconds_, double _lrdb_sign_,
                                                PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _shortcode_ << ' ' << _indep_market_view_.secname() << ' ' << _fractional_seconds_
              << ' ' << _lrdb_sign_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, StudPriceDiff*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new StudPriceDiff(t_dbglogger_, r_watch_, concise_indicator_description_, _shortcode_, _indep_market_view_,
                          _fractional_seconds_, _lrdb_sign_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

StudPriceDiff::StudPriceDiff(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                             const std::string& concise_indicator_description_, std::string _shortcode_,
                             const SecurityMarketView& _indep_market_view_, double _fractional_seconds_,
                             double _lrdb_sign_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      price_type_(_price_type_),
      moving_avg_dep_(0),
      last_dep_price_(0),
      current_dep_price_(0),
      moving_avg_indep_(0),
      last_indep_price_(0),
      current_indep_price_(0),
      stdev_dep_(-1),
      stdev_indep_(-1),
      dep_interrupted_(false),
      indep_interrupted_(false),
      lrdb_sign_(_lrdb_sign_),
      is_dep_portfolio_(false),
      dep_portfolio_(nullptr) {
  trend_history_msecs_ = std::max(20, (int)round(1000 * _fractional_seconds_));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();

  if (ShortcodeSecurityMarketViewMap::StaticCheckValidPortWithoutExit(_shortcode_)) {
    dep_portfolio_ = PricePortfolio::GetUniqueInstance(t_dbglogger_, r_watch_, _shortcode_, 5, _price_type_);

    dep_portfolio_->add_unweighted_indicator_listener(0, this);
    is_dep_portfolio_ = true;
  } else {
    ShortcodeSecurityMarketViewMap::StaticCheckValid(_shortcode_);

    dep_market_view_ = (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(_shortcode_));
    if (!dep_market_view_->subscribe_price_type(this, _price_type_)) {
      PriceType_t t_error_price_type_ = _price_type_;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
                << " passed " << t_error_price_type_ << std::endl;
    }
  }

  if (!indep_market_view_.subscribe_price_type(this, _price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }

  SlowStdevCalculator* std_1 = SlowStdevCalculator::GetUniqueInstance(t_dbglogger_, r_watch_, _shortcode_);
  if (std_1 != NULL) {
    std_1->AddSlowStdevCalculatorListener(this);
    std_1->add_unweighted_indicator_listener(1, this);
  }

  SlowStdevCalculator* std_2 =
      SlowStdevCalculator::GetUniqueInstance(t_dbglogger_, r_watch_, indep_market_view_.shortcode());
  if (std_2 != NULL) {
    std_2->AddSlowStdevCalculatorListener(this);
  }
  if (!is_dep_portfolio_ && _lrdb_sign_ > 0) {
    if (OfflineReturnsLRDB::GetUniqueInstance(t_dbglogger_, r_watch_, _shortcode_)
            .GetLRCoeff(_shortcode_, indep_market_view_.shortcode())
            .lr_correlation_ < 0) {
      lrdb_sign_ = -1;
    }
  } else {
    lrdb_sign_ = 1;
  }
}

void StudPriceDiff::OnStdevUpdate(const unsigned int _security_id_, const double& _new_stdev_value_) {
  if (_security_id_ == indep_market_view_.security_id()) {
    stdev_indep_ = _new_stdev_value_;
  } else {
    stdev_dep_ = _new_stdev_value_;
  }
}

void StudPriceDiff::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}
void StudPriceDiff::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (_indicator_index_ == 0) {
    current_dep_price_ = _new_value_;
    if (!is_ready_) {
      if (indep_market_view_.is_ready_complex(2) && dep_portfolio_->is_ready()) {
        is_ready_ = true;
        InitializeValues();
      }
    } else if (!data_interrupted_) {
      UpdateComputedVariables();
    }
  } else if (_indicator_index_ == 1) {
    stdev_dep_ = _new_value_;
  }
}
void StudPriceDiff::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  if (_security_id_ == indep_market_view_.security_id()) {
    current_indep_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  } else {
    current_dep_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  }
  if (!is_dep_portfolio_) {
    if (!is_ready_) {
      if (indep_market_view_.is_ready_complex(2) && dep_market_view_->is_ready_complex(2)) {
        is_ready_ = true;
        InitializeValues();
      }
    } else if (!data_interrupted_) {
      UpdateComputedVariables();
    }
  } else {
    if (!is_ready_) {
      if (indep_market_view_.is_ready_complex(2) && dep_portfolio_->is_ready()) {
        is_ready_ = true;
        InitializeValues();
      }
    } else if (!data_interrupted_) {
      UpdateComputedVariables();
    }
  }
}

void StudPriceDiff::UpdateComputedVariables() {
  if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
    moving_avg_dep_ += inv_decay_sum_ * (current_dep_price_ - last_dep_price_);
    moving_avg_indep_ += inv_decay_sum_ * (current_indep_price_ - last_indep_price_);
  } else {
    int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
    if (num_pages_to_add_ >= (int)decay_vector_.size()) {
      InitializeValues();
    } else {
      if (num_pages_to_add_ == 1) {
        moving_avg_dep_ = (current_dep_price_ * inv_decay_sum_) + (moving_avg_dep_ * decay_vector_[1]);
        moving_avg_indep_ = (current_indep_price_ * inv_decay_sum_) + (moving_avg_indep_ * decay_vector_[1]);
      } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
        moving_avg_dep_ = (current_dep_price_ * inv_decay_sum_) +
                          (last_dep_price_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                          (moving_avg_dep_ * decay_vector_[num_pages_to_add_]);
        moving_avg_indep_ = (current_indep_price_ * inv_decay_sum_) +
                            (last_indep_price_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                            (moving_avg_indep_ * decay_vector_[num_pages_to_add_]);
      }
      last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
    }
  }

  last_dep_price_ = current_dep_price_;
  last_indep_price_ = current_indep_price_;

  indicator_value_ = lrdb_sign_ * (current_indep_price_ - moving_avg_indep_) / stdev_indep_ -
                     (current_dep_price_ - moving_avg_dep_) / stdev_dep_;

  NotifyIndicatorListeners(indicator_value_);
}

void StudPriceDiff::InitializeValues() {
  moving_avg_dep_ = current_dep_price_;
  moving_avg_indep_ = current_indep_price_;

  last_indep_price_ = current_indep_price_;
  last_dep_price_ = current_dep_price_;

  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = 0;
}

// market_interrupt_listener interface
void StudPriceDiff::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    indep_interrupted_ = true;
  } else if (!is_dep_portfolio_ && dep_market_view_->security_id() == _security_id_) {
    dep_interrupted_ = true;
  }
  if (indep_interrupted_ || dep_interrupted_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void StudPriceDiff::OnMarketDataResumed(const unsigned int _security_id_) {
  if (data_interrupted_) {
    if (indep_market_view_.security_id() == _security_id_) {
      indep_interrupted_ = false;
    } else if (!is_dep_portfolio_ && dep_market_view_->security_id() == _security_id_) {
      dep_interrupted_ = false;
    }
    if (!(dep_interrupted_ || indep_interrupted_)) {
      InitializeValues();
      data_interrupted_ = false;
    }
  }
}
}
