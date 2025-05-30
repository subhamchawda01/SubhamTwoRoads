/**
    \file IndicatorsCode/ors_self_exec.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/ors_self_exec_td.hpp"

namespace HFSAT {

void ORSSelfExecTD::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                      std::vector<std::string>& _ors_source_needed_vec_,
                                      const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_ors_source_needed_vec_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

ORSSelfExecTD* ORSSelfExecTD::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                const std::vector<const char*>& r_tokens_,
                                                PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _factor_reduced_
  if (r_tokens_.size() < 5) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "BidAskToPay incorrect syntax. Should be INDICATOR _this_weight_ _indicator_string_ "
                "_indep_market_view_ factor_reduced_");
  }
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), atof(r_tokens_[5]));
}

ORSSelfExecTD* ORSSelfExecTD::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                SecurityMarketView& _indep_market_view_, double _factor_reduced_,
                                                double _decay_factor_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _factor_reduced_ << ' ' << _decay_factor_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, ORSSelfExecTD*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new ORSSelfExecTD(
        t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, _factor_reduced_, _decay_factor_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

ORSSelfExecTD::ORSSelfExecTD(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                             const std::string& concise_indicator_description_,
                             SecurityMarketView& t_indep_market_view_, double _factor_reduced_, double _decay_factor_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(t_indep_market_view_),
      factor_reduced_(_factor_reduced_),
      decay_factor_(_decay_factor_) {
  t_indep_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);
}

void ORSSelfExecTD::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& cr_market_update_info_) {
  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      indicator_value_ = 0;
      NotifyIndicatorListeners(indicator_value_);
    }
  } else {
    indicator_value_ = indicator_value_ * decay_factor_;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void ORSSelfExecTD::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                                 const MarketUpdateInfo& _market_update_info_) {
  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      indicator_value_ = 0;
      NotifyIndicatorListeners(indicator_value_);
    }
  } else {
    indicator_value_ = indicator_value_ * decay_factor_;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void ORSSelfExecTD::OrderExecuted(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                  const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                  const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                                  const int _size_executed_, const int _client_position_, const int _global_position_,
                                  const int r_int_price_, const int32_t server_assigned_message_sequence,
                                  const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  if (r_buysell_ == kTradeTypeBuy) {
    if (r_int_price_ < indep_market_view_.market_update_info_.bestbid_int_price_) {
      indicator_value_ = _price_ - indep_market_view_.market_update_info_.mkt_size_weighted_price_;
    } else {
      int reduced_bid_size_ = indep_market_view_.market_update_info_.bestbid_size_;
      double adjusted_price_ = indep_market_view_.market_update_info_.mkt_size_weighted_price_;

      // here we can use a much more informed guess using a logic similar to the estimation of queue_size_ahead in
      // base_order_manager
      // it is very likely that if I got filled others also cancelled their orders and size has gone down further.
      // right now using a fixed reduction_factor.

      int size_to_be_removed_ = (1 - factor_reduced_) * reduced_bid_size_;
      // if the size_to_be_removed_ is less than the size_executed_ output lower level as bestbid_price_
      if (size_to_be_removed_ < _size_executed_ ||
          size_to_be_removed_ > indep_market_view_.market_update_info_.bestbid_size_) {
        adjusted_price_ =
            (indep_market_view_.market_update_info_.bestbid_price_ - indep_market_view_.min_price_increment() +
             indep_market_view_.market_update_info_.bestask_price_) /
            2.0;
        indicator_value_ = adjusted_price_ - indep_market_view_.market_update_info_.mkt_size_weighted_price_;
      } else {
        reduced_bid_size_ -= size_to_be_removed_;
        if (indep_market_view_.spread_increments() > 1) {
          adjusted_price_ =
              (indep_market_view_.market_update_info_.bestbid_price_ - indep_market_view_.min_price_increment() +
               indep_market_view_.market_update_info_.bestask_price_) /
              2.0;
          indicator_value_ = adjusted_price_ - indep_market_view_.market_update_info_.mkt_size_weighted_price_;
        } else {
          // output mkt_size_weighted_price with reduced_bid_size_ as the correct price_
          adjusted_price_ = (indep_market_view_.market_update_info_.bestbid_price_ *
                                 indep_market_view_.market_update_info_.bestask_size_ +
                             indep_market_view_.market_update_info_.bestask_price_ * reduced_bid_size_) /
                            (indep_market_view_.market_update_info_.bestask_size_ + reduced_bid_size_);
          indicator_value_ = adjusted_price_ - indep_market_view_.market_update_info_.mkt_size_weighted_price_;
        }
      }
    }
  } else {
    if (r_int_price_ > indep_market_view_.market_update_info_.bestask_int_price_) {
      indicator_value_ = _price_ - indep_market_view_.market_update_info_.mkt_size_weighted_price_;
    } else {
      int reduced_ask_size_ = indep_market_view_.market_update_info_.bestask_size_;
      double adjusted_price_ = indep_market_view_.market_update_info_.mkt_size_weighted_price_;
      // here we can use a much more informed guess using a logic similar to the estimation of queue_size_ahead in
      // base_order_manager
      // it is very likely that if I got filled others also cancelled their orders and size has gone down further.
      // right now using a fixed reduction_factor.
      int size_to_be_removed_ = (1 - factor_reduced_) * reduced_ask_size_;
      // if the size_to_be_removed_ is less than the size_executed_ put output higher level as best_ask_price_
      if (size_to_be_removed_ < _size_executed_ ||
          size_to_be_removed_ > indep_market_view_.market_update_info_.bestask_size_) {
        adjusted_price_ =
            (indep_market_view_.market_update_info_.bestask_price_ + indep_market_view_.min_price_increment() +
             indep_market_view_.market_update_info_.bestask_price_) /
            2.0;
        indicator_value_ = adjusted_price_ - indep_market_view_.market_update_info_.mkt_size_weighted_price_;
      } else {
        reduced_ask_size_ -= size_to_be_removed_;
        if (indep_market_view_.spread_increments() > 1) {
          adjusted_price_ =
              (indep_market_view_.market_update_info_.bestask_price_ + indep_market_view_.min_price_increment() +
               indep_market_view_.market_update_info_.bestbid_price_) /
              2.0;
          indicator_value_ = adjusted_price_ - indep_market_view_.market_update_info_.mkt_size_weighted_price_;
        } else {
          // output mkt_size_weighted_price with reduced_bid_size_ as the correct price_
          adjusted_price_ = (indep_market_view_.market_update_info_.bestbid_price_ * reduced_ask_size_ +
                             indep_market_view_.market_update_info_.bestask_price_ *
                                 indep_market_view_.market_update_info_.bestbid_size_) /
                            (indep_market_view_.market_update_info_.bestbid_size_ + reduced_ask_size_);
          indicator_value_ = adjusted_price_ - indep_market_view_.market_update_info_.mkt_size_weighted_price_;
        }
      }
    }
  }
  NotifyIndicatorListeners(indicator_value_);
}

void ORSSelfExecTD::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void ORSSelfExecTD::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
  }
}
}
