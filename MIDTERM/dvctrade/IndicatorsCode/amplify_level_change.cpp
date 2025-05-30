/**
    \file IndicatorsCode/amplify_level_change.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/amplify_level_change.hpp"

#define MAX_QUOTE_VEC_LEN 10

namespace HFSAT {

void AmplifyLevelChange::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                           std::vector<std::string>& _ors_source_needed_vec_,
                                           const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

AmplifyLevelChange* AmplifyLevelChange::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                          const std::vector<const char*>& r_tokens_,
                                                          PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _fractional_seconds_ _growth_size_ _depletion_size_
  // _stable_value_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), atoi(r_tokens_[5]), atoi(r_tokens_[6]), atof(r_tokens_[7]));
}

AmplifyLevelChange* AmplifyLevelChange::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                          const SecurityMarketView& _indep_market_view_,
                                                          double _fractional_seconds_, int t_growth_size_,
                                                          int t_depletion_size_, double t_stable_value_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _fractional_seconds_ << ' '
              << t_growth_size_ << ' ' << t_depletion_size_ << ' ' << t_stable_value_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, AmplifyLevelChange*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new AmplifyLevelChange(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_,
                               _fractional_seconds_, t_growth_size_, t_depletion_size_, t_stable_value_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

AmplifyLevelChange::AmplifyLevelChange(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                       const std::string& concise_indicator_description_,
                                       const SecurityMarketView& _indep_market_view_, double _fractional_seconds_,
                                       int t_growth_size_, int t_depletion_size_, double t_stable_value_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      stability_msecs_(std::max(20, (int)round(1000 * _fractional_seconds_))),
      growth_size_(t_growth_size_),
      depletion_size_(t_depletion_size_),
      stable_value_(t_stable_value_),
      bid_start_index_(0),
      bid_end_index_(0),
      ask_start_index_(0),
      ask_end_index_(0) {
  if (!indep_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice)) {
    PriceType_t t_error_price_type_ = kPriceTypeMktSizeWPrice;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }

  SizeIntPrice_t dummy_quote(0, -1000, 0);
  bid_quotes_.resize(MAX_QUOTE_VEC_LEN, dummy_quote);
  ask_quotes_.resize(MAX_QUOTE_VEC_LEN, dummy_quote);
}

void AmplifyLevelChange::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void AmplifyLevelChange::OnMarketUpdate(const unsigned int _security_id_,
                                        const MarketUpdateInfo& _market_update_info_) {
  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
    }
  } else {
    // not sure if individual variable assignment is going to be faster given small struct size
    bid_quotes_[bid_end_index_] = SizeIntPrice_t(_market_update_info_.bestbid_size_,
                                                 _market_update_info_.bestbid_int_price_, watch_.msecs_from_midnight());
    ask_quotes_[ask_end_index_] = SizeIntPrice_t(_market_update_info_.bestask_size_,
                                                 _market_update_info_.bestask_int_price_, watch_.msecs_from_midnight());
    UpdateQuoteVec();
    bool bid_increasing_ = false;
    bool ask_increasing_ = false;

    double c_stable_value_ =
        stable_value_ * (_market_update_info_.bestask_price_ - _market_update_info_.bestbid_price_);

    if ((bid_quotes_[bid_end_index_].int_price_ > bid_quotes_[bid_start_index_].int_price_) &&
        /*
         * if first line is true then
         * there is no mkt_update in bid_quotes which has higher_price than the one at end_index
         * and since we are making sure than sizes  for same price are incremental,
         * this condition should be sufficient to say that bid is increasing
         */
        (_market_update_info_.mkt_size_weighted_price_ - _market_update_info_.bestbid_price_ < c_stable_value_) &&
        (bid_quotes_[bid_end_index_].size_ > growth_size_)) {
      bid_increasing_ = true;
    }

    if ((ask_quotes_[ask_end_index_].int_price_ < ask_quotes_[ask_start_index_].int_price_) &&  // same logic as bid
        (_market_update_info_.bestask_price_ - _market_update_info_.mkt_size_weighted_price_ < c_stable_value_) &&
        (ask_quotes_[ask_end_index_].size_ > growth_size_)) {
      ask_increasing_ = true;
    }

    if (bid_increasing_ && !ask_increasing_) {
      indicator_value_ = std::max(0.0, ((_market_update_info_.bestbid_price_ + c_stable_value_) -
                                        _market_update_info_.mkt_size_weighted_price_));
    } else {
      if (ask_increasing_ && !bid_increasing_) {
        indicator_value_ = std::min(0.0, ((_market_update_info_.bestask_price_ - c_stable_value_) -
                                          _market_update_info_.mkt_size_weighted_price_));
      } else {
        if (!bid_increasing_ && !ask_increasing_) {
          int num_bid_elems = bid_end_index_ > bid_start_index_ ? bid_end_index_ - bid_start_index_ + 1
                                                                : bid_end_index_ + 11 - bid_start_index_;  // 10 + 1
          unsigned second_last_index = (bid_end_index_ + 10 - 1) % 10;

          if ((num_bid_elems >= 2) &&
              ((bid_quotes_[bid_end_index_].int_price_ == bid_quotes_[second_last_index].int_price_) &&
               (bid_quotes_[bid_end_index_].size_ <= bid_quotes_[second_last_index].size_) &&
               (bid_quotes_[bid_end_index_].size_ < depletion_size_))) {
            indicator_value_ = (_market_update_info_.bestbid_price_ - _market_update_info_.mkt_size_weighted_price_);
          } else {
            int num_ask_elems = ask_end_index_ > ask_start_index_ ? ask_end_index_ - ask_start_index_ + 1
                                                                  : ask_end_index_ + 11 - ask_start_index_;  // 10 + 1
            unsigned second_last_index = (ask_end_index_ + 10 - 1) % 10;

            if ((num_ask_elems >= 2) &&
                ((ask_quotes_[ask_end_index_].int_price_ == ask_quotes_[second_last_index].int_price_) &&
                 (ask_quotes_[ask_end_index_].size_ <= ask_quotes_[second_last_index].size_) &&
                 (ask_quotes_[ask_end_index_].size_ < depletion_size_))) {
              indicator_value_ = (_market_update_info_.bestask_price_ - _market_update_info_.mkt_size_weighted_price_);
            } else {
              indicator_value_ = 0;
            }
          }
        } else {
          indicator_value_ = 0;
        }
      }
    }

    NotifyIndicatorListeners(indicator_value_);
    SetEndIndex();
  }
}

// market_interrupt_listener interface
void AmplifyLevelChange::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                 const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void AmplifyLevelChange::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
  }
}

void AmplifyLevelChange::SetEndIndex() {  // set the end_index for next_event
  bid_end_index_ = (bid_end_index_ + 1) % 10;
  ask_end_index_ = (ask_end_index_ + 1) % 10;
}

void AmplifyLevelChange::UpdateQuoteVec() {
  unsigned int prev_bid_end_index_ = (bid_end_index_ + 9) % 10;  // (bid_end_index_ + 10 -1)%10
  unsigned int prev_ask_end_index_ = (ask_end_index_ + 9) % 10;  // (ask_end_index_ + 10 -1)%10

  // bid
  if (bid_start_index_ != bid_end_index_) {
    // only if more than 1 elements to consider

    if (bid_quotes_[bid_end_index_].int_price_ < bid_quotes_[prev_bid_end_index_].int_price_ ||
        (bid_quotes_[bid_end_index_].int_price_ == bid_quotes_[prev_bid_end_index_].int_price_ &&
         bid_quotes_[bid_end_index_].size_ < bid_quotes_[prev_bid_end_index_].size_)) {
      // bid decreased, reset everything
      bid_start_index_ = bid_end_index_;
    }

    // remove updates till we have all updates within stability_msecs
    while (bid_start_index_ != bid_end_index_) {
      if (bid_quotes_[bid_end_index_].time_stamp_ - bid_quotes_[bid_start_index_].time_stamp_ > stability_msecs_) {
        bid_start_index_ = (bid_start_index_ + 1) % 10;
      } else {
        break;
      }
    }
  }

  // ask
  if (ask_start_index_ != ask_end_index_) {
    // only if more than 1 elements to consider
    if (ask_quotes_[ask_end_index_].int_price_ > ask_quotes_[prev_ask_end_index_].int_price_ ||
        (ask_quotes_[ask_end_index_].int_price_ == ask_quotes_[prev_ask_end_index_].int_price_ &&
         ask_quotes_[ask_end_index_].size_ < ask_quotes_[prev_ask_end_index_].size_)) {
      // ask decreased, reset everything
      ask_start_index_ = ask_end_index_;
    }

    while (ask_start_index_ != ask_end_index_) {
      if (ask_quotes_[ask_end_index_].time_stamp_ - ask_quotes_[ask_start_index_].time_stamp_ > stability_msecs_) {
        ask_start_index_ = (ask_start_index_ + 1) % 10;
      } else {
        break;
      }
    }
  }
}
}
