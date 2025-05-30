/**
    \file ExecLogicCode/price_pair_based_aggressive_trading.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "dvctrade/ExecLogic/price_pair_based_aggressive_trading.hpp"
#include "dvccode/CDef/math_utils.hpp"
// exec_logic_code / defines.hpp was empty

namespace HFSAT {

void PricePairBasedAggressiveTrading::CollectORSShortCodes(DebugLogger& _dbglogger_,
                                                           const std::string& r_dep_shortcode_,
                                                           std::vector<std::string>& source_shortcode_vec_,
                                                           std::vector<std::string>& ors_source_needed_vec_) {
  HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, r_dep_shortcode_);
  HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, r_dep_shortcode_);

  // Adding NK for SGX_NK price pair trading
  if (r_dep_shortcode_.compare("SGX_NK_0") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("NK_0"));
  }
  if (r_dep_shortcode_.compare("BR_WIN_0") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("BR_IND_0"));
    //	HFSAT::VectorUtils::UniqueVectorAdd ( ors_source_needed_vec_, std::string("BR_IND_0") );
  } else if (r_dep_shortcode_.compare("BR_IND_0") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("BR_WIN_0"));
    //	HFSAT::VectorUtils::UniqueVectorAdd ( ors_source_needed_vec_, std::string("BR_WIN_0") );
  }

  if (r_dep_shortcode_.compare("HSI_0") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("MHI_0"));
    //      HFSAT::VectorUtils::UniqueVectorAdd ( ors_source_needed_vec_, std::string("BR_IND_0") );
  } else if (r_dep_shortcode_.compare("MHI_0") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("HSI_0"));
    //      HFSAT::VectorUtils::UniqueVectorAdd ( ors_source_needed_vec_, std::string("BR_WIN_0") );
  } else if (r_dep_shortcode_.compare("HHI_0") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("MCH_0"));
  } else if (r_dep_shortcode_.compare("MCH_0") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("HHI_0"));
  } else if (r_dep_shortcode_.compare("NK_0") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("NKM_0"));
  } else if (r_dep_shortcode_.compare("NKM_0") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("NK_0"));
  } else if (r_dep_shortcode_.compare("BR_WDO_0") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("BR_DOL_0"));
  } else if (r_dep_shortcode_.compare("BR_DOL_0") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("BR_WDO_0"));
  } else if (r_dep_shortcode_.compare("Si_0") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("USD000UTSTOM"));
  } else if (r_dep_shortcode_.compare("BR_ISP_0") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("ES_0"));
  } else if (r_dep_shortcode_.compare("FDXM_0") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("FDAX_0"));
  } else if (r_dep_shortcode_.compare("FDAX_0") == 0) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("FDXM_0"));
  }
}

PricePairBasedAggressiveTrading::PricePairBasedAggressiveTrading(
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

  dep_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);
  dep_market_view_.subscribe_tradeprints(this);
  dep_sec_id_ = dep_market_view_.security_id();
  // Adding a min price increment in the threshold
  // so that it doesn't need to remove at run time at every call
  //
  param_set_.mkt_price_improve_bias_ = param_set_.mkt_price_improve_bias_ * dep_market_view_.min_price_increment() +
                                       dep_market_view_.min_price_increment();
}

/*
Function that loads up certain data structures to warm the CPU cache for actual trade scenarios.
*/
void PricePairBasedAggressiveTrading::FakeTradingLogic() {
  // Loads indep_market_view
  int indep_bid_int_price_ = indep_market_view_.market_update_info_.bestbid_int_price_;
  // Loads dep_market_view
  int min_price_increment = dep_market_view_.min_price_increment();
  // Loads param_set_
  int cooloff_interval_ = param_set_.bigtrades_cooloff_interval_;

  // Impossible if branch so that compiler doesn't optimize it out.
  if (indep_bid_int_price_ == -10001) {
    dbglogger_ << "Error: These values shouldn't be present " << indep_bid_int_price_ << min_price_increment
               << cooloff_interval_ << DBGLOG_ENDL_FLUSH;
  }

  order_manager_.SumBidAskSizesFake();

  // Call both of them or alternate between these two?
  order_manager_.FakeCancel();
  order_manager_.FakeSendTrade();
}

void PricePairBasedAggressiveTrading::TradingLogic() {
  if (!indep_market_view_.is_ready_complex(2)) {
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

  double safe_ticks_bid_ = param_set_.indep_safe_ticks_;
  int safe_size_bid_ = param_set_.indep_safe_size_;

  double safe_ticks_ask_ = param_set_.indep_safe_ticks_;
  int safe_size_ask_ = param_set_.indep_safe_size_;

  if (param_set_.read_max_unit_ratio_pp_) {
    if (my_position_ > param_set_.max_position_pp_) {
      safe_ticks_bid_ = param_set_.indep_safe_ticks_low_pos_;
      safe_size_bid_ = param_set_.indep_safe_size_low_pos_;
    } else if (-my_position_ > param_set_.max_position_pp_) {
      safe_ticks_ask_ = param_set_.indep_safe_ticks_low_pos_;
      safe_size_ask_ = param_set_.indep_safe_size_low_pos_;
    }
  }

  int indep_bid_int_price_ = indep_market_view_.market_update_info_.bestbid_int_price_;
  int indep_bid_price_ = indep_market_view_.market_update_info_.bestbid_price_;
  int indep_bid_size_ = indep_market_view_.market_update_info_.bestbid_size_;
  double dep_safe_bid_int_price_ = best_nonself_bid_int_price_ + safe_ticks_bid_;

  int indep_ask_int_price_ = indep_market_view_.market_update_info_.bestask_int_price_;
  int indep_ask_price_ = indep_market_view_.market_update_info_.bestask_price_;
  int indep_ask_size_ = indep_market_view_.market_update_info_.bestask_size_;
  double dep_safe_ask_int_price_ = best_nonself_ask_int_price_ - safe_ticks_ask_;

  bool mini_top_bid_agg_place_ = false;
  bool mini_top_bid_improve_place_ = false;
  bool mini_top_bid_place_ = false;
  bool mini_top_bid_keep_ = true;

  double agg_buy_price_ = best_nonself_ask_price_;
  int agg_buy_int_price_ = best_nonself_ask_int_price_;
  double improve_buy_price_ = best_nonself_bid_price_ + dep_market_view_.min_price_increment();
  int improve_buy_int_price_ = best_nonself_bid_int_price_ + 1;

  double agg_sell_price_ = best_nonself_bid_price_;
  int agg_sell_int_price_ = best_nonself_bid_int_price_;
  double improve_sell_price_ = best_nonself_ask_price_ - dep_market_view_.min_price_increment();
  int improve_sell_int_price_ = best_nonself_ask_int_price_ - 1;

  // Mkt price to be used to check condition for improve
  // based on rules
  double indep_mkt_price_ =
      SecurityMarketView::GetPriceFromType(kPriceTypeMktSizeWPrice, indep_market_view_.market_update_info());

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

  if (  // check if we have any allowance to place orders at top level
      (current_tradevarset_.l1bid_trade_size_ > 0) &&
      // If the sell trades on the source crosses the cancel threshold it will not place orders on the buy side
      (last_bigtrades_bid_cancel_msecs_ <= 0 ||
       (watch_.msecs_from_midnight() - last_bigtrades_bid_cancel_msecs_ >= param_set_.bigtrades_cooloff_interval_)) &&
      // first check for cooloff_interval
      ((last_buy_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_buy_msecs_ >= param_set_.cooloff_interval_) ||
       ((best_nonself_bid_int_price_ - last_buy_int_price_) <
        (param_set_.px_band_ - 1)))  // later change setting of last_buy_int_price_ to include px_band_
      ) {
    double indep_base_price_minus_spread_agg_ = std::max(
        (double)indep_bid_int_price_,
        indep_base_price_in_ticks_ - std::max(param_set_.fixed_factor_agg_,
                                              param_set_.spread_factor_agg_ * indep_market_view_.spread_increments()));

    if (param_set_.mini_agg_logic_ && indep_base_price_minus_spread_agg_ > best_nonself_ask_int_price_) {
      mini_top_bid_agg_place_ = true;
    }

    // Improving based on mini only if the conditions are met
    // and aggress on mini based on rules is false
    if (param_set_.mini_improve_logic_ && !mini_top_bid_agg_place_) {
      if (((indep_bid_int_price_ - param_set_.indep_safe_improve_ticks_ > best_nonself_bid_int_price_ + 1) ||
           ((indep_bid_int_price_ - param_set_.indep_safe_improve_ticks_ >= best_nonself_bid_int_price_ + 1) &&
            indep_bid_size_ >= param_set_.indep_safe_improve_size_)) &&
          ((indep_mkt_price_ - indep_bid_price_) >= param_set_.mkt_price_improve_bias_)) {
        mini_top_bid_improve_place_ = true;
      }
    }

    if ((indep_base_price_minus_spread_place_ > dep_safe_bid_int_price_) ||
        (indep_base_price_minus_spread_place_ >= dep_safe_bid_int_price_ && indep_bid_size_ >= safe_size_bid_)) {
      mini_top_bid_place_ = true;
    }

    /*
      if ( param_set_.mini_agg_logic_ && indep_bid_int_price_ > best_nonself_ask_int_price_ )
          {
            mini_top_bid_agg_place_ = true;
          }

        if ( param_set_.mini_top_keep_logic_ && indep_bid_int_price_ < best_nonself_bid_int_price_ )
          {
            mini_top_bid_keep_ = false;
          }

        if ( indep_bid_int_price_ > dep_safe_bid_int_price_ ||
            ( indep_bid_int_price_ >= dep_safe_bid_int_price_ && indep_bid_size_ >= param_set_.indep_safe_size_ ) )
          {
            mini_top_bid_place_ = true;
          }
     */
    // check if the margin of buying at the price ( best_nonself_bid_price_ )
    // i.e. ( target_price_ - best_nonself_bid_price_ )
    // exceeds the threshold current_tradevarset_.l1bid_place_
    if ((best_nonself_bid_size_ > param_set_.safe_distance_) || (mini_top_bid_place_) || mini_top_bid_agg_place_ ||
        mini_top_bid_improve_place_ ||
        // If the buy trades on the source crosses place threshold then it will place on best level irrespective of the
        // indicators
        (watch_.msecs_from_midnight() - last_bigtrades_bid_place_msecs_ <=
         param_set_.bigtrades_place_cooloff_interval_) ||
        // If the buy trades on the source crosses aggress threshold then it will place on best level irrespective of
        // the indicators
        (watch_.msecs_from_midnight() - last_bigtrades_bid_aggress_msecs_ <=
         param_set_.bigtrades_aggress_cooloff_interval_) ||
        ((target_price_ - best_nonself_bid_price_ >= current_tradevarset_.l1bid_place_ - l1_bias_ - l1_order_bias_) &&
         (best_nonself_bid_size_ > param_set_.min_size_to_join_))) {
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
             // If the buy trades on the source crosses aggress threshold then it will aggress
             (watch_.msecs_from_midnight() - last_bigtrades_bid_aggress_msecs_ <=
              param_set_.bigtrades_aggress_cooloff_interval_) ||
             mini_top_bid_agg_place_) &&  // only LIFT offer if the margin of buying here i.e. ( target_price_ -
                                          // best_nonself_ask_price_ ) exceeds the threshold
                                          // current_tradevarset_.l1bid_aggressive_
            (my_position_ <=
             param_set_.max_position_to_lift_) &&  // Don't LIFT offer when my_position_ is already decently long
            (dep_market_view_.spread_increments() <=
             param_set_.max_int_spread_to_cross_) &&  // Don't LIFT when effective spread is to much
            ((last_buy_msecs_ <= 0) ||
             (watch_.msecs_from_midnight() - last_buy_msecs_ >= param_set_.cooloff_interval_) ||
             ((best_nonself_ask_int_price_ - last_buy_int_price_) <
              (param_set_.px_band_ - 1)))  // TODO_OPT : later change setting of last_buy_int_price_ to include px_band_
            ) {
          top_ask_lift_ = true;

          if (mini_top_bid_agg_place_) {
            int indep_safe_buy_int_price_ =
                std::max(indep_bid_int_price_, indep_ask_int_price_ - param_set_.mini_safe_indep_agg_ticks_);
            if (param_set_.read_spread_factor_) {
              indep_safe_buy_int_price_ = floor(indep_base_price_minus_spread_agg_);
            }
            agg_buy_int_price_ = std::min(agg_buy_int_price_ + param_set_.mini_agg_ticks_, indep_safe_buy_int_price_);
            agg_buy_price_ = dep_market_view_.GetDoublePx(agg_buy_int_price_);
          }

          // when we are already long and ask_lift is set to true,
          // place and keep at top bid are set false so that
          // we cancel top level orders before we place new ones at aggressive prices
          if (my_position_ >= param_set_.max_position_to_cancel_on_lift_) {
            top_bid_place_ = false;
            top_bid_keep_ = false;
          }
        } else {
          top_ask_lift_ = false;

          // conditions to place market improving bid orders:
          // ALLOWED_TO_IMPROVE
          // position is not too long already
          // spread wide
          // signal strong
          if ((param_set_.allowed_to_improve_) &&
              (my_position_ <=
               param_set_
                   .max_position_to_bidimprove_) &&  // Don't improve bid when my_position_ is already decently long
              (dep_market_view_.spread_increments() >= param_set_.min_int_spread_to_improve_) &&
              ((target_price_ - best_nonself_bid_price_ - dep_market_view_.min_price_increment() >=
                current_tradevarset_.l1bid_improve_) ||
               // If the buy trades on the source crosses aggress threshold then it will try to improve the best level
               (watch_.msecs_from_midnight() - last_bigtrades_bid_aggress_msecs_ <=
                param_set_.bigtrades_aggress_cooloff_interval_) ||
               mini_top_bid_improve_place_) &&
              ((last_buy_msecs_ <= 0) ||
               (watch_.msecs_from_midnight() - last_buy_msecs_ >= param_set_.cooloff_interval_) ||
               (((best_nonself_bid_int_price_ + 1) - last_buy_int_price_) <
                (param_set_.px_band_ -
                 1)))  // TODO_OPT : later change setting of last_buy_int_price_ to include px_band_
              ) {
            top_bid_improve_ = true;
          } else {
            top_bid_improve_ = false;
          }
        }
      }

    } else {
      // either size less than min_size_to_join_
      // or signal is not strong enough for placing bids at best_nonself_bid_price_
      // check if we should retain existing bids due to place_in_line

      // TODO : perhaps we should make place_in_line effect bestbid_queue_hysterisis_ smarter
      //        for instance when short term volatility in the market is very high
      //        then being high in the queue should count for less.
      if ((target_price_ - best_nonself_bid_price_ + bestbid_queue_hysterisis_) >=
          current_tradevarset_.l1bid_keep_ - l1_bias_ - l1_order_bias_) {
        top_bid_keep_ = true;
      } else {
        top_bid_keep_ = false;
      }
    }

    if (((dep_market_view_.spread_increments() > 1) &&
         (target_price_ - best_nonself_bid_price_) >=
             current_tradevarset_.l1bid_improve_keep_ - l1_bias_ - l1_order_bias_)) {
      bid_improve_keep_ = true;
    } else {
      bid_improve_keep_ = false;
    }
  }

  top_ask_place_ = false;
  top_ask_keep_ = false;
  top_ask_improve_ = false;
  top_bid_hit_ = false;
  ask_improve_keep_ = false;

  bool mini_top_ask_agg_place_ = false;
  bool mini_top_ask_improve_place_ = false;
  bool mini_top_ask_place_ = false;
  bool mini_top_ask_keep_ = true;

  double indep_base_price_plus_spread_place_ = std::min(
      (double)indep_ask_int_price_,
      indep_base_price_in_ticks_ + std::max(param_set_.fixed_factor_place_,
                                            param_set_.spread_factor_place_ * indep_market_view_.spread_increments()));

  if (param_set_.mini_top_keep_logic_ && indep_base_price_plus_spread_place_ > best_nonself_ask_int_price_) {
    mini_top_ask_keep_ = false;
  }

  if (  // check if we have any allowance to place orders at top level
      (current_tradevarset_.l1ask_trade_size_ > 0) &&
      // If the buy trades on the source crosses the cancel threshold it will not place orders on the sell side
      (last_bigtrades_ask_cancel_msecs_ <= 0 ||
       (watch_.msecs_from_midnight() - last_bigtrades_ask_cancel_msecs_ >= param_set_.bigtrades_cooloff_interval_)) &&
      // first check for cooloff_interval
      ((last_sell_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_sell_msecs_ >= param_set_.cooloff_interval_) ||
       (last_sell_int_price_ - best_nonself_ask_int_price_ <
        (param_set_.px_band_ - 1)))  // later change setting of last_sell_int_price_ to include px_band_
      ) {
    double indep_base_price_minus_spread_agg_ = std::min(
        (double)indep_ask_int_price_,
        indep_base_price_in_ticks_ + std::max(param_set_.fixed_factor_agg_,
                                              param_set_.spread_factor_agg_ * indep_market_view_.spread_increments()));

    if (param_set_.mini_agg_logic_ && indep_base_price_minus_spread_agg_ < best_nonself_bid_int_price_) {
      mini_top_ask_agg_place_ = true;
    }

    // Improving based on mini only if the conditions are met
    // and aggress on mini based on rules is false
    if (param_set_.mini_improve_logic_ && !mini_top_ask_agg_place_) {
      if (((indep_ask_int_price_ + param_set_.indep_safe_improve_ticks_ < best_nonself_ask_int_price_ - 1) ||
           ((indep_ask_int_price_ + param_set_.indep_safe_improve_ticks_ <= best_nonself_ask_int_price_ - 1) &&
            indep_ask_size_ >= param_set_.indep_safe_improve_size_)) &&
          (indep_ask_price_ - indep_mkt_price_) >= param_set_.mkt_price_improve_bias_) {
        mini_top_ask_improve_place_ = true;
      }
    }

    if ((indep_base_price_plus_spread_place_ < dep_safe_ask_int_price_) ||
        (indep_base_price_plus_spread_place_ <= dep_safe_ask_int_price_ && indep_ask_size_ >= safe_size_ask_)) {
      mini_top_ask_place_ = true;
    }
    /*
                if ( param_set_.mini_agg_logic_ && indep_ask_int_price_ < best_nonself_bid_int_price_ )
          {
            mini_top_ask_agg_place_ = true;
          }

        if ( param_set_.mini_top_keep_logic_ && indep_ask_int_price_ > best_nonself_ask_int_price_ )
          {
            mini_top_ask_keep_ = false;
          }

        if ( indep_ask_int_price_ < dep_safe_ask_int_price_ ||
            ( indep_ask_int_price_ <= dep_safe_ask_int_price_ && indep_ask_size_ >= param_set_.indep_safe_size_ ) )
          {
            mini_top_ask_place_ = true;
          }
     */
    // check if the margin of placing limit ask orders at the price ( best_nonself_ask_price_ )
    // i.e. ( best_nonself_ask_price_ - target_price_ )
    //      exceeds the threshold current_tradevarset_.l1ask_place_
    if ((best_nonself_ask_size_ > param_set_.safe_distance_) || (mini_top_ask_place_) || mini_top_ask_agg_place_ ||
        mini_top_ask_improve_place_ ||
        // If the sell trades on the source crosses place threshold then it will place on best level irrespective of the
        // indicators
        (watch_.msecs_from_midnight() - last_bigtrades_ask_place_msecs_ <=
         param_set_.bigtrades_place_cooloff_interval_) ||
        // If the sell trades on the source crosses aggress threshold then it will place on best level irrespective of
        // the indicators
        (watch_.msecs_from_midnight() - last_bigtrades_ask_aggress_msecs_ <=
         param_set_.bigtrades_aggress_cooloff_interval_) ||
        ((best_nonself_ask_price_ - target_price_ >= current_tradevarset_.l1ask_place_ - l1_bias_ - l1_order_bias_) &&
         (best_nonself_ask_size_ > param_set_.min_size_to_join_))) {
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
            (  // If the sell trades on the source crosses aggress threshold then it will aggress
                (watch_.msecs_from_midnight() - last_bigtrades_ask_aggress_msecs_ <=
                 param_set_.bigtrades_aggress_cooloff_interval_) ||
                (best_nonself_bid_price_ - target_price_ >= current_tradevarset_.l1ask_aggressive_) ||
                (mini_top_ask_agg_place_)) &&
            ((last_sell_msecs_ <= 0) ||
             (watch_.msecs_from_midnight() - last_sell_msecs_ >= param_set_.cooloff_interval_) ||
             ((last_sell_int_price_ - best_nonself_bid_int_price_) <
              (param_set_.px_band_ -
               1)))  // TODO_OPT : later change setting of last_sell_int_price_ to include px_band_
            )

        {
          top_bid_hit_ = true;

          if (mini_top_ask_agg_place_) {
            int indep_safe_sell_int_price_ =
                std::min(indep_ask_int_price_, indep_bid_int_price_ + param_set_.mini_safe_indep_agg_ticks_);
            if (param_set_.read_spread_factor_) indep_safe_sell_int_price_ = ceil(indep_base_price_minus_spread_agg_);
            agg_sell_int_price_ =
                std::max(agg_sell_int_price_ - param_set_.mini_agg_ticks_, indep_safe_sell_int_price_);
            agg_sell_price_ = dep_market_view_.GetDoublePx(agg_sell_int_price_);
          }

          // when we are already short and bit_hit is set to true,
          // place and keep at top ask are set false so that
          // we cancel top level orders before we place new ones at aggressive prices
          if (my_position_ <= param_set_.min_position_to_cancel_on_hit_) {
            top_ask_place_ = false;
            top_ask_keep_ = false;
          }
        } else {
          top_bid_hit_ = false;
          if ((param_set_.allowed_to_improve_) &&
              (my_position_ >=
               param_set_
                   .min_position_to_askimprove_) &&  // Don't improve ask when my_position_ is already decently short
              (best_nonself_ask_int_price_ - best_nonself_bid_int_price_ >= param_set_.min_int_spread_to_improve_) &&
              (  // If the sell trades on the source crosses aggress threshold then it will try to improve the best
                  // level
                  (watch_.msecs_from_midnight() - last_bigtrades_ask_aggress_msecs_ <=
                   param_set_.bigtrades_aggress_cooloff_interval_) ||
                  (best_nonself_ask_price_ - target_price_ - dep_market_view_.min_price_increment() >=
                   current_tradevarset_.l1ask_improve_) ||
                  mini_top_ask_improve_place_) &&
              ((last_sell_msecs_ <= 0) ||
               (watch_.msecs_from_midnight() - last_sell_msecs_ >= param_set_.cooloff_interval_) ||
               (((last_sell_int_price_ - 1) - best_nonself_ask_int_price_) <
                (param_set_.px_band_ -
                 1)))  // TODO_OPT : later change setting of last_sell_int_price_ to include px_band_
              ) {
            top_ask_improve_ = true;
          } else {
            top_ask_improve_ = false;
          }
        }
      }

    } else {
      // signal not strog enough to place limit orders at the best ask level
      if ((best_nonself_ask_price_ - target_price_ + bestask_queue_hysterisis_) >=
          current_tradevarset_.l1ask_keep_ - l1_bias_ -
              l1_order_bias_) {  // but with place in line effect enough to keep the live order there
        top_ask_keep_ = true;
      } else {
        top_ask_keep_ = false;
      }
    }

    if ((dep_market_view_.spread_increments() > 1) &&
        ((best_nonself_ask_price_ - target_price_) >=
         current_tradevarset_.l1ask_improve_keep_ - l1_bias_ - l1_order_bias_)) {
      ask_improve_keep_ = true;
    } else {
      ask_improve_keep_ = false;
    }
  }

  // After setting top-level directives ...
  // get to order placing or canceling part

  // Active BID order management
  placed_bids_this_round_ = false;
  canceled_bids_this_round_ = false;
  if (top_ask_lift_) {
    // only place aggressive bid orders when there is no active unconfirmed order at or above best_nonself_bid_price_
    // and no confirmed orders above the best_nonself_bid_price_
    if (((order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_ + 1) == 0) &&
         (order_manager_.SumBidSizeConfirmedAboveIntPrice(best_nonself_bid_int_price_) == 0)) ||
        mini_top_bid_agg_place_) {
      int _canceled_size_ = 0;
      if (!top_bid_keep_) {  // if because of my_position_ being greater than max_position_to_cancel_on_lift_
        // placing aggressive LIFT order requires us to cancel bids from all active levels
        // where an actve level is >= best_nonself_bid_price_
        _canceled_size_ += order_manager_.CancelBidsEqAboveIntPrice(best_nonself_bid_int_price_);
        if (_canceled_size_ > 0) {
          canceled_bids_this_round_ = true;
        }
      }

      if (!mini_top_bid_keep_) {
        _canceled_size_ += order_manager_.CancelBidsAboveIntPrice(indep_bid_int_price_);
        if (_canceled_size_ > 0) {
          canceled_bids_this_round_ = true;
        }
      }

      int allowance_for_aggressive_buy_ = my_position_ + order_manager_.SumBidSizes() +
                                          current_tradevarset_.l1bid_trade_size_ - param_set_.worst_case_position_;

      if (param_set_.use_new_aggress_logic_) {
        allowance_for_aggressive_buy_ = my_position_ + order_manager_.SumBidSizes() +
                                        current_tradevarset_.l1bid_trade_size_ -
                                        std::max(param_set_.worst_case_position_, param_set_.max_position_);
      }

      if (allowance_for_aggressive_buy_ >= 0) {
        if (allowance_for_aggressive_buy_ > _canceled_size_) {
          _canceled_size_ += order_manager_.CancelBidsFromFar(current_tradevarset_.l1bid_trade_size_);
        }
      } else {
        // Place new order
        order_manager_.SendTrade(agg_buy_price_, agg_buy_int_price_, current_tradevarset_.l1bid_trade_size_,
                                 kTradeTypeBuy, 'A', kOrderDay);
        placed_bids_this_round_ = true;
        last_agg_buy_msecs_ = watch_.msecs_from_midnight();

        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << "Sending aggressive B at px " << agg_buy_price_ << " position " << my_position_
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
    } else {
      // only after canceling them can we be allowed to place aggressive orders
      order_manager_.CancelBidsAboveIntPrice(best_nonself_bid_int_price_);
    }
  }

  if ((!placed_bids_this_round_) && (top_bid_improve_)) {
    // only place Bid Improve orders when there is no active unconfirmed order at or above the
    // best_nonself_bid_int_price_
    //    and no confirmed orders above the best price
    if ((order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_ + 1) == 0) &&
        (order_manager_.SumBidSizeConfirmedAboveIntPrice(best_nonself_bid_int_price_) == 0)) {
      // if we are getting very close to worst_case_position_ with cnf and uncnf orders on the bid side
      if (my_position_ + order_manager_.SumBidSizes() + current_tradevarset_.l1bid_trade_size_ >=
          param_set_.worst_case_position_) {
        int _canceled_size_ = 0;
        // then cancel Bids from bottom levels for the required size
        _canceled_size_ += order_manager_.CancelBidsFromFar(current_tradevarset_.l1bid_trade_size_);
      } else {
        // Place new order
        order_manager_.SendTrade(improve_buy_price_, improve_buy_int_price_, current_tradevarset_.l1bid_trade_size_,
                                 kTradeTypeBuy, 'I');
        placed_bids_this_round_ = true;
        last_agg_buy_msecs_ = watch_.msecs_from_midnight();
        // Adding to the logging vec
        if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
          if (mini_top_bid_improve_place_) {
            std::string this_improve_time_stamp_ =
                std::to_string(watch_.tv().tv_sec) + "." + std::to_string(watch_.tv().tv_usec);
            improve_buy_time_stamp_vec_.push_back(this_improve_time_stamp_);
          }
        }

        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << "Sending improve B at px " << best_nonself_bid_int_price_ + 1 << " position "
                                 << my_position_ << DBGLOG_ENDL_FLUSH;
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
          (stdev_ <= param_set_.low_stdev_lvl_ ||
           (dep_market_view_.spread_increments() <= param_set_.max_int_spread_to_place_)))  // Don't place any new
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
        int canceled_size_ = order_manager_.CancelBidsAboveIntPrice(indep_bid_int_price_);
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
  if (top_bid_hit_) {
    // only place aggressive orders when there is no active unconfirmed order and no confirmed orders above the best
    // price
    if (((order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_ - 1) == 0) &&
         (order_manager_.SumAskSizeConfirmedAboveIntPrice(best_nonself_ask_int_price_) == 0)) ||
        mini_top_ask_agg_place_) {
      int _canceled_size_ = 0;
      if (!top_ask_keep_) {  // if due to position placing aggressive LIFT order requires us to cancel asks from all
                             // active levels
        _canceled_size_ += order_manager_.CancelAsksEqAboveIntPrice(best_nonself_ask_int_price_);
        if (_canceled_size_ > 0) {
          canceled_asks_this_round_ = true;
        }
      }

      if (!mini_top_ask_keep_) {
        _canceled_size_ += order_manager_.CancelAsksAboveIntPrice(indep_ask_int_price_);
        if (_canceled_size_ > 0) {
          canceled_asks_this_round_ = true;
        }
      }

      int allowance_for_aggressive_sell_ = -my_position_ + order_manager_.SumAskSizes() +
                                           current_tradevarset_.l1ask_trade_size_ - param_set_.worst_case_position_;

      if (param_set_.use_new_aggress_logic_) {
        allowance_for_aggressive_sell_ = -my_position_ + order_manager_.SumAskSizes() +
                                         current_tradevarset_.l1ask_trade_size_ -
                                         std::max(param_set_.worst_case_position_, param_set_.max_position_);
      }

      if (allowance_for_aggressive_sell_ >= 0) {
        if (allowance_for_aggressive_sell_ > _canceled_size_) {
          _canceled_size_ += order_manager_.CancelAsksFromFar(
              current_tradevarset_.l1ask_trade_size_);  ///< then cancel Asks from bottom levels for the required size
        }
      } else {
        // Place new order
        order_manager_.SendTrade(agg_sell_price_, agg_sell_int_price_, current_tradevarset_.l1ask_trade_size_,
                                 kTradeTypeSell, 'A', kOrderDay);
        placed_asks_this_round_ = true;
        last_agg_sell_msecs_ = watch_.msecs_from_midnight();

        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << "Sending aggressive S at px " << agg_sell_price_ << " position " << my_position_
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
    } else {
      order_manager_.CancelAsksAboveIntPrice(
          best_nonself_ask_int_price_);  // only after canceling them can we be allowed to place aggressive orders
    }
  }

  if ((!placed_asks_this_round_) && (top_ask_improve_)) {
    // only place Ask Improve orders when there is no active unconfirmed order and no confirmed orders above the best
    // price
    if ((order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_ - 1) == 0) &&
        (order_manager_.SumAskSizeConfirmedAboveIntPrice(best_nonself_ask_int_price_) == 0)) {
      // if we are getting very close to worst_case_position_ with cnf and uncnf orders on the ask side
      if (-my_position_ + order_manager_.SumAskSizes() + current_tradevarset_.l1ask_trade_size_ +
              current_tradevarset_.l1ask_trade_size_ >
          param_set_.worst_case_position_) {
        int _canceled_size_ = 0;
        _canceled_size_ += order_manager_.CancelAsksFromFar(
            current_tradevarset_.l1ask_trade_size_);  ///< then cancel Asks from bottom levels for the required size
      } else {
        // Place new order
        order_manager_.SendTrade(improve_sell_price_, improve_sell_int_price_, current_tradevarset_.l1ask_trade_size_,
                                 kTradeTypeSell, 'I');
        placed_asks_this_round_ = true;
        last_agg_sell_msecs_ = watch_.msecs_from_midnight();
        // Adding to logging vec
        if (dbglogger_.CheckLoggingLevel(OM_INFO)) {
          if (mini_top_ask_improve_place_) {
            std::string this_improve_time_stamp_ =
                std::to_string(watch_.tv().tv_sec) + "." + std::to_string(watch_.tv().tv_usec);
            improve_sell_time_stamp_vec_.push_back(this_improve_time_stamp_);
          }
        }

        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << "Sending improve S at px " << best_nonself_ask_int_price_ - 1 << " position "
                                 << my_position_ << DBGLOG_ENDL_FLUSH;
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
        int canceled_size_ = order_manager_.CancelAsksAboveIntPrice(indep_ask_int_price_);
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

  if (!placed_asks_this_round_ && !placed_bids_this_round_ && !canceled_asks_this_round_ &&
      !canceled_bids_this_round_) {
    FakeTradingLogic();
  }
}

void PricePairBasedAggressiveTrading::PrintFullStatus() {
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
