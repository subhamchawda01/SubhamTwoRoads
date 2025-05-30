/**
    \file ExecLogicCode/price_pair_based_aggressive_trading.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/ExecLogic/projected_price_pair_based_aggressive_trading.hpp"
// exec_logic_code / defines.hpp was empty

namespace HFSAT {

void ProjectedPricePairBasedAggressiveTrading::CollectORSShortCodes(DebugLogger& _dbglogger_,
                                                                    const std::string& r_dep_shortcode_,
                                                                    std::vector<std::string>& source_shortcode_vec_,
                                                                    std::vector<std::string>& ors_source_needed_vec_) {
  HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, r_dep_shortcode_);
  HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, r_dep_shortcode_);

  HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_,
                                      HFSAT::ExecLogicUtils::GetIndepShortcodeForPricePair(r_dep_shortcode_));
}

ProjectedPricePairBasedAggressiveTrading::ProjectedPricePairBasedAggressiveTrading(
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
  if (HFSAT::ExecLogicUtils::IsProjectedPriceRequired(dep_market_view_.shortcode())) {
    projected_bid_price_ = ExecLogicUtils::GetProjectedBidPrice(
        _dbglogger_, _watch_,
        *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(dep_market_view_.shortcode())),
        *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(_indep_market_view_.shortcode())),
        param_set_.projected_price_duration_);
    projected_ask_price_ = ExecLogicUtils::GetProjectedAskPrice(
        _dbglogger_, _watch_,
        *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(dep_market_view_.shortcode())),
        *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(_indep_market_view_.shortcode())),
        param_set_.projected_price_duration_);
    indep_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);
    if (projected_bid_price_ != NULL && projected_ask_price_ != NULL) {
      use_projected_prices_ = true;
    }

    if (dep_market_view_.shortcode() == "HHI_0") {
      indep_market_view_.subscribe_price_type(this, kPriceTypeValidLevelMidPrice);
      dep_market_view_.subscribe_price_type(this, kPriceTypeValidLevelMidPrice);
      // use_stable_prices_ = true ;
    }
  } else {
    ExitVerbose(kExitErrorCodeGeneral, "Projected price not set for this shortcode\n");
  }
}

void ProjectedPricePairBasedAggressiveTrading::TradingLogic() {
  int indep_bid_int_price_ = indep_market_view_.market_update_info_.bestbid_int_price_;
  int indep_ask_int_price_ = indep_market_view_.market_update_info_.bestask_int_price_;
  double indep_bid_price_ = indep_market_view_.market_update_info_.bestbid_price_;
  double indep_ask_price_ = indep_market_view_.market_update_info_.bestask_price_;
  bool t_is_ready_ = false;
  indep_bid_int_price_ = dep_market_view_.GetIntPx(projected_bid_price_->indicator_value(t_is_ready_));
  indep_ask_int_price_ = dep_market_view_.GetIntPx(projected_ask_price_->indicator_value(t_is_ready_));
  indep_bid_price_ = projected_bid_price_->indicator_value(t_is_ready_);
  indep_ask_price_ = projected_ask_price_->indicator_value(t_is_ready_);

  if (!indep_market_view_.is_ready()) {
    return;
  }

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

      dbglogger_ << " projectedprice: " << indep_bid_int_price_ << " " << indep_ask_int_price_
                 << " actual : " << indep_market_view_.market_update_info_.bestbid_int_price_ << " "
                 << indep_market_view_.market_update_info_.bestask_int_price_
                 << " dep: " << dep_market_view_.market_update_info_.bestbid_int_price_ << " "
                 << dep_market_view_.market_update_info_.bestask_int_price_ << DBGLOG_ENDL_FLUSH;

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

  bool mini_top_bid_agg_place_ = false;
  bool mini_top_bid_place_ = false;
  bool mini_top_bid_keep_ = true;

  double agg_buy_price_ = best_nonself_ask_price_;
  int agg_buy_int_price_ = best_nonself_ask_int_price_;
  double agg_sell_price_ = best_nonself_bid_price_;
  int agg_sell_int_price_ = best_nonself_bid_int_price_;

  if (  // check if we have any allowance to place orders at top level
      (current_tradevarset_.l1bid_trade_size_ > 0) &&
      // first check for cooloff_interval
      ((last_buy_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_buy_msecs_ >= param_set_.cooloff_interval_) ||
       ((best_nonself_bid_int_price_ - last_buy_int_price_) <
        (param_set_.px_band_ - 1)))  // later change setting of last_buy_int_price_ to include px_band_
      ) {
    if (param_set_.mini_agg_logic_ &&
        indep_bid_price_ >
            best_nonself_ask_price_ +
                param_set_.mini_agg_thresh_ *
                    dep_market_view_.min_price_increment())  // indep_bid_int_price_ > best_nonself_ask_int_price_ )
    {
      mini_top_bid_agg_place_ = true;
    }

    if (param_set_.mini_top_keep_logic_ &&
        indep_bid_price_ <
            best_nonself_bid_price_ -
                param_set_.mini_keep_thresh_ *
                    dep_market_view_.min_price_increment())  // indep_bid_int_price_ < best_nonself_bid_int_price_ )
    {
      mini_top_bid_keep_ = false;
    }

    if (indep_bid_price_ >=
        best_nonself_bid_price_ + param_set_.indep_safe_ticks_ * dep_market_view_.min_price_increment()) {
      mini_top_bid_place_ = true;
    }

    if (indep_bid_price_ >
        best_nonself_bid_price_ +
            (param_set_.indep_safe_ticks_ - param_set_.pp_place_keep_diff_) * dep_market_view_.min_price_increment()) {
      top_bid_keep_ = true;
    }

    // check if the margin of buying at the price ( best_nonself_bid_price_ )
    // i.e. ( target_price_ - best_nonself_bid_price_ )
    // exceeds the threshold current_tradevarset_.l1bid_place_
    if ((best_nonself_bid_size_ > param_set_.safe_distance_) || (mini_top_bid_place_) || mini_top_bid_agg_place_ ||
        ((target_price_ - best_nonself_bid_price_ >= current_tradevarset_.l1bid_place_) &&
         (best_nonself_bid_size_ >
          param_set_.min_size_to_join_) &&  // Don't place any orders at a level if less than X size there
         (!dep_market_view_.trade_update_implied_quote()))) {
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
              (target_price_ - best_nonself_bid_price_ - dep_market_view_.min_price_increment() >=
               current_tradevarset_.l1bid_improve_) &&
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
      if ((target_price_ - best_nonself_bid_price_ + bestbid_queue_hysterisis_) >= current_tradevarset_.l1bid_keep_) {
        top_bid_keep_ = true;
      } else {
        top_bid_keep_ = false;
      }
    }

    if (((dep_market_view_.spread_increments() > 1) &&
         (target_price_ - best_nonself_bid_price_) >= current_tradevarset_.l1bid_improve_keep_)) {
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
  bool mini_top_ask_place_ = false;
  bool mini_top_ask_keep_ = true;

  if (  // check if we have any allowance to place orders at top level
      (current_tradevarset_.l1ask_trade_size_ > 0) &&
      // first check for cooloff_interval
      ((last_sell_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_sell_msecs_ >= param_set_.cooloff_interval_) ||
       (last_sell_int_price_ - best_nonself_ask_int_price_ <
        (param_set_.px_band_ - 1)))  // later change setting of last_sell_int_price_ to include px_band_
      ) {
    if (param_set_.mini_agg_logic_ &&
        indep_ask_price_ <
            best_nonself_bid_price_ -
                param_set_.mini_agg_thresh_ *
                    dep_market_view_.min_price_increment())  // indep_ask_int_price_ < best_nonself_bid_int_price_ )
    {
      mini_top_ask_agg_place_ = true;
    }

    if (param_set_.mini_top_keep_logic_ &&
        indep_ask_price_ >
            best_nonself_ask_price_ +
                param_set_.mini_keep_thresh_ *
                    dep_market_view_.min_price_increment())  // indep_ask_int_price_ > best_nonself_ask_int_price_ )
    {
      mini_top_ask_keep_ = false;
    }

    if (indep_ask_price_ <=
        best_nonself_ask_price_ - param_set_.indep_safe_ticks_ * dep_market_view_.min_price_increment()) {
      mini_top_ask_place_ = true;
    }

    if (indep_ask_price_ <
        best_nonself_ask_price_ -
            (param_set_.indep_safe_ticks_ - param_set_.pp_place_keep_diff_) * dep_market_view_.min_price_increment()) {
      top_ask_keep_ = false;
    }

    // check if the margin of placing limit ask orders at the price ( best_nonself_ask_price_ )
    // i.e. ( best_nonself_ask_price_ - target_price_ )
    //      exceeds the threshold current_tradevarset_.l1ask_place_
    if ((best_nonself_ask_size_ > param_set_.safe_distance_) || (mini_top_ask_place_) || mini_top_ask_agg_place_ ||
        ((best_nonself_ask_price_ - target_price_ >= current_tradevarset_.l1ask_place_) &&
         (best_nonself_ask_size_ > param_set_.min_size_to_join_) && (!dep_market_view_.trade_update_implied_quote()))) {
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
            (best_nonself_bid_price_ - target_price_ >= current_tradevarset_.l1ask_aggressive_ ||
             mini_top_ask_agg_place_) &&
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
              (best_nonself_ask_price_ - target_price_ - dep_market_view_.min_price_increment() >=
               current_tradevarset_.l1ask_improve_) &&
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
          current_tradevarset_.l1ask_keep_) {  // but with place in line effect enough to keep the live order there
        top_ask_keep_ = true;
      } else {
        top_ask_keep_ = false;
      }
    }

    if ((dep_market_view_.spread_increments() > 1) &&
        ((best_nonself_ask_price_ - target_price_) >= current_tradevarset_.l1ask_improve_keep_)) {
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
                                 kTradeTypeBuy, 'A');
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
        order_manager_.SendTradeIntPx((best_nonself_bid_int_price_ + 1), current_tradevarset_.l1bid_trade_size_,
                                      kTradeTypeBuy, 'I');
        placed_bids_this_round_ = true;
        last_agg_buy_msecs_ = watch_.msecs_from_midnight();
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
                                        dep_market_view_.min_price_increment()
                                 << " thresh_t: "
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
                                          dep_market_view_.min_price_increment()
                                   << " thresh_t: "
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
                                          dep_market_view_.min_price_increment()
                                   << " thresh_t: "
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
                                 kTradeTypeSell, 'A');
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
        order_manager_.SendTradeIntPx((best_nonself_ask_int_price_ - 1), current_tradevarset_.l1ask_trade_size_,
                                      kTradeTypeSell, 'I');
        placed_asks_this_round_ = true;
        last_agg_sell_msecs_ = watch_.msecs_from_midnight();
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
                                        dep_market_view_.min_price_increment()
                                 << " thresh_t: "
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
                                          dep_market_view_.min_price_increment()
                                   << " thresh_t: "
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
                                          dep_market_view_.min_price_increment()
                                   << " thresh_t: "
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

void ProjectedPricePairBasedAggressiveTrading::PrintFullStatus() {
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
