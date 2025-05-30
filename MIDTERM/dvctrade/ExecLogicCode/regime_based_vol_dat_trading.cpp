/**
    \file ExecLogicCode/price_based_vol_trading.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/ExecLogic/regime_based_vol_dat_trading.hpp"
// exec_logic_code / defines.hpp was empty
#define SMALL_PX_DIFF 1e-05
#define SMALL_COOLOFF_MSECS_ 10
namespace HFSAT {

RegimeBasedVolDatTrading::RegimeBasedVolDatTrading(
    DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
    SmartOrderManager& _order_manager_, const std::string& _paramfilename_, const bool _livetrading_,
    MulticastSenderSocket* _p_strategy_param_sender_socket_, EconomicEventsManager& t_economic_events_manager_,
    const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_, const int t_runtime_id_,
    const std::vector<std::string> _this_model_source_shortcode_vec_)
    : BaseTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_, _livetrading_,
                  _p_strategy_param_sender_socket_, t_economic_events_manager_, t_trading_start_utc_mfm_,
                  t_trading_end_utc_mfm_, t_runtime_id_, _this_model_source_shortcode_vec_) {
  SetShouldIncreaseThreshold();
  m_stdev_ = 0;
  if (stdev_calculator_ == NULL) {
    bool to_set_slow_stdev_calc_ = false;
    for (auto i = 0u; i < param_set_vec_.size(); i++) {
      if (((param_set_vec_[i].stdev_cap_ > 0) && (param_set_vec_[i].stdev_fact_ > 0))) {
        // liberal conditions specific to regime_based_vol_dat_trading
        to_set_slow_stdev_calc_ = true;
        break;
      }
    }
    if (to_set_slow_stdev_calc_) {
      SlowStdevCalculator* t_stdev_ =
          SlowStdevCalculator::GetUniqueInstance(_dbglogger_, _watch_, _dep_market_view_.shortcode(), 100u * 1000u);
      t_stdev_->AddSlowStdevCalculatorListener(this);  // not setting stdev_calculator_, because it will be used to
                                                       // decide whether to call onstdevupdate of basetrading or not
      m_stdev_ = std::min(1.00, param_set_.stdev_cap_);
    }
  } else
    m_stdev_ = std::min(1.00, param_set_.stdev_cap_);

  base_bid_price_ = 0;
  base_ask_price_ = 0;
  best_bid_place_cxl_px_ = 0;
  best_ask_place_cxl_px_ = 0;
  best_int_bid_place_cxl_px_ = 0;
  best_int_ask_place_cxl_px_ = 0;
  base_iter_bid_px_ = 0;
  base_iter_ask_px_ = 0;
  tot_buy_placed_ = 0;
  tot_sell_placed_ = 0;
  band_level_ = 0;
  low_band_px_ = 0;
  current_px_ = 0;
  current_band_ordered_sz_ = 0;
  current_band_target_sz_ = 0;
  retval_ = 0;
  bid_retval_ = 0;
  ask_retval_ = 0;

  last_bid_agg_msecs_ = 0;
  last_ask_agg_msecs_ = 0;
  last_bid_imp_msecs_ = 0;
  last_ask_imp_msecs_ = 0;
}

void RegimeBasedVolDatTrading::TradingLogic() {
  // currently for testing purposes using param index to use as regime
  // identifier
  // 0 for vol strat and 1 for dat strats
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << " Current Param : " << param_index_to_use_ << DBGLOG_ENDL_FLUSH;
  }
  if (param_index_to_use_ == 0u) {
    VolTradingLogic();
  } else if (param_index_to_use_ == 1u) {
    // Threshold updating lagging ?
    SetShouldIncreaseThreshold();
    DatTradingLogic();
  }
}
void RegimeBasedVolDatTrading::VolTradingLogic() {
  /// need stdev normalization to bring it into the
  /// intpx space
  base_bid_price_ = target_price_ - m_stdev_;
  base_ask_price_ = target_price_ + m_stdev_;

  /// first define the best price at which one would be willing to place new orders
  /// to buy and sell - don't have separate place and cxl thresholds
  best_bid_place_cxl_px_ = base_bid_price_ - current_tradevarset_.l1bid_place_;
  best_ask_place_cxl_px_ = base_ask_price_ + current_tradevarset_.l1ask_place_;

  /// move to intpx space for future computations
  best_int_bid_place_cxl_px_ = floor(best_bid_place_cxl_px_ / dep_market_view_.min_price_increment());
  best_int_ask_place_cxl_px_ = ceil(best_ask_place_cxl_px_ / dep_market_view_.min_price_increment());

  /// to reduce message count -- place-keep diff logic
  {
    int t_best_int_bid_place_cxl_px_check_ =
        floor((best_bid_place_cxl_px_ + current_tradevarset_.l1bid_place_ - current_tradevarset_.l1bid_keep_) /
              dep_market_view_.min_price_increment());
    int t_best_int_ask_place_cxl_px_check_ =
        ceil((best_ask_place_cxl_px_ - current_tradevarset_.l1ask_place_ + current_tradevarset_.l1ask_keep_) /
             dep_market_view_.min_price_increment());

    if (t_best_int_bid_place_cxl_px_check_ > best_int_bid_place_cxl_px_ &&
        order_manager_.GetTotalBidSizeOrderedAtIntPx(t_best_int_bid_place_cxl_px_check_) > 0) {
      best_int_bid_place_cxl_px_ = t_best_int_bid_place_cxl_px_check_;
    }

    if (t_best_int_ask_place_cxl_px_check_ < best_int_ask_place_cxl_px_ &&
        order_manager_.GetTotalAskSizeOrderedAtIntPx(t_best_int_ask_place_cxl_px_check_) > 0) {
      best_int_ask_place_cxl_px_ = t_best_int_ask_place_cxl_px_check_;
    }
  }

  /// avoid aggressive and improve orders if param settings so dictate
  if (!param_set_.allowed_to_aggress_) {
    if (!param_set_.allowed_to_improve_) {
      best_int_bid_place_cxl_px_ = std::min(best_nonself_bid_int_price_, best_int_bid_place_cxl_px_);
      best_int_ask_place_cxl_px_ = std::max(best_nonself_ask_int_price_, best_int_ask_place_cxl_px_);
    } else {
      best_int_bid_place_cxl_px_ = std::min(best_nonself_ask_int_price_ - 1, best_int_bid_place_cxl_px_);
      best_int_ask_place_cxl_px_ = std::max(best_nonself_bid_int_price_ + 1, best_int_ask_place_cxl_px_);
    }
  } else  /// sanity check for HUGE value instances et al
  {
    best_int_bid_place_cxl_px_ = std::min(best_nonself_ask_int_price_, best_int_bid_place_cxl_px_);
    best_int_ask_place_cxl_px_ = std::max(best_nonself_bid_int_price_, best_int_ask_place_cxl_px_);
  }

  /// implement cooloff logic
  if (param_set_.allowed_to_aggress_) {
    if (watch_.msecs_from_midnight() - last_bid_agg_msecs_ < param_set_.agg_cooloff_interval_) {
      best_int_bid_place_cxl_px_ = std::min(best_nonself_ask_int_price_ - 1, best_int_bid_place_cxl_px_);
    }
    if (watch_.msecs_from_midnight() - last_ask_agg_msecs_ < param_set_.agg_cooloff_interval_) {
      best_int_ask_place_cxl_px_ = std::max(best_nonself_bid_int_price_ + 1, best_int_ask_place_cxl_px_);
    }
  }

  if (param_set_.allowed_to_improve_) {
    if (watch_.msecs_from_midnight() - last_bid_imp_msecs_ < param_set_.improve_cooloff_interval_ &&
        watch_.msecs_from_midnight() - last_buy_msecs_ < param_set_.improve_cooloff_interval_) {
      best_int_bid_place_cxl_px_ = std::min(best_nonself_bid_int_price_, best_int_bid_place_cxl_px_);
    }
    if (watch_.msecs_from_midnight() - last_ask_agg_msecs_ < param_set_.improve_cooloff_interval_ &&
        watch_.msecs_from_midnight() - last_sell_msecs_ < param_set_.improve_cooloff_interval_) {
      best_int_ask_place_cxl_px_ = std::max(best_nonself_ask_int_price_, best_int_ask_place_cxl_px_);
    }
  }

  /// to safeguard against ORS - MDS gap delays - quite visible in Liffe & BMF
  /// would be better done via book changes
  if (last_buy_int_price_ <= best_nonself_bid_int_price_ &&
      watch_.msecs_from_midnight() - last_buy_msecs_ < SMALL_COOLOFF_MSECS_) {
    best_int_bid_place_cxl_px_ = std::min(last_buy_int_price_ - 1, best_int_bid_place_cxl_px_);
  }

  if (last_sell_int_price_ >= best_nonself_ask_int_price_ &&
      watch_.msecs_from_midnight() - last_sell_msecs_ < SMALL_COOLOFF_MSECS_) {
    best_int_ask_place_cxl_px_ = std::max(last_sell_int_price_ + 1, best_int_ask_place_cxl_px_);
  }

  /*                              CANCEL LOGIC                                */
  /// On Bid Side - depending on whether we intend to place on top level or not
  if (current_tradevarset_.l1bid_trade_size_ > 0) {
    retval_ = order_manager_.CancelBidsAboveIntPrice(best_int_bid_place_cxl_px_);
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO) && retval_ > 0) {
      DBGLOG_TIME_CLASS_FUNC << " Cancelled " << retval_ << " bid orders above " << best_int_bid_place_cxl_px_
                             << " TLA\nTarget: " << target_price_
                             << "\nBest_Bid_Place_Cxl_Px: " << best_bid_place_cxl_px_ << "\nPosition: " << my_position_
                             << '\n';
      order_manager_.ShowOrderBook();
    }
    base_iter_bid_px_ = best_int_bid_place_cxl_px_;
  } else {
    retval_ = order_manager_.CancelBidsAboveIntPrice(best_nonself_bid_int_price_ - param_set_.px_band_);
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO) && retval_ > 0) {
      DBGLOG_TIME_CLASS_FUNC << " Cancelled " << retval_ << " top band bid orders above "
                             << best_nonself_bid_int_price_ - param_set_.px_band_ << "\nPosition: " << my_position_
                             << '\n';
      order_manager_.ShowOrderBook();
    }
    base_iter_bid_px_ = best_nonself_bid_int_price_ - param_set_.px_band_;
  }
  bid_retval_ = order_manager_.SumBidSizeConfirmedAboveIntPrice(base_iter_bid_px_);

  /// Symmetric for Ask Side
  if (current_tradevarset_.l1ask_trade_size_ > 0) {
    retval_ = order_manager_.CancelAsksAboveIntPrice(best_int_ask_place_cxl_px_);
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO) && retval_ > 0) {
      DBGLOG_TIME_CLASS_FUNC << " Cancelled " << retval_ << " ask orders above " << best_int_ask_place_cxl_px_
                             << " TLA\nTarget: " << target_price_
                             << "\nBest_Ask_Place_Cxl_Px: " << best_ask_place_cxl_px_ << "\nPosition: " << my_position_
                             << '\n';
      order_manager_.ShowOrderBook();
    }
    base_iter_ask_px_ = best_int_ask_place_cxl_px_;
  } else {
    retval_ = order_manager_.CancelAsksAboveIntPrice(best_nonself_ask_int_price_ + param_set_.px_band_);
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO) && retval_ > 0) {
      DBGLOG_TIME_CLASS_FUNC << " Cancelled " << retval_ << " top band ask orders better than  "
                             << best_nonself_ask_int_price_ + param_set_.px_band_ << "\nPosition: " << my_position_
                             << '\n';
      order_manager_.ShowOrderBook();
    }
    base_iter_ask_px_ = best_nonself_ask_int_price_ + param_set_.px_band_;
  }
  ask_retval_ = order_manager_.SumAskSizeConfirmedAboveIntPrice(base_iter_ask_px_);

  /*                                       PLACE LOGIC                                        */
  /// first bid side
  tot_buy_placed_ = bid_retval_;
  band_level_ = 0;
  ;
  int old_tot_buy_placed_ = 0;
  while (my_position_ + tot_buy_placed_ < (int)(param_set_.worst_case_position_)) {
    /// first compute the total size shown in this px band
    /// lower limit of px_band
    old_tot_buy_placed_ = tot_buy_placed_;
    low_band_px_ = base_iter_bid_px_ - param_set_.px_band_;
    current_px_ = base_iter_bid_px_;
    current_band_ordered_sz_ = 0;
    if (current_tradevarset_.l1bid_trade_size_ > 0 && band_level_ == 0) {
      current_band_target_sz_ = current_tradevarset_.l1bid_trade_size_;
    } else {
      current_band_target_sz_ =
          std::min(param_set_.max_position_ - my_position_ - tot_buy_placed_, param_set_.unit_trade_size_);
    }

    while (current_px_ > low_band_px_) {
      current_band_ordered_sz_ += order_manager_.GetTotalBidSizeOrderedAtIntPx(current_px_);
      current_px_--;
    }

    tot_buy_placed_ += current_band_ordered_sz_;
    if (current_band_ordered_sz_ < current_band_target_sz_) {
      order_manager_.SendTradeIntPx(base_iter_bid_px_, current_band_target_sz_ - current_band_ordered_sz_,
                                    kTradeTypeBuy, GetOrderLevelIndicator(kTradeTypeBuy, base_iter_bid_px_));
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << " SendTrade "
                               << " Buy Order at px " << base_iter_bid_px_ << " size "
                               << current_band_target_sz_ - current_band_ordered_sz_ << " Mkt "
                               << best_nonself_bid_price_ << " --- " << best_nonself_ask_price_
                               << "\nTarget Px: " << target_price_
                               << "\nBest_Bid_Place_Cxl_Price: " << best_bid_place_cxl_px_
                               << "\nPosition: " << my_position_ << '\n';
        order_manager_.ShowOrderBook();
      }
      tot_buy_placed_ += current_band_target_sz_ - current_band_ordered_sz_;
    }
    if (!(tot_buy_placed_ - old_tot_buy_placed_ > 0)) break;
    base_iter_bid_px_ = low_band_px_;
    band_level_++;
  }

  // cancel bids below this
  retval_ = order_manager_.CancelBidsEqBelowIntPrice(base_iter_bid_px_);
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO) && retval_ > 0) {
    DBGLOG_TIME_CLASS_FUNC << " Cancelled " << retval_ << " Supporting Orders on Bid Side below " << base_iter_bid_px_
                           << "\nPosition: " << my_position_ << '\n';
    order_manager_.ShowOrderBook();
  }

  /// Symetric treatment for ASK side
  tot_sell_placed_ = ask_retval_;
  band_level_ = 0;
  int old_tot_sell_placed_ = 0;
  while (my_position_ - tot_sell_placed_ + (int)(param_set_.worst_case_position_) > 0) {
    /// first compute the total size shown in this px band
    /// lower limit of px_band
    old_tot_sell_placed_ = tot_sell_placed_;
    low_band_px_ = base_iter_ask_px_ + param_set_.px_band_;
    current_px_ = base_iter_ask_px_;
    current_band_ordered_sz_ = 0;
    if (current_tradevarset_.l1ask_trade_size_ > 0 && band_level_ == 0) {
      current_band_target_sz_ = current_tradevarset_.l1ask_trade_size_;
    } else {
      current_band_target_sz_ =
          std::min(param_set_.max_position_ + my_position_ - tot_sell_placed_, param_set_.unit_trade_size_);
    }

    while (current_px_ < low_band_px_) {
      current_band_ordered_sz_ += order_manager_.GetTotalAskSizeOrderedAtIntPx(current_px_);
      current_px_++;
    }

    tot_sell_placed_ += current_band_ordered_sz_;
    if (current_band_ordered_sz_ < current_band_target_sz_) {
      order_manager_.SendTradeIntPx(base_iter_ask_px_, current_band_target_sz_ - current_band_ordered_sz_,
                                    kTradeTypeSell, GetOrderLevelIndicator(kTradeTypeSell, base_iter_ask_px_));
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << " SendTrade "
                               << " Sell Order at px " << base_iter_ask_px_ << " size "
                               << current_band_target_sz_ - current_band_ordered_sz_ << " Mkt "
                               << best_nonself_bid_price_ << " --- " << best_nonself_ask_price_
                               << "\nTarget_Px: " << target_price_
                               << "\nBest_Ask_Place_Cxl_Px: " << best_ask_place_cxl_px_
                               << "\nPosition: " << my_position_ << '\n';
        order_manager_.ShowOrderBook();
      }
      tot_sell_placed_ += current_band_target_sz_ - current_band_ordered_sz_;
    }
    if (!(tot_sell_placed_ - old_tot_sell_placed_ > 0)) break;
    base_iter_ask_px_ = low_band_px_;
    band_level_++;
  }

  // cancel asks above this
  retval_ = order_manager_.CancelAsksEqBelowIntPrice(base_iter_ask_px_);
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO) && retval_ > 0) {
    DBGLOG_TIME_CLASS_FUNC << " Cancelled " << retval_ << " Supporting Orders on Ask Side above " << base_iter_ask_px_
                           << "\nPosition: " << my_position_ << '\n';
    order_manager_.ShowOrderBook();
  }
}

void RegimeBasedVolDatTrading::DatTradingLogic() {
  int num_max_orders_ = 1;
  int our_bid_orders_ = 0;
  int our_ask_orders_ = 0;
  int effective_bid_position_ = my_position_;
  int effective_ask_position_ = my_position_;
  int effective_bid_position_to_keep_ = my_position_;
  int effective_ask_position_to_keep_ = my_position_;

  if (param_set_.place_multiple_orders_) {
    num_max_orders_ = param_set_.max_unit_size_at_level_;
    our_bid_orders_ = order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_) +
                      order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_);
    our_ask_orders_ = order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_) +
                      order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_);

    effective_bid_position_ =
        our_bid_orders_ + my_position_ - order_manager_.SumBidSizeCancelRequested(best_nonself_bid_int_price_);
    effective_ask_position_ =
        -our_ask_orders_ + my_position_ + order_manager_.SumAskSizeCancelRequested(best_nonself_ask_int_price_);
    effective_bid_position_to_keep_ = effective_bid_position_ - current_tradevarset_.l1bid_trade_size_;
    effective_ask_position_to_keep_ = effective_ask_position_ + current_tradevarset_.l1ask_trade_size_;

    current_bid_tradevarset_ = position_tradevarset_map_[GetPositonToTradeVarsetMapIndex(effective_bid_position_)];
    current_bid_keep_tradevarset_ =
        position_tradevarset_map_[GetPositonToTradeVarsetMapIndex(effective_bid_position_to_keep_)];
    current_ask_keep_tradevarset_ =
        position_tradevarset_map_[GetPositonToTradeVarsetMapIndex(effective_ask_position_to_keep_)];
    current_ask_tradevarset_ = position_tradevarset_map_[GetPositonToTradeVarsetMapIndex(effective_ask_position_)];
    UpdateThresholds();  // too many calls?
  }

  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC
        << " BidOrders: " << our_bid_orders_ << " AskOrders: " << our_ask_orders_ << " Pos: " << my_position_ << " # "
        << effective_bid_position_ << " # " << effective_ask_position_ << " Thresh: "
        << position_tradevarset_map_[GetPositonToTradeVarsetMapIndex(effective_bid_position_)].l1bid_place_ << " # "
        << position_tradevarset_map_[(GetPositonToTradeVarsetMapIndex(effective_ask_position_))].l1ask_place_
        << " tgtbias:" << targetbias_numbers_ << DBGLOG_ENDL_FLUSH;
  }

  // setting top level directives
  top_bid_place_ = false;
  top_bid_keep_ = false;
  top_bid_improve_ = false;
  top_ask_lift_ = false;
  bid_improve_keep_ = false;

  // if based on current risk level or if trading is stopped ... check if we have any allowance to place orders at top
  // level
  if (current_tradevarset_.l1bid_trade_size_ > 0) {
    // first check for cooloff_interval
    if ((last_buy_msecs_ > 0) && (watch_.msecs_from_midnight() - last_buy_msecs_ < param_set_.cooloff_interval_) &&
        (best_nonself_bid_int_price_ >= last_buy_int_price_)) {
      // no bids at this or higher prices now
    } else {
      // check if the margin of buying
      // i.e. ( targetbias_numbers_ )
      // exceeds the threshold current_tradevarset_.l1bid_place_
      if ((best_nonself_bid_size_ > param_set_.safe_distance_) ||
          ((targetbias_numbers_ + ((dep_market_view_.spread_increments() > moving_avg_dep_bidask_spread_)
                                       ? (param_set_.high_spread_allowance_)
                                       : 0.0) >=
            current_bid_tradevarset_.l1bid_place_ - l1_bias_ - l1_order_bias_ - short_positioning_bias_) &&
           (best_nonself_bid_size_ > param_set_.min_size_to_join_) &&
           (param_set_.place_on_trade_update_implied_quote_ || !dep_market_view_.trade_update_implied_quote()))) {
        top_bid_place_ = true;
        top_bid_keep_ = true;

        if (watch_.msecs_from_midnight() - last_agg_buy_msecs_ > param_set_.agg_cooloff_interval_) {
          /* aggressive and improve */
          if ((param_set_.allowed_to_aggress_) && /* external control on aggressing */
              (targetbias_numbers_ >=
               current_tradevarset_.l1bid_aggressive_) &&  // only LIFT offer if the margin of buying exceeds the
                                                           // threshold current_tradevarset_.l1bid_aggressive_
              (my_position_ <=
               param_set_.max_position_to_lift_) && /* Don't LIFT offer when my_position_ is already decently long */
              (dep_market_view_.spread_increments() <=
               param_set_.max_int_spread_to_cross_) &&  // Don't LIFT when effective spread is to much
              ((last_buy_msecs_ <= 0) ||
               (watch_.msecs_from_midnight() - last_buy_msecs_ >= param_set_.cooloff_interval_) ||
               ((best_nonself_ask_int_price_ - last_buy_int_price_) <
                (param_set_.px_band_ -
                 1)))  // TODO_OPT : later change setting of last_buy_int_price_ to include px_band_
              )

          {
            top_ask_lift_ = true;

            // when we are already long and ask_lift is set to true,
            // place and keep at top bid are set false so that
            // we cancel top level orders before we place new ones at aggressive prices */
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
                     .max_position_to_bidimprove_) && /* Don't improve bid when my_position_ is already decently long */
                (dep_market_view_.spread_increments() >= param_set_.min_int_spread_to_improve_) &&
                (targetbias_numbers_ >= current_tradevarset_.l1bid_improve_) &&
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

      } else {  // signal is not strong enough for placing bids at best_nonself_bid_price_
        // check if we should retain exisiting bids due to place_in_line

        // TODO : perhaps we should make place_in_line effect bestbid_queue_hysterisis_ smarter
        //        for instance when short term volatility in the market is very high
        //        then being high in the queue should count for less.

        if ((targetbias_numbers_ + ((dep_market_view_.spread_increments() > moving_avg_dep_bidask_spread_)
                                        ? (param_set_.high_spread_allowance_)
                                        : 0.0) +
             bestbid_queue_hysterisis_) >=
            current_bid_keep_tradevarset_.l1bid_keep_ - l1_bias_ - short_positioning_bias_ - l1_order_bias_) {
          top_bid_keep_ = true;
        } else {
          top_bid_keep_ = false;
        }
      }

      if ((dep_market_view_.spread_increments() > 1) &&
          (targetbias_numbers_ + ((dep_market_view_.spread_increments() > moving_avg_dep_bidask_spread_)
                                      ? (param_set_.high_spread_allowance_)
                                      : 0.0)) >=
              current_tradevarset_.l1bid_improve_keep_ - l1_bias_ - short_positioning_bias_ - l1_order_bias_) {
        bid_improve_keep_ = true;
      } else {
        bid_improve_keep_ = false;
      }
    }
  }

  top_ask_place_ = false;
  top_ask_keep_ = false;
  top_ask_improve_ = false;
  top_bid_hit_ = false;
  ask_improve_keep_ = false;

  // if based on current risk level or if trading is stopped ... check if we have any allowance to place orders at top
  // level
  if (  // check if we have any allowance to place orders at top level
      (current_tradevarset_.l1ask_trade_size_ > 0) &&
      // first check for cooloff_interval
      ((last_sell_msecs_ <= 0) || (watch_.msecs_from_midnight() - last_sell_msecs_ >= param_set_.cooloff_interval_) ||
       (last_sell_int_price_ - best_nonself_ask_int_price_ <
        (param_set_.px_band_ - 1)))  // later change setting of last_sell_int_price_ to include px_band_
      ) {
    // check if the margin of placing limit ask orders at the price ( best_nonself_ask_price_ )
    // i.e. ( best_nonself_ask_price_ - target_price_ )
    //      exceeds the threshold current_tradevarset_.l1ask_place_

    if ((best_nonself_ask_size_ > param_set_.safe_distance_) ||
        (((-targetbias_numbers_ +
           ((dep_market_view_.spread_increments() > 1) ? (param_set_.high_spread_allowance_) : 0.0)) >=
          current_ask_tradevarset_.l1ask_place_ - l1_bias_ - long_positioning_bias_ - l1_order_bias_) &&
         (best_nonself_ask_size_ > param_set_.min_size_to_join_) &&
         (param_set_.place_on_trade_update_implied_quote_ || !dep_market_view_.trade_update_implied_quote()))) {
      top_ask_place_ = true;
      top_ask_keep_ = true;

      if (watch_.msecs_from_midnight() - last_agg_sell_msecs_ > param_set_.agg_cooloff_interval_) {
        /* aggressive and improve */
        // conditions to place aggressive orders:
        // ALLOWED_TO_AGGRESS
        // position is not too short already
        // spread narrow
        // signal strong
        if ((param_set_.allowed_to_aggress_) && /* external control on aggressing */
            (my_position_ >=
             param_set_.min_position_to_hit_) && /* Don't HIT bid when my_position_ is already decently short */
            (best_nonself_ask_int_price_ - best_nonself_bid_int_price_ <=
             param_set_.max_int_spread_to_cross_) && /* Don't HIT ( cross ) when effective spread is to much */
            (-targetbias_numbers_ >= current_tradevarset_.l1ask_aggressive_) &&
            ((last_sell_msecs_ <= 0) ||
             (watch_.msecs_from_midnight() - last_sell_msecs_ >= param_set_.cooloff_interval_) ||
             ((last_sell_int_price_ - best_nonself_bid_int_price_) <
              (param_set_.px_band_ -
               1)))  // TODO_OPT : later change setting of last_sell_int_price_ to include px_band_
            )

        {
          top_bid_hit_ = true;

          /* when we are already short and bit_hit is set to true,
             place and keep at top ask are set false so that
             we cancel top level orders before we place new ones at aggressive prices */
          if (my_position_ <= param_set_.min_position_to_cancel_on_hit_) {
            top_ask_place_ = false;
            top_ask_keep_ = false;
          }
        } else {
          top_bid_hit_ = false;
          if ((param_set_.allowed_to_improve_) &&
              (my_position_ >=
               param_set_
                   .min_position_to_askimprove_) && /* Don't improve ask when my_position_ is already decently short */
              (best_nonself_ask_int_price_ - best_nonself_bid_int_price_ >= param_set_.min_int_spread_to_improve_) &&
              (targetbias_numbers_ >= current_tradevarset_.l1ask_improve_) &&
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
      if ((-targetbias_numbers_ +
           ((dep_market_view_.spread_increments() > moving_avg_dep_bidask_spread_) ? (param_set_.high_spread_allowance_)
                                                                                   : 0.0) +
           bestask_queue_hysterisis_) >=
          current_ask_keep_tradevarset_.l1ask_keep_ - l1_bias_ - l1_order_bias_ -
              long_positioning_bias_) {  // but with place in line effect enough to keep the live order there
        top_ask_keep_ = true;
      } else {
        top_ask_keep_ = false;
      }
    }
    if ((dep_market_view_.spread_increments() > 1) &&
        (-targetbias_numbers_ + ((dep_market_view_.spread_increments() > moving_avg_dep_bidask_spread_)
                                     ? (param_set_.high_spread_allowance_)
                                     : 0.0)) >=
            current_tradevarset_.l1ask_improve_keep_ - l1_bias_ - l1_order_bias_ - long_positioning_bias_) {
      ask_improve_keep_ = true;
    } else {
      ask_improve_keep_ = false;
    }
  }

  // After setting top-level directives ...
  // get to order placing or canceling part

  // Active BID rder management
  placed_bids_this_round_ = false;
  canceled_bids_this_round_ = false;
  if (top_ask_lift_) {
    // only place aggressive orders when there is no active unconfirmed order and no confirmed orders above the best
    // price
    // and no confirmed orders above the best_nonself_bid_price_
    if ((order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_ + 1) == 0) &&
        (order_manager_.SumBidSizeConfirmedAboveIntPrice(best_nonself_bid_int_price_) == 0)) {
      int _canceled_size_ = 0;
      if (!top_bid_keep_) {  // if due to position placing aggressive LIFT order requires us to cancel bids from all
                             // active levels
        _canceled_size_ += order_manager_.CancelBidsEqAboveIntPrice(best_nonself_bid_int_price_);
        if (_canceled_size_ > 0) {
          canceled_bids_this_round_ = true;
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC_LINE << " Cancelling bids eq above : " << best_nonself_bid_int_price_
                                        << " Size: " << _canceled_size_ << DBGLOG_ENDL_FLUSH;
          }
        }
      }

      int allowance_for_aggressive_buy_ = my_position_ + order_manager_.SumBidSizes() +
                                          current_tradevarset_.l1bid_trade_size_ - param_set_.worst_case_position_;

      if (param_set_.use_new_aggress_logic_) {
        allowance_for_aggressive_buy_ = my_position_ + order_manager_.SumBidSizes() +
                                        current_tradevarset_.l1bid_trade_size_ -
                                        std::max(param_set_.worst_case_position_, param_set_.max_position_);
      }

      // if we are getting very close to worst_case_position_ with cnf and uncnf orders on the bid side
      if (allowance_for_aggressive_buy_ >= 0) {
        // and if size canceled already is less than l1bid_trade_size_
        if (allowance_for_aggressive_buy_ > _canceled_size_) {
          // then cancel Bids from bottom levels for the required size
          _canceled_size_ += order_manager_.CancelBidsFromFar(current_tradevarset_.l1bid_trade_size_);

          if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
            DBGLOG_TIME_CLASS_FUNC_LINE << " Cancelling bids eq above : " << best_nonself_bid_int_price_
                                        << " Size: " << _canceled_size_ << DBGLOG_ENDL_FLUSH;
          }
        }

        // if ( _canceled_size_ >= current_tradevarset_.l1bid_trade_size_ )
        //   {
        //     order_manager_.SendTrade ( best_nonself_ask_price_, best_nonself_ask_int_price_,
        //     current_tradevarset_.l1bid_trade_size_, kTradeTypeBuy, 'A' ) ;
        //     placed_bids_this_round_ = true;
        //     last_agg_buy_msecs_ = watch_.msecs_from_midnight();
        // if ( dbglogger_.CheckLoggingLevel ( TRADING_ERROR ) )
        //   {
        //     DBGLOG_TIME_CLASS_FUNC
        //       << "Sending aggressive B at px " << best_nonself_ask_int_price_
        //       << " position " << my_position_ << " cancelled size " << _canceled_size_
        //       << " low bid sizes "
        //       << DBGLOG_ENDL_FLUSH ;
        //   }
        //   }
      } else {
        /* Place new order */
        order_manager_.SendTrade(best_nonself_ask_price_, best_nonself_ask_int_price_,
                                 current_tradevarset_.l1bid_trade_size_, kTradeTypeBuy, 'A');
        placed_bids_this_round_ = true;
        last_agg_buy_msecs_ = watch_.msecs_from_midnight();
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << "Sending aggressive B at px " << best_nonself_ask_int_price_ << " position "
                                 << my_position_ << " cancelled size " << _canceled_size_ << " low bid sizes "
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
    } else {
      int canceled_size_ = order_manager_.CancelBidsAboveIntPrice(
          best_nonself_bid_int_price_);  // only after canceling them can we be allowed to place aggressive orders
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " Cancelling bids eq above : " << best_nonself_bid_int_price_
                                    << " Size: " << canceled_size_ << DBGLOG_ENDL_FLUSH;
      }
    }
  }

  if ((!placed_bids_this_round_) && (top_bid_improve_)) {
    // only place Bid Improve orders when there is no active unconfirmed order and no confirmed orders above the best
    // price
    if ((order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_ + 1) == 0) &&
        (order_manager_.SumBidSizeConfirmedAboveIntPrice(best_nonself_bid_int_price_) == 0)) {
      // if we are getting very close to worst_case_position_ with cnf and uncnf orders on the bid side
      if (my_position_ + order_manager_.SumBidSizes() + current_tradevarset_.l1bid_trade_size_ >=
          param_set_.worst_case_position_) {
        int _canceled_size_ = 0;
        // then cancel Bids from bottom levels for the required size
        _canceled_size_ += order_manager_.CancelBidsFromFar(current_tradevarset_.l1bid_trade_size_);

        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " Cancelling bids eq above : " << best_nonself_bid_int_price_
                                      << " Size: " << _canceled_size_ << DBGLOG_ENDL_FLUSH;
        }

        // if ( _canceled_size_ >= current_tradevarset_.l1bid_trade_size_ )
        //   {
        //     order_manager_.SendTradeIntPx ( (best_nonself_bid_int_price_ + 1),
        //     current_tradevarset_.l1bid_trade_size_, kTradeTypeBuy, 'A' ) ;
        //     last_agg_buy_msecs_ = watch_.msecs_from_midnight();
        //     placed_bids_this_round_ = true;
        // if ( dbglogger_.CheckLoggingLevel ( TRADING_ERROR ) )
        //   {
        //     DBGLOG_TIME_CLASS_FUNC
        //       << "Sending improve B at px " << best_nonself_bid_int_price_ +1
        //       << " position " << my_position_ << " cancelled size " << _canceled_size_
        //       << " low bid sizes "
        //       << DBGLOG_ENDL_FLUSH ;
        //   }
        //   }
      } else {
        /* Place new order */
        order_manager_.SendTradeIntPx((best_nonself_bid_int_price_ + 1), current_tradevarset_.l1bid_trade_size_,
                                      kTradeTypeBuy, 'A');
        last_agg_buy_msecs_ = watch_.msecs_from_midnight();
        placed_bids_this_round_ = true;
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << "Sending improve B at px " << best_nonself_bid_int_price_ + 1 << " position "
                                 << my_position_ << DBGLOG_ENDL_FLUSH;
        }
      }
    }

  } else {
    if ((dep_market_view_.spread_increments() > 1) &&
        (!bid_improve_keep_))  // can have size check here for optimization purpose
    {
      int cancelled_size_ = order_manager_.CancelBidsAboveIntPrice(best_nonself_bid_int_price_);
      if (cancelled_size_ > 0) {
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << " Cancelled Improve Bid orders Above: " << best_nonself_bid_int_price_
                                 << " Position: " << my_position_
                                 << " tgt_bias: " << targetbias_numbers_ / dep_market_view_.min_price_increment()
                                 << " thresh_t: "
                                 << current_tradevarset_.l1bid_improve_keep_ / dep_market_view_.min_price_increment()
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  }

  if (!placed_bids_this_round_) {  // only come here if no aggressive or improve bid orders sent this cycle
    if (top_bid_place_) {
      // only place Bid orders when there is no active unconfirmed order and no confirmed order at or above the best
      // price
      if (((param_set_.place_multiple_orders_ && CanPlaceNextOrder(best_nonself_bid_int_price_, kTradeTypeBuy) &&
            (our_bid_orders_ < num_max_orders_ * param_set_.unit_trade_size_) &&
            ((our_bid_orders_ + my_position_ + current_tradevarset_.l1bid_trade_size_) <= param_set_.max_position_)) ||
           ((order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice(best_nonself_bid_int_price_) == 0) &&
            (order_manager_.SumBidSizeConfirmedEqAboveIntPrice(best_nonself_bid_int_price_) == 0))) &&
          (stdev_ <= param_set_.low_stdev_lvl_ ||
           (dep_market_view_.spread_increments() <= param_set_.max_int_spread_to_place_)))  // Don't place any new
                                                                                            // orders in inside market
                                                                                            // if the spread is too wide
      // ( ( param_set_.worst_case_position_ > 0 ) ||
      //   ( ( order_manager_.SumBidSizeConfirmedEqAboveIntPrice ( best_nonself_bid_int_price_ - 100 ) +
      //       order_manager_.SumBidSizeUnconfirmedEqAboveIntPrice ( best_nonself_bid_int_price_ - 100 ) ) <=
      //       param_set_.worst_case_position_ ) ) ) // TODO ... maintain a sum_bid_placed in ordermanager
      {
        order_manager_.SendTrade(best_nonself_bid_price_, best_nonself_bid_int_price_,
                                 current_tradevarset_.l1bid_trade_size_, kTradeTypeBuy, 'B');
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << "SendTrade B of " << current_tradevarset_.l1bid_trade_size_ << " @ "
                                 << best_nonself_bid_price_
                                 << " tgt_bias: " << targetbias_numbers_ / dep_market_view_.min_price_increment()
                                 << " thresh_t: "
                                 << current_bid_tradevarset_.l1bid_place_ / dep_market_view_.min_price_increment()
                                 << " Int Px: " << best_nonself_bid_int_price_ << " tMktSz: " << best_nonself_bid_size_
                                 << " Mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << "  ---  "
                                 << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
        }
      }
    } else {
      if (!top_bid_keep_) {  // cancel all orders at best_nonself_bid_price_

        int canceled_size_ = 0;
        if (param_set_.place_multiple_orders_) {
          canceled_size_ =
              order_manager_.CancelBidsEqAboveIntPrice(best_nonself_bid_int_price_, param_set_.unit_trade_size_);
        } else {
          canceled_size_ = order_manager_.CancelBidsEqAboveIntPrice(best_nonself_bid_int_price_);
        }

        if (canceled_size_ > 0) {
          canceled_bids_this_round_ = true;
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
          {
            DBGLOG_TIME << "Canceled B of " << canceled_size_ << " EqAbove " << best_nonself_bid_price_
                        << " tgt_bias: " << targetbias_numbers_ / dep_market_view_.min_price_increment()
                        << " thresh_t: "
                        << current_bid_keep_tradevarset_.l1bid_keep_ / dep_market_view_.min_price_increment()
                        << " tMktSz: " << best_nonself_bid_size_ << DBGLOG_ENDL_FLUSH;
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
    if ((order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_ - 1) == 0) &&
        (order_manager_.SumAskSizeConfirmedAboveIntPrice(best_nonself_ask_int_price_) == 0)) {
      int _canceled_size_ = 0;
      if (!top_ask_keep_) {  // if due to position placing aggressive LIFT order requires us to cancel asks from all
                             // active levels
        _canceled_size_ += order_manager_.CancelAsksEqAboveIntPrice(best_nonself_ask_int_price_);
      }

      int allowance_for_aggressive_sell_ = -my_position_ + order_manager_.SumAskSizes() +
                                           current_tradevarset_.l1ask_trade_size_ - param_set_.worst_case_position_;

      if (param_set_.use_new_aggress_logic_) {
        allowance_for_aggressive_sell_ = -my_position_ + order_manager_.SumAskSizes() +
                                         current_tradevarset_.l1ask_trade_size_ -
                                         std::max(param_set_.max_position_, param_set_.max_position_);
      }

      // if we are getting very close to worst_case_position_ with cnf and uncnf orders on the ask side
      if (allowance_for_aggressive_sell_ >= 0) {
        // and if size canceled already is less than l1ask_trade_size_
        if (allowance_for_aggressive_sell_ > _canceled_size_) {
          _canceled_size_ += order_manager_.CancelAsksFromFar(
              current_tradevarset_.l1ask_trade_size_);  ///< then cancel Asks from bottom levels for the required size
        }

        // if ( _canceled_size_ >= current_tradevarset_.l1ask_trade_size_ )
        //   {
        //     order_manager_.SendTrade ( best_nonself_bid_price_, best_nonself_bid_int_price_,
        //     current_tradevarset_.l1ask_trade_size_, kTradeTypeSell, 'A' ) ;
        //     last_agg_sell_msecs_ = watch_.msecs_from_midnight();
        //     placed_asks_this_round_ = true;
        // if ( dbglogger_.CheckLoggingLevel ( TRADING_ERROR ) )
        //   {
        //     DBGLOG_TIME_CLASS_FUNC
        //       << "Sending aggressive S at px " << best_nonself_bid_int_price_
        //       << " position " << my_position_ << " cancelled size " << _canceled_size_
        //       << " low ask sizes "
        //       << DBGLOG_ENDL_FLUSH ;
        //   }
        //   }
      } else {
        /* Place new order */
        order_manager_.SendTrade(best_nonself_bid_price_, best_nonself_bid_int_price_,
                                 current_tradevarset_.l1ask_trade_size_, kTradeTypeSell, 'A');
        placed_asks_this_round_ = true;
        last_agg_sell_msecs_ = watch_.msecs_from_midnight();
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << "Sending aggressive S at px " << best_nonself_bid_int_price_ << " position "
                                 << my_position_ << DBGLOG_ENDL_FLUSH;
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
      if (-my_position_ + order_manager_.SumAskSizes() + current_tradevarset_.l1ask_trade_size_ >=
          param_set_.worst_case_position_) {
        int _canceled_size_ = 0;
        _canceled_size_ += order_manager_.CancelAsksFromFar(
            current_tradevarset_.l1ask_trade_size_);  ///< then cancel Asks from bottom levels for the required size

        // if ( _canceled_size_ >= current_tradevarset_.l1ask_trade_size_ )
        //   {
        //     order_manager_.SendTradeIntPx ( (best_nonself_ask_int_price_ - 1),
        //     current_tradevarset_.l1ask_trade_size_, kTradeTypeSell, 'A' ) ;
        //     last_agg_sell_msecs_ = watch_.msecs_from_midnight();
        //     placed_asks_this_round_ = true;
        // if ( dbglogger_.CheckLoggingLevel ( TRADING_ERROR ) )
        //   {
        //     DBGLOG_TIME_CLASS_FUNC
        //       << "Sending improve S at px " << best_nonself_ask_int_price_ -1
        //       << " position " << my_position_ << " cancelled size " << _canceled_size_
        //       << " low ask sizes "
        //       << DBGLOG_ENDL_FLUSH ;
        //   }
        //   }
      } else {
        /* Place new order */
        order_manager_.SendTradeIntPx((best_nonself_ask_int_price_ - 1), current_tradevarset_.l1ask_trade_size_,
                                      kTradeTypeSell, 'A');
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
    if ((dep_market_view_.spread_increments() > 1) && (!ask_improve_keep_))
    /*
     * Need to cancel these kind of orders only when the spread > 1
     * In other cases it wont be benificial except wasting 1 message
     */
    {
      int cancelled_size_ = order_manager_.CancelAsksAboveIntPrice(best_nonself_ask_int_price_);
      if (cancelled_size_ > 0) {
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME_CLASS_FUNC << " Cancelling Improve Ask orders Above: " << best_nonself_ask_int_price_
                                 << " position: " << my_position_
                                 << " tgt_bias: " << targetbias_numbers_ / dep_market_view_.min_price_increment()
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
      if (((param_set_.place_multiple_orders_ && CanPlaceNextOrder(best_nonself_ask_int_price_, kTradeTypeSell) &&
            (our_ask_orders_ < param_set_.unit_trade_size_ * num_max_orders_) &&
            ((our_ask_orders_ - my_position_ + current_tradevarset_.l1ask_trade_size_) <= param_set_.max_position_)) ||
           ((order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice(best_nonself_ask_int_price_) == 0) &&
            (order_manager_.SumAskSizeConfirmedEqAboveIntPrice(best_nonself_ask_int_price_) == 0))) &&
          (stdev_ <= param_set_.low_stdev_lvl_ ||
           (dep_market_view_.spread_increments() <= param_set_.max_int_spread_to_place_)))  // Don't place any new
                                                                                            // orders in inside market
                                                                                            // if the spread is too wide
      // ( ( param_set_.worst_case_position_ > 0 ) ||
      //   ( ( order_manager_.SumAskSizeConfirmedEqAboveIntPrice ( best_nonself_ask_int_price_ + 100 ) +
      //       order_manager_.SumAskSizeUnconfirmedEqAboveIntPrice ( best_nonself_ask_int_price_ + 100 ) ) <=
      //       param_set_.worst_case_position_ ) ) ) // TODO ... maintain a sum_ask_placed in ordermanager
      {
        order_manager_.SendTrade(best_nonself_ask_price_, best_nonself_ask_int_price_,
                                 current_tradevarset_.l1ask_trade_size_, kTradeTypeSell, 'B');
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
        {
          DBGLOG_TIME_CLASS_FUNC << "SendTrade S of " << current_tradevarset_.l1ask_trade_size_ << " @ "
                                 << best_nonself_ask_price_
                                 << " tgt_bias: " << -targetbias_numbers_ / dep_market_view_.min_price_increment()
                                 << " thresh_t: "
                                 << current_ask_tradevarset_.l1ask_place_ / dep_market_view_.min_price_increment()
                                 << " Int Px: " << best_nonself_ask_int_price_ << " tMktSz: " << best_nonself_ask_size_
                                 << " Mkt: " << best_nonself_bid_size_ << " @ " << best_nonself_bid_price_ << " ---- "
                                 << best_nonself_ask_price_ << " @ " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
        }
      }
    } else {
      if (!top_ask_keep_) {
        // cancel all orders at best_nonself_ask_price_
        int canceled_size_ = 0;
        if (param_set_.place_multiple_orders_) {
          order_manager_.CancelAsksEqAboveIntPrice(best_nonself_ask_int_price_, param_set_.unit_trade_size_);
        } else {
          order_manager_.CancelAsksEqAboveIntPrice(best_nonself_ask_int_price_);
        }
        if (canceled_size_ > 0) {
          canceled_asks_this_round_ = true;
          if (dbglogger_.CheckLoggingLevel(TRADING_INFO))  // zero logging
          {
            DBGLOG_TIME << "Canceled S of " << canceled_size_ << " EqAbove " << best_nonself_ask_price_
                        << " tgt_bias: " << -targetbias_numbers_ / dep_market_view_.min_price_increment()
                        << " thresh_t: "
                        << current_ask_keep_tradevarset_.l1ask_keep_ / dep_market_view_.min_price_increment()
                        << " tMktSz: " << best_nonself_ask_size_ << DBGLOG_ENDL_FLUSH;
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

void RegimeBasedVolDatTrading::PrintFullStatus() {
  DBGLOG_TIME << "tgt: " << target_price_ << " ExpBidPft@ " << best_nonself_bid_price_ << " X "
              << best_nonself_bid_size_ << ' ' << (target_price_ - best_nonself_bid_price_) << " ExpAskPft@ "
              << best_nonself_ask_price_ << " X " << best_nonself_ask_size_ << ' '
              << (best_nonself_ask_price_ - target_price_) << " signalbias: "
              << ((target_price_ - dep_market_view_.mkt_size_weighted_price()) / dep_market_view_.min_price_increment())
              << " pos: " << my_position_ << " l1_SBS "
              << order_manager_.intpx_2_sum_bid_confirmed(best_nonself_bid_int_price_) << " l1_SAS "
              << order_manager_.intpx_2_sum_ask_confirmed(best_nonself_ask_int_price_) << DBGLOG_ENDL_FLUSH;
}

char RegimeBasedVolDatTrading::GetOrderLevelIndicator(TradeType_t order_side, int int_order_px) {
  if (order_side == kTradeTypeBuy) {
    if (int_order_px >= best_nonself_ask_int_price_) {
      last_bid_agg_msecs_ = watch_.msecs_from_midnight();
      return 'A';
    } else if (int_order_px > best_nonself_bid_int_price_) {
      last_bid_imp_msecs_ = watch_.msecs_from_midnight();
      return 'I';
    } else if (int_order_px == best_nonself_bid_int_price_) {
      return 'B';
    } else
      return 'S';
  } else {
    if (int_order_px <= best_nonself_bid_int_price_) {
      last_ask_agg_msecs_ = watch_.msecs_from_midnight();
      return 'A';
    } else if (int_order_px < best_nonself_ask_int_price_) {
      last_ask_imp_msecs_ = watch_.msecs_from_midnight();
      return 'I';
    } else if (int_order_px == best_nonself_ask_int_price_) {
      return 'B';
    } else
      return 'S';
  }
}

void RegimeBasedVolDatTrading::OnExec(const int t_new_position_, const int _exec_quantity_, const TradeType_t _buysell_,
                                      const double _price_, const int r_int_price_, const int _security_id_) {
  if (param_index_to_use_ == 1u) {
    BaseTrading::OnExec(t_new_position_, _exec_quantity_, _buysell_, _price_, r_int_price_, _security_id_);
    return;
  }

  if (t_new_position_ > my_position_) {
    last_buy_msecs_ = watch_.msecs_from_midnight();
    last_buy_int_price_ = r_int_price_;
  } else if (t_new_position_ < my_position_) {
    last_sell_msecs_ = watch_.msecs_from_midnight();
    last_sell_int_price_ = r_int_price_;
  }
}

void RegimeBasedVolDatTrading::OnCancelReject(const TradeType_t _buysell_, const double _price_, const int r_int_price_,
                                              const int _security_id_) {
  if (param_index_to_use_ == 1u) {
    BaseTrading::OnCancelReject(_buysell_, _price_, r_int_price_, _security_id_);
    return;
  }

  if (_buysell_ == kTradeTypeBuy) {
    last_buy_msecs_ = watch_.msecs_from_midnight();
    last_buy_int_price_ = r_int_price_;
  } else {
    last_sell_msecs_ = watch_.msecs_from_midnight();
    last_sell_int_price_ = r_int_price_;
  }
}
}
