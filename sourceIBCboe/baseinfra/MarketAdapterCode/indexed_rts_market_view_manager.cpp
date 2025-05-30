/**
    \file MarketAdapterCode/indexed_rts_market_view_manager.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#include "baseinfra/MarketAdapter/indexed_rts_market_view_manager.hpp"
#define CCPROFILING_TRADEINIT 0

namespace HFSAT {

#define LOW_ACCESS_INDEX 50
#define CCPROFILING_TRADEINIT 0

IndexedRtsMarketViewManager::IndexedRtsMarketViewManager(
    DebugLogger& dbglogger, const Watch& watch, const SecurityNameIndexer& sec_name_indexer,
    const std::vector<SecurityMarketView*>& security_market_view_map)
    : BaseMarketViewManager(dbglogger, watch, sec_name_indexer, security_market_view_map),
      sec_id_to_prev_update_was_quote_(sec_name_indexer.NumSecurityId(), false),
      last_trade_aggressor_side_(sec_name_indexer.NumSecurityId(), kTradeTypeNoInfo),
      last_trade_int_price_(sec_name_indexer.NumSecurityId(), 0),
      last_l1_bid_del_int_price_(sec_name_indexer.NumSecurityId(), 0),
      last_l1_ask_del_int_price_(sec_name_indexer.NumSecurityId(), 0) {}

void IndexedRtsMarketViewManager::OnPriceLevelNew(const unsigned int sec_id, const TradeType_t buysell,
                                                  const int level_added, const double price, const int new_size,
                                                  const int new_ordercount, const bool intermediate_message) {
  SecurityMarketView& smv_ = *security_market_view_map_[sec_id];

  int int_price_ = smv_.GetIntPx(price);

  if (!smv_.initial_book_constructed_) {
    BuildIndex(sec_id, buysell, int_price_);
  }

  int bid_index_ = 0;
  int ask_index_ = 0;

  int old_size_ = 0;
  int old_order_count_ = 0;

  switch (buysell) {
    case kTradeTypeBuy: {
      bid_index_ = smv_.GetBidIndex(int_price_);

      // There are 0 levels on the bid side => first level will not have any size
      if (smv_.GetL1BidSize() <= 0) {
        if (bid_index_ < LOW_ACCESS_INDEX) {
          RebuildIndexLowAccess(sec_id, buysell, int_price_);
          bid_index_ = smv_.base_bid_index_;

        } else if (bid_index_ >= (int)smv_.max_tick_range_) {
          RebuildIndexHighAccess(sec_id, buysell, int_price_);
          bid_index_ = smv_.base_bid_index_;

        } else {
          // bid_index_ falls within the range. So, set base_bid_index_ as bid_index_
          smv_.base_bid_index_ = bid_index_;
        }

        smv_.l1_price_changed_ = true;

      } else if (bid_index_ < 0) {
        // Skip the message if we don't have any info for this level
        return;
      }

      if (bid_index_ >= (int)smv_.max_tick_range_) {
        RebuildIndexHighAccess(sec_id, buysell, int_price_);
        bid_index_ = smv_.base_bid_index_;

        smv_.l1_price_changed_ = true;
      }

      // Check if levels are in sync
      if (level_added == 1 && smv_.GetL1BidIntPrice() > int_price_) {
        // Reset levels above current l1
        int index = smv_.base_bid_index_;
        for (; index > bid_index_ && index >= 0; index--) {
          smv_.ResetBidLevel(index);
        }

        if (index < 0) {
          smv_.is_ready_ = false;
          DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " Bid side empty after l1 sanitization "
                                      << DBGLOG_ENDL_FLUSH;
          DBGLOG_DUMP;
          return;
        }

        // Set new base_bid_index
        smv_.base_bid_index_ = bid_index_;

        // Check if we need to re-centre the base_ask_index_
        if (smv_.base_bid_index_ < LOW_ACCESS_INDEX) {
          RebuildIndexLowAccess(sec_id, kTradeTypeBuy, int_price_);
        }
      }

      old_size_ = smv_.GetBidSize(bid_index_);
      old_order_count_ = smv_.GetBidOrders(bid_index_);

      // due to legacy L1/L2 duplication - avoid propagating if information is unchanged
      if (old_size_ == new_size) {
        break;
      }

      // Update the size and order count
      smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ = new_size;
      smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ = new_ordercount;

      if (bid_index_ > (int)smv_.base_bid_index_) {
        smv_.base_bid_index_ = bid_index_;
        smv_.l1_price_changed_ = true;
      } else if (bid_index_ == (int)smv_.base_bid_index_) {
        smv_.l1_size_changed_ = true;
      } else {
        smv_.l2_changed_since_last_ = true;
      }

      // Sanitise ASK side
      if (int_price_ >= smv_.GetL1AskIntPrice()) {
        if (smv_.GetL1AskSize() <= 0) {
          // We should not do sanitization, if the other side is not ready
          smv_.is_ready_ = false;
          break;
        }

        int index_ = smv_.base_ask_index_;
        for (; index_ >= 0; index_--) {
          if (smv_.market_update_info_.asklevels_[index_].limit_int_price_ > int_price_ &&
              smv_.market_update_info_.asklevels_[index_].limit_size_ > 0) {
            smv_.base_ask_index_ = index_;
            break;
          }
          smv_.market_update_info_.asklevels_[index_].limit_size_ = 0;
          smv_.market_update_info_.asklevels_[index_].limit_ordercount_ = 0;
        }

        if (index_ < 0) {
          smv_.is_ready_ = false;
          DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " Ask side empty after sanitization "
                                      << DBGLOG_ENDL_FLUSH;
          DBGLOG_DUMP;
          return;
        }

        // Check if we need to re-centre the base_ask_index_
        if (smv_.base_ask_index_ < LOW_ACCESS_INDEX) {
          RebuildIndexLowAccess(sec_id, kTradeTypeSell,
                                smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_);
        }

        // Update best variables
        UpdateBestAskVariables(sec_id, smv_.base_ask_index_);

        smv_.l1_price_changed_ = true;
      }

      if (smv_.l1_price_changed_ || smv_.l1_size_changed_) {
        UpdateBestBidVariables(sec_id, smv_.base_bid_index_);
      }
      break;
    }

    case kTradeTypeSell: {
      ask_index_ = smv_.GetAskIndex(int_price_);

      // There are 0 levels on the ask side
      if (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ <= 0) {
        if (ask_index_ < LOW_ACCESS_INDEX) {
          RebuildIndexLowAccess(sec_id, buysell, int_price_);
          ask_index_ = smv_.base_ask_index_;
        } else if (ask_index_ >= (int)smv_.max_tick_range_) {
          RebuildIndexHighAccess(sec_id, buysell, int_price_);
          ask_index_ = smv_.base_ask_index_;
        } else {
          // ask_index_ falls within the range. So, set base_ask_index_ as ask_index_
          smv_.base_ask_index_ = ask_index_;
        }

        smv_.l1_price_changed_ = true;
      } else if (ask_index_ < 0) {
        // Skip the message if we don't have info for this level
        return;
      }

      if (ask_index_ >= (int)smv_.max_tick_range_) {
        RebuildIndexHighAccess(sec_id, buysell, int_price_);
        ask_index_ = smv_.base_ask_index_;

        smv_.l1_price_changed_ = true;
      }

      // Check if levels are in sync
      if (level_added == 1 && smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ < int_price_) {
        // Reset levels above current l1
        int index = smv_.base_ask_index_;
        for (; index > ask_index_ && index >= 0; index--) {
          smv_.market_update_info_.asklevels_[index].limit_size_ = 0;
          smv_.market_update_info_.asklevels_[index].limit_ordercount_ = 0;
        }

        if (index < 0) {
          smv_.is_ready_ = false;
          DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " Ask side empty after l1 sanitization "
                                      << DBGLOG_ENDL_FLUSH;
          DBGLOG_DUMP;
          return;
        }

        // Update base_ask_index
        smv_.base_ask_index_ = ask_index_;

        // Check if we need to re-centre the base_ask_index_
        if (smv_.base_ask_index_ < LOW_ACCESS_INDEX) {
          RebuildIndexLowAccess(sec_id, kTradeTypeSell, int_price_);
        }
      }

      old_size_ = smv_.market_update_info_.asklevels_[ask_index_].limit_size_;
      old_order_count_ = smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_;

      // due to legacy L1/L2 duplication - avoid propagating if information is unchanged
      if (old_size_ == new_size) {
        break;
      }

      // Update the size and order count at corresponding level
      smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ = new_ordercount;
      smv_.market_update_info_.asklevels_[ask_index_].limit_size_ = new_size;

      if (ask_index_ > (int)smv_.base_ask_index_) {
        smv_.base_ask_index_ = ask_index_;
        smv_.l1_price_changed_ = true;
      } else if (ask_index_ == (int)smv_.base_ask_index_) {
        smv_.l1_size_changed_ = true;
      } else {
        smv_.l2_changed_since_last_ = true;
      }

      // Sanitise BID side
      if (int_price_ <= smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_) {
        if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ <= 0) {
          // We should not do sanitization, if the other side is not ready
          smv_.is_ready_ = false;
          break;
        }

        int index_ = smv_.base_bid_index_;
        for (; index_ >= 0; index_--) {
          if (smv_.market_update_info_.bidlevels_[index_].limit_int_price_ < int_price_ &&
              smv_.market_update_info_.bidlevels_[index_].limit_size_ > 0) {
            smv_.base_bid_index_ = index_;
            break;
          }
          smv_.market_update_info_.bidlevels_[index_].limit_size_ = 0;
          smv_.market_update_info_.bidlevels_[index_].limit_ordercount_ = 0;
        }

        if (index_ < 0) {
          smv_.is_ready_ = false;
          DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " Bid side empty after sanitization "
                                      << DBGLOG_ENDL_FLUSH;
          DBGLOG_DUMP;
          return;
        }

        // Check if we need to re-centre the base_bid_index_
        if (smv_.base_bid_index_ < LOW_ACCESS_INDEX) {
          RebuildIndexLowAccess(sec_id, kTradeTypeBuy,
                                smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_);
        }

        // Update best variables
        UpdateBestBidVariables(sec_id, smv_.base_bid_index_);

        smv_.l1_price_changed_ = true;
      }

      if (smv_.l1_price_changed_ || smv_.l1_size_changed_) {
        UpdateBestAskVariables(sec_id, smv_.base_ask_index_);
      }
    } break;
    default:
      break;
  }

  sec_id_to_prev_update_was_quote_[sec_id] = true;

  if (smv_.l1_price_changed_ || smv_.l1_size_changed_) {
    // Changing it here because we update prices in both bid/ask update
    UpdateBestBidVariablesUsingOurOrders(sec_id);
    UpdateBestAskVariablesUsingOurOrders(sec_id);
    smv_.UpdateL1Prices();
  }

  if (!smv_.is_ready_) {
    if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ > 0 &&
        smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ > 0 && smv_.ArePricesComputed()) {
      smv_.is_ready_ = true;
    }
  }

  if (!smv_.is_ready_ || intermediate_message) {
    return;
  }

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(9);
#endif

  if (smv_.pl_change_listeners_present_) {
    if (buysell == kTradeTypeBuy && (int)smv_.base_bid_index_ >= bid_index_ &&
        smv_.base_bid_index_ - bid_index_ <= 10) {
      /// check if level = int_price_level should be fixed or left as is
      int level_ = smv_.base_bid_index_ - bid_index_;
      int int_price_level_ = smv_.base_bid_index_ - bid_index_;
      smv_.NotifyOnPLChangeListeners(sec_id, smv_.market_update_info_, buysell, level_, int_price_, int_price_level_,
                                     old_size_, smv_.market_update_info_.bidlevels_[bid_index_].limit_size_,
                                     old_order_count_, new_ordercount, intermediate_message,
                                     old_size_ == 0 ? 'N' : 'C');
    } else if (buysell == kTradeTypeSell && (int)smv_.base_ask_index_ >= ask_index_ &&
               smv_.base_ask_index_ - ask_index_ <= 10) {
      int level_ = smv_.base_ask_index_ - ask_index_;
      int int_price_level_ = smv_.base_ask_index_ - ask_index_;
      smv_.NotifyOnPLChangeListeners(sec_id, smv_.market_update_info_, buysell, level_, int_price_, int_price_level_,
                                     old_size_, smv_.market_update_info_.asklevels_[ask_index_].limit_size_,
                                     old_order_count_, new_ordercount, intermediate_message,
                                     old_size_ == 0 ? 'N' : 'C');
    }
  }

  if (smv_.l1_price_changed_) {
    smv_.NotifyL1PriceListeners();

    smv_.l1_price_changed_ = false;
    smv_.l1_size_changed_ = false;
  } else if (smv_.l1_size_changed_) {
    smv_.NotifyL1SizeListeners();

    smv_.l1_size_changed_ = false;
  } else {
    smv_.NotifyL2Listeners();
  }

  if (smv_.l2_changed_since_last_) {
    smv_.NotifyL2OnlyListeners();

    smv_.l2_changed_since_last_ = false;
  }

  smv_.NotifyOnReadyListeners();
}

void IndexedRtsMarketViewManager::OnPriceLevelDelete(const unsigned int security_id, const TradeType_t buysell,
                                                     const int level_removed, const double price,
                                                     const bool intermediate) {
  SecurityMarketView& smv_ = *security_market_view_map_[security_id];

  int int_price = smv_.GetIntPx(price);

  if (!smv_.initial_book_constructed_) {
    BuildIndex(security_id, buysell, int_price);
  }

  int old_size = 0;
  int old_order_count = 0;

  switch (buysell) {
    case kTradeTypeBuy: {
      bool prev_l1_changed = smv_.l1_price_changed_;
      old_size = smv_.bid_size_at_int_price(int_price);
      old_order_count = smv_.bid_order_at_int_price(int_price);

      bool success = smv_.DeleteBid(int_price);

      if (success && !prev_l1_changed && smv_.l1_price_changed_) {
        last_l1_bid_del_int_price_[security_id] = int_price;
      }
      break;
    }

    case kTradeTypeSell: {
      bool prev_l1_changed = smv_.l1_price_changed_;
      old_size = smv_.ask_size_at_int_price(int_price);
      old_order_count = smv_.ask_order_at_int_price(int_price);

      bool success = smv_.DeleteAsk(int_price);

      if (success && !prev_l1_changed && smv_.l1_price_changed_) {
        last_l1_ask_del_int_price_[security_id] = int_price;
      }
      break;
    }
    default:
      break;
  }

  sec_id_to_prev_update_was_quote_[security_id] = true;

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(9);
#endif

  smv_.NotifyDeleteListeners(buysell, int_price, level_removed, old_size, old_order_count, intermediate);
}

TradeType_t IndexedRtsMarketViewManager::GetAggressorSide(int security_id, int int_price, TradeType_t orig_buysell) {
  SecurityMarketView& smv_ = *security_market_view_map_[security_id];

  if (orig_buysell == kTradeTypeNoInfo) {
    // Determine side: Simple heuristic
    if (int_price <= smv_.GetL1BidIntPrice()) {
      return kTradeTypeSell;
    } else if (int_price >= smv_.GetL1AskIntPrice()) {
      return kTradeTypeBuy;
    }
    // Another heuristic: // In case of successive trade messages,
    else if (!sec_id_to_prev_update_was_quote_[security_id] &&
             last_trade_aggressor_side_[security_id] == kTradeTypeSell &&
             int_price <= last_trade_int_price_[security_id]) {
      return kTradeTypeSell;
    } else if (!sec_id_to_prev_update_was_quote_[security_id] &&
               last_trade_aggressor_side_[security_id] == kTradeTypeBuy &&
               int_price >= last_trade_int_price_[security_id]) {
      return kTradeTypeBuy;
    } else {
      if (last_l1_ask_del_int_price_[security_id] > last_l1_bid_del_int_price_[security_id]) {
        if (int_price >= last_l1_ask_del_int_price_[security_id]) {
          return kTradeTypeBuy;
        } else if (int_price <= last_l1_bid_del_int_price_[security_id]) {
          return kTradeTypeSell;
        } else {
          double last_mid_price = (smv_.GetL1BidIntPrice() + smv_.GetL1AskIntPrice()) / 2.0;
          if (last_mid_price < (double)int_price) {
            return kTradeTypeBuy;
          } else if (last_mid_price > (double)int_price) {
            return kTradeTypeSell;
          }
        }
      }
    }
  }

  return orig_buysell;
}

void IndexedRtsMarketViewManager::OnTrade(const unsigned int t_security_id_, const double t_trade_price_,
                                          const int t_trade_size_, const TradeType_t t_buysell_) {
  SecurityMarketView& smv_ = *security_market_view_map_[t_security_id_];

  int t_trade_int_price_ = smv_.GetIntPx(t_trade_price_);

  // not till book is ready
  if ((smv_.market_update_info_.bestbid_int_price_ <= kInvalidIntPrice) ||
      (smv_.market_update_info_.bestask_int_price_ <= kInvalidIntPrice)) {
    return;
  }

  TradeType_t buysell_ = GetAggressorSide(t_security_id_, t_trade_int_price_, t_buysell_);

  last_trade_int_price_[t_security_id_] = t_trade_int_price_;
  last_trade_aggressor_side_[t_security_id_] = buysell_;

  // same as SecurityMarketView::OnTrade - not particularly relevant but maintaining
  // for consistency
  if (smv_.trade_print_info_.computing_last_book_tdiff_) {
    if (sec_id_to_prev_update_was_quote_[t_security_id_]) {
      // the last time the update was a book message
      smv_.market_update_info_.last_book_mkt_size_weighted_price_ =
          smv_.market_update_info_
              .mkt_size_weighted_price_;  // noting the mktpx as it was justbefore the first trade message
    }
  }

  smv_.StorePreTrade();

  // Sanitize the book based on trade price
  switch (buysell_) {
    case kTradeTypeBuy:  // Aggressive sell
    {
      int trade_ask_index_ =
          smv_.base_ask_index_ +
          (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - t_trade_int_price_);

      if (trade_ask_index_ > (int)smv_.base_ask_index_ || trade_ask_index_ < 0) {
        break;
      }

      // If the last update was trade, handle this trade differently
      if (!sec_id_to_prev_update_was_quote_[t_security_id_]) {
        if (t_trade_int_price_ <= smv_.market_update_info_.bestask_int_price_) {
          if (t_trade_int_price_ == smv_.market_update_info_.bestask_int_price_) {
            // Update best variables using already set best variables
            if (t_trade_size_ >= smv_.market_update_info_.bestask_size_) {
              int next_ask_index_ = trade_ask_index_ - 1;
              for (; next_ask_index_ >= 0 && smv_.market_update_info_.asklevels_[next_ask_index_].limit_size_ <= 0;
                   next_ask_index_--)
                ;

              if (next_ask_index_ < 0) {
                smv_.market_update_info_.bestask_int_price_ = t_trade_int_price_;
                smv_.market_update_info_.bestask_ordercount_ = 1;
                smv_.market_update_info_.bestask_price_ = smv_.GetDoublePx(t_trade_int_price_);
                smv_.market_update_info_.bestask_size_ = 1;
                break;
              }

              smv_.UpdateBestAsk(next_ask_index_);
            } else {
              smv_.market_update_info_.bestask_size_ -= t_trade_size_;
            }
          }
        } else {
          smv_.market_update_info_.bestask_int_price_ = t_trade_int_price_;
          smv_.market_update_info_.bestask_ordercount_ =
              std::max(smv_.market_update_info_.asklevels_[trade_ask_index_].limit_ordercount_, 1);
          smv_.market_update_info_.bestask_price_ = smv_.GetDoublePx(t_trade_int_price_);
          smv_.market_update_info_.bestask_size_ =
              std::max(smv_.market_update_info_.asklevels_[trade_ask_index_].limit_size_ - t_trade_size_, 1);
        }

        break;
      }

      // At this point, trade_ask_index_ == smv_.base_ask_index_ has to hold.
      int iter_ask_index_ =
          std::max(trade_ask_index_,
                   (int)smv_.base_ask_index_);  // Don't want to set base_bid_index_, just updating best variables
      {
        if (t_trade_size_ >= smv_.market_update_info_.asklevels_[iter_ask_index_].limit_size_) {
          int next_ask_index_ = iter_ask_index_ - 1;
          for (; next_ask_index_ >= 0 && smv_.market_update_info_.asklevels_[next_ask_index_].limit_size_ <= 0;
               next_ask_index_--)
            ;

          if (next_ask_index_ < 0) {
            return;
          }

          smv_.UpdateBestAsk(next_ask_index_);
        } else {
          smv_.market_update_info_.bestask_int_price_ =
              smv_.market_update_info_.asklevels_[iter_ask_index_].limit_int_price_;
          smv_.market_update_info_.bestask_ordercount_ =
              smv_.market_update_info_.asklevels_[iter_ask_index_].limit_ordercount_;
          smv_.market_update_info_.bestask_price_ = smv_.market_update_info_.asklevels_[iter_ask_index_].limit_price_;
          smv_.market_update_info_.bestask_size_ =
              smv_.market_update_info_.asklevels_[iter_ask_index_].limit_size_ - t_trade_size_;
        }
      }
      UpdateBestAskVariablesUsingOurOrders(t_security_id_);
      smv_.UpdateL1Prices();
    } break;
    case kTradeTypeSell:  // Aggressive buy
    {
      int trade_bid_index_ =
          smv_.base_bid_index_ -
          (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - t_trade_int_price_);

      if (trade_bid_index_ > (int)smv_.base_bid_index_ || trade_bid_index_ < 0) {
        break;
      }

      // If the last update was trade, handle this trade differently
      if (!sec_id_to_prev_update_was_quote_[t_security_id_]) {
        if (t_trade_int_price_ >= smv_.market_update_info_.bestbid_int_price_) {
          if (t_trade_int_price_ == smv_.market_update_info_.bestbid_int_price_) {
            // Update best variables using already set best variables
            if (t_trade_size_ >= smv_.market_update_info_.bestbid_size_) {
              int next_bid_index_ = trade_bid_index_ - 1;
              for (; next_bid_index_ >= 0 && smv_.market_update_info_.bidlevels_[next_bid_index_].limit_size_ <= 0;
                   next_bid_index_--)
                ;

              if (next_bid_index_ < 0) {
                smv_.market_update_info_.bestbid_int_price_ = t_trade_int_price_;
                smv_.market_update_info_.bestbid_ordercount_ = 1;
                smv_.market_update_info_.bestbid_price_ = smv_.GetDoublePx(t_trade_int_price_);
                smv_.market_update_info_.bestbid_size_ = 1;
                break;
              }

              smv_.UpdateBestBid(next_bid_index_);
            } else {
              smv_.market_update_info_.bestbid_size_ -= t_trade_size_;
            }
          }
        } else {
          smv_.market_update_info_.bestbid_int_price_ = t_trade_int_price_;
          smv_.market_update_info_.bestbid_ordercount_ =
              std::max(smv_.market_update_info_.bidlevels_[trade_bid_index_].limit_ordercount_, 1);
          smv_.market_update_info_.bestbid_price_ = smv_.GetDoublePx(t_trade_int_price_);
          smv_.market_update_info_.bestbid_size_ =
              std::max(smv_.market_update_info_.bidlevels_[trade_bid_index_].limit_size_ - t_trade_size_, 1);
        }
        break;
      }

      // At this point, trade_bid_index_ == smv_.base_bid_index_ has to hold.
      int iter_bid_index_ =
          std::min(trade_bid_index_,
                   (int)smv_.base_bid_index_);  // Don't want to set base_bid_index_, just updating best variables
      {
        if (t_trade_size_ >= smv_.market_update_info_.bidlevels_[iter_bid_index_].limit_size_) {
          int next_bid_index_ = iter_bid_index_ - 1;
          for (; next_bid_index_ >= 0 && smv_.market_update_info_.bidlevels_[next_bid_index_].limit_size_ <= 0;
               next_bid_index_--)
            ;

          if (next_bid_index_ < 0) {
            return;
          }

          smv_.UpdateBestBid(next_bid_index_);
        } else {
          smv_.market_update_info_.bestbid_int_price_ =
              smv_.market_update_info_.bidlevels_[iter_bid_index_].limit_int_price_;
          smv_.market_update_info_.bestbid_ordercount_ =
              smv_.market_update_info_.bidlevels_[iter_bid_index_].limit_ordercount_;
          smv_.market_update_info_.bestbid_price_ = smv_.market_update_info_.bidlevels_[iter_bid_index_].limit_price_;
          smv_.market_update_info_.bestbid_size_ =
              smv_.market_update_info_.bidlevels_[iter_bid_index_].limit_size_ - t_trade_size_;
        }
      }
      UpdateBestBidVariablesUsingOurOrders(t_security_id_);
      smv_.UpdateL1Prices();
    } break;
    default:
      break;
  }

  // set the primary variables
  smv_.trade_print_info_.trade_price_ = t_trade_price_;
  smv_.trade_print_info_.size_traded_ = t_trade_size_;
  smv_.trade_print_info_.int_trade_price_ = t_trade_int_price_;
  smv_.trade_print_info_.buysell_ = buysell_;

  smv_.trade_print_info_.num_trades_++;
  /// semantics should differ for trade vs quote ordering - TODO
  smv_.SetTradeVarsForIndicatorsIfRequired();

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(9);
#endif

  // Same as SecurityMarketView::OnTrade
  if (smv_.is_ready_) {
    smv_.NotifyTradeListeners();
    smv_.NotifyOnReadyListeners();
  }

  sec_id_to_prev_update_was_quote_[t_security_id_] = false;
}
}
