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
#include "dvctrade/ExecLogic/price_based_vol_trading.hpp"
// exec_logic_code / defines.hpp was empty
#include "dvctrade/ExecLogic/vol_utils.hpp"

#define SMALL_PX_DIFF 1e-05
#define SMALL_COOLOFF_MSECS_ 10

namespace HFSAT {

PriceBasedVolTrading::PriceBasedVolTrading(
    DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
    SmartOrderManager& _order_manager_, const std::string& _paramfilename_, const bool _livetrading_,
    MulticastSenderSocket* _p_strategy_param_sender_socket_, EconomicEventsManager& t_economic_events_manager_,
    const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_, const int t_runtime_id_,
    const std::vector<std::string> _this_model_source_shortcode_vec_)
    : BaseTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_, _livetrading_,
                  _p_strategy_param_sender_socket_, t_economic_events_manager_, t_trading_start_utc_mfm_,
                  t_trading_end_utc_mfm_, t_runtime_id_, _this_model_source_shortcode_vec_) {
  should_increase_thresholds_in_volatile_times_ = false;
  // setting slow stdev calculator if not already set
  m_stdev_ = 0;
  unsigned int t_stdev_duration_ = 100u;
  if (stdev_calculator_ == NULL) {
    bool to_set_slow_stdev_calc_ = false;
    for (auto i = 0u; i < param_set_vec_.size(); i++) {
      if (((param_set_vec_[i].stdev_cap_ > 0) && (param_set_vec_[i].stdev_fact_ > 0))) {
        // liberal conditions specific to voltrading
        to_set_slow_stdev_calc_ = true;
      }
      t_stdev_duration_ = std::max(t_stdev_duration_, param_set_vec_[i].stdev_duration_);
    }
    if (to_set_slow_stdev_calc_) {
      if (exec_logic_indicators_helper_) {
        exec_logic_indicators_helper_->ComputeMovingStdev(&param_set_);
      } else {
        ExitVerbose(kExitErrorCodeGeneral, "ExecLogicIndicatorHelper class is not there");
      }
      m_stdev_ = std::min(1.00, param_set_.stdev_cap_);
    }
  } else
    m_stdev_ = std::min(1.00, param_set_.stdev_cap_);

  spread_diff_factor_ = 0.0;
  hist_avg_spread_ = 0.0;
  if (param_set_.read_spread_factor_) {
    std::map<int, double> feature_avg_spread_;
    hist_avg_spread_ =
        SampleDataUtil::GetAvgForPeriod(_dep_market_view_.shortcode(), watch_.YYYYMMDD(), 20, trading_start_utc_mfm_,
                                        trading_end_utc_mfm_, "BidAskSpread", feature_avg_spread_);
  }

  double moving_bidask_spread_duration_ = 3600;
  bool read_moving_bidask_spread_duration_ = false;
  for (auto i = 0u; i < param_set_vec_.size(); i++) {
    if (param_set_vec_[i].read_moving_bidask_spread_duration_) {
      if (param_set_vec_[i].moving_bidask_spread_duration_ < moving_bidask_spread_duration_) {
        moving_bidask_spread_duration_ = param_set_vec_[i].moving_bidask_spread_duration_;
        read_moving_bidask_spread_duration_ = true;
      }
    }
  }

  if (read_moving_bidask_spread_duration_) {
    param_set_.moving_bidask_spread_duration_ = moving_bidask_spread_duration_;
    if (exec_logic_indicators_helper_) {
      exec_logic_indicators_helper_->ComputeMovingBidAskSpread(&param_set_);
    } else {
      ExitVerbose(kExitErrorCodeGeneral, "Exec Logic helper is not instantiated ");
    }
  }

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

  agg_only_no_improve_ = (param_set_.allowed_to_aggress_ && !param_set_.allowed_to_improve_);
  last_bid_agg_msecs_ = 0;
  last_ask_agg_msecs_ = 0;
  last_bid_imp_msecs_ = 0;
  last_ask_imp_msecs_ = 0;
  cancel_bids_above_ = 0;
  cancel_asks_above_ = 0;

  px_to_be_placed_at_vec_.clear();
  int_px_to_be_placed_at_vec_.clear();
  size_to_be_placed_vec_.clear();
  order_level_indicator_vec_.clear();

  use_stable_book_ = false;
  if (param_set_.read_use_stable_book_) {
    use_stable_book_ = param_set_.use_stable_book_;
  }
}

void PriceBasedVolTrading::TradingLogic() {
  // m_stdev_ = exec_logic_indicators_helper_->stdev(); // computed/updated in pvol hpp
  moving_avg_dep_bidask_spread_ = exec_logic_indicators_helper_->moving_average_spread();

  /// need stdev normalization to bring it into the
  /// intpx space
  base_bid_price_ = target_price_ - m_stdev_;
  base_ask_price_ = target_price_ + m_stdev_;

  {
    // Using current vs historical spread
    spread_diff_factor_ = std::max(0.0, dep_market_view_.min_price_increment() * param_set_.spread_factor_ *
                                            ((moving_avg_dep_bidask_spread_ - hist_avg_spread_) / (2.0)));

    base_bid_price_ = base_bid_price_ - spread_diff_factor_;
    base_ask_price_ = base_ask_price_ + spread_diff_factor_;
  }

  /// first define the best price at which one would be willing to place new orders
  /// to buy and sell - don't have separate place and cxl thresholds
  best_bid_place_cxl_px_ = base_bid_price_ - current_tradevarset_.l1bid_place_;
  best_ask_place_cxl_px_ = base_ask_price_ + current_tradevarset_.l1ask_place_;

  /// move to intpx space for future computations
  best_int_bid_place_cxl_px_ = floor(best_bid_place_cxl_px_ / dep_market_view_.min_price_increment());
  best_int_ask_place_cxl_px_ = ceil(best_ask_place_cxl_px_ / dep_market_view_.min_price_increment());

  SetCancelThresholds();
  /// avoid aggressive and improve orders if param settings so dictate
  if (!param_set_.allowed_to_aggress_) {
    if (!param_set_.allowed_to_improve_) {
      best_int_bid_place_cxl_px_ = std::min(best_nonself_bid_int_price_, best_int_bid_place_cxl_px_);
      best_int_ask_place_cxl_px_ = std::max(best_nonself_ask_int_price_, best_int_ask_place_cxl_px_);
      if (param_set_.read_min_size_to_join_) {
        if (best_nonself_bid_size_ < param_set_.min_size_to_join_) {
          best_int_bid_place_cxl_px_ = std::min((best_nonself_bid_int_price_ - 1), best_int_bid_place_cxl_px_);
        }
        if (best_nonself_ask_size_ < param_set_.min_size_to_join_) {
          best_int_ask_place_cxl_px_ = std::max((best_nonself_ask_int_price_ + 1), best_int_ask_place_cxl_px_);
        }
      }

    } else {
      SetImproveBidAskPrices();
    }
  } else  /// sanity check for HUGE value instances et al
  {
    if (param_set_.allowed_to_improve_) {
      best_int_bid_place_cxl_px_ = std::min(best_nonself_ask_int_price_, best_int_bid_place_cxl_px_);
      best_int_ask_place_cxl_px_ = std::max(best_nonself_bid_int_price_, best_int_ask_place_cxl_px_);
    } else {
      // Special handling when agg is true while improve is false
      // Bid side price
      SetAggNoImproveBidAskPrices();
    }
  }

  /// implement cooloff logic
  if (param_set_.allowed_to_aggress_) {
    CheckAggressCoolOff();
  }

  if (param_set_.allowed_to_improve_) {
    CheckImproveCoolOff();
  }

  /// to safeguard against ORS - MDS gap delays - quite visible in Liffe & BMF
  /// would be better done via book changes
  CheckORSMDSDelay();

  /*                              CANCEL LOGIC                                */
  /// On Bid Side - depending on whether we intend to place on top level or not
  BidCancelLogic();
  bid_retval_ = order_manager_.SumBidSizeConfirmedAboveIntPrice(base_iter_bid_px_);

  /// Symmetric for Ask Side
  AskCancelLogic();

  ask_retval_ = order_manager_.SumAskSizeConfirmedAboveIntPrice(base_iter_ask_px_);

  /*                                       PLACE LOGIC                                        */
  /// first bid side
  tot_buy_placed_ = bid_retval_;
  band_level_ = 0;
  ;
  int old_tot_buy_placed_ = 0;
  px_to_be_placed_at_vec_.clear();
  int_px_to_be_placed_at_vec_.clear();
  size_to_be_placed_vec_.clear();
  order_level_indicator_vec_.clear();
  while (my_position_ + tot_buy_placed_ < (int)std::min(param_set_.worst_case_position_, param_set_.max_position_) &&
         tot_buy_placed_ < (int)param_set_.max_total_size_to_place_) {
    /// first compute the total size shown in this px band
    /// lower limit of px_band
    old_tot_buy_placed_ = tot_buy_placed_;
    current_px_ = base_iter_bid_px_;
    current_band_ordered_sz_ = 0;
    if (use_stable_book_) {
      ExecLogicUtils::GetLowBandPx(&dep_market_view_, low_band_px_, base_iter_bid_px_, kTradeTypeBuy);
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " curPrice: " << base_iter_bid_px_ << " low_band_px_ : " << low_band_px_
                                    << DBGLOG_ENDL_FLUSH;
      }
    } else {
      low_band_px_ = base_iter_bid_px_ - param_set_.px_band_;

      if (agg_only_no_improve_) {
        // Setting low band px as bid price or lower to avoid improve
        low_band_px_ = std::min(best_nonself_bid_int_price_, base_iter_bid_px_ - param_set_.px_band_);
        int t_low_band_px_ = low_band_px_ + 1;
        // Cancelling bid orders between best bid and best ask
        // to avoid improve orders
        if (current_px_ > best_nonself_bid_int_price_) {
          while (t_low_band_px_ < current_px_) {
            order_manager_.CancelBidsAtIntPrice(t_low_band_px_);
            t_low_band_px_++;
          }
        }
      }
    }
    if (low_band_px_ == kInvalidIntPrice) {
      break;
    }

    if (current_tradevarset_.l1bid_trade_size_ > 0 && band_level_ == 0) {
      current_band_target_sz_ = current_tradevarset_.l1bid_trade_size_;
    } else {
      current_band_target_sz_ =
          std::min(param_set_.max_position_ - my_position_ - tot_buy_placed_, param_set_.unit_trade_size_);
    }

    if (current_band_target_sz_ <= 0) {
      break;
    }

    while (current_px_ > low_band_px_) {
      current_band_ordered_sz_ += order_manager_.GetTotalBidSizeOrderedAtIntPx(current_px_);
      current_px_--;
    }

    if (use_stable_book_) {
      ExecLogicUtils::CancelOrdersInBand(&order_manager_, base_iter_bid_px_, low_band_px_, current_band_target_sz_,
                                         current_band_ordered_sz_, kTradeTypeBuy);
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << " Cancelled " << retval_ << " Band Orders: " << base_iter_bid_px_ << " - "
                               << low_band_px_ + 1 << "\nPosition: " << my_risk_ << '\n';
        order_manager_.ShowOrderBook();
      }
    }

    tot_buy_placed_ += current_band_ordered_sz_;
    if (current_band_ordered_sz_ < current_band_target_sz_) {
      // pushing the prices and sizes for all orders in a vector and placing them after cancel has been called
      if (current_band_target_sz_ - current_band_ordered_sz_ >= param_set_.min_allowed_unit_trade_size_) {
        int_px_to_be_placed_at_vec_.push_back(base_iter_bid_px_);
        px_to_be_placed_at_vec_.push_back(dep_market_view_.GetDoublePx(base_iter_bid_px_));
        size_to_be_placed_vec_.push_back(current_band_target_sz_ - current_band_ordered_sz_);
        order_level_indicator_vec_.push_back(GetOrderLevelIndicator(kTradeTypeBuy, base_iter_bid_px_));
        tot_buy_placed_ += current_band_target_sz_ - current_band_ordered_sz_;
      }
    }
    if (!(tot_buy_placed_ - old_tot_buy_placed_ > 0)) break;
    base_iter_bid_px_ = low_band_px_;
    band_level_++;
  }

  if (param_set_.allow_modify_orders_) {
    /*
     * Send modify for each place-cancel pair, then send Place or cancel depending on which side orders are remaining
     */
    retval_ = order_manager_.CancelReplaceBidOrdersEqAboveAndEqBelowIntPrice(
        cancel_bids_above_ + 1, base_iter_bid_px_, px_to_be_placed_at_vec_, int_px_to_be_placed_at_vec_,
        size_to_be_placed_vec_, order_level_indicator_vec_, kOrderDay);
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO) && retval_ >= 0) {
      DBGLOG_TIME_CLASS_FUNC << " modified " << retval_ << " bid orders " << DBGLOG_ENDL_FLUSH;
    }

  } else {
    // cancel bids below this
    retval_ = order_manager_.CancelBidsEqBelowIntPrice(base_iter_bid_px_);
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO) && retval_ > 0) {
      DBGLOG_TIME_CLASS_FUNC << " Cancelled " << retval_ << " Supporting Orders on Bid Side below " << base_iter_bid_px_
                             << "\nPosition: " << my_position_ << '\n';
      order_manager_.ShowOrderBook();
    }

    for (auto i = 0u; i < px_to_be_placed_at_vec_.size(); i++) {
      order_manager_.SendTradeIntPx(int_px_to_be_placed_at_vec_[i], size_to_be_placed_vec_[i], kTradeTypeBuy,
                                    GetOrderLevelIndicator(kTradeTypeBuy, int_px_to_be_placed_at_vec_[i]));
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << " SendTrade "
                               << " Buy Order at px " << px_to_be_placed_at_vec_[i] << " size "
                               << size_to_be_placed_vec_[i] << " Mkt " << best_nonself_bid_price_ << " --- "
                               << best_nonself_ask_price_ << "\nTarget Px: " << target_price_
                               << "\nBest_Bid_Place_Cxl_Price: " << best_bid_place_cxl_px_
                               << "\nPosition: " << my_position_ << '\n';
        order_manager_.ShowOrderBook();
      }
    }
  }

  /// Symetric treatment for ASK side
  tot_sell_placed_ = ask_retval_;
  band_level_ = 0;
  int old_tot_sell_placed_ = 0;
  px_to_be_placed_at_vec_.clear();
  int_px_to_be_placed_at_vec_.clear();
  size_to_be_placed_vec_.clear();
  order_level_indicator_vec_.clear();
  while (my_position_ - tot_sell_placed_ + (int)std::min(param_set_.worst_case_position_, param_set_.max_position_) >
             0 &&
         tot_sell_placed_ < (int)param_set_.max_total_size_to_place_) {
    /// first compute the total size shown in this px band
    /// lower limit of px_band
    old_tot_sell_placed_ = tot_sell_placed_;
    current_px_ = base_iter_ask_px_;
    current_band_ordered_sz_ = 0;
    if (use_stable_book_) {
      ExecLogicUtils::GetLowBandPx(&dep_market_view_, low_band_px_, base_iter_ask_px_, kTradeTypeSell);
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " curPrice: " << low_band_px_ << " low_band_px_ : " << base_iter_ask_px_
                                    << DBGLOG_ENDL_FLUSH;
      }
    } else {
      low_band_px_ = base_iter_ask_px_ + param_set_.px_band_;

      if (agg_only_no_improve_) {
        // Setting low band px as best ask or higher to avoid improve orders
        low_band_px_ = std::max(best_nonself_ask_int_price_, base_iter_ask_px_ + param_set_.px_band_);
        int t_low_band_px_ = low_band_px_ - 1;
        // Cancelling ask orders between best ask and best bid
        // to avoid improve orders
        if (current_px_ < best_nonself_ask_int_price_) {
          while (t_low_band_px_ > current_px_) {
            order_manager_.CancelAsksAtIntPrice(t_low_band_px_);
            t_low_band_px_--;
          }
        }
      }
    }
    if (low_band_px_ == kInvalidIntPrice) {
      break;
    }

    if (current_tradevarset_.l1ask_trade_size_ > 0 && band_level_ == 0) {
      current_band_target_sz_ = current_tradevarset_.l1ask_trade_size_;
    } else {
      current_band_target_sz_ =
          std::min(param_set_.max_position_ + my_position_ - tot_sell_placed_, param_set_.unit_trade_size_);
    }

    if (current_band_target_sz_ <= 0) {
      break;
    }

    while (current_px_ < low_band_px_) {
      current_band_ordered_sz_ += order_manager_.GetTotalAskSizeOrderedAtIntPx(current_px_);
      current_px_++;
    }

    if (use_stable_book_) {
      ExecLogicUtils::CancelOrdersInBand(&order_manager_, base_iter_ask_px_, low_band_px_, current_band_target_sz_,
                                         current_band_ordered_sz_, kTradeTypeSell);
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << " Cancelled " << retval_ << " Band Orders: " << base_iter_ask_px_ << " - "
                               << low_band_px_ + 1 << "\nPosition: " << my_risk_ << '\n';
        order_manager_.ShowOrderBook();
      }
    }

    tot_sell_placed_ += current_band_ordered_sz_;
    if (current_band_ordered_sz_ < current_band_target_sz_) {
      if (current_band_target_sz_ - current_band_ordered_sz_ >= param_set_.min_allowed_unit_trade_size_) {
        px_to_be_placed_at_vec_.push_back(dep_market_view_.GetDoublePx(base_iter_ask_px_));
        int_px_to_be_placed_at_vec_.push_back(base_iter_ask_px_);
        size_to_be_placed_vec_.push_back(current_band_target_sz_ - current_band_ordered_sz_);
        order_level_indicator_vec_.push_back(GetOrderLevelIndicator(kTradeTypeSell, base_iter_ask_px_));
        tot_sell_placed_ += current_band_target_sz_ - current_band_ordered_sz_;
      }
    }
    if (!(tot_sell_placed_ - old_tot_sell_placed_ > 0)) break;
    base_iter_ask_px_ = low_band_px_;
    band_level_++;
  }

  if (param_set_.allow_modify_orders_) {
    /*
       * Send modify for each place-cancel pair, then send Place or cancel depending on which side orders are remaining
       */
    retval_ = order_manager_.CancelReplaceAskOrdersEqAboveAndEqBelowIntPrice(
        cancel_asks_above_ - 1, base_iter_ask_px_, px_to_be_placed_at_vec_, int_px_to_be_placed_at_vec_,
        size_to_be_placed_vec_, order_level_indicator_vec_, kOrderDay);
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO) && retval_ > 0) {
      DBGLOG_TIME_CLASS_FUNC << " modifid " << retval_ << " ask orders \n";
      order_manager_.ShowOrderBook();
    }
  } else {
    // cancel asks above this
    retval_ = order_manager_.CancelAsksEqBelowIntPrice(base_iter_ask_px_);
    for (auto i = 0u; i < int_px_to_be_placed_at_vec_.size(); i++) {
      order_manager_.SendTradeIntPx(int_px_to_be_placed_at_vec_[i], size_to_be_placed_vec_[i], kTradeTypeSell,
                                    GetOrderLevelIndicator(kTradeTypeSell, int_px_to_be_placed_at_vec_[i]));
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << " SendTrade "
                               << " Sell Order at px " << int_px_to_be_placed_at_vec_[i] << " size "
                               << size_to_be_placed_vec_[i] << " Mkt " << best_nonself_bid_price_ << " --- "
                               << best_nonself_ask_price_ << "\nTarget_Px: " << target_price_
                               << "\nBest_Ask_Place_Cxl_Px: " << best_ask_place_cxl_px_
                               << "\nPosition: " << my_position_ << '\n';
        order_manager_.ShowOrderBook();
      }
    }
  }
}

void PriceBasedVolTrading::SetCancelThresholds()  // to reduce message count -- place-keep diff logic
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

void PriceBasedVolTrading::SetImproveBidAskPrices() {
  int best_int_bid_improve_place_cxl_px_ = best_int_bid_place_cxl_px_;
  int best_int_ask_improve_place_cxl_px_ = best_int_ask_place_cxl_px_;

  if (best_nonself_bid_int_price_ < best_int_bid_place_cxl_px_) {
    best_int_bid_improve_place_cxl_px_ =
        std::max(best_nonself_bid_int_price_, (int)(best_int_bid_place_cxl_px_ - param_set_.improve_ticks_));
  }

  if (best_nonself_ask_int_price_ > best_int_ask_place_cxl_px_) {
    best_int_ask_improve_place_cxl_px_ =
        std::min(best_nonself_ask_int_price_, (int)(best_int_ask_place_cxl_px_ + param_set_.improve_ticks_));
  }

  best_int_bid_place_cxl_px_ = std::min(best_nonself_ask_int_price_ - 1, best_int_bid_improve_place_cxl_px_);
  best_int_ask_place_cxl_px_ = std::max(best_nonself_bid_int_price_ + 1, best_int_ask_improve_place_cxl_px_);
}

void PriceBasedVolTrading::SetAggNoImproveBidAskPrices() {
  // Bid side price
  if ((base_bid_price_ - current_tradevarset_.l1bid_aggressive_ >= best_nonself_ask_price_) &&
      (dep_market_view_.spread_increments() <= param_set_.max_int_spread_to_cross_)) {
    best_bid_place_cxl_px_ = base_bid_price_ - current_tradevarset_.l1bid_aggressive_;
    best_int_bid_place_cxl_px_ = floor(best_bid_place_cxl_px_ / dep_market_view_.min_price_increment());
  }

  if ((best_int_bid_place_cxl_px_ < best_nonself_ask_int_price_) &&
      (best_int_bid_place_cxl_px_ >= best_nonself_bid_int_price_)) {
    best_int_bid_place_cxl_px_ = best_nonself_bid_int_price_;
  }

  // Ask side price
  if ((base_ask_price_ + current_tradevarset_.l1ask_aggressive_ <= best_nonself_bid_price_) &&
      (dep_market_view_.spread_increments() <= param_set_.max_int_spread_to_cross_)) {
    best_ask_place_cxl_px_ = base_ask_price_ + current_tradevarset_.l1ask_aggressive_;
    best_int_ask_place_cxl_px_ = ceil(best_ask_place_cxl_px_ / dep_market_view_.min_price_increment());
  }

  if ((best_int_ask_place_cxl_px_ > best_nonself_bid_int_price_) &&
      (best_int_ask_place_cxl_px_ <= best_nonself_ask_int_price_)) {
    best_int_ask_place_cxl_px_ = best_nonself_ask_int_price_;
  }
}

void PriceBasedVolTrading::CheckAggressCoolOff() {
  if (watch_.msecs_from_midnight() - last_bid_agg_msecs_ < param_set_.agg_cooloff_interval_) {
    best_int_bid_place_cxl_px_ = std::min(best_nonself_ask_int_price_ - 1, best_int_bid_place_cxl_px_);

    if (agg_only_no_improve_) {
      best_int_bid_place_cxl_px_ = std::min(best_nonself_bid_int_price_, best_int_bid_place_cxl_px_);
    }
  }
  if (watch_.msecs_from_midnight() - last_ask_agg_msecs_ < param_set_.agg_cooloff_interval_) {
    best_int_ask_place_cxl_px_ = std::max(best_nonself_bid_int_price_ + 1, best_int_ask_place_cxl_px_);

    if (agg_only_no_improve_) {
      best_int_ask_place_cxl_px_ = std::max(best_nonself_ask_int_price_, best_int_ask_place_cxl_px_);
    }
  }
}

void PriceBasedVolTrading::CheckImproveCoolOff() {
  if (watch_.msecs_from_midnight() - last_bid_imp_msecs_ < param_set_.improve_cooloff_interval_ &&
      watch_.msecs_from_midnight() - last_buy_msecs_ < param_set_.cooloff_interval_) {
    best_int_bid_place_cxl_px_ = std::min(best_nonself_bid_int_price_, best_int_bid_place_cxl_px_);
  }
  if (watch_.msecs_from_midnight() - last_ask_imp_msecs_ < param_set_.improve_cooloff_interval_ &&
      watch_.msecs_from_midnight() - last_sell_msecs_ < param_set_.cooloff_interval_) {
    best_int_ask_place_cxl_px_ = std::max(best_nonself_ask_int_price_, best_int_ask_place_cxl_px_);
  }
}

void PriceBasedVolTrading::CheckORSMDSDelay() {
  if (last_buy_int_price_ <= best_nonself_bid_int_price_ &&
      watch_.msecs_from_midnight() - last_buy_msecs_ < SMALL_COOLOFF_MSECS_) {
    best_int_bid_place_cxl_px_ = std::min(last_buy_int_price_ - 1, best_int_bid_place_cxl_px_);
  }

  if (last_sell_int_price_ >= best_nonself_ask_int_price_ &&
      watch_.msecs_from_midnight() - last_sell_msecs_ < SMALL_COOLOFF_MSECS_) {
    best_int_ask_place_cxl_px_ = std::max(last_sell_int_price_ + 1, best_int_ask_place_cxl_px_);
  }
}

void PriceBasedVolTrading::BidCancelLogic() {
  if (current_tradevarset_.l1bid_trade_size_ > 0) {
    if (!param_set_.allow_modify_orders_) {
      retval_ = order_manager_.CancelBidsAboveIntPrice(best_int_bid_place_cxl_px_);
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO) && retval_ > 0) {
        DBGLOG_TIME_CLASS_FUNC << " Cancelled " << retval_ << " bid orders above " << best_int_bid_place_cxl_px_
                               << " TLA\nTarget: " << target_price_
                               << "\nBest_Bid_Place_Cxl_Px: " << best_bid_place_cxl_px_
                               << "\nPosition: " << my_position_ << '\n';
        order_manager_.ShowOrderBook();
      }
    }
    base_iter_bid_px_ = best_int_bid_place_cxl_px_;
  } else {
    if (!param_set_.allow_modify_orders_) {
      retval_ = order_manager_.CancelBidsAboveIntPrice(best_nonself_bid_int_price_ - param_set_.px_band_);
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO) && retval_ > 0) {
        DBGLOG_TIME_CLASS_FUNC << " Cancelled " << retval_ << " top band bid orders above "
                               << best_nonself_bid_int_price_ - param_set_.px_band_ << "\nPosition: " << my_position_
                               << '\n';
        order_manager_.ShowOrderBook();
      }
      base_iter_bid_px_ = best_nonself_bid_int_price_ - param_set_.px_band_;
    }
    base_iter_bid_px_ = best_nonself_bid_int_price_ - param_set_.px_band_;
  }
  cancel_bids_above_ = base_iter_bid_px_;
}

void PriceBasedVolTrading::AskCancelLogic() {
  if (current_tradevarset_.l1ask_trade_size_ > 0) {
    if (!param_set_.allow_modify_orders_) {
      retval_ = order_manager_.CancelAsksAboveIntPrice(best_int_ask_place_cxl_px_);
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO) && retval_ > 0) {
        DBGLOG_TIME_CLASS_FUNC << " Cancelled " << retval_ << " ask orders above " << best_int_ask_place_cxl_px_
                               << " TLA\nTarget: " << target_price_
                               << "\nBest_Ask_Place_Cxl_Px: " << best_ask_place_cxl_px_
                               << "\nPosition: " << my_position_ << '\n';
        order_manager_.ShowOrderBook();
      }
    }
    base_iter_ask_px_ = best_int_ask_place_cxl_px_;
  } else {
    if (!param_set_.allow_modify_orders_) {
      retval_ = order_manager_.CancelAsksAboveIntPrice(best_nonself_ask_int_price_ + param_set_.px_band_);
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO) && retval_ > 0) {
        DBGLOG_TIME_CLASS_FUNC << " Cancelled " << retval_ << " top band ask orders better than  "
                               << best_nonself_ask_int_price_ + param_set_.px_band_ << "\nPosition: " << my_position_
                               << '\n';
        order_manager_.ShowOrderBook();
      }
      base_iter_ask_px_ = best_nonself_ask_int_price_ + param_set_.px_band_;
    }
  }
  cancel_asks_above_ = base_iter_ask_px_;
}

void PriceBasedVolTrading::PrintFullStatus() {
  DBGLOG_TIME << "tgt: " << target_price_ << " ExpBidPft@ " << best_nonself_bid_price_ << " X "
              << best_nonself_bid_size_ << ' ' << (target_price_ - best_nonself_bid_price_) << " ExpAskPft@ "
              << best_nonself_ask_price_ << " X " << best_nonself_ask_size_ << ' '
              << (best_nonself_ask_price_ - target_price_) << " signalbias: "
              << ((target_price_ - dep_market_view_.mkt_size_weighted_price()) / dep_market_view_.min_price_increment())
              << " pos: " << my_position_ << " l1_SBS "
              << order_manager_.intpx_2_sum_bid_confirmed(best_nonself_bid_int_price_) << " l1_SAS "
              << order_manager_.intpx_2_sum_ask_confirmed(best_nonself_ask_int_price_) << DBGLOG_ENDL_FLUSH;
}

char PriceBasedVolTrading::GetOrderLevelIndicator(TradeType_t order_side, int int_order_px) {
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

void PriceBasedVolTrading::OnExec(const int t_new_position_, const int _exec_quantity_, const TradeType_t _buysell_,
                                  const double _price_, const int r_int_price_, const int _security_id_) {
  if (t_new_position_ > my_position_) {
    last_buy_msecs_ = watch_.msecs_from_midnight();
    last_buy_int_price_ = r_int_price_;
  } else if (t_new_position_ < my_position_) {
    last_sell_msecs_ = watch_.msecs_from_midnight();
    last_sell_int_price_ = r_int_price_;
  }
}

void PriceBasedVolTrading::OnCancelReject(const TradeType_t _buysell_, const double _price_, const int r_int_price_,
                                          const int _security_id_) {
  if (_buysell_ == kTradeTypeBuy) {
    last_buy_msecs_ = watch_.msecs_from_midnight();
    last_buy_int_price_ = r_int_price_;
  } else {
    last_sell_msecs_ = watch_.msecs_from_midnight();
    last_sell_int_price_ = r_int_price_;
  }
}
}
