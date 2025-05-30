/**
    \file IndicatorsCode/l1_bid_ask_size_flow.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/l1_bid_ask_size_flow_mini.hpp"

namespace HFSAT {

void L1BidAskSizeFlowMini::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                             std::vector<std::string>& _ors_source_needed_vec_,
                                             const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
}

L1BidAskSizeFlowMini* L1BidAskSizeFlowMini::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                              const std::vector<const char*>& r_tokens_,
                                                              PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _num_events_
  if (r_tokens_.size() < 6) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "L1BidAskSizeFlowMini incorrect syntax. Should be INDICATOR _this_weight_ _indicator_string_ "
                "_dep_market_view_ _dep_market_view_ _num_events_");
  }
  return GetUniqueInstance(
      t_dbglogger_, r_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
      *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])), std::max(5, atoi(r_tokens_[5])));
}

L1BidAskSizeFlowMini* L1BidAskSizeFlowMini::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                              SecurityMarketView& _dep_market_view_,
                                                              SecurityMarketView& _indep_market_view_,
                                                              unsigned int _num_events_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _indep_market_view_.secname() << ' '
              << _num_events_;

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, L1BidAskSizeFlowMini*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new L1BidAskSizeFlowMini(
        t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_, _indep_market_view_, _num_events_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

L1BidAskSizeFlowMini::L1BidAskSizeFlowMini(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                           const std::string& concise_indicator_description_,
                                           SecurityMarketView& _dep_market_view_,
                                           SecurityMarketView& _indep_market_view_, unsigned int _num_events_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      indep_market_view_(_indep_market_view_),
      decay_factor_(1 - (2.0 / _num_events_)),
      decay_value_(1),
      last_dep_bid_int_price_(0),
      last_dep_ask_int_price_(0),
      last_indep_bid_int_price_(0),
      last_indep_ask_int_price_(0),
      current_bid_ask_bias_(0),
      moving_avg_bid_ask_bias_(0),
      event_count_(0) {
  dep_sec_id_ = dep_market_view_.security_id();
  indep_sec_id_ = indep_market_view_.security_id();

  if (!indep_market_view_.subscribe_price_type(this, HFSAT::kPriceTypeMktSizeWPrice)) {
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << HFSAT::kPriceTypeMktSizeWPrice << std::endl;
  }

  if (!dep_market_view_.subscribe_price_type(this, HFSAT::kPriceTypeMktSizeWPrice)) {
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << HFSAT::kPriceTypeMktSizeWPrice << std::endl;
  }
}

void L1BidAskSizeFlowMini::OnMarketUpdate(const unsigned int _security_id_,
                                          const MarketUpdateInfo& cr_market_update_info_) {
  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex(2) && dep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      indicator_value_ = 0;
      NotifyIndicatorListeners(indicator_value_);
    }
  } else {
    // computing moving avg bias and the indicator value
    // of the leading(major) security
    if (_security_id_ == indep_sec_id_) {
      if (cr_market_update_info_.bestask_int_price_ - cr_market_update_info_.bestbid_int_price_ > 1) {
        current_bid_ask_bias_ = 0;
        event_count_ = 0;
        decay_value_ = 1.0;
      } else if ((cr_market_update_info_.bestbid_int_price_ != last_indep_bid_int_price_) ||
                 (cr_market_update_info_.bestask_int_price_ != last_indep_ask_int_price_)) {
        event_count_ = 1;
        decay_value_ = decay_factor_;
      } else {
        event_count_++;
        decay_value_ *= decay_factor_;
      }

      if (event_count_ == 0) {
        indicator_value_ = 0;
        moving_avg_bid_ask_bias_ = 0;
        decay_value_ = 1.0;
      } else if (event_count_ > 0) {
        current_bid_ask_bias_ = (double(cr_market_update_info_.bestbid_size_ - cr_market_update_info_.bestask_size_)) /
                                (cr_market_update_info_.bestbid_size_ + cr_market_update_info_.bestask_size_);

        if (event_count_ == 1) {
          moving_avg_bid_ask_bias_ = (1 - decay_factor_) * current_bid_ask_bias_;
          indicator_value_ = 0;
        } else {
          moving_avg_bid_ask_bias_ =
              moving_avg_bid_ask_bias_ * decay_factor_ + (1 - decay_factor_) * current_bid_ask_bias_;
          indicator_value_ = (current_bid_ask_bias_ - (moving_avg_bid_ask_bias_ / (1 - decay_value_)));
        }
      }
      last_indep_bid_int_price_ = cr_market_update_info_.bestbid_int_price_;
      last_indep_ask_int_price_ = cr_market_update_info_.bestask_int_price_;
    }

    // updating the mini secuirty best variables
    if (_security_id_ == dep_sec_id_) {
      last_dep_bid_int_price_ = cr_market_update_info_.bestbid_int_price_;
      last_dep_ask_int_price_ = cr_market_update_info_.bestask_int_price_;
    }

    if (data_interrupted_) {
      indicator_value_ = 0;
    }

    // only using the bias indicator if the book is same
    // for major and mini contract
    if ((last_dep_bid_int_price_ != last_indep_bid_int_price_) ||
        (last_dep_ask_int_price_ != last_indep_ask_int_price_)) {
      indicator_value_ = 0;
    }

    NotifyIndicatorListeners(indicator_value_);
  }
}

void L1BidAskSizeFlowMini::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                   const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  indicator_value_ = 0;
  current_bid_ask_bias_ = 0;
  moving_avg_bid_ask_bias_ = 0;
  NotifyIndicatorListeners(indicator_value_);
}

void L1BidAskSizeFlowMini::OnMarketDataResumed(const unsigned int _security_id_) { data_interrupted_ = false; }
}
