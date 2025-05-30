/**
    \file ExecLogicCode/price_pair_based_aggressive_trading_v2.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "dvctrade/ExecLogic/price_pair_based_aggressive_trading_v2.hpp"
#include "dvccode/CDef/math_utils.hpp"
// exec_logic_code / defines.hpp was empty

namespace HFSAT {

void PricePairBasedAggressiveTradingV2::CollectORSShortCodes(DebugLogger& _dbglogger_,
                                                             const std::string& r_dep_shortcode_,
                                                             std::vector<std::string>& source_shortcode_vec_,
                                                             std::vector<std::string>& ors_source_needed_vec_) {
  HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, r_dep_shortcode_);
  HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, r_dep_shortcode_);

  if (r_dep_shortcode_.compare("BR_WIN_0") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("BR_IND_0"));
    //	HFSAT::VectorUtils::UniqueVectorAdd ( ors_source_needed_vec_, std::string("BR_IND_0") );
  } else if (r_dep_shortcode_.compare("BR_IND_0") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("BR_WIN_0"));
    //	HFSAT::VectorUtils::UniqueVectorAdd ( ors_source_needed_vec_, std::string("BR_WIN_0") );
  } else if (r_dep_shortcode_.compare("NK_0") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("NKM_0"));
  } else if (r_dep_shortcode_.compare("NKM_0") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("NK_0"));
  } else if (r_dep_shortcode_.compare("BR_WDO_0") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("BR_DOL_0"));
  } else if (r_dep_shortcode_.compare("BR_DOL_0") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("BR_WDO_0"));
  } else if (r_dep_shortcode_.compare("FDXM_0") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("FDAX_0"));
  } else if (r_dep_shortcode_.compare("FDAX_0") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("FDXM_0"));
  }
}

PricePairBasedAggressiveTradingV2::PricePairBasedAggressiveTradingV2(
    DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
    const SecurityMarketView& _indep_market_view_, SmartOrderManager& _order_manager_,
    const std::string& _paramfilename_, const bool _livetrading_,
    MulticastSenderSocket* _p_strategy_param_sender_socket_, EconomicEventsManager& t_economic_events_manager_,
    const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_, const int t_runtime_id_,
    const std::vector<std::string> _this_model_source_shortcode_vec_)
    : BaseTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_, _livetrading_,
                  _p_strategy_param_sender_socket_, t_economic_events_manager_, t_trading_start_utc_mfm_,
                  t_trading_end_utc_mfm_, t_runtime_id_, _this_model_source_shortcode_vec_),
      indep_market_view_(_indep_market_view_) {
  indep_price_type_ = StringToPriceType_t(param_set_.price_type_);
  indep_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);
  indep_market_view_.subscribe_tradeprints(this);
  indep_sec_id_ = indep_market_view_.security_id();

  mini_top_bid_agg_place_ = false;
  mini_top_bid_place_ = false;
  mini_top_bid_keep_ = true;
  last_mini_top_bid_cancel_msecs_ = 0;

  mini_top_ask_agg_place_ = false;
  mini_top_ask_place_ = false;
  mini_top_ask_keep_ = true;
  last_mini_top_ask_cancel_msecs_ = 0;

  agg_buy_price_ = 0.0;
  agg_buy_int_price_ = 0;
  agg_sell_price_ = 0.0;
  agg_sell_int_price_ = 0;

  ioc_orders_sent_.clear();

  dep_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);
  dep_market_view_.subscribe_tradeprints(this);
  dep_sec_id_ = dep_market_view_.security_id();
}

void PricePairBasedAggressiveTradingV2::TradingLogic() {
  if (!indep_market_view_.is_ready()) {
    return;
  }

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance().End(25);
  HFSAT::CpucycleProfiler::GetUniqueInstance().Start(26);
  HFSAT::CpucycleProfiler::GetUniqueInstance().Start(27);
#endif
  // setting top level directives
  top_bid_place_ = false;
  top_bid_keep_ = false;
  top_bid_improve_ = false;
  top_ask_lift_ = false;
  bid_improve_keep_ = false;

  if (dbglogger_.CheckLoggingLevel(OLSMM_INFO)) {
    if (indep_market_view_.market_update_info_.bestbid_int_price_ >
            dep_market_view_.market_update_info_.bestask_int_price_ ||
        indep_market_view_.market_update_info_.bestask_int_price_ <
            dep_market_view_.market_update_info_.bestbid_int_price_) {
      int our_bid_orders_ = order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_) +
                            order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_);
      int our_ask_orders_ = order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_) +
                            order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_);
      DBGLOG_TIME_CLASS_FUNC << "Main_mkt: [ " << indep_market_view_.market_update_info_.bestbid_ordercount_ << " "
                             << indep_market_view_.market_update_info_.bestbid_size_ << " "
                             << indep_market_view_.market_update_info_.bestbid_price_ << " * "
                             << indep_market_view_.market_update_info_.bestask_price_ << " "
                             << indep_market_view_.market_update_info_.bestask_size_ << " "
                             << indep_market_view_.market_update_info_.bestask_ordercount_ << " ]. mini_mkt: [ "
                             << dep_market_view_.market_update_info_.bestbid_ordercount_ << " "
                             << dep_market_view_.market_update_info_.bestbid_size_ << " "
                             << dep_market_view_.market_update_info_.bestbid_price_ << " * "
                             << dep_market_view_.market_update_info_.bestask_price_ << " "
                             << dep_market_view_.market_update_info_.bestask_size_ << " "
                             << dep_market_view_.market_update_info_.bestask_ordercount_ << " ]" << DBGLOG_ENDL_FLUSH;
      dbglogger_ << "BidOrders: " << our_bid_orders_ << " AskOrders: " << our_ask_orders_ << " Pos: " << my_position_
                 << " Tgt Px: " << target_price_ << " best bid: " << best_nonself_bid_price_
                 << " best ask: " << best_nonself_ask_price_ << DBGLOG_ENDL_FLUSH;
      dbglogger_ << "Thresh: place: " << current_tradevarset_.l1bid_place_ << " # " << current_tradevarset_.l1ask_place_
                 << " keep: " << current_tradevarset_.l1bid_keep_ << " # " << current_tradevarset_.l1ask_keep_
                 << " agg: " << current_tradevarset_.l1bid_aggressive_ << " # "
                 << current_tradevarset_.l1ask_aggressive_ << " improve: " << current_tradevarset_.l1bid_improve_
                 << " # " << current_tradevarset_.l1ask_improve_ << DBGLOG_ENDL_FLUSH;
    }
  }

  int safe_ticks_bid_ = param_set_.indep_safe_ticks_;
  int safe_size_bid_ = param_set_.indep_safe_size_;

  int safe_ticks_ask_ = param_set_.indep_safe_ticks_;
  int safe_size_ask_ = param_set_.indep_safe_size_;

  int indep_bid_int_price_ = indep_market_view_.market_update_info_.bestbid_int_price_;
  int indep_bid_size_ = indep_market_view_.market_update_info_.bestbid_size_;
  int dep_safe_bid_int_price_ = best_nonself_bid_int_price_ + safe_ticks_bid_;

  int indep_ask_int_price_ = indep_market_view_.market_update_info_.bestask_int_price_;
  int indep_ask_size_ = indep_market_view_.market_update_info_.bestask_size_;
  int dep_safe_ask_int_price_ = best_nonself_ask_int_price_ - safe_ticks_ask_;

  mini_top_bid_agg_place_ = false;
  mini_top_bid_place_ = false;
  mini_top_bid_keep_ = true;

  agg_buy_price_ = best_nonself_ask_price_;
  agg_buy_int_price_ = best_nonself_ask_int_price_;

  agg_sell_price_ = best_nonself_bid_price_;
  agg_sell_int_price_ = best_nonself_bid_int_price_;

  double indep_base_price_in_ticks_ =
      SecurityMarketView::GetPriceFromType(indep_price_type_, indep_market_view_.market_update_info()) /
      indep_market_view_.min_price_increment();

  double indep_base_price_minus_spread_place_ = std::max(
      (double)indep_bid_int_price_,
      indep_base_price_in_ticks_ - std::max(param_set_.fixed_factor_place_,
                                            param_set_.spread_factor_place_ * indep_market_view_.spread_increments()));

  if (param_set_.mini_top_keep_logic_ && indep_base_price_minus_spread_place_ < best_nonself_bid_int_price_) {
    mini_top_bid_keep_ = false;
  }

  if (param_set_.mini_top_keep_logic_ &&
      watch_.msecs_from_midnight() - last_mini_top_bid_cancel_msecs_ < param_set_.mini_sweep_cancel_cooloff_) {
    mini_top_bid_keep_ = false;
  }

  if (  // check if we have any allowance to place orders at top level
      (current_tradevarset_.l1bid_trade_size_ > 0) &&
      // first check for cooloff_interval
      ((last_buy_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_buy_msecs_ >= param_set_.cooloff_interval_) ||
       ((best_nonself_bid_int_price_ - last_buy_int_price_) <
        (param_set_.px_band_ - 1)))  // later change setting of last_buy_int_price_ to include px_band_
      ) {
    /*
        double indep_base_price_minus_spread_agg_ = std::max(
            (double)indep_bid_int_price_,
            indep_base_price_in_ticks_ - std::max(param_set_.fixed_factor_agg_,
                                                  param_set_.spread_factor_agg_ *
       indep_market_view_.spread_increments()));
    */

    /*
        if (param_set_.mini_agg_logic_ && indep_base_price_minus_spread_agg_ > best_nonself_ask_int_price_) {
          mini_top_bid_agg_place_ = false;
          int indep_safe_buy_int_price_ =
              std::max(indep_bid_int_price_, indep_ask_int_price_ - param_set_.mini_safe_indep_agg_ticks_);
          if (param_set_.read_spread_factor_) {
            indep_safe_buy_int_price_ = floor(indep_base_price_minus_spread_agg_);
          }
          agg_buy_int_price_ = std::min(agg_buy_int_price_ + param_set_.mini_agg_ticks_, indep_safe_buy_int_price_);
          agg_buy_price_ = dep_market_view_.GetDoublePx(agg_buy_int_price_);
        }
    */

    if ((indep_base_price_minus_spread_place_ > dep_safe_bid_int_price_) ||
        (indep_base_price_minus_spread_place_ >= dep_safe_bid_int_price_ && indep_bid_size_ >= safe_size_bid_)) {
      mini_top_bid_place_ = true;
    }

    // check if the margin of buying at the price ( best_nonself_bid_price_ )
    // i.e. ( target_price_ - best_nonself_bid_price_ )
    // exceeds the threshold current_tradevarset_.l1bid_place_
    if (mini_top_bid_place_ || mini_top_bid_agg_place_ ||
        ((target_price_ - best_nonself_bid_price_ >= current_tradevarset_.l1bid_place_))) {
      top_bid_place_ = true;
      top_bid_keep_ = true;

      if (watch_.msecs_from_midnight() - last_agg_buy_msecs_ > param_set_.agg_cooloff_interval_) {
        // conditions to place aggressive orders:
        // ALLOWED_TO_AGGRESS
        // position is not too long already
        // spread narrow
        // signal strong
        if ((param_set_.allowed_to_aggress_) &&  // external control on aggressing
            (target_price_ - best_nonself_ask_price_ >= current_tradevarset_.l1bid_aggressive_ ||
             mini_top_bid_agg_place_) &&  // only LIFT offer if the margin of buying here i.e. ( target_price_ -
                                          // best_nonself_ask_price_ ) exceeds the threshold
                                          // current_tradevarset_.l1bid_aggressive_
            (my_position_ <= param_set_.max_position_to_lift_) &&
            (dep_market_view_.spread_increments() <= param_set_.max_int_spread_to_cross_)) {
          top_ask_lift_ = true;
        } else {
          top_ask_lift_ = false;
        }
      }
    } else {
      if ((target_price_ - best_nonself_bid_price_) >= current_tradevarset_.l1bid_keep_) {
        top_bid_keep_ = true;
      } else {
        top_bid_keep_ = false;
      }
    }
  }

  top_ask_place_ = false;
  top_ask_keep_ = false;
  top_ask_improve_ = false;
  top_bid_hit_ = false;
  ask_improve_keep_ = false;

  mini_top_ask_agg_place_ = false;
  mini_top_ask_place_ = false;
  mini_top_ask_keep_ = true;

  double indep_base_price_plus_spread_place_ = std::min(
      (double)indep_ask_int_price_,
      indep_base_price_in_ticks_ + std::max(param_set_.fixed_factor_place_,
                                            param_set_.spread_factor_place_ * indep_market_view_.spread_increments()));

  if (param_set_.mini_top_keep_logic_ && indep_base_price_plus_spread_place_ > best_nonself_ask_int_price_) {
    mini_top_ask_keep_ = false;
  }

  if (param_set_.mini_top_keep_logic_ &&
      watch_.msecs_from_midnight() - last_mini_top_ask_cancel_msecs_ < param_set_.mini_sweep_cancel_cooloff_) {
    mini_top_ask_keep_ = false;
  }

  // check if we have any allowance to place orders at top level
  if (current_tradevarset_.l1ask_trade_size_ > 0) {
    /*
        double indep_base_price_minus_spread_agg_ = std::min(
                    (double)indep_ask_int_price_,
                            indep_base_price_in_ticks_ + std::max(param_set_.fixed_factor_agg_,
                                            param_set_.spread_factor_agg_ * indep_market_view_.spread_increments()));
    */

    /*
        if (param_set_.mini_agg_logic_ && indep_base_price_minus_spread_agg_ < best_nonself_bid_int_price_) {
          mini_top_ask_agg_place_ = false;
          int indep_safe_sell_int_price_ =
              std::min(indep_ask_int_price_, indep_bid_int_price_ + param_set_.mini_safe_indep_agg_ticks_);
          if (param_set_.read_spread_factor_) indep_safe_sell_int_price_ = ceil(indep_base_price_minus_spread_agg_);
          agg_sell_int_price_ =
              std::max(agg_sell_int_price_ - param_set_.mini_agg_ticks_, indep_safe_sell_int_price_);
          agg_sell_price_ = dep_market_view_.GetDoublePx(agg_sell_int_price_);
        }
    */

    if ((indep_base_price_plus_spread_place_ < dep_safe_ask_int_price_) ||
        (indep_base_price_plus_spread_place_ <= dep_safe_ask_int_price_ && indep_ask_size_ >= safe_size_ask_)) {
      mini_top_ask_place_ = true;
    }
    // check if the margin of placing limit ask orders at the price ( best_nonself_ask_price_ )
    // i.e. ( best_nonself_ask_price_ - target_price_ )
    //      exceeds the threshold current_tradevarset_.l1ask_place_
    if (mini_top_ask_place_ || mini_top_ask_agg_place_ ||
        ((best_nonself_ask_price_ - target_price_ >= current_tradevarset_.l1ask_place_ - l1_bias_ - l1_order_bias_))) {
      top_ask_place_ = true;
      top_ask_keep_ = true;

      if (watch_.msecs_from_midnight() - last_agg_sell_msecs_ > param_set_.agg_cooloff_interval_) {
        // conditions to place aggressive orders:
        // ALLOWED_TO_AGGRESS
        // position is not too short already
        // spread narrow
        // signal strong
        if ((param_set_.allowed_to_aggress_) &&  // external control on aggressing
            (my_position_ >=
             param_set_.min_position_to_hit_) &&  // Don't HIT bid when my_position_ is already decently short
            (best_nonself_ask_int_price_ - best_nonself_bid_int_price_ <=
             param_set_.max_int_spread_to_cross_) &&  // Don't HIT ( cross ) when effective spread is to much
            (best_nonself_bid_price_ - target_price_ >= current_tradevarset_.l1ask_aggressive_)) {
          top_bid_hit_ = true;
        } else {
          top_bid_hit_ = false;
        }
      }
    } else {
      // signal not strog enough to place limit orders at the best ask level
      if ((best_nonself_ask_price_ - target_price_) >= current_tradevarset_.l1ask_keep_) {
        top_ask_keep_ = true;
      } else {
        top_ask_keep_ = false;
      }
    }
  }
  OrderPlacingLogic();
}

void PricePairBasedAggressiveTradingV2::OrderPlacingLogic() {
  // Active BID order management
  placed_bids_this_round_ = false;
  canceled_bids_this_round_ = false;

  if (top_ask_lift_ || mini_top_bid_agg_place_) {
    int allowance_for_aggressive_buy_ = my_position_ + order_manager_.SumBidSizes() +
                                        current_tradevarset_.l1bid_trade_size_ -
                                        std::max(param_set_.worst_case_position_, param_set_.max_position_);

    if (allowance_for_aggressive_buy_ >= 0) {
      order_manager_.CancelBidsFromFar(current_tradevarset_.l1bid_trade_size_);
    } else {
      if (mini_top_bid_agg_place_) {
        unsigned int size_to_aggress_ = -allowance_for_aggressive_buy_;
        order_manager_.SendTrade(agg_buy_price_, agg_buy_int_price_, size_to_aggress_, kTradeTypeBuy, 'A', kOrderIOC);

        ioc_orders_sent_.push_back(watch_.msecs_from_midnight());
        DBGLOG_TIME_CLASS_FUNC << " Send IOC Agg Buy @ " << agg_buy_price_ << DBGLOG_ENDL_FLUSH;
        placed_bids_this_round_ = true;
      } else {
        if (((order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_ + 1) == 0) &&
             (order_manager_.SumBidSizeConfirmedAboveIntPrice(best_nonself_bid_int_price_) == 0))) {
          order_manager_.SendTrade(best_nonself_ask_price_, best_nonself_ask_int_price_,
                                   current_tradevarset_.l1bid_trade_size_, kTradeTypeBuy, 'A', kOrderDay);
          placed_bids_this_round_ = true;
        } else {
          order_manager_.CancelBidsAboveIntPrice(best_nonself_bid_int_price_);
        }
      }
    }
  } else {
    if ((dep_market_view_.spread_increments() > 1) && (!bid_improve_keep_)) {
      int cancelled_size_ = order_manager_.CancelBidsAboveIntPrice(best_nonself_bid_int_price_);
      if (cancelled_size_ > 0) {
        canceled_bids_this_round_ = true;
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << " Cancelled Improve Bid orders Above: " << best_nonself_bid_int_price_
                                 << " Position: " << my_position_ << " ebp_t: "
                                 << (target_price_ - best_nonself_bid_price_ + bestbid_queue_hysterisis_) /
                                        dep_market_view_.min_price_increment() << " thresh_t: "
                                 << current_tradevarset_.l1bid_improve_keep_ / dep_market_view_.min_price_increment()
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  }

  if (!placed_bids_this_round_) {  // only come here if no aggressive or improve bid orders sent this cycle
    if (top_bid_place_) {
      // only place best level Bid orders when there is no active unconfirmed order at or above
      // best_nonself_bid_int_price_
      // and no confirmed order at or above the best price
      if ((order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_) == 0) &&
          (order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_) == 0) &&
          (dep_market_view_.spread_increments() <= param_set_.max_int_spread_to_place_))  // Don't place any new
                                                                                          // orders in inside market
      // if the spread is too wide
      {
        order_manager_.SendTrade(best_nonself_bid_price_, best_nonself_bid_int_price_,
                                 current_tradevarset_.l1bid_trade_size_, kTradeTypeBuy, 'B');
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << "SendTrade B of " << current_tradevarset_.l1bid_trade_size_ << " @ "
                                 << best_nonself_bid_price_ << " ebp_t: "
                                 << (target_price_ - best_nonself_bid_price_) / dep_market_view_.min_price_increment()
                                 << " thresh_t: "
                                 << current_tradevarset_.l1bid_place_ / dep_market_view_.min_price_increment()
                                 << " IntPx: " << best_nonself_bid_int_price_ << " tMktSz: " << best_nonself_bid_size_
                                 << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                 << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_
                                 << " StdevSpreadFactor: " << stdev_scaled_capped_in_ticks_ << DBGLOG_ENDL_FLUSH;
        }

        placed_bids_this_round_ = true;
      }
    } else {
      if (!top_bid_keep_) {  // cancel all orders at best_nonself_bid_price_
        int canceled_size_ = order_manager_.CancelBidsEqAboveIntPrice(best_nonself_bid_int_price_);
        if (canceled_size_ > 0) {
          canceled_bids_this_round_ = true;
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
          {
            DBGLOG_TIME_CLASS_FUNC << "Canceled B of " << canceled_size_ << " @ " << best_nonself_bid_price_
                                   << " ebp_t: "
                                   << (target_price_ - best_nonself_bid_price_ + bestbid_queue_hysterisis_) /
                                          dep_market_view_.min_price_increment() << " thresh_t: "
                                   << current_tradevarset_.l1bid_keep_ / dep_market_view_.min_price_increment()
                                   << " tMktSz: " << best_nonself_bid_size_ << " mkt: " << best_nonself_bid_size_
                                   << " @ " << best_nonself_bid_price_ << " X " << best_nonself_ask_price_ << " @ "
                                   << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
      }

      if (!mini_top_bid_keep_) {  // cancel all orders at best_nonself_bid_price_
        int canceled_size_ =
            order_manager_.CancelBidsAboveIntPrice(indep_market_view_.market_update_info_.bestbid_int_price_);
        if (canceled_size_ > 0) {
          canceled_bids_this_round_ = true;
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
          {
            DBGLOG_TIME_CLASS_FUNC << "Canceled B of " << canceled_size_ << " @ " << best_nonself_bid_price_
                                   << " ebp_t: "
                                   << (target_price_ - best_nonself_bid_price_ + bestbid_queue_hysterisis_) /
                                          dep_market_view_.min_price_increment() << " thresh_t: "
                                   << current_tradevarset_.l1bid_keep_ / dep_market_view_.min_price_increment()
                                   << " tMktSz: " << best_nonself_bid_size_ << " mkt: " << best_nonself_bid_size_
                                   << " @ " << best_nonself_bid_price_ << " X " << best_nonself_ask_price_ << " @ "
                                   << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
      }
    }
  }

  // Active ASK order management
  placed_asks_this_round_ = false;
  canceled_asks_this_round_ = false;
  if (top_bid_hit_ || mini_top_ask_agg_place_) {
    int allowance_for_aggressive_sell_ = -my_position_ + order_manager_.SumAskSizes() +
                                         current_tradevarset_.l1ask_trade_size_ -
                                         std::max(param_set_.worst_case_position_, param_set_.max_position_);

    if (allowance_for_aggressive_sell_ >= 0) {
      order_manager_.CancelAsksFromFar(
          current_tradevarset_.l1ask_trade_size_);  ///< then cancel Asks from bottom levels for the required size
    } else {
      if (mini_top_ask_agg_place_) {
        unsigned int size_to_aggress_ = -allowance_for_aggressive_sell_;
        order_manager_.SendTrade(agg_sell_price_, agg_sell_int_price_, size_to_aggress_, kTradeTypeSell, 'A',
                                 kOrderIOC);

        ioc_orders_sent_.push_back(watch_.msecs_from_midnight());
        placed_asks_this_round_ = true;
        DBGLOG_TIME_CLASS_FUNC << " Send IOC Agg Sell @ " << agg_sell_price_ << DBGLOG_ENDL_FLUSH;
      } else {
        if (((order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_ - 1) == 0) &&
             (order_manager_.SumAskSizeConfirmedAboveIntPrice(best_nonself_ask_int_price_) == 0))) {
          order_manager_.SendTrade(best_nonself_bid_price_, best_nonself_bid_int_price_,
                                   current_tradevarset_.l1ask_trade_size_, kTradeTypeSell, 'A', kOrderDay);
          placed_asks_this_round_ = true;
        } else {
          order_manager_.CancelAsksAboveIntPrice(
              best_nonself_ask_int_price_);  // only after canceling them can we be allowed to place aggressive orders
          placed_asks_this_round_ = true;
        }
      }
    }
  } else {
    if (dep_market_view_.spread_increments() > 1 && !ask_improve_keep_) {
      int cancelled_size_ = order_manager_.CancelAsksAboveIntPrice(best_nonself_ask_int_price_);

      if (cancelled_size_ > 0) {
        canceled_asks_this_round_ = true;
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << " Cancelling Improve Ask orders Above: " << best_nonself_ask_int_price_
                                 << " position: " << my_position_ << " eap_t: "
                                 << (best_nonself_ask_price_ - target_price_ + bestask_queue_hysterisis_) /
                                        dep_market_view_.min_price_increment() << " thresh_t: "
                                 << current_tradevarset_.l1ask_improve_keep_ / dep_market_view_.min_price_increment()
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  }

  if (!placed_asks_this_round_) {
    // get to this location if no aggressive or improve orders placed in this cycle
    if (top_ask_place_) {
      // only place Ask orders when there is no active unconfirmed order at or above best_nonself_ask_int_price_
      //      and no confirmed order at or above the best price
      if ((order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_) == 0) &&
          (order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_) == 0) &&
          (stdev_ <= param_set_.low_stdev_lvl_ ||
           (dep_market_view_.spread_increments() <= param_set_.max_int_spread_to_place_)))  // Don't place any new
                                                                                            // orders in inside market
      // if the spread is too wide
      {
        order_manager_.SendTrade(best_nonself_ask_price_, best_nonself_ask_int_price_,
                                 current_tradevarset_.l1ask_trade_size_, kTradeTypeSell, 'B');
        placed_asks_this_round_ = true;
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << "SendTrade S of " << current_tradevarset_.l1ask_trade_size_ << " @ "
                                 << best_nonself_ask_price_ << " eap_t: "
                                 << (best_nonself_ask_price_ - target_price_) / dep_market_view_.min_price_increment()
                                 << " thresh_t: "
                                 << current_tradevarset_.l1ask_place_ / dep_market_view_.min_price_increment()
                                 << " IntPx: " << best_nonself_ask_int_price_ << " tMktSz: " << best_nonself_ask_size_
                                 << " mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " X "
                                 << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_
                                 << " StdevSpreadFactor: " << stdev_scaled_capped_in_ticks_ << DBGLOG_ENDL_FLUSH;
        }
      }
    } else {
      if (!top_ask_keep_) {
        // cancel all orders at best_nonself_ask_price_
        int canceled_size_ = order_manager_.CancelAsksEqAboveIntPrice(best_nonself_ask_int_price_);
        if (canceled_size_ > 0) {
          canceled_asks_this_round_ = true;
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
          {
            DBGLOG_TIME_CLASS_FUNC << "Canceled S of " << canceled_size_ << " @ " << best_nonself_ask_price_
                                   << " eap_t: "
                                   << (best_nonself_ask_price_ - target_price_ + bestask_queue_hysterisis_) /
                                          dep_market_view_.min_price_increment() << " thresh_t: "
                                   << current_tradevarset_.l1ask_keep_ / dep_market_view_.min_price_increment()
                                   << " tMktSz: " << best_nonself_ask_size_ << " mkt: " << best_nonself_bid_size_
                                   << " @ " << best_nonself_bid_price_ << " X " << best_nonself_ask_price_ << " @ "
                                   << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
      }

      if (!mini_top_ask_keep_) {
        // cancel all orders at best_nonself_ask_price_
        int canceled_size_ =
            order_manager_.CancelAsksAboveIntPrice(indep_market_view_.market_update_info_.bestask_int_price_);
        if (canceled_size_ > 0) {
          canceled_asks_this_round_ = true;
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
          {
            DBGLOG_TIME_CLASS_FUNC << "Canceled S of " << canceled_size_ << " @ " << best_nonself_ask_price_
                                   << " eap_t: "
                                   << (best_nonself_ask_price_ - target_price_ + bestask_queue_hysterisis_) /
                                          dep_market_view_.min_price_increment() << " thresh_t: "
                                   << current_tradevarset_.l1ask_keep_ / dep_market_view_.min_price_increment()
                                   << " tMktSz: " << best_nonself_ask_size_ << " mkt: " << best_nonself_bid_size_
                                   << " @ " << best_nonself_bid_price_ << " X " << best_nonself_ask_price_ << " @ "
                                   << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
      }
    }
  }

  // zero logging
  if ((dbglogger_.CheckLoggingLevel(TRADING_INFO) && (placed_bids_this_round_ || placed_asks_this_round_))) {
    PrintFullStatus();
  }

  // temporarily printing indicators every time
  if ((livetrading_ || dbglogger_.CheckLoggingLevel(TRADING_INFO)) &&
      (placed_bids_this_round_ || placed_asks_this_round_ || canceled_bids_this_round_ || canceled_asks_this_round_)) {
    dump_inds = true;
  }
}

void PricePairBasedAggressiveTradingV2::PrintFullStatus() {
  DBGLOG_TIME << "tgt: " << target_price_ << " mkt " << best_nonself_bid_price_ << " X " << best_nonself_bid_size_
              << " pft: " << (target_price_ - best_nonself_bid_price_) << " -- " << best_nonself_ask_price_ << " X "
              << best_nonself_ask_size_ << " pft: " << (best_nonself_ask_price_ - target_price_) << " bias: "
              << ((target_price_ - dep_market_view_.mkt_size_weighted_price()) / dep_market_view_.min_price_increment())
              << " pos: " << my_position_ << " smaps "
              << order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_) << " "
              << order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_) << " "
              << order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_) << " "
              << order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_)
              << " opnl: " << order_manager_.base_pnl().opentrade_unrealized_pnl() << " gpos: " << my_global_position_
              << DBGLOG_ENDL_FLUSH;
}
}
