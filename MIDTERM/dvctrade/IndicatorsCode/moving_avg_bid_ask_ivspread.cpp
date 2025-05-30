/**
    \file IndicatorsCode/moving_avg_bid_ask_ivspread.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#include "dvctrade/Indicators/moving_avg_bid_ask_ivspread.hpp"

namespace HFSAT {

void MovingAvgBidAskIVSpread::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                std::vector<std::string>& _ors_source_needed_vec_,
                                                const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

MovingAvgBidAskIVSpread* MovingAvgBidAskIVSpread::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                    const std::vector<const char*>& r_tokens_,
                                                                    PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  ShortcodeSecurityMarketViewMap::StaticCheckValid(
      NSESecurityDefinitions::GetFutureShortcodeFromOptionShortCode(r_tokens_[3]));
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atoi(r_tokens_[4]), StringToPriceType_t(r_tokens_[5]));
}

MovingAvgBidAskIVSpread* MovingAvgBidAskIVSpread::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                    const SecurityMarketView& _dep_market_view_,
                                                                    int _history_seconds_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _history_seconds_ << " "
              << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  if (global_concise_indicator_description_map_.find(concise_indicator_description_) ==
      global_concise_indicator_description_map_.end()) {
    global_concise_indicator_description_map_[concise_indicator_description_] = new MovingAvgBidAskIVSpread(
        t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_, _history_seconds_, _price_type_);
  }
  return dynamic_cast<MovingAvgBidAskIVSpread*>(
      global_concise_indicator_description_map_[concise_indicator_description_]);
}

MovingAvgBidAskIVSpread::MovingAvgBidAskIVSpread(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                 const std::string& concise_indicator_description_,
                                                 const SecurityMarketView& _dep_market_view_, int _history_seconds_,
                                                 PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      fut_market_view_(*(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(
          NSESecurityDefinitions::GetFutureShortcodeFromOptionShortCode(dep_market_view_.shortcode())))),
      price_type_(_price_type_),
      dep_bid_implied_vol_(0),
      dep_ask_implied_vol_(0),
      fut_price_(0),
      last_spread_(0),
      current_spread_(0),
      moving_average_spread_(0) {
  trend_history_msecs_ = std::max(20, (int)round(1000 * _history_seconds_));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();

  if (!dep_market_view_.subscribe_price_type(this, price_type_)) {
    PriceType_t t_error_price_type_ = price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }

  if (!fut_market_view_.subscribe_price_type(this, price_type_)) {
    PriceType_t t_error_price_type_ = price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }

  dep_option_ = OptionObject::GetUniqueInstance(t_dbglogger_, r_watch_, dep_market_view_.shortcode());
}

void MovingAvgBidAskIVSpread::WhyNotReady() {
  if (!is_ready_) {
    if (!(dep_market_view_.is_ready())) {
      DBGLOG_TIME_CLASS << dep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void MovingAvgBidAskIVSpread::InitializeValues() {
  indicator_value_ = 0;
  fut_price_ = fut_market_view_.mid_price();  // We are taking the mid price for first time, then we will update in
                                              // OnMarketUpdate with price_type_
  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
}

void MovingAvgBidAskIVSpread::OnMarketUpdate(const unsigned int _security_id_,
                                             const MarketUpdateInfo& _market_update_info_) {
  if (!is_ready_) {
    if ((dep_market_view_.is_ready_complex2(1)) && (fut_market_view_.is_ready_complex(2))) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    if (_security_id_ == fut_market_view_.security_id())
      fut_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);

    dep_ask_implied_vol_ = dep_option_->MktImpliedVol(fut_price_, dep_market_view_.bestask_price());
    dep_bid_implied_vol_ = dep_option_->MktImpliedVol(fut_price_, dep_market_view_.bestbid_price());

    current_spread_ = dep_ask_implied_vol_ - dep_bid_implied_vol_;
    std::cerr << "KP " << current_spread_ << "\n";

    if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
      moving_average_spread_ += inv_decay_sum_ * (current_spread_ - last_spread_);
    } else {
      int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
      if (num_pages_to_add_ >= (int)decay_vector_.size()) {
        InitializeValues();
      } else {
        if (num_pages_to_add_ == 1) {
          moving_average_spread_ = (current_spread_ * inv_decay_sum_) + (moving_average_spread_ * decay_vector_[1]);
        } else {
          moving_average_spread_ = (current_spread_ * inv_decay_sum_) +
                                   (last_spread_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                                   (moving_average_spread_ * decay_vector_[num_pages_to_add_]);
        }
        last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
      }
    }

    last_spread_ = current_spread_;
    indicator_value_ = moving_average_spread_;
    NotifyIndicatorListeners(indicator_value_);
  }
}

// market_interrupt_listener interface
void MovingAvgBidAskIVSpread::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                      const int msecs_since_last_receive_) {
  if ((dep_market_view_.security_id() == _security_id_)) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void MovingAvgBidAskIVSpread::OnMarketDataResumed(const unsigned int _security_id_) {
  if ((dep_market_view_.security_id() == _security_id_)) {
    InitializeValues();
    data_interrupted_ = false;
  }
}

}
