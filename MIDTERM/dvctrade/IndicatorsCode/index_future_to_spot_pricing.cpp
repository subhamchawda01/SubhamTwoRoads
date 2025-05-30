/**
    \file IndicatorsCode/future_to_spot_pricing.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/index_future_to_spot_pricing.hpp"
#include "dvctrade/Indicators/synthetic_index.hpp"

namespace HFSAT {

void IndexFutureToSpotPricing::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                 std::vector<std::string>& _ors_source_needed_vec_,
                                                 const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  SyntheticIndex::CollectShortCodes(_shortcodes_affecting_this_indicator_, _ors_source_needed_vec_, r_tokens_);
}

IndexFutureToSpotPricing* IndexFutureToSpotPricing::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                      const std::vector<const char*>& r_tokens_,
                                                                      PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ _indep_market_view_ _fractional_seconds_ _price_type_

  if (r_tokens_.size() < 6) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "   FutureToSoptPriceing Incorrect Syntax. Correct syntax would b  INDICATOR _this_weight_ "
                "_indicator_string_ _dep_market_view_  _fractional_seconds_ _price_type_ ");
  }

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), StringToPriceType_t(r_tokens_[5]));
}

IndexFutureToSpotPricing* IndexFutureToSpotPricing::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                      SecurityMarketView* _dep_market_view_,
                                                                      double _history_secs_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;

  t_temp_oss_ << VarName() << ' ' << _dep_market_view_->secname() << ' ' << PriceType_t_To_String(_price_type_) << " "
              << _history_secs_;

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, IndexFutureToSpotPricing*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new IndexFutureToSpotPricing(
        t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_, _history_secs_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

IndexFutureToSpotPricing::IndexFutureToSpotPricing(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                   const std::string& concise_indicator_description_,
                                                   SecurityMarketView* _dep_market_view_, double _history_secs_,
                                                   PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      price_type_(_price_type_),
      tradingdate_(r_watch_.YYYYMMDD()),
      dep_interrupted_(false),
      indep_interrupted_(false),
      current_dep_price_(0),
      current_synthetic_price_(0),
      moving_average_diff_(0),
      last_diff_recorded_(0),
      current_diff_(0) {
  if (!dep_market_view_->subscribe_price_type(this, _price_type_)) {
    PriceType_t t_error_price_type_ = price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << " to DEP " << std::endl;
  }
  trend_history_msecs_ = std::max(MIN_MSEC_HISTORY_INDICATORS, (int)round(1000 * _history_secs_));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();
  days_to_expiry_ = 0;
  synthetic_index_ = SyntheticIndex::GetUniqueInstance(t_dbglogger_, r_watch_, _dep_market_view_, _price_type_);
  if (synthetic_index_ == NULL) {
    ExitVerbose(kExitErrorCodeGeneral, " Could not create the sythetic index indicator\n");
  }
  synthetic_index_->add_unweighted_indicator_listener(1u, this);
}

void IndexFutureToSpotPricing::WhyNotReady() {
  if (!is_ready_) {
    is_ready_ = true;
    if (!(dep_market_view_->is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << dep_market_view_->secname() << " is_ready_complex(2) = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      is_ready_ = false;
    } else {
      current_dep_price_ = dep_market_view_->price_from_type(price_type_);
    }

    if (!synthetic_index_->IsIndicatorReady()) {
      synthetic_index_->WhyNotReady();
      DBGLOG_TIME_CLASS_FUNC_LINE << "syntheticindex: not ready " << DBGLOG_ENDL_FLUSH;
      is_ready_ = false;
    } else {
      bool t_is_ready_ = true;
      current_synthetic_price_ = synthetic_index_->indicator_value(t_is_ready_);
      is_ready_ = t_is_ready_;
    }

    if (is_ready_) {
      InitializeValues();
    }
  }
}

void IndexFutureToSpotPricing::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (_indicator_index_ == 1) {
    current_synthetic_price_ = _new_value_;
    if (is_ready_) {
      ComputePriceDiff();
      indicator_value_ = moving_average_diff_ - current_diff_;
      NotifyIndicatorListeners(indicator_value_);
    }
  }
}

void IndexFutureToSpotPricing::OnMarketUpdate(const unsigned int _security_id_,
                                              const MarketUpdateInfo& cr_market_update_info_) {
  current_dep_price_ = SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);
  if (dep_market_view_->IsBidBookEmpty() || dep_market_view_->IsAskBookEmpty()) {
    return;
  }

  if (!is_ready_) {
    if (dep_market_view_->is_ready_complex(2) && synthetic_index_->IsIndicatorReady()) {
      is_ready_ = true;
      bool t_is_ready_ = true;
      current_synthetic_price_ = synthetic_index_->indicator_value(t_is_ready_);
      InitializeValues();
    }
  } else if (!data_interrupted_ && dep_market_view_->security_id() == _security_id_) {
    ComputePriceDiff();
    indicator_value_ = moving_average_diff_ - current_diff_;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void IndexFutureToSpotPricing::ComputePriceDiff() {
  current_diff_ = current_synthetic_price_ - current_dep_price_;
  if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
    moving_average_diff_ += inv_decay_sum_ * (current_diff_ - last_diff_recorded_);
  } else {  // new page(s)
    int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
    if (num_pages_to_add_ >= (int)decay_vector_.size()) {
      InitializeValues();
    } else {
      if (num_pages_to_add_ == 1) {
        moving_average_diff_ = (current_diff_ * inv_decay_sum_) + (moving_average_diff_ * decay_page_factor_);
      } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
        moving_average_diff_ = (current_diff_ * inv_decay_sum_) +
                               (last_diff_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                               (moving_average_diff_ * decay_vector_[num_pages_to_add_]);
      }
      last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
    }
  }

  last_diff_recorded_ = current_diff_;
}

void IndexFutureToSpotPricing::InitializeValues() {
  moving_average_diff_ = current_synthetic_price_ - current_dep_price_;
  last_diff_recorded_ = current_synthetic_price_ - current_dep_price_;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = 0;
}

void IndexFutureToSpotPricing::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                       const int msecs_since_last_receive_) {
  if (dep_market_view_->security_id() == _security_id_) {
    dep_interrupted_ = true;
  }
  if (indep_interrupted_ || dep_interrupted_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void IndexFutureToSpotPricing::OnMarketDataResumed(const unsigned int _security_id_) {
  if (data_interrupted_) {
    if (dep_market_view_->security_id() == _security_id_) {
      dep_interrupted_ = false;
    }
    if (!(dep_interrupted_ || indep_interrupted_)) {
      InitializeValues();
      data_interrupted_ = false;
    }
  }
}
}
