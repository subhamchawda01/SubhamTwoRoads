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
#include "dvctrade/ExecLogic/vol_mm_trading.hpp"
// exec_logic_code / defines.hpp was empty
#include "dvctrade/ExecLogic/vol_utils.hpp"

#define SMALL_PX_DIFF 1e-05
#define SMALL_COOLOFF_MSECS_ 10

namespace HFSAT {

VolMMTrading::VolMMTrading(DebugLogger& dbglogger, const Watch& _watch_, const SecurityMarketView& dep_market_view,
                           SmartOrderManager& order_manager, const std::string& paramfilename, const bool livetrading,
                           MulticastSenderSocket* p_strategy_param_sender_socket,
                           EconomicEventsManager& economic_events_manager, const int trading_start_utc_mfm,
                           const int trading_end_utc_mfm, const int runtime_id,
                           const std::vector<std::string> this_model_source_shortcode_vec)
    : BaseTrading(dbglogger, _watch_, dep_market_view, order_manager, paramfilename, livetrading,
                  p_strategy_param_sender_socket, economic_events_manager, trading_start_utc_mfm, trading_end_utc_mfm,
                  runtime_id, this_model_source_shortcode_vec) {
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
        SampleDataUtil::GetAvgForPeriod(dep_market_view.shortcode(), watch_.YYYYMMDD(), 20, trading_start_utc_mfm_,
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

  last_bid_agg_msecs_ = 0;
  last_ask_agg_msecs_ = 0;
  last_bid_imp_msecs_ = 0;
  last_ask_imp_msecs_ = 0;
  cancel_bids_above_ = 0;
  cancel_asks_above_ = 0;

  int_px_to_be_placed_at_vec_.clear();
  size_to_be_placed_vec_.clear();

  use_stable_book_ = false;
  if (param_set_.read_use_stable_book_) {
    use_stable_book_ = param_set_.use_stable_book_;
  }
}

void VolMMTrading::TradingLogic() {
  m_stdev_ = exec_logic_indicators_helper_->stdev();
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
    best_int_bid_place_cxl_px_ = std::min(best_nonself_ask_int_price_, best_int_bid_place_cxl_px_);
    best_int_ask_place_cxl_px_ = std::max(best_nonself_bid_int_price_, best_int_ask_place_cxl_px_);
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

  /*
   * While placing improve orders as well as aggressive orders, following can happen
   * If the spread is high enough and px_band is small ,we will end up placing multiple improve orders
   *
   * Similar can happen with aggressive, we might end up placing aggressive as well as improve order.
   *
   * First case shouldn't make much effect because if signal is strong enough to place improve order high enough
   * to cross more than px_band size, it would go same with second order in most of the cases,
   */

  std::vector<BaseOrder*> bid_orders_to_cancel;
  std::vector<std::vector<BaseOrder*> >& bid_order_vec = order_manager_.BidOrderVec();

  int bid_int_px_to_start_at = best_int_bid_place_cxl_px_;

  int order_manager_top_bid_index = order_manager_.GetOrderVecTopBidIndex();
  int order_manager_bottom_bid_index = order_manager_.GetOrderVecBottomBidIndex();

  int order_manager_top_bid_int_price = kInvalidIntPrice;
  //  int order_manager_bottom_bid_int_price = kInvalidIntPrice;
  if (order_manager_top_bid_index != -1) {
    order_manager_top_bid_int_price = order_manager_.GetBidIntPrice(order_manager_top_bid_index);
    // order_manager_bottom_bid_int_price = order_manager_.GetBidIntPrice(order_manager_bottom_bid_index);
    bid_int_px_to_start_at = std::max(bid_int_px_to_start_at, order_manager_top_bid_int_price);
  }

  for (int intpx = bid_int_px_to_start_at; intpx > best_int_bid_place_cxl_px_; intpx--) {
    // Should  come here only if we have orders above best_int_bid_place_cxl_px_
    int om_idx = order_manager_.GetBidIndex(intpx);
    for (auto order : bid_order_vec[om_idx]) {
      bid_orders_to_cancel.push_back(order);
    }
  }

  tot_buy_placed_ = 0;
  int old_tot_buy_placed = 0;
  int_px_to_be_placed_at_vec_.clear();
  size_to_be_placed_vec_.clear();
  base_iter_bid_px_ = best_int_bid_place_cxl_px_;

  while (tot_buy_placed_ < (int)param_set_.max_total_size_to_place_ &&
         (my_position_ + tot_buy_placed_ < param_set_.max_position_)) {
    current_px_ = base_iter_bid_px_;
    current_band_ordered_sz_ = 0;

    low_band_px_ = base_iter_bid_px_ - param_set_.px_band_;

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

    bool size_already_available_in_band = false;
    while (current_px_ > low_band_px_) {
      int size_at_this_price = order_manager_.GetTotalBidSizeOrderedAtIntPx(current_px_);
      current_band_ordered_sz_ += size_at_this_price;
      current_px_--;
      if (size_already_available_in_band && size_at_this_price > 0) {
        // If size has crossed the target limit, start canceling
        int order_vec_index = order_manager_.GetBidIndex(current_px_);
        for (auto order : bid_order_vec[order_vec_index]) {
          bid_orders_to_cancel.push_back(order);
        }
      } else if (current_band_ordered_sz_ >= current_band_target_sz_) {
        // If sum of size till this point has already crossed the required amount, Start canceling
        size_already_available_in_band = true;
      }
    }

    tot_buy_placed_ += current_band_ordered_sz_;
    if (current_band_ordered_sz_ < current_band_target_sz_) {
      // pushing the prices and sizes for all orders in a vector and placing them after cancel has been called
      if (current_band_target_sz_ - current_band_ordered_sz_ >= param_set_.min_allowed_unit_trade_size_) {
        int_px_to_be_placed_at_vec_.push_back(base_iter_bid_px_);
        size_to_be_placed_vec_.push_back(current_band_target_sz_ - current_band_ordered_sz_);
        tot_buy_placed_ += current_band_target_sz_ - current_band_ordered_sz_;
      }
    }
    if (!(tot_buy_placed_ - old_tot_buy_placed > 0)) break;
    base_iter_bid_px_ = low_band_px_;
    band_level_++;
  }

  int bid_prices_to_place_till_idx = order_manager_.GetBidIndex(low_band_px_);
  for (int idx = bid_prices_to_place_till_idx;
       (order_manager_bottom_bid_index != -1) && idx >= order_manager_bottom_bid_index; idx--) {
    for (auto order : bid_order_vec[idx]) {
      bid_orders_to_cancel.push_back(order);
    }
  }

  int cancel_index = 0, place_index = 0, could_not_cancel_index = 0;
  while (cancel_index < (int)bid_orders_to_cancel.size() && place_index < (int)int_px_to_be_placed_at_vec_.size()) {
    if (order_manager_.Modify(bid_orders_to_cancel[cancel_index],
                              dep_market_view_.GetDoublePx(int_px_to_be_placed_at_vec_[place_index]),
                              int_px_to_be_placed_at_vec_[place_index], size_to_be_placed_vec_[place_index])) {
      // To set last*_msecs_ variables
      GetOrderLevelIndicator(kTradeTypeBuy, int_px_to_be_placed_at_vec_[place_index]);
      place_index++;
    } else {
      could_not_cancel_index++;
    }
    cancel_index++;
  }

  while (cancel_index < (int)bid_orders_to_cancel.size()) {
    if (order_manager_.Cancel(*bid_orders_to_cancel[cancel_index])) {
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << " Canceled order " << GetTradeTypeChar(bid_orders_to_cancel[cancel_index]->buysell())
                               << " of size " << bid_orders_to_cancel[cancel_index]->size_remaining()
                               << " Price: " << bid_orders_to_cancel[cancel_index]->price() << DBGLOG_ENDL_FLUSH;
      }
    }
    cancel_index++;
  }

  while (place_index < (int)int_px_to_be_placed_at_vec_.size() - could_not_cancel_index) {
    // Not placing for orders which could not be canceled.
    // Also giving preference to orders lcloser to best
    SendTradeAndLog(dep_market_view_.GetDoublePx(int_px_to_be_placed_at_vec_[place_index]),
                    int_px_to_be_placed_at_vec_[place_index], size_to_be_placed_vec_[place_index], kTradeTypeBuy,
                    GetOrderLevelIndicator(kTradeTypeBuy, int_px_to_be_placed_at_vec_[place_index]), "PlaceInBand");
    place_index++;
  }

  ///------------------------------------------------------------------------------------///

  std::vector<BaseOrder*> ask_orders_to_cancel;
  std::vector<std::vector<BaseOrder*> >& ask_order_vec = order_manager_.AskOrderVec();

  int ask_int_px_to_start_at = best_int_ask_place_cxl_px_;

  int order_manager_top_ask_index = order_manager_.GetOrderVecTopAskIndex();
  int order_manager_bottom_ask_index = order_manager_.GetOrderVecBottomAskIndex();

  int order_manager_top_ask_int_price = kInvalidIntPrice;
  //  int order_manager_bottom_ask_int_price = kInvalidIntPrice;
  if (order_manager_top_ask_index != -1) {
    order_manager_top_ask_int_price = order_manager_.GetAskIntPrice(order_manager_top_ask_index);
    // order_manager_bottom_ask_int_price = order_manager_.GetAskIntPrice(order_manager_bottom_ask_index);
    ask_int_px_to_start_at = std::min(ask_int_px_to_start_at, order_manager_top_ask_int_price);
  }

  for (int intpx = ask_int_px_to_start_at; intpx < best_int_ask_place_cxl_px_; intpx++) {
    // Should  come here only if we have orders above best_int_bid_place_cxl_px_
    int om_idx = order_manager_.GetAskIndex(intpx);
    for (auto order : ask_order_vec[om_idx]) {
      ask_orders_to_cancel.push_back(order);
    }
  }

  /***************************************************************************************************************/
  tot_sell_placed_ = 0;
  int old_tot_sell_placed = 0;
  int_px_to_be_placed_at_vec_.clear();
  size_to_be_placed_vec_.clear();
  base_iter_ask_px_ = best_int_ask_place_cxl_px_;

  while (tot_sell_placed_ < (int)param_set_.max_total_size_to_place_ &&
         (my_position_ - tot_sell_placed_ + param_set_.max_position_ > 0)) {
    current_px_ = base_iter_ask_px_;
    current_band_ordered_sz_ = 0;
    low_band_px_ = base_iter_ask_px_ + param_set_.px_band_;

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

    bool size_already_available_in_band = false;
    while (current_px_ < low_band_px_) {
      int size_at_this_price = order_manager_.GetTotalAskSizeOrderedAtIntPx(current_px_);
      current_band_ordered_sz_ += size_at_this_price;
      current_px_++;

      if (size_already_available_in_band && size_at_this_price > 0) {
        // Cancel all extra orders in this band
        int order_vec_index = order_manager_.GetAskIndex(current_px_);
        for (auto order : ask_order_vec[order_vec_index]) {
          ask_orders_to_cancel.push_back(order);
        }
      } else if (current_band_ordered_sz_ >= current_band_target_sz_) {
        // If sum of size till this point has already crossed the required amount, Start canceling
        size_already_available_in_band = true;
      }
    }

    tot_sell_placed_ += current_band_ordered_sz_;
    if (current_band_ordered_sz_ < current_band_target_sz_) {
      if (current_band_target_sz_ - current_band_ordered_sz_ >= param_set_.min_allowed_unit_trade_size_) {
        int_px_to_be_placed_at_vec_.push_back(base_iter_ask_px_);
        size_to_be_placed_vec_.push_back(current_band_target_sz_ - current_band_ordered_sz_);
        tot_sell_placed_ += current_band_target_sz_ - current_band_ordered_sz_;
      }
    }
    if (!(tot_sell_placed_ - old_tot_sell_placed > 0)) break;
    base_iter_ask_px_ = low_band_px_;
    band_level_++;
  }

  int ask_prices_to_place_till_idx = order_manager_.GetAskIndex(low_band_px_);
  for (int idx = ask_prices_to_place_till_idx;
       (order_manager_bottom_ask_index != -1) && idx >= order_manager_bottom_ask_index; idx--) {
    for (auto order : ask_order_vec[idx]) {
      ask_orders_to_cancel.push_back(order);
    }
  }

  cancel_index = 0, place_index = 0, could_not_cancel_index = 0;
  while (cancel_index < (int)ask_orders_to_cancel.size() && place_index < (int)int_px_to_be_placed_at_vec_.size()) {
    if (order_manager_.Modify(ask_orders_to_cancel[cancel_index],
                              dep_market_view_.GetDoublePx(int_px_to_be_placed_at_vec_[place_index]),
                              int_px_to_be_placed_at_vec_[place_index], size_to_be_placed_vec_[place_index])) {
      // Set last*msecs variables
      GetOrderLevelIndicator(kTradeTypeSell, int_px_to_be_placed_at_vec_[place_index]);
      place_index++;
    } else {
      could_not_cancel_index++;
    }
    cancel_index++;
  }

  while (cancel_index < (int)ask_orders_to_cancel.size()) {
    if (order_manager_.Cancel(*ask_orders_to_cancel[cancel_index])) {
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC << " Canceled order " << GetTradeTypeChar(ask_orders_to_cancel[cancel_index]->buysell())
                               << " of size " << ask_orders_to_cancel[cancel_index]->size_remaining()
                               << " Price: " << ask_orders_to_cancel[cancel_index]->price() << DBGLOG_ENDL_FLUSH;
      }
    }
    cancel_index++;
  }

  while (place_index < (int)int_px_to_be_placed_at_vec_.size() - could_not_cancel_index) {
    // Not placing for orders which could not be canceled.
    // Also giving preference to orders lcloser to best
    SendTradeAndLog(dep_market_view_.GetDoublePx(int_px_to_be_placed_at_vec_[place_index]),
                    int_px_to_be_placed_at_vec_[place_index], size_to_be_placed_vec_[place_index], kTradeTypeSell,
                    GetOrderLevelIndicator(kTradeTypeSell, int_px_to_be_placed_at_vec_[place_index]), "PlaceInBand");
    place_index++;
  }

  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    // Showing book here
    DBGLOG_TIME_CLASS_FUNC << " mkt: [ " << best_nonself_bid_size_ << " " << best_nonself_bid_price_ << " x "
                           << best_nonself_ask_price_ << " " << best_nonself_ask_size_
                           << " ] \nTarget Px: " << target_price_
                           << "\nBest_Bid_Place_Cxl_Price: " << best_bid_place_cxl_px_
                           << "\nBest_Ask_Place_Cxl_Price: " << best_ask_place_cxl_px_ << "\nPosition: " << my_position_
                           << '\n' << DBGLOG_ENDL_FLUSH;
    order_manager_.ShowOrderBook();
  }
  ask_orders_to_cancel.clear();
  bid_orders_to_cancel.clear();
}

void VolMMTrading::SetCancelThresholds()  // to reduce message count -- place-keep diff logic
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

void VolMMTrading::SetImproveBidAskPrices() {
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

void VolMMTrading::CheckAggressCoolOff() {
  if (watch_.msecs_from_midnight() - last_bid_agg_msecs_ < param_set_.agg_cooloff_interval_) {
    best_int_bid_place_cxl_px_ = std::min(best_nonself_ask_int_price_ - 1, best_int_bid_place_cxl_px_);
  }
  if (watch_.msecs_from_midnight() - last_ask_agg_msecs_ < param_set_.agg_cooloff_interval_) {
    best_int_ask_place_cxl_px_ = std::max(best_nonself_bid_int_price_ + 1, best_int_ask_place_cxl_px_);
  }
}

void VolMMTrading::CheckImproveCoolOff() {
  if (watch_.msecs_from_midnight() - last_bid_imp_msecs_ < param_set_.improve_cooloff_interval_ &&
      watch_.msecs_from_midnight() - last_buy_msecs_ < param_set_.cooloff_interval_) {
    best_int_bid_place_cxl_px_ = std::min(best_nonself_bid_int_price_, best_int_bid_place_cxl_px_);
  }
  if (watch_.msecs_from_midnight() - last_ask_imp_msecs_ < param_set_.improve_cooloff_interval_ &&
      watch_.msecs_from_midnight() - last_sell_msecs_ < param_set_.cooloff_interval_) {
    best_int_ask_place_cxl_px_ = std::max(best_nonself_ask_int_price_, best_int_ask_place_cxl_px_);
  }
}

void VolMMTrading::CheckORSMDSDelay() {
  if (last_buy_int_price_ <= best_nonself_bid_int_price_ &&
      watch_.msecs_from_midnight() - last_buy_msecs_ < SMALL_COOLOFF_MSECS_) {
    best_int_bid_place_cxl_px_ = std::min(last_buy_int_price_ - 1, best_int_bid_place_cxl_px_);
  }

  if (last_sell_int_price_ >= best_nonself_ask_int_price_ &&
      watch_.msecs_from_midnight() - last_sell_msecs_ < SMALL_COOLOFF_MSECS_) {
    best_int_ask_place_cxl_px_ = std::max(last_sell_int_price_ + 1, best_int_ask_place_cxl_px_);
  }
}

void VolMMTrading::BidCancelLogic() {
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

void VolMMTrading::AskCancelLogic() {
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

void VolMMTrading::PrintFullStatus() {
  DBGLOG_TIME << "tgt: " << target_price_ << " ExpBidPft@ " << best_nonself_bid_price_ << " X "
              << best_nonself_bid_size_ << ' ' << (target_price_ - best_nonself_bid_price_) << " ExpAskPft@ "
              << best_nonself_ask_price_ << " X " << best_nonself_ask_size_ << ' '
              << (best_nonself_ask_price_ - target_price_) << " signalbias: "
              << ((target_price_ - dep_market_view_.mkt_size_weighted_price()) / dep_market_view_.min_price_increment())
              << " pos: " << my_position_ << " l1_SBS "
              << order_manager_.intpx_2_sum_bid_confirmed(best_nonself_bid_int_price_) << " l1_SAS "
              << order_manager_.intpx_2_sum_ask_confirmed(best_nonself_ask_int_price_) << DBGLOG_ENDL_FLUSH;
}

char VolMMTrading::GetOrderLevelIndicator(TradeType_t order_side, int int_order_px) {
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

void VolMMTrading::OnExec(const int new_position, const int exec_quantity, const TradeType_t buysell,
                          const double price, const int int_price, const int security_id) {
  if (new_position > my_position_) {
    last_buy_msecs_ = watch_.msecs_from_midnight();
    last_buy_int_price_ = int_price;
  } else if (new_position < my_position_) {
    last_sell_msecs_ = watch_.msecs_from_midnight();
    last_sell_int_price_ = int_price;
  }
}

void VolMMTrading::OnCancelReject(const TradeType_t buysell, const double price, const int int_price,
                                  const int security_id) {
  if (buysell == kTradeTypeBuy) {
    last_buy_msecs_ = watch_.msecs_from_midnight();
    last_buy_int_price_ = int_price;
  } else {
    last_sell_msecs_ = watch_.msecs_from_midnight();
    last_sell_int_price_ = int_price;
  }
}
}
