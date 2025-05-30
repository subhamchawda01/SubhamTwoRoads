/**
    \file MarketAdapter/indexed_eobi_market_view_manager.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#include "baseinfra/MarketAdapter/indexed_nse_market_view_manager_2.hpp"

namespace HFSAT {
#define NSE_V2_SANITIZE_TIME_THRESHOLD 50  // 50 msecs
#define NSE_TRIGGER_MKT_UPDATE_ON_TRADE 0  // if set to non-1 this will suppress OnMarketUpdate call on trade
                                           // if a trade message reflecting the same book state has been sent.
#define NSE_V2_EPSILON 1e-5
#define DISABLE_TRIGGER_ON_WIDE_SPREAD 0
#define WIDE_SPREAD_FOR_DISABLE_TRIGGER \
  0.01  // don't call trigger trade if spread > WIDE_SPREAD_FOR_DISABLE_TRIGGER*px and px >
        // MIN_PRICE_FOR_DISABLE_TRIGGER
#define MIN_PRICE_FOR_DISABLE_TRIGGER 20  //

IndexedNSEMarketViewManager2::IndexedNSEMarketViewManager2(
    DebugLogger &t_dbglogger_, const Watch &t_watch_, const SecurityNameIndexer &t_sec_name_indexer_,
    const std::vector<SecurityMarketView *> &t_security_market_view_map_, bool _use_self_book_)
    : BaseMarketViewManager(t_dbglogger_, t_watch_, t_sec_name_indexer_, t_security_market_view_map_),
      trade_time_manager_(TradeTimeManager::GetUniqueInstance(t_sec_name_indexer_, t_watch_.YYYYMMDD())),
      order_history_instance(t_sec_name_indexer_.NumSecurityId()),
      self_order_history_instance(t_sec_name_indexer_.NumSecurityId()),
      px_level_manager_(t_dbglogger_, t_watch_, t_sec_name_indexer_.NumSecurityId()),
      use_self_book_(_use_self_book_),
      big_trades_listener_vec_(t_sec_name_indexer_.NumSecurityId()) {
  dbglogger_ << "IndexedNSEMarketViewManager2::IndexedNSEMarketViewManager2 constructor called\n";
  // Set px level manager for all NSE shortcodes
  self_trader_ids_.clear();
  for (unsigned int t_ctr_ = 0; t_ctr_ < t_sec_name_indexer_.NumSecurityId(); t_ctr_++) {
    if (strncmp((t_security_market_view_map_[t_ctr_]->shortcode()).c_str(), "NSE_", 4) == 0) {
      t_security_market_view_map_[t_ctr_]->SetPxLevelManager(&px_level_manager_);
      SecurityMarketView &smv_ = *(t_security_market_view_map_[t_ctr_]);
      market_view_ptr_ = &smv_;
      SetSMVBestVars(t_ctr_);
      px_level_manager_.CheckAndSetPredictiveUncross(t_ctr_, t_security_market_view_map_[t_ctr_]->shortcode());
      smv_.using_predictive_uncross_ = px_level_manager_.UsingPredictiveUncross(t_ctr_);
      HFSAT::NSETradingLimit_t *t_trading_limit_ =
          HFSAT::NSESecurityDefinitions::GetTradingLimits(t_security_market_view_map_[t_ctr_]->shortcode());
      if ((t_trading_limit_ != NULL) && (t_trading_limit_->upper_limit_ != 0) &&
          (t_trading_limit_->lower_limit_ != 0)) {
        smv_.upper_int_price_limit_ = t_trading_limit_->upper_limit_;
        smv_.lower_int_price_limit_ = t_trading_limit_->lower_limit_;
      } else {
        dbglogger_ << "Error: Price Limit not present for " << smv_.shortcode() << "\n";
      }
      dbglogger_ << "Price Limit for " << smv_.shortcode() << " : "
                 << smv_.lower_int_price_limit_ * smv_.min_price_increment_ << "-"
                 << smv_.upper_int_price_limit_ * smv_.min_price_increment_ << "\n";
    }
  }
}

// debug function - not intented to be used in prod
void IndexedNSEMarketViewManager2::CheckL1Consistency(int security_id_) {
  SecurityMarketView &smv_ = *market_view_ptr_;
  // Check consistency of security markey view market_update_info struct with
  // sparse_index_book_px_level_manager state - should be the state at every call
  // to listeners and end of processing of messages
  if (smv_.market_update_info_.bestbid_size_ != px_level_manager_.GetSynBidSize(security_id_, 0) ||
      smv_.market_update_info_.bestask_size_ != px_level_manager_.GetSynAskSize(security_id_, 0) ||
      fabs(smv_.market_update_info_.bestbid_price_ - px_level_manager_.GetSynBidPrice(security_id_, 0)) >
          NSE_V2_EPSILON ||
      fabs(smv_.market_update_info_.bestask_price_ - px_level_manager_.GetSynAskPrice(security_id_, 0)) >
          NSE_V2_EPSILON) {
    dbglogger_ << " PxMgrDiff_L1 " << smv_.market_update_info_.bestbid_size_ << " @ "
               << smv_.market_update_info_.bestbid_price_ << " --- " << smv_.market_update_info_.bestask_price_ << " @ "
               << smv_.market_update_info_.bestask_size_ << " <> " << px_level_manager_.GetSynBidSize(security_id_, 0)
               << " @ " << px_level_manager_.GetSynBidPrice(security_id_, 0) << " --- "
               << px_level_manager_.GetSynAskPrice(security_id_, 0) << " @ "
               << px_level_manager_.GetSynAskSize(security_id_, 0) << '\n';
  }
}

/*
* Function that sends trade notifications for any incoming aggressive order.
*/
void IndexedNSEMarketViewManager2::TriggerTrade(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                                const int t_int_price_, const uint32_t t_size_,
                                                int t_int_price_level_) {
  MarketUpdateInfoLevelStruct *uncrossed_pl_ = NULL;
  int t_curr_price_ = t_int_price_;

  switch (t_buysell_) {
    // Corresponds to an aggressive buy order
    case kTradeTypeBuy: {
      uncrossed_pl_ = px_level_manager_.GetSynAskPL(t_security_id_, 0);
      int num_levels_cleared_ = (uncrossed_pl_ != NULL)
                                    ? (std::min(MAX_NUM_SORTED_BOOK_LEVELS, uncrossed_pl_->limit_int_price_level_))
                                    : px_level_manager_.NumValidAskLevels(t_security_id_);

      // Case 1. Best synthetic ask price is unlatered. Trade is triggered only for
      // traded size
      if (uncrossed_pl_ != NULL && uncrossed_pl_->limit_int_price_ == t_int_price_) {
        NotifyListenersOnTrade(t_security_id_, t_int_price_, (t_size_ - uncrossed_pl_->limit_size_), t_buysell_, false,
                               0);
      } else {
        // Case 2. Top level is completely traded away
        int prev_px = t_curr_price_;
        uint32_t prev_size = t_size_;
        int cur_lvl_ = t_int_price_level_;

        MarketUpdateInfoLevelStruct *t_next_pl_;
        if (t_int_price_level_ == DEEP_LEVEL_MARKER) {
          num_levels_cleared_ = num_levels_cleared_ - px_level_manager_.NumValidAskLevels(t_security_id_);
          t_next_pl_ = (px_level_manager_.underlying_ask_ds_)->GetHigherPriceLevel(t_security_id_, t_curr_price_);
        } else {
          num_levels_cleared_ = num_levels_cleared_ - t_int_price_level_;
          cur_lvl_++;
          t_next_pl_ = px_level_manager_.GetAskPL(t_security_id_, cur_lvl_);
        }

        while (t_next_pl_ != NULL &&
               (uncrossed_pl_ == NULL || t_next_pl_->limit_int_price_ <= uncrossed_pl_->limit_int_price_)) {
          t_curr_price_ = t_next_pl_->limit_int_price_;
          if (uncrossed_pl_ == NULL || t_next_pl_->limit_int_price_ < uncrossed_pl_->limit_int_price_) {
            // entire non-top level is traded away by agg order
            NotifyListenersOnTrade(t_security_id_, prev_px, prev_size, t_buysell_, true, num_levels_cleared_);

            prev_px = t_curr_price_;
            prev_size = t_next_pl_->limit_size_;
          } else {
            // potentially partsize at non-top level is traded away by agg order
            if (t_next_pl_->limit_size_ > uncrossed_pl_->limit_size_) {
              NotifyListenersOnTrade(t_security_id_, prev_px, prev_size, t_buysell_, true, num_levels_cleared_);

              prev_px = t_curr_price_;
              prev_size = t_next_pl_->limit_size_ - uncrossed_pl_->limit_size_;
            }
          }
          if (t_int_price_level_ == DEEP_LEVEL_MARKER) {
            t_next_pl_ = (px_level_manager_.underlying_ask_ds_)->GetHigherPriceLevel(t_security_id_, t_curr_price_);
          } else {
            cur_lvl_++;
            t_next_pl_ = px_level_manager_.GetAskPL(t_security_id_, cur_lvl_);
          }
        }
        NotifyListenersOnTrade(t_security_id_, prev_px, prev_size, t_buysell_, false, num_levels_cleared_);
      }
    } break;
    case kTradeTypeSell: {
      uncrossed_pl_ = px_level_manager_.GetSynBidPL(t_security_id_, 0);
      int num_levels_cleared_ = (uncrossed_pl_ != NULL)
                                    ? (std::min(MAX_NUM_SORTED_BOOK_LEVELS, uncrossed_pl_->limit_int_price_level_))
                                    : px_level_manager_.NumValidBidLevels(t_security_id_);

      // Case 1. Best synthetic bid price is unlatered. Trade is triggered only for
      // traded size
      if (uncrossed_pl_ != NULL && uncrossed_pl_->limit_int_price_ == t_int_price_) {
        NotifyListenersOnTrade(t_security_id_, t_int_price_, (t_size_ - uncrossed_pl_->limit_size_), t_buysell_, false,
                               num_levels_cleared_);
      } else {
        // Case 2. Top level is completely traded away
        int prev_px = t_curr_price_;
        uint32_t prev_size = t_size_;
        int cur_lvl_ = t_int_price_level_;

        MarketUpdateInfoLevelStruct *t_next_pl_;
        if (t_int_price_level_ == DEEP_LEVEL_MARKER) {
          num_levels_cleared_ = num_levels_cleared_ - px_level_manager_.NumValidBidLevels(t_security_id_);
          t_next_pl_ = (px_level_manager_.underlying_bid_ds_)->GetLowerPriceLevel(t_security_id_, t_curr_price_);
        } else {
          num_levels_cleared_ = num_levels_cleared_ - t_int_price_level_;
          cur_lvl_++;
          t_next_pl_ = px_level_manager_.GetBidPL(t_security_id_, cur_lvl_);
        }

        while (t_next_pl_ != NULL &&
               (uncrossed_pl_ == NULL || t_next_pl_->limit_int_price_ >= uncrossed_pl_->limit_int_price_)) {
          t_curr_price_ = t_next_pl_->limit_int_price_;
          if (uncrossed_pl_ == NULL || t_next_pl_->limit_int_price_ > uncrossed_pl_->limit_int_price_) {
            // entire non-top level is traded away by agg order
            NotifyListenersOnTrade(t_security_id_, prev_px, prev_size, t_buysell_, true, num_levels_cleared_);

            prev_px = t_curr_price_;
            prev_size = t_next_pl_->limit_size_;
          } else {
            // potentially partsize at non-top level is traded away by agg order
            if (t_next_pl_->limit_size_ > uncrossed_pl_->limit_size_) {
              NotifyListenersOnTrade(t_security_id_, prev_px, prev_size, t_buysell_, true, num_levels_cleared_);

              prev_px = t_curr_price_;
              prev_size = t_next_pl_->limit_size_ - uncrossed_pl_->limit_size_;
            }
          }
          if (t_int_price_level_ == DEEP_LEVEL_MARKER) {
            t_next_pl_ = (px_level_manager_.underlying_bid_ds_)->GetLowerPriceLevel(t_security_id_, t_curr_price_);
          } else {
            cur_lvl_++;
            t_next_pl_ = px_level_manager_.GetBidPL(t_security_id_, cur_lvl_);
          }
        }
        NotifyListenersOnTrade(t_security_id_, prev_px, prev_size, t_buysell_, false, num_levels_cleared_);
      }
    } break;
    default:
      return;
  }
  NotifyListenersOnLevelChange(t_security_id_, k_l1price);
}

void IndexedNSEMarketViewManager2::OnOrderAdd(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                              const uint64_t order_id_, const double t_price_, const uint32_t t_size_,
                                              const bool t_is_intermediate_) {
  //  std::cout << watch_.tv() << " OrderID: " <<  order_id_ << " OrderAdd@" << t_price_ << " of size " << t_size_ << "
  //  " << ((t_buysell_ == HFSAT::TradeType_t::kTradeTypeSell) ? "SELL":"BUY") << std::endl;
  SecurityMarketView &smv_ = *(security_market_view_map_[t_security_id_]);
  market_view_ptr_ = &smv_;
  int int_price_ = smv_.GetIntPx(t_price_);
  if (smv_.is_book_crossed_) {
    SanitizeBookOnCrossedBook(t_security_id_);
  } else if (!big_trades_listener_vec_[t_security_id_].empty()) {
    if ((t_buysell_ == kTradeTypeBuy) ? ((smv_.syn_best_ask_level_info_ != NULL) &&
                                         (smv_.syn_best_ask_level_info_->limit_int_price_ <= int_price_))
                                      : ((smv_.syn_best_bid_level_info_ != NULL) &&
                                         (smv_.syn_best_bid_level_info_->limit_int_price_ >= int_price_))) {
      int size_remaning_ = 0;
      bool is_valid_book_end_ = false;
      int last_traded_int_price_ = -1;
      int min_price_levels_cleared_ = px_level_manager_.FindCrossedLevels(
          t_security_id_, t_size_, int_price_, t_buysell_, size_remaning_, is_valid_book_end_, last_traded_int_price_);
      for (auto listener_ : big_trades_listener_vec_[t_security_id_]) {
        listener_->OnLargeDirectionalTrades(t_security_id_, t_buysell_, min_price_levels_cleared_,
                                            last_traded_int_price_, (t_size_ - size_remaning_), is_valid_book_end_);
      }
    }
  }

  if ((smv_.lower_int_price_limit_ != -1) &&
      ((int_price_ > smv_.upper_int_price_limit_) || (int_price_ < smv_.lower_int_price_limit_))) {
    HFSAT::NSETradingLimit_t *t_trading_limit_ =
        HFSAT::NSESecurityDefinitions::ChangeTradingLimits(smv_.shortcode(), int_price_);
    if (t_trading_limit_ != NULL) {
      dbglogger_ << "Price Limit crossed for shortcode " << smv_.shortcode()
                 << " PrevUL: " << smv_.upper_int_price_limit_ * 0.05
                 << " PrevLL: " << smv_.lower_int_price_limit_ * 0.05 << " OrderPrice: " << t_price_
                 << " NewUL: " << t_trading_limit_->upper_limit_ * 0.05
                 << " NewLL: " << t_trading_limit_->lower_limit_ * 0.05 << '\n';
      smv_.upper_int_price_limit_ = t_trading_limit_->upper_limit_;
      smv_.lower_int_price_limit_ = t_trading_limit_->lower_limit_;
    } else {
      dbglogger_ << "ERROR: Trading limit existed before but not now for " << smv_.shortcode() << '\n';
    }
  }

  if (use_self_book_) {
    // noop if self order
    if (self_order_history_instance.IsOrderSeen(t_security_id_, order_id_)) {
      return;
    }
  }

  LevelChangeType_t event_type_ = k_nochange;
  bool is_crossing_index_ = false;

  //  bool is_order_seen = order_history_instance.IsOrderSeen(t_security_id_, order_id_);
  //  if (is_order_seen) {
  //    dbglogger_ << "Incorrect AddOrder state - existing orderid added " << order_id_ << '\n';
  //    return;
  //  }

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      MarketUpdateInfoLevelStruct *t_syn_ask_level_ = smv_.syn_best_ask_level_info_;
      // check and trigger trades on agg add
      if (t_syn_ask_level_ != NULL && t_syn_ask_level_->limit_int_price_ <= int_price_) {
        pre_predicted_best_ask_int_price_ = t_syn_ask_level_->limit_int_price_;
        pre_predicted_best_ask_size_ = t_syn_ask_level_->limit_size_;
        pre_predicted_best_ask_lvl_ = t_syn_ask_level_->limit_int_price_level_;
        is_crossing_index_ = true;
      }
      event_type_ = px_level_manager_.AddOrderToPxLevel(t_security_id_, t_buysell_, int_price_, t_price_, t_size_);
      SetSMVBestVars(t_security_id_);
      if (is_crossing_index_ && smv_.using_predictive_uncross_) {
        if (DISABLE_TRIGGER_ON_WIDE_SPREAD != 1 ||
            smv_.market_update_info_.bestask_price_ <
                (1.0 + WIDE_SPREAD_FOR_DISABLE_TRIGGER) * smv_.market_update_info_.bestbid_price_ ||
            smv_.market_update_info_.bestbid_price_ < MIN_PRICE_FOR_DISABLE_TRIGGER) {
          // Send a pre emptive trade notification
          TriggerTrade(t_security_id_, t_buysell_, pre_predicted_best_ask_int_price_, pre_predicted_best_ask_size_,
                       pre_predicted_best_ask_lvl_);
        }
      }
    } break;
    case kTradeTypeSell: {
      MarketUpdateInfoLevelStruct *t_syn_bid_level_ = smv_.syn_best_bid_level_info_;
      // check and trigger trades on agg add
      if (t_syn_bid_level_ != NULL && t_syn_bid_level_->limit_int_price_ >= int_price_) {
        pre_predicted_best_bid_int_price_ = t_syn_bid_level_->limit_int_price_;
        pre_predicted_best_bid_size_ = t_syn_bid_level_->limit_size_;
        pre_predicted_best_bid_lvl_ = t_syn_bid_level_->limit_int_price_level_;
        is_crossing_index_ = true;
      }
      event_type_ = px_level_manager_.AddOrderToPxLevel(t_security_id_, t_buysell_, int_price_, t_price_, t_size_);
      SetSMVBestVars(t_security_id_);
      if (is_crossing_index_ && smv_.using_predictive_uncross_) {
        if (DISABLE_TRIGGER_ON_WIDE_SPREAD != 1 ||
            smv_.market_update_info_.bestask_price_ <
                (1.0 + WIDE_SPREAD_FOR_DISABLE_TRIGGER) * smv_.market_update_info_.bestbid_price_ ||
            smv_.market_update_info_.bestbid_price_ < MIN_PRICE_FOR_DISABLE_TRIGGER) {
          // Send a pre emptive trade notification
          TriggerTrade(t_security_id_, t_buysell_, pre_predicted_best_bid_int_price_, pre_predicted_best_bid_size_,
                       pre_predicted_best_bid_lvl_);
        }
      }

    } break;
    default:
      break;
  }

  if (!is_crossing_index_ || !(smv_.using_predictive_uncross_))
    NotifyListenersOnLevelChange(t_security_id_, event_type_);

  smv_.is_book_crossed_ = px_level_manager_.IsBookCrossed(t_security_id_);
  if(order_history_instance.IsOrderSeen(t_security_id_, order_id_)){
    OrderDetailsStruct *t_order_ = order_history_instance.GetOrderDetails(t_security_id_, order_id_);
    OnOrderDelete(t_security_id_, t_buysell_, order_id_, t_order_->order_price, true, false);
  }
  order_history_instance.AddOrderToOrderHistory(t_security_id_, t_buysell_, t_price_, t_size_, order_id_);

  //  std::cout << smv_.ShowMarket() << std::endl;
  //  CheckL1Consistency(t_security_id_);
}

// Notify listeners on Trade
void IndexedNSEMarketViewManager2::NotifyListenersOnTrade(const uint32_t t_security_id_, const int t_trade_int_price_,
                                                          const int t_trade_size_, const TradeType_t t_buysell_,
                                                          bool is_intermediate, int _num_levels_cleared_) {
  SecurityMarketView &smv_ = *market_view_ptr_;
  smv_.is_ready_ = true;

  double trade_price_ = smv_.GetDoublePx(t_trade_int_price_);

  smv_.trade_print_info_.trade_price_ = trade_price_;
  smv_.trade_print_info_.size_traded_ = t_trade_size_;
  smv_.trade_print_info_.int_trade_price_ = t_trade_int_price_;
  smv_.trade_print_info_.buysell_ = t_buysell_;
  smv_.trade_print_info_.is_intermediate_ = is_intermediate;
  smv_.trade_print_info_.num_levels_cleared_ = _num_levels_cleared_;

  smv_.raw_trade_print_info_.trade_price_ = trade_price_;
  smv_.raw_trade_print_info_.size_traded_ = t_trade_size_;
  smv_.raw_trade_print_info_.int_trade_price_ = t_trade_int_price_;

  //  CheckL1Consistency(t_security_id_);
  if ((CheckValidTime(t_security_id_)) && ((t_trade_int_price_ <= smv_.upper_int_trade_range_limit_) &&
                                           (t_trade_int_price_ >= smv_.lower_int_trade_range_limit_))) {
    smv_.NotifyTradeListeners();
    smv_.NotifyOnReadyListeners();
    
    smv_.NotifyRawTradeListeners();
  }
}

void IndexedNSEMarketViewManager2::SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_) {
  for (auto i = 0u; i < security_market_view_map_.size(); i++) {
    SecurityMarketView *smv_ = security_market_view_map_[i];
    smv_->set_skip_listener_notification_end_time(r_start_time_);
  }
}

void IndexedNSEMarketViewManager2::SetSMVBestVars(int t_security_id_) {
  SecurityMarketView &smv_ = *market_view_ptr_;
  MarketUpdateInfoLevelStruct *best_bid_level_ = px_level_manager_.GetSynBidPL(t_security_id_, 0);
  MarketUpdateInfoLevelStruct *best_ask_level_ = px_level_manager_.GetSynAskPL(t_security_id_, 0);

  smv_.market_update_info_.bestbid_price_ = (best_bid_level_ != NULL ? best_bid_level_->limit_price_ : kInvalidPrice);
  smv_.market_update_info_.bestbid_int_price_ =
      (best_bid_level_ != NULL ? best_bid_level_->limit_int_price_ : kInvalidIntPrice);
  smv_.market_update_info_.bestbid_size_ = (best_bid_level_ != NULL ? best_bid_level_->limit_size_ : kInvalidSize);
  smv_.market_update_info_.bestbid_ordercount_ = (best_bid_level_ != NULL ? best_bid_level_->limit_ordercount_ : 0);
  smv_.market_update_info_.bestask_price_ = (best_ask_level_ != NULL ? best_ask_level_->limit_price_ : kInvalidPrice);
  smv_.market_update_info_.bestask_int_price_ =
      (best_ask_level_ != NULL ? best_ask_level_->limit_int_price_ : kInvalidIntPrice);
  smv_.market_update_info_.bestask_size_ = (best_ask_level_ != NULL ? best_ask_level_->limit_size_ : kInvalidSize);
  smv_.market_update_info_.bestask_ordercount_ = (best_ask_level_ != NULL ? best_ask_level_->limit_ordercount_ : 0);
  smv_.syn_best_bid_level_info_ = best_bid_level_;
  smv_.syn_best_ask_level_info_ = best_ask_level_;

  // mid/mkt prices and spread increments might be used in pnl etc classes so those need to be set as well
  if (smv_.price_type_subscribed_[kPriceTypeMidprice]) {
    if (best_bid_level_ == NULL || best_ask_level_ == NULL) {
      // smv_.is_ready_ = false;
      smv_.market_update_info_.mid_price_ = kInvalidPrice;
    } else {
      smv_.market_update_info_.mid_price_ =
          (smv_.market_update_info_.bestbid_price_ + smv_.market_update_info_.bestask_price_) * 0.5;
    }
  }

  smv_.market_update_info_.spread_increments_ =
      smv_.market_update_info_.bestask_int_price_ - smv_.market_update_info_.bestbid_int_price_;

  if (smv_.price_type_subscribed_[kPriceTypeMktSizeWPrice]) {
    if (best_bid_level_ == NULL || best_ask_level_ == NULL) {
      // smv_.is_ready_ = false;
      smv_.market_update_info_.mkt_size_weighted_price_ = kInvalidPrice;
    } else {
      if (smv_.market_update_info_.spread_increments_ <= 1) {
        smv_.market_update_info_.mkt_size_weighted_price_ =
            (smv_.market_update_info_.bestbid_price_ * smv_.market_update_info_.bestask_size_ +
             smv_.market_update_info_.bestask_price_ * smv_.market_update_info_.bestbid_size_) /
            (smv_.market_update_info_.bestbid_size_ + smv_.market_update_info_.bestask_size_);
      } else {
        smv_.market_update_info_.mkt_size_weighted_price_ =
            ((smv_.market_update_info_.bestbid_price_ * smv_.market_update_info_.bestask_size_ +
              smv_.market_update_info_.bestask_price_ * smv_.market_update_info_.bestbid_size_) /
                 (smv_.market_update_info_.bestbid_size_ + smv_.market_update_info_.bestask_size_) +
             (smv_.market_update_info_.mid_price_)) /
            2.0;
      }
    }
  }

  // set is_ready
  //  if (!smv_.is_ready_ && best_bid_level_ != NULL && best_ask_level_ != NULL) {
  //    smv_.is_ready_ = true;
  //  }

  smv_.market_update_info_.spread_increments_ =
      smv_.market_update_info_.bestask_int_price_ - smv_.market_update_info_.bestbid_int_price_;
}

// Notifying listeners on a level change. Happens after order add/modify or delete.
void IndexedNSEMarketViewManager2::NotifyListenersOnLevelChange(const uint32_t t_security_id_,
                                                                LevelChangeType_t event_type_) {
  SecurityMarketView &smv_ = *market_view_ptr_;
  smv_.is_ready_ = true;

  //  CheckL1Consistency(t_security_id_);
  if (CheckValidTime(t_security_id_)) {
    // Notify relevant listeners about the update
    if (event_type_ == k_l1price) {
      smv_.NotifyL1PriceListeners();
    } else if (event_type_ == k_l1size) {
      smv_.NotifyL1SizeListeners();
    } else if (event_type_ == k_l2change) {
      smv_.NotifyL2Listeners();
      smv_.NotifyL2OnlyListeners();
    }
  }
}

// Handling Order Modify message:
void IndexedNSEMarketViewManager2::OnOrderModify(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                                 const uint64_t order_id_, const double t_price_,
                                                 const uint32_t t_size_) {
  //  std::cout << watch_.tv() << " OrderID: " <<  order_id_ << " OrderModify@" << t_price_ << " of size " << t_size_ <<
  //  " " << ((t_buysell_ == HFSAT::TradeType_t::kTradeTypeSell) ? "SELL":"BUY") << std::endl;
  SecurityMarketView &smv_ = *(security_market_view_map_[t_security_id_]);
  market_view_ptr_ = &smv_;

  if (smv_.is_book_crossed_) {
    SanitizeBookOnCrossedBook(t_security_id_);
  }

  OrderDetailsStruct *t_order_ = order_history_instance.GetOrderDetails(t_security_id_, order_id_);
  if (t_order_ == NULL) {
    // Order isn't present in our history. Simulate an OnOrderAdd
    OnOrderAdd(t_security_id_, t_buysell_, order_id_, t_price_, t_size_, false);
    return;
  }

  bool is_trade_triggered_ = false;
  LevelChangeType_t event_type_;

  int int_price_ = smv_.GetIntPx(t_price_);
  if ((smv_.lower_int_price_limit_ != -1) &&
      ((int_price_ > smv_.upper_int_price_limit_) || (int_price_ < smv_.lower_int_price_limit_))) {
    HFSAT::NSETradingLimit_t *t_trading_limit_ =
        HFSAT::NSESecurityDefinitions::ChangeTradingLimits(smv_.shortcode(), int_price_);
    if (t_trading_limit_ != NULL) {
      dbglogger_ << "Price Limit crossed for shortcode " << smv_.shortcode()
                 << " PrevUL: " << smv_.upper_int_price_limit_ * 0.05
                 << " PrevLL: " << smv_.lower_int_price_limit_ * 0.05 << " OrderPrice: " << t_price_
                 << " NewUL: " << t_trading_limit_->upper_limit_ * 0.05
                 << " NewLL: " << t_trading_limit_->lower_limit_ * 0.05 << '\n';
      smv_.upper_int_price_limit_ = t_trading_limit_->upper_limit_;
      smv_.lower_int_price_limit_ = t_trading_limit_->lower_limit_;
    } else {
      dbglogger_ << "ERROR: Trading limit existed before but not now for " << smv_.shortcode() << '\n';
    }
  }

  // Modify buy order
  if (t_order_->is_buy_order) {
    // store pre-modify L1 ask values
    MarketUpdateInfoLevelStruct *t_syn_ask_level_ = smv_.syn_best_ask_level_info_;
    if (t_syn_ask_level_ != NULL) {
      pre_predicted_best_ask_int_price_ = t_syn_ask_level_->limit_int_price_;
      pre_predicted_best_ask_size_ = t_syn_ask_level_->limit_size_;
      pre_predicted_best_ask_lvl_ = t_syn_ask_level_->limit_int_price_level_;
    } else {
      pre_predicted_best_ask_int_price_ = kInvalidIntPrice;
      pre_predicted_best_ask_size_ = kInvalidSize;
      pre_predicted_best_ask_lvl_ = SP_INDEX_BOOK_INVALID_PX_LEVEL;
    }
    if (smv_.DblPxCompare(t_order_->order_price, t_price_)) {
      event_type_ = px_level_manager_.ModifySizeAtPxLevel(t_security_id_, t_buysell_, int_price_,
                                                          t_size_ - t_order_->order_size, false);
    } else {
      event_type_ =
          px_level_manager_.ModifyOrderAtDiffLevels(t_security_id_, t_buysell_, smv_.GetIntPx(t_order_->order_price),
                                                    t_order_->order_size, int_price_, t_price_, t_size_);
    }
    SetSMVBestVars(t_security_id_);
    MarketUpdateInfoLevelStruct *t_syn_post_ask_level_ = smv_.syn_best_ask_level_info_;
    if ((pre_predicted_best_ask_size_ != kInvalidSize) &&
        ((t_syn_post_ask_level_ == NULL) ||
         (t_syn_post_ask_level_ != NULL &&
          ((t_syn_post_ask_level_->limit_int_price_ > pre_predicted_best_ask_int_price_) ||
           (t_syn_post_ask_level_->limit_int_price_ == pre_predicted_best_ask_int_price_ &&
            t_syn_post_ask_level_->limit_size_ < pre_predicted_best_ask_size_)))) &&
        smv_.using_predictive_uncross_) {
      // Send a pre emptive trade notification
      //      dbglogger_ << "Pre-modify trade L1 Ask: " << pre_predicted_best_ask_size_ << " @ " << (
      //      pre_predicted_best_ask_int_price_/20.0 )
      //      		 << " Order MODIFY BUY " << t_size_ << " @ " << t_price_ << " OID " << order_id_ << " Pre_modify
      //      "
      //		 << t_order_->order_size << " @ " << t_order_->order_price <<  ' ' << smv_.shortcode() << '\n';
      //      px_level_manager_.DumpBook(t_security_id_);
      if (DISABLE_TRIGGER_ON_WIDE_SPREAD != 1 ||
          smv_.market_update_info_.bestask_price_ <
              (1.0 + WIDE_SPREAD_FOR_DISABLE_TRIGGER) * smv_.market_update_info_.bestbid_price_ ||
          smv_.market_update_info_.bestbid_price_ < MIN_PRICE_FOR_DISABLE_TRIGGER) {
        TriggerTrade(t_security_id_, t_buysell_, pre_predicted_best_ask_int_price_, pre_predicted_best_ask_size_,
                     pre_predicted_best_ask_lvl_);
        is_trade_triggered_ = true;
      }
      //      dbglogger_ << "Post-modify trade L1 Ask: " << t_syn_post_ask_level_->limit_size_ << " @ " <<
      //      t_syn_post_ask_level_->limit_price_ << '\n';
    }
  } else {
    // Modify sell order
    // store pre-modify L1 bid values
    MarketUpdateInfoLevelStruct *t_syn_bid_level_ = smv_.syn_best_bid_level_info_;
    if (t_syn_bid_level_ != NULL) {
      pre_predicted_best_bid_int_price_ = t_syn_bid_level_->limit_int_price_;
      pre_predicted_best_bid_size_ = t_syn_bid_level_->limit_size_;
      pre_predicted_best_bid_lvl_ = t_syn_bid_level_->limit_int_price_level_;
    } else {
      pre_predicted_best_bid_int_price_ = kInvalidIntPrice;
      pre_predicted_best_bid_size_ = kInvalidSize;
      pre_predicted_best_bid_lvl_ = SP_INDEX_BOOK_INVALID_PX_LEVEL;
    }
    if (smv_.DblPxCompare(t_order_->order_price, t_price_)) {
      event_type_ = px_level_manager_.ModifySizeAtPxLevel(t_security_id_, t_buysell_, int_price_,
                                                          t_size_ - t_order_->order_size, false);
    } else {
      event_type_ =
          px_level_manager_.ModifyOrderAtDiffLevels(t_security_id_, t_buysell_, smv_.GetIntPx(t_order_->order_price),
                                                    t_order_->order_size, int_price_, t_price_, t_size_);
    }
    SetSMVBestVars(t_security_id_);
    MarketUpdateInfoLevelStruct *t_syn_post_bid_level_ = smv_.syn_best_bid_level_info_;
    if ((pre_predicted_best_bid_size_ != kInvalidSize) &&
        ((t_syn_post_bid_level_ == NULL) ||
         (t_syn_post_bid_level_ != NULL &&
          ((t_syn_post_bid_level_->limit_int_price_ < pre_predicted_best_bid_int_price_) ||
           (t_syn_post_bid_level_->limit_int_price_ == pre_predicted_best_bid_int_price_ &&
            t_syn_post_bid_level_->limit_size_ < pre_predicted_best_bid_size_)))) &&
        smv_.using_predictive_uncross_) {
      // Send a pre emptive trade notification
      //      dbglogger_ << "Pre-modify trade L1 Buy: " << pre_predicted_best_bid_size_ << " @ " << (
      //      pre_predicted_best_bid_int_price_/20.0 )
      //      		 << " Order MODIFY SELL " << t_size_ << " @ " << t_price_ << " OID " << order_id_ << "
      //      Pre_modify
      //      "
      //		 << t_order_->order_size << " @ " << t_order_->order_price << ' ' << smv_.shortcode() << '\n';
      //      px_level_manager_.DumpBook(t_security_id_);
      if (DISABLE_TRIGGER_ON_WIDE_SPREAD != 1 ||
          smv_.market_update_info_.bestask_price_ <
              (1.0 + WIDE_SPREAD_FOR_DISABLE_TRIGGER) * smv_.market_update_info_.bestbid_price_ ||
          smv_.market_update_info_.bestbid_price_ < MIN_PRICE_FOR_DISABLE_TRIGGER) {
        TriggerTrade(t_security_id_, t_buysell_, pre_predicted_best_bid_int_price_, pre_predicted_best_bid_size_,
                     pre_predicted_best_bid_lvl_);
        is_trade_triggered_ = true;
      }
      //      dbglogger_ << "Post-modify trade L1 Bid: " << t_syn_post_bid_level_->limit_size_ << " @ " <<
      //      t_syn_post_bid_level_->limit_price_ << '\n';
    }
  }

  t_order_->order_size = t_size_;
  t_order_->order_price = t_price_;

  if (!is_trade_triggered_) {
    NotifyListenersOnLevelChange(t_security_id_, event_type_);
  }
  smv_.is_book_crossed_ = px_level_manager_.IsBookCrossed(t_security_id_);
  //  std::cout << smv_.ShowMarket() << std::endl;
}

void IndexedNSEMarketViewManager2::OnOrderDelete(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                                 const uint64_t order_id_, const double t_price_,
                                                 const bool t_delete_order_, const bool t_is_intermediate_) {
  SecurityMarketView &smv_ = *(security_market_view_map_[t_security_id_]);
  market_view_ptr_ = &smv_;

  if (smv_.is_book_crossed_) {
    SanitizeBookOnCrossedBook(t_security_id_);
  }

  OrderDetailsStruct *t_order_ = order_history_instance.GetOrderDetails(t_security_id_, order_id_);
  LevelChangeType_t event_type_ = k_nochange;
  if (t_order_ == NULL) {
    // Order is not seen. Delete call is an invalid one. Return ..
    // Removing debug information since this happens a few times.
    // dbglogger_ << " OrderDelete called for order not present in map " << order_id_ << '\n';
    return;
  } else {
    event_type_ = px_level_manager_.ModifySizeAtPxLevel(
        t_security_id_, t_buysell_, smv_.GetIntPx(t_order_->order_price), 0 - t_order_->order_size, true);
  }
  //  std::cout << watch_.tv() << " OrderID: " <<  order_id_ << " OrderDelete@" << t_price_ << " of size " <<
  //  t_order_->order_size << " " << ((t_buysell_ == HFSAT::TradeType_t::kTradeTypeSell) ? "SELL":"BUY") << std::endl;
  order_history_instance.DeleteOrderFromHistory(t_security_id_, t_order_->order_id);

  SetSMVBestVars(t_security_id_);
  NotifyListenersOnLevelChange(t_security_id_, event_type_);
  smv_.is_book_crossed_ = px_level_manager_.IsBookCrossed(t_security_id_);

  //  std::cout << smv_.ShowMarket() << std::endl;
  //  if (!t_is_intermediate_) {
  //  CheckL1Consistency(t_security_id_);
  //  }
}

void IndexedNSEMarketViewManager2::OnTrade(const unsigned int t_security_id_, double const t_trade_price_,
                                           const int t_trade_size_, const uint64_t t_buy_order_num_,
                                           const uint64_t t_sell_order_num_) {
  //  std::cout << watch_.tv() << " BuyOrderID: " <<  t_buy_order_num_ << " SellOrderID: " << t_sell_order_num_ << "
  //  OnTrade@" << t_trade_price_ << " of size " << t_trade_size_ << std::endl;

  SecurityMarketView &smv_ = *(security_market_view_map_[t_security_id_]);
  market_view_ptr_ = &smv_;
  bool t_book_crossed_ = smv_.is_book_crossed_;
  SanitizeBookOnCrossedTrade(t_security_id_, t_trade_price_);

  if (NSE_TRIGGER_MKT_UPDATE_ON_TRADE == 1 || (t_book_crossed_ && smv_.using_predictive_uncross_)) {
    MarketUpdateInfoLevelStruct *t_syn_ask_level_ = smv_.syn_best_ask_level_info_;
    if (t_syn_ask_level_ != NULL) {
      pre_predicted_best_ask_int_price_ = t_syn_ask_level_->limit_int_price_;
      pre_predicted_best_ask_size_ = t_syn_ask_level_->limit_size_;
    } else {
      pre_predicted_best_ask_int_price_ = kInvalidIntPrice;
      pre_predicted_best_ask_size_ = kInvalidSize;
    }
    MarketUpdateInfoLevelStruct *t_syn_bid_level_ = smv_.syn_best_bid_level_info_;
    if (t_syn_bid_level_ != NULL) {
      pre_predicted_best_bid_int_price_ = t_syn_bid_level_->limit_int_price_;
      pre_predicted_best_bid_size_ = t_syn_bid_level_->limit_size_;
    } else {
      pre_predicted_best_bid_int_price_ = kInvalidIntPrice;
      pre_predicted_best_bid_size_ = kInvalidSize;
    }
  }

  int t_trade_int_price_ = smv_.GetIntPx(t_trade_price_);

  bool is_buy_order_seen_ = order_history_instance.IsOrderSeen(t_security_id_, t_buy_order_num_);
  bool is_sell_order_seen_ = order_history_instance.IsOrderSeen(t_security_id_, t_sell_order_num_);

  TradeType_t trd_type_ = kTradeTypeBuy;
  LevelChangeType_t event_type_ = k_nochange;

  if (is_buy_order_seen_) {
    OrderDetailsStruct *t_order_ = order_history_instance.GetOrderDetails(t_security_id_, t_buy_order_num_);
    int t_del_size_ = std::min(t_order_->order_size, t_trade_size_);
    event_type_ = px_level_manager_.ModifySizeAtPxLevel(t_security_id_, HFSAT::kTradeTypeBuy,
                                                        smv_.GetIntPx(t_order_->order_price), -t_del_size_,
                                                        (t_del_size_ == t_order_->order_size));
    trd_type_ = kTradeTypeSell;
    t_order_->order_size -= t_del_size_;
    if (t_order_->order_size == 0) {
      order_history_instance.DeleteOrderFromHistory(t_security_id_, t_order_->order_id);
    }
  }

  if (is_sell_order_seen_) {
    OrderDetailsStruct *t_order_ = order_history_instance.GetOrderDetails(t_security_id_, t_sell_order_num_);
    int t_del_size_ = std::min(t_order_->order_size, t_trade_size_);
    LevelChangeType_t t_event_type_ = px_level_manager_.ModifySizeAtPxLevel(
        t_security_id_, HFSAT::kTradeTypeSell, smv_.GetIntPx(t_order_->order_price), -t_del_size_,
        (t_del_size_ == t_order_->order_size));
    if ((event_type_ != k_l1price) && (event_type_ != k_l1size)) {
      event_type_ = t_event_type_;
    }
    trd_type_ = kTradeTypeBuy;
    t_order_->order_size -= t_del_size_;
    if (t_order_->order_size == 0) {
      order_history_instance.DeleteOrderFromHistory(t_security_id_, t_order_->order_id);
    }
  }
  SetSMVBestVars(t_security_id_);

  // For cases of pre-emptive trade (ie. no hidden orders ) we choose not to propagate trades when book
  // is crossed.
  if (!t_book_crossed_ && smv_.using_predictive_uncross_) {
    // Trades not reflected in book will get passed as Buy trades.
    // TODO - heuristic can be improved a bit.
    NotifyListenersOnTrade(t_security_id_, t_trade_int_price_, t_trade_size_, trd_type_, false,
                           ((event_type_ == k_l1price) ? 1 : 0));
    NotifyListenersOnLevelChange(t_security_id_, k_l1price);
  }

  if (!smv_.using_predictive_uncross_) {
    if (!is_buy_order_seen_ || !is_sell_order_seen_) {
      NotifyListenersOnTrade(t_security_id_, t_trade_int_price_, t_trade_size_, trd_type_, false,
                             ((event_type_ == k_l1price) ? 1 : 0));
    } else {
      // book should be crossed and trade type is dictated by direction of initial cross
      if (px_level_manager_.IsCrossingSideBid(t_security_id_)) {
        trd_type_ = HFSAT::kTradeTypeBuy;
      } else {
        trd_type_ = HFSAT::kTradeTypeSell;
      }
      NotifyListenersOnTrade(t_security_id_, t_trade_int_price_, t_trade_size_, trd_type_, false,
                             ((event_type_ == k_l1price) ? 1 : 0));
    }
  }

  if (NSE_TRIGGER_MKT_UPDATE_ON_TRADE == 1 || (t_book_crossed_ && smv_.using_predictive_uncross_)) {
    // Trigger level updates where applicable - TODO in case finer differentiation of
    // L1 price/size/L2 change is needed we might need to put in support - currently it
    // triggers l1price;
    // If NSE_TRIGGER_MKT_UPDATE_ON_TRADE == 0 there will be very few of these calls ( in FO )
    // corresponding to situations like BANKNIFTY_FUT0 1465372074.302145 ( my guess would be that
    // these are trigger orders (stop loss market) or alternately packet drop scenarios )
    MarketUpdateInfoLevelStruct *t_syn_ask_level_ = smv_.syn_best_ask_level_info_;
    MarketUpdateInfoLevelStruct *t_syn_bid_level_ = smv_.syn_best_bid_level_info_;
    if ((t_syn_bid_level_ == NULL && pre_predicted_best_bid_int_price_ != kInvalidIntPrice) ||
        (t_syn_ask_level_ == NULL && pre_predicted_best_ask_int_price_ != kInvalidIntPrice) ||
        (t_syn_bid_level_ != NULL && (t_syn_bid_level_->limit_int_price_ != pre_predicted_best_bid_int_price_ ||
                                      t_syn_bid_level_->limit_size_ != pre_predicted_best_bid_size_)) ||
        (t_syn_ask_level_ != NULL && (t_syn_ask_level_->limit_int_price_ != pre_predicted_best_ask_int_price_ ||
                                      t_syn_ask_level_->limit_size_ != pre_predicted_best_ask_size_))) {
      NotifyListenersOnLevelChange(t_security_id_, k_l1price);
    }
  }
  smv_.is_book_crossed_ = px_level_manager_.IsBookCrossed(t_security_id_);
  //  std::cout << smv_.ShowMarket() << std::endl;
  //  CheckL1Consistency(t_security_id_);
}

void IndexedNSEMarketViewManager2::OnTradeExecRange(const unsigned int t_security_id_, double const t_low_exec_band_,
                                                    double const t_high_exec_band_) {
  SecurityMarketView &smv_ = *(security_market_view_map_[t_security_id_]);
  // std::cout << "LP: " << t_low_exec_band_ << " HP: " << t_high_exec_band_ << std::endl;
  smv_.upper_int_trade_range_limit_ = smv_.GetIntPx(t_high_exec_band_);
  smv_.lower_int_trade_range_limit_ = smv_.GetIntPx(t_low_exec_band_);
}

// Called in cases where atleast one of the ordrs involved in the trade is a stop-loss order
void IndexedNSEMarketViewManager2::OnHiddenTrade(const unsigned int t_security_id_, double const t_trade_price_,
                                                 const int t_trade_size_, const uint64_t t_buy_order_num_,
                                                 const uint64_t t_sell_order_num_) {
  // Defunct - only called for historical NSE provided data in Stop Loss scenarios - won't occur in Live
  OnTrade(t_security_id_, t_trade_price_, t_trade_size_, t_buy_order_num_, t_sell_order_num_);
}

double IndexedNSEMarketViewManager2::GetSanitizePx(int t_security_id_, TradeType_t sanitize_side_,
                                                   MarketUpdateInfoLevelStruct *best_crossed_bid_level_,
                                                   MarketUpdateInfoLevelStruct *best_crossed_ask_level_) {
  SecurityMarketView &smv_ = *market_view_ptr_;
  double t_best_syn_bpx_;
  double t_best_syn_apx_;
  // Case 1: We are using predictive uncross
  if (smv_.using_predictive_uncross_) {
    t_best_syn_bpx_ = px_level_manager_.GetSynBidPrice(t_security_id_, 0);
    t_best_syn_apx_ = px_level_manager_.GetSynAskPrice(t_security_id_, 0);
  } else {
    MarketUpdateInfoLevelStruct *t_bid_level_ = NULL;
    MarketUpdateInfoLevelStruct *t_ask_level_ = NULL;
    int t_bid_size_to_delete_ = 0;
    int t_ask_size_to_delete_ = 0;
    px_level_manager_.SetUncrossSizesandPrices(t_security_id_, t_bid_level_, t_ask_level_, t_bid_size_to_delete_,
                                               t_ask_size_to_delete_);
    if (t_bid_level_ != NULL) {
      t_best_syn_bpx_ = t_bid_level_->limit_price_;
    } else {
      t_best_syn_bpx_ = kInvalidPrice;
    }
    if (t_ask_level_ != NULL) {
      t_best_syn_apx_ = t_ask_level_->limit_price_;
    } else {
      t_best_syn_apx_ = kInvalidPrice;
    }
  }

  if (sanitize_side_ == HFSAT::kTradeTypeBuy) {
    return std::max(t_best_syn_bpx_, best_crossed_ask_level_->limit_price_);
  } else {
    // deal with separate case of one side being empty after sanitize
    if (t_best_syn_apx_ > kInvalidPrice + NSE_V2_EPSILON && t_best_syn_apx_ < best_crossed_bid_level_->limit_price_) {
      return t_best_syn_apx_;
    } else {
      return best_crossed_bid_level_->limit_price_;
    }
  }
}

void IndexedNSEMarketViewManager2::SanitizeBookOnCrossedBook(const uint32_t t_security_id_) {
  if (px_level_manager_.IsBookCrossed(t_security_id_) &&
      watch_.msecs_from_midnight() - px_level_manager_.InitBookCrossTime(t_security_id_) >
          NSE_V2_SANITIZE_TIME_THRESHOLD) {
    MarketUpdateInfoLevelStruct *t_best_bid_level_ = px_level_manager_.GetBidPL(t_security_id_, 0);
    MarketUpdateInfoLevelStruct *t_best_ask_level_ = px_level_manager_.GetAskPL(t_security_id_, 0);
    TradeType_t side_to_sanitize_ =
        (t_best_bid_level_->mod_time_ < t_best_ask_level_->mod_time_ ? kTradeTypeBuy : kTradeTypeSell);
    // sanitize price can be adjusted for desired tradeoff
    double t_px_to_sanitize_ = GetSanitizePx(t_security_id_, side_to_sanitize_, t_best_bid_level_, t_best_ask_level_);
    dbglogger_ << " SanitizeBookOnCrossedBook triggered at " << watch_.tv() << '\n';
    SanitizeBook(t_security_id_, side_to_sanitize_, t_px_to_sanitize_);
    // check and uncross book after Sanitize - by deleting orders of opposite side is needed
    t_best_bid_level_ = px_level_manager_.GetBidPL(t_security_id_, 0);
    t_best_ask_level_ = px_level_manager_.GetAskPL(t_security_id_, 0);
    if (t_best_bid_level_ && t_best_ask_level_) {
      if (side_to_sanitize_ == HFSAT::kTradeTypeBuy &&
          t_best_bid_level_->limit_price_ > t_best_ask_level_->limit_price_ - NSE_V2_EPSILON) {
        dbglogger_ << " Second Sanitize called on Sell; Buy Sanitize Px " << t_px_to_sanitize_ << " Sell L1 "
                   << t_best_ask_level_->limit_price_ << '\n';
        SanitizeBook(t_security_id_, HFSAT::kTradeTypeSell, t_px_to_sanitize_ + NSE_V2_EPSILON);
      } else if (side_to_sanitize_ == HFSAT::kTradeTypeSell &&
                 t_best_ask_level_->limit_price_ < t_best_bid_level_->limit_price_ + NSE_V2_EPSILON) {
        dbglogger_ << " Second Sanitize called on Buy; Sell Sanitize Px " << t_px_to_sanitize_ << " Buy L1 "
                   << t_best_bid_level_->limit_price_ << '\n';
        SanitizeBook(t_security_id_, HFSAT::kTradeTypeBuy, t_px_to_sanitize_ - NSE_V2_EPSILON);
      }
    }
  }
}

// Sanitize on Sub-best Trade is ignored only in one specific subcase of using predictive logic and sub-best trade
// condition is
// holding for aggressor side eg orig book 1000 @ 141.05 --- 141.2 @ 2000
//                                          500 @ 141.0  --- 141.3 @ 1000
// Agg buy 2500@141.25 will cause incorrect sanitization of B 500@141.25 on trade
void IndexedNSEMarketViewManager2::SanitizeBookOnCrossedTrade(const uint32_t t_security_id_, double t_tradepx_) {
  // Called when underlying book is uncrossed. level corresponding to t_tradepx_ is not sanitized
  SecurityMarketView &smv_ = *market_view_ptr_;
  MarketUpdateInfoLevelStruct *t_best_bid_level_ = px_level_manager_.GetSynBidPL(t_security_id_, 0);
  MarketUpdateInfoLevelStruct *t_best_ask_level_ = px_level_manager_.GetSynAskPL(t_security_id_, 0);
  bool t_book_crossed_ = px_level_manager_.IsBookCrossed(t_security_id_);
  // sub best ask trade
  if ((!t_book_crossed_ || !(smv_.using_predictive_uncross_) || px_level_manager_.IsCrossingSideBid(t_security_id_)) &&
      t_best_ask_level_ != NULL && t_tradepx_ > t_best_ask_level_->limit_price_ + NSE_V2_EPSILON &&
      t_best_ask_level_->limit_int_price_ != kInvalidIntPrice) {
    dbglogger_ << " SanitizeBookOnCrossedTrade triggered at " << watch_.tv() << '\n';
    SanitizeBook(t_security_id_, HFSAT::kTradeTypeSell, t_tradepx_ - 2 * NSE_V2_EPSILON);
  } else if ((!t_book_crossed_ || !(smv_.using_predictive_uncross_) ||
              !(px_level_manager_.IsCrossingSideBid(t_security_id_))) &&
             t_best_bid_level_ != NULL && t_tradepx_ < t_best_bid_level_->limit_price_ - NSE_V2_EPSILON) {
    dbglogger_ << " SanitizeBookOnCrossedTrade triggered at " << watch_.tv() << '\n';
    SanitizeBook(t_security_id_, HFSAT::kTradeTypeBuy, t_tradepx_ + 2 * NSE_V2_EPSILON);
  }
}

// Deletes the old side if book has remained crossed for long enough. False positives will increase as
// NSE_V2_SANITIZE_TIME_THRESHOLD
// is decreased. Most false positives correspond to cases where there is a long delay between an Aggressive limit order
// add and subsequent
// trade. Ex SBIN_FUT_0 1496720702.542972
void IndexedNSEMarketViewManager2::SanitizeBook(const uint32_t t_security_id_, HFSAT::TradeType_t t_sanitize_side_,
                                                double t_sanitize_price_) {
  SecurityMarketView &smv_ = *market_view_ptr_;
  MarketUpdateInfoLevelStruct *t_best_bid_level_ = px_level_manager_.GetBidPL(t_security_id_, 0);
  MarketUpdateInfoLevelStruct *t_best_ask_level_ = px_level_manager_.GetAskPL(t_security_id_, 0);
  DBGLOG_TIME_CLASS_FUNC_LINE << " Book state before Sanitize: "
                              << (t_best_bid_level_ != NULL ? t_best_bid_level_->limit_size_ : kInvalidSize) << " @ "
                              << (t_best_bid_level_ != NULL ? t_best_bid_level_->limit_price_ : kInvalidPrice)
                              << " --- "
                              << (t_best_ask_level_ != NULL ? t_best_ask_level_->limit_price_ : kInvalidPrice) << " @ "
                              << (t_best_ask_level_ != NULL ? t_best_ask_level_->limit_size_ : kInvalidSize) << ' '
                              << smv_.shortcode() << DBGLOG_ENDL_FLUSH;

  int order_count_ = 0;
  int level_ = 0;

  if (t_sanitize_side_ == kTradeTypeBuy) {
    MarketUpdateInfoLevelStruct *t_bid_level_ = px_level_manager_.GetBidPL(t_security_id_, level_);
    while (t_bid_level_ != NULL && t_bid_level_->limit_price_ > t_sanitize_price_ - NSE_V2_EPSILON) {
      order_count_ += t_bid_level_->limit_ordercount_;
      level_++;
      t_bid_level_ = px_level_manager_.GetBidPL(t_security_id_, level_);
    }
  } else {
    MarketUpdateInfoLevelStruct *t_ask_level_ = px_level_manager_.GetAskPL(t_security_id_, level_);
    while (t_ask_level_ != NULL && t_ask_level_->limit_price_ < t_sanitize_price_ + NSE_V2_EPSILON) {
      order_count_ += t_ask_level_->limit_ordercount_;
      level_++;
      t_ask_level_ = px_level_manager_.GetAskPL(t_security_id_, level_);
    }
  }

  // Delete offending orders in maps and px_level_manager
  std::vector<OrderDetailsStruct *> &order_cache_ = order_history_instance.GetOrderCache(t_security_id_);
  ska::flat_hash_map<uint64_t, OrderDetailsStruct *> &order_history_ =
      order_history_instance.GetOrderMaps(t_security_id_);

  auto t_vectiter = order_cache_.begin();
  for (; t_vectiter != order_cache_.end();) {
    if (order_count_ <= 0) break;
    OrderDetailsStruct *t_live_order_ = *t_vectiter;
    if (t_live_order_ == NULL) {
      t_vectiter++;
      continue;
    }
    // live order contradicting sanitize price is deleted
    if ((t_live_order_->is_buy_order && t_sanitize_side_ == kTradeTypeBuy &&
         (t_live_order_->order_price > t_sanitize_price_ - NSE_V2_EPSILON)) ||
        (!t_live_order_->is_buy_order && t_sanitize_side_ == kTradeTypeSell &&
         (t_live_order_->order_price < t_sanitize_price_ + NSE_V2_EPSILON))) {
      order_count_--;
      t_vectiter++;
      px_level_manager_.ModifySizeAtPxLevel(
          t_security_id_, (t_live_order_->is_buy_order ? HFSAT::kTradeTypeBuy : HFSAT::kTradeTypeSell),
          smv_.GetIntPx(t_live_order_->order_price), -t_live_order_->order_size, true);
      //        dbglogger_ << "Sanitizing Orderid " << t_live_order_->order_id << ' ' << t_live_order_->order_size
      //		   << " @ " << t_live_order_->order_price << '\n';
      order_history_instance.DeleteOrderFromHistory(t_security_id_, t_live_order_->order_id);
    } else {
      t_vectiter++;
    }
  }

  auto t_mapiter_ = order_history_.begin();

  for (; t_mapiter_ != order_history_.end();) {
    if (order_count_ <= 0) break;
    OrderDetailsStruct *t_live_order_ = (*t_mapiter_).second;
    // live order contradicting bestprice is deleted
    if ((t_live_order_->is_buy_order && t_sanitize_side_ == kTradeTypeBuy &&
         (t_live_order_->order_price > t_sanitize_price_ - NSE_V2_EPSILON)) ||
        (!t_live_order_->is_buy_order && t_sanitize_side_ == kTradeTypeSell &&
         (t_live_order_->order_price < t_sanitize_price_ + NSE_V2_EPSILON))) {
      order_count_--;
      t_mapiter_++;
      px_level_manager_.ModifySizeAtPxLevel(
          t_security_id_, (t_live_order_->is_buy_order ? HFSAT::kTradeTypeBuy : HFSAT::kTradeTypeSell),
          smv_.GetIntPx(t_live_order_->order_price), -t_live_order_->order_size, true);
      //        dbglogger_ << "Sanitizing Orderid " << t_live_order_->order_id << ' ' << t_live_order_->order_size
      //	 	   << " @ " << t_live_order_->order_price << '\n';
      ska::flat_hash_map<uint64_t, OrderDetailsStruct *>::iterator iter =
          order_history_instance.DeleteOrderFromHistory(t_security_id_, t_live_order_->order_id);
      if (iter != order_history_.end()) t_mapiter_ = iter;
    } else {
      t_mapiter_++;
    }
  }

  t_best_bid_level_ = px_level_manager_.GetBidPL(t_security_id_, 0);
  t_best_ask_level_ = px_level_manager_.GetAskPL(t_security_id_, 0);
  SetSMVBestVars(t_security_id_);
  DBGLOG_TIME_CLASS_FUNC_LINE << "Book state after Sanitize: "
                              << (t_best_bid_level_ != NULL ? t_best_bid_level_->limit_size_ : kInvalidSize) << " @ "
                              << (t_best_bid_level_ != NULL ? t_best_bid_level_->limit_price_ : kInvalidPrice)
                              << " --- "
                              << (t_best_ask_level_ != NULL ? t_best_ask_level_->limit_price_ : kInvalidPrice) << " @ "
                              << (t_best_ask_level_ != NULL ? t_best_ask_level_->limit_size_ : kInvalidSize)
                              << DBGLOG_ENDL_FLUSH;
  //  CheckL1Consistency(t_security_id_);
}

// Checks and returns if current time is trading time for the product
bool IndexedNSEMarketViewManager2::CheckValidTime(int sec_id) {
  return trade_time_manager_.isValidTimeToTrade(sec_id, watch_.tv().tv_sec % 86400);
}

// add entry to self_order_history_instance and delete order from MD feed if already reflected
void IndexedNSEMarketViewManager2::OrderConfirmed(
    const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t r_buysell_, const int _size_remaining_, const int _size_executed_, const int _client_position_,
    const int _global_position_, const int r_int_price_, const int32_t server_assigned_message_sequence,
    const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  if (use_self_book_) {
    // we don't want to filter orders from other strategies if self_trader_ids is populated
    if (!self_trader_ids_.empty() && self_trader_ids_.find(t_server_assigned_client_id_) == self_trader_ids_.end()) {
      return;
    }
    self_order_history_instance.AddOrderToOrderHistory(_security_id_, r_buysell_, _price_, _size_remaining_,
                                                       exchange_order_id);
    // if order is reflected in the mkt book delete it from the book
    if (order_history_instance.IsOrderSeen(_security_id_, exchange_order_id)) {
      OnOrderDelete(_security_id_, r_buysell_, exchange_order_id, 0, true, false);
    }
  }
}

// delete entry from self_order_history_instance
void IndexedNSEMarketViewManager2::OrderCanceled(const int t_server_assigned_client_id_,
                                                 const int _client_assigned_order_sequence_,
                                                 const int _server_assigned_order_sequence_,
                                                 const unsigned int _security_id_, const double _price_,
                                                 const TradeType_t r_buysell_, const int _size_remaining_,
                                                 const int _client_position_, const int _global_position_,
                                                 const int r_int_price_, const int32_t server_assigned_message_sequence,
                                                 const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  if (use_self_book_) {
    if (self_order_history_instance.IsOrderSeen(_security_id_, exchange_order_id)) {
      self_order_history_instance.DeleteOrderFromHistory(_security_id_, exchange_order_id);
    }
  }
}

void IndexedNSEMarketViewManager2::OrderExecuted(
    const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t r_buysell_, const int _size_remaining_, const int _size_executed_, const int _client_position_,
    const int _global_position_, const int r_int_price_, const int32_t server_assigned_message_sequence,
    const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  if (use_self_book_) {
    if (self_order_history_instance.IsOrderSeen(_security_id_, exchange_order_id) && _size_remaining_ == 0) {
      self_order_history_instance.DeleteOrderFromHistory(_security_id_, exchange_order_id);
    }
  }
}
}
