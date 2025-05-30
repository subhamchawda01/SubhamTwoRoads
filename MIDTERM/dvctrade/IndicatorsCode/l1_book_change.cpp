/**
    \file IndicatorsCode/simple_trend.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/l1_book_change.hpp"

// indicator based on this paper
// The price impact of order book events
// Rama Cont, Arseniy Kukanov and Sasha Stoikov
// http://arxiv.org/pdf/1011.6402.pdf

namespace HFSAT {

void L1BookChange::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                     std::vector<std::string>& _ors_source_needed_vec_,
                                     const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

L1BookChange* L1BookChange::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                              const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _fractional_seconds_ _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), StringToPriceType_t(r_tokens_[5]));
}

L1BookChange* L1BookChange::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                              const SecurityMarketView& _indep_market_view_,
                                              double _fractional_seconds_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _fractional_seconds_ << ' '
              << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, L1BookChange*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new L1BookChange(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_,
                         _fractional_seconds_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

L1BookChange::L1BookChange(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                           const std::string& concise_indicator_description_,
                           const SecurityMarketView& _indep_market_view_, double _fractional_seconds_,
                           PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      price_type_(_price_type_),
      moving_avg_price_(0),
      last_price_recorded_(0),
      current_indep_price_(0) {
  trend_history_msecs_ = std::max(20, (int)round(1000 * _fractional_seconds_));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();
  if (!indep_market_view_.subscribe_price_type(this, _price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
}

void L1BookChange::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void L1BookChange::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& cr_market_update_info_) {
  current_indep_price_ = SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);
  l1_event_ = false;  // by default it is not an l1 event, otherwise set to true

  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    // bid side logic
    if (cr_market_update_info_.bestbid_price_ > last_bid_price_) {
      E_ = cr_market_update_info_.bestbid_size_;
      l1_event_ = true;
    } else if (cr_market_update_info_.bestbid_price_ < last_bid_price_) {
      E_ = -last_bid_size_;
      l1_event_ = true;
    } else if (cr_market_update_info_.bestbid_price_ == last_bid_price_) {
      if ((cr_market_update_info_.bestask_size_ != last_ask_size_)) {
        E_ = -cr_market_update_info_.bestask_size_ + last_ask_size_;
        l1_event_ = true;
      }
      if ((cr_market_update_info_.bestbid_size_ != last_bid_size_)) {
        E_ = cr_market_update_info_.bestbid_size_ - last_bid_size_;
        l1_event_ = true;
      }
    }
    // ask side logic
    if (cr_market_update_info_.bestask_price_ > last_ask_price_) {
      E_ = -cr_market_update_info_.bestask_size_;
      l1_event_ = true;
    } else if (cr_market_update_info_.bestask_price_ < last_ask_price_) {
      E_ = last_ask_size_;
      l1_event_ = true;
    } else if (cr_market_update_info_.bestask_price_ == last_ask_price_) {
      if ((cr_market_update_info_.bestask_size_ != last_ask_size_)) {
        E_ = -cr_market_update_info_.bestask_size_ + last_ask_size_;
        l1_event_ = true;
      }
      if ((cr_market_update_info_.bestbid_size_ != last_bid_size_)) {
        E_ = cr_market_update_info_.bestbid_size_ - last_bid_size_;
        l1_event_ = true;
      }
    }

    if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
      moving_avg_E_ += inv_decay_sum_ * (E_ - last_E_recorded_);
    } else {
      int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
      if (num_pages_to_add_ >= (int)decay_vector_.size()) {
        InitializeValues();
      } else {
        if (num_pages_to_add_ == 1) {
          moving_avg_E_ = (E_ * inv_decay_sum_) + (moving_avg_E_ * decay_vector_[1]);
        } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
          moving_avg_E_ = (E_ * inv_decay_sum_) +
                          (last_E_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                          (moving_avg_E_ * decay_vector_[num_pages_to_add_]);
        }
        last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
      }
    }
    if (l1_event_) {
      indicator_value_ = moving_avg_E_;
      last_price_recorded_ = current_indep_price_;
      last_bid_size_ = cr_market_update_info_.bestbid_size_;
      last_bid_price_ = cr_market_update_info_.bestbid_price_;
      last_ask_size_ = cr_market_update_info_.bestask_size_;
      last_ask_price_ = cr_market_update_info_.bestask_price_;
      last_E_recorded_ = E_;
      NotifyIndicatorListeners(indicator_value_);
    }
  }
}

void L1BookChange::InitializeValues() {
  E_ = 0;
  moving_avg_E_ = E_;
  last_E_recorded_ = E_;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = 0;
}

// market_interrupt_listener interface
void L1BookChange::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void L1BookChange::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    InitializeValues();
    data_interrupted_ = false;
  }
}
}
