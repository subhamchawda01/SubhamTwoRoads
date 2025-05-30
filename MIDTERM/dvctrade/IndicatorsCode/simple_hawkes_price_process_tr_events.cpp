/**
    \file IndicatorsCode/simple_hawkes_price_process_tr_events.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/simple_hawkes_price_process_tr_events.hpp"

namespace HFSAT {

void SimpleHawkesPriceProcessTREvents::CollectShortCodes(
    std::vector<std::string>& _shortcodes_affecting_this_indicator_, std::vector<std::string>& _ors_source_needed_vec_,
    const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

SimpleHawkesPriceProcessTREvents* SimpleHawkesPriceProcessTREvents::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::vector<const char*>& r_tokens_,
    PriceType_t _basepx_pxtype_) {
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), _basepx_pxtype_);
}

SimpleHawkesPriceProcessTREvents* SimpleHawkesPriceProcessTREvents::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const SecurityMarketView& _indep_market_view_, double _beta_,
    PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _beta_ << ' ' << ' '
              << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, SimpleHawkesPriceProcessTREvents*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new SimpleHawkesPriceProcessTREvents(
        t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, _beta_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

SimpleHawkesPriceProcessTREvents::SimpleHawkesPriceProcessTREvents(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                   const std::string& concise_indicator_description_,
                                                                   const SecurityMarketView& _indep_market_view_,
                                                                   double _beta_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      num_moves_(5),  // hardcoded as it is not causing much difference
      beta_(_beta_),
      bid_trade_index_(-1),
      bid_cycle_flag_(0),
      ask_trade_index_(-1),
      ask_cycle_flag_(0),
      last_bid_int_price_(0),
      last_ask_int_price_(0),
      num_trades_(0) {
  if (!indep_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice)) {
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << kPriceTypeMktSizeWPrice << std::endl;
  }

  for (int i = 0; i < num_moves_; i++) {
    ask_trade_vec_.push_back(0);
    bid_trade_vec_.push_back(0);
  }

  exp_beta_.clear();
  for (int i = 0; i < 100; i++) {
    exp_beta_.push_back(exp(-beta_ * i));
  }
}

void SimpleHawkesPriceProcessTREvents::OnTradePrint(const unsigned int _security_id_,
                                                    const TradePrintInfo& cr_trade_print_info_,
                                                    const MarketUpdateInfo& cr_market_update_info_) {
  if (cr_trade_print_info_.num_trades_ > num_trades_) {
    if (!is_ready_) {
      if (indep_market_view_.is_ready() &&
          (indep_market_view_.spread_increments() < (2 * indep_market_view_.normal_spread_increments()))) {
        is_ready_ = true;

        indicator_value_ = 0;
      }
    } else if (!data_interrupted_) {
      if ((last_bid_int_price_ != cr_market_update_info_.bestbid_int_price_) ||
          (last_ask_int_price_ != cr_market_update_info_.bestask_int_price_)) {
        AdjustLevelVars();
        last_bid_int_price_ = cr_market_update_info_.bestbid_int_price_;
        last_ask_int_price_ = cr_market_update_info_.bestask_int_price_;
      }

      if (cr_trade_print_info_.buysell_ == kTradeTypeBuy) {
        bid_cycle_flag_ = 1;
        bid_trade_index_ = (bid_trade_index_ + 1) % bid_trade_vec_.size();
        bid_trade_vec_[bid_trade_index_] = cr_trade_print_info_.num_trades_;
      } else if (cr_trade_print_info_.buysell_ == kTradeTypeSell) {
        ask_cycle_flag_ = 1;
        ask_trade_index_ = (ask_trade_index_ + 1) % ask_trade_vec_.size();
        ask_trade_vec_[ask_trade_index_] = cr_trade_print_info_.num_trades_;
      }

      num_trades_ = cr_trade_print_info_.num_trades_;

      double ask_trade_prob_ = 0.0;
      double bid_trade_prob_ = 0.0;

      for (int i = 0; i <= std::max(bid_cycle_flag_ * num_moves_ - 1, bid_trade_index_); i++) {
        int exponent_ = num_trades_ - bid_trade_vec_[i];

        if (exponent_ < 100) {
          bid_trade_prob_ += exp_beta_[exponent_];
        } else {
          bid_trade_prob_ += exp(-beta_ * exponent_);
        }
      }
      bid_trade_prob_ = bid_trade_prob_ / num_moves_;

      for (int i = 0; i <= std::max(ask_cycle_flag_ * num_moves_ - 1, ask_trade_index_); i++) {
        int exponent_ = num_trades_ - ask_trade_vec_[i];

        if (exponent_ < 100) {
          ask_trade_prob_ += exp_beta_[exponent_];
        } else {
          ask_trade_prob_ += exp(-beta_ * exponent_);
        }
      }
      ask_trade_prob_ = ask_trade_prob_ / num_moves_;

      indicator_value_ = bid_trade_prob_ - ask_trade_prob_;
    }

    NotifyIndicatorListeners(indicator_value_);
  }
}

void SimpleHawkesPriceProcessTREvents::InitializeValues() { indicator_value_ = 0; }

// market_interrupt_listener interface
void SimpleHawkesPriceProcessTREvents::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                               const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void SimpleHawkesPriceProcessTREvents::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    InitializeValues();
    AdjustLevelVars();
    data_interrupted_ = false;
  }
}
}
