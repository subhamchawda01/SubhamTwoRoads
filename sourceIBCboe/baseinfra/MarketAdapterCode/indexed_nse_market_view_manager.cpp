/**
    \file MarketAdapter/indexed_eobi_market_view_manager.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#include "baseinfra/MarketAdapter/indexed_nse_market_view_manager.hpp"

#include "dvccode/Profiler/cpucycle_profiler.hpp"

int current_notification = 1;

namespace HFSAT {

//#define INDEXED_BOOK_SIZE 1023
//#define INITIAL_BASE_INDEX 511
#define HIGH_ACCESS_INDEX 1023
#define LOW_ACCESS_INDEX 50
#define HISTORY_BUFF_LEN 5
// If there are these many sub-best trades against the same best price, and
// across atleast TIME_THRESHOLD we sanitize the book
#define NSE_SANITIZE_COUNT_THRESHOLD 2
#define NSE_SANITIZE_TIME_THRESHOLD 2000  // 2 seconds
// File listing days where there have been dropped packets in logged data.
// To avoid redundant emails
#define NSE_CROSSED_DATA_DAYS_FILE "NSE_Files/crossed_data_days.txt"
//#define IS_PROFILING_ENABLED 0
#define NSE_SANITIZE_ON_SUB_BEST_TRADE 1

IndexedNSEMarketViewManager::IndexedNSEMarketViewManager(
    DebugLogger &t_dbglogger_, const Watch &t_watch_, const SecurityNameIndexer &t_sec_name_indexer_,
    const std::vector<SecurityMarketView *> &t_security_market_view_map_)
    : BaseMarketViewManager(t_dbglogger_, t_watch_, t_sec_name_indexer_, t_security_market_view_map_),
      l1_price_changed_(t_sec_name_indexer_.NumSecurityId(), false),
      l1_size_changed_(t_sec_name_indexer_.NumSecurityId(), false),
      sid_to_side_to_be_sanitized_(t_sec_name_indexer_.NumSecurityId(), kTradeTypeNoInfo),
      sid_to_sanitize_count_(t_sec_name_indexer_.NumSecurityId(), 0),
      sid_to_int_px_to_sanitize_(t_sec_name_indexer_.NumSecurityId(), -1),
      sid_to_msecs_at_first_cross_(t_sec_name_indexer_.NumSecurityId(), 0),
      sid_to_size_at_sanitize_px_(t_sec_name_indexer_.NumSecurityId(), 0),
      trade_time_manager_(TradeTimeManager::GetUniqueInstance(t_sec_name_indexer_, t_watch_.YYYYMMDD())),
      currently_trading_(t_sec_name_indexer_.NumSecurityId(), false),
      order_history_instance(t_sec_name_indexer_.NumSecurityId()) {}

bool IndexedNSEMarketViewManager::IsBookCrossed() {
  SecurityMarketView &smv_ = *market_view_ptr_;

  if (smv_.base_bid_index_ > 0 && smv_.base_ask_index_ > 0) {
    if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ >=
            smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ ||
        smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ == kInvalidIntPrice ||
        smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ == kInvalidIntPrice)
      return true;
  }
  return false;
}

// Sets bestbid and ask variables if they are changed from syn variables.
void IndexedNSEMarketViewManager::CheckIfL1Changed(const int t_security_id_) {
  SecurityMarketView &smv_ = *market_view_ptr_;

  if (smv_.market_update_info_.bestbid_int_price_ != smv_.syn_bestbid_int_price_) {
    l1_price_changed_[t_security_id_] = true;
    l1_size_changed_[t_security_id_] = true;
    smv_.ComputeBestVarsFromSyntheticVars(kTradeTypeBuy);
  } else if (smv_.market_update_info_.bestbid_size_ != smv_.syn_bestbid_size_) {
    l1_size_changed_[t_security_id_] = true;
    smv_.ComputeBestVarsFromSyntheticVars(kTradeTypeBuy);
  }

  if (smv_.market_update_info_.bestask_int_price_ != smv_.syn_bestask_int_price_) {
    l1_price_changed_[t_security_id_] = true;
    l1_size_changed_[t_security_id_] = true;
    smv_.ComputeBestVarsFromSyntheticVars(kTradeTypeSell);
  } else if (smv_.market_update_info_.bestask_size_ != smv_.syn_bestask_size_) {
    l1_size_changed_[t_security_id_] = true;
    smv_.ComputeBestVarsFromSyntheticVars(kTradeTypeSell);
  }
}

// Helper function to simply get the next valid level index whose size > 0. Returns LOW_ACCESS_INDEX if it cannot find
// one.
uint32_t IndexedNSEMarketViewManager::GetNextIndex(uint32_t base_index_, TradeType_t t_buysell_) {
  SecurityMarketView &smv_ = *market_view_ptr_;

  std::vector<MarketUpdateInfoLevelStruct> &base_levels_ =
      (t_buysell_ == kTradeTypeSell) ? smv_.market_update_info_.asklevels_ : smv_.market_update_info_.bidlevels_;

  int index_ = base_index_ - 1;
  for (; index_ >= LOW_ACCESS_INDEX && base_levels_[index_].limit_size_ <= 0; index_--)
    ;
  return index_;
}

// Get an uncrossed view of the underlying book. Sets the uncrossed variables as synthetic variables
bool IndexedNSEMarketViewManager::GetUncrossedSynVars(const int t_security_id_, const bool t_is_intermediate_) {
  SecurityMarketView &smv_ = *market_view_ptr_;

  if (t_is_intermediate_) return false;

  if (!IsBookCrossed()) {
    smv_.ComputeSyntheticVarsFromBaseVars(smv_.base_bid_index_, kTradeTypeBuy);
    smv_.ComputeSyntheticVarsFromBaseVars(smv_.base_ask_index_, kTradeTypeSell);

    // Can be optimized to be called only at the time of actual uncrossing. TODO: Praful
    sid_to_msecs_at_first_cross_[t_security_id_] = watch_.msecs_from_midnight();
    CheckIfL1Changed(t_security_id_);
    return false;
  }

  // Book is crossed. Check for potential sanitization if it is still crossed for more than NSE_SANITIZATION_THRESHOLD
  // time
  if (watch_.msecs_from_midnight() - sid_to_msecs_at_first_cross_[t_security_id_] > NSE_SANITIZE_TIME_THRESHOLD) {
    // Book should not be crossed for such a large time. Sanitization is required.
    int next_index_ = 0;
    int sanitize_int_price_ = 0;
    if (sid_to_side_to_be_sanitized_[t_security_id_] == kTradeTypeBuy) {
      next_index_ = GetNextIndex(smv_.base_bid_index_, kTradeTypeBuy);
      sanitize_int_price_ = smv_.market_update_info_.bidlevels_[next_index_].limit_int_price_;
    } else {
      next_index_ = GetNextIndex(smv_.base_ask_index_, kTradeTypeSell);
      sanitize_int_price_ = smv_.market_update_info_.asklevels_[next_index_].limit_int_price_;
    }

    SanitizeBook(t_security_id_, sanitize_int_price_);
    sid_to_int_px_to_sanitize_[t_security_id_] = -1;
    sid_to_sanitize_count_[t_security_id_] = 0;
    sid_to_msecs_at_first_cross_[t_security_id_] = watch_.msecs_from_midnight();
  }

  smv_.ComputeSyntheticVarsFromBaseVars(smv_.base_bid_index_, kTradeTypeBuy);
  smv_.ComputeSyntheticVarsFromBaseVars(smv_.base_ask_index_, kTradeTypeSell);

  // Get a correct view from the base indices.
  int base_bid_index_ = smv_.base_bid_index_;
  int base_ask_index_ = smv_.base_ask_index_;

  int buy_size = smv_.market_update_info_.bidlevels_[base_bid_index_].limit_size_;
  int sell_size = smv_.market_update_info_.asklevels_[base_ask_index_].limit_size_;

  while (base_bid_index_ > LOW_ACCESS_INDEX && base_ask_index_ > LOW_ACCESS_INDEX &&
         smv_.market_update_info_.bidlevels_[base_bid_index_].limit_int_price_ >=
             smv_.market_update_info_.asklevels_[base_ask_index_].limit_int_price_) {
    if (buy_size > sell_size) {
      base_ask_index_ = GetNextIndex(base_ask_index_, kTradeTypeSell);
      buy_size -= sell_size;
      sell_size = smv_.market_update_info_.asklevels_[base_ask_index_].limit_size_;
    } else if (buy_size < sell_size) {
      base_bid_index_ = GetNextIndex(base_bid_index_, kTradeTypeBuy);
      sell_size -= buy_size;
      buy_size = smv_.market_update_info_.bidlevels_[base_bid_index_].limit_size_;
    } else {
      base_bid_index_ = GetNextIndex(base_bid_index_, kTradeTypeBuy);
      base_ask_index_ = GetNextIndex(base_ask_index_, kTradeTypeSell);
      sell_size = smv_.market_update_info_.asklevels_[base_ask_index_].limit_size_;
      buy_size = smv_.market_update_info_.bidlevels_[base_bid_index_].limit_size_;
    }
  }

  if (base_bid_index_ <= LOW_ACCESS_INDEX) {
    smv_.SetSynVarsAsInvalid(kTradeTypeBuy);
    CheckIfL1Changed(t_security_id_);
    return true;
  }

  if (base_ask_index_ <= LOW_ACCESS_INDEX) {
    smv_.SetSynVarsAsInvalid(kTradeTypeSell);
    CheckIfL1Changed(t_security_id_);
    return true;
  }

  smv_.ComputeSyntheticVarsFromBaseVars(base_bid_index_, kTradeTypeBuy);
  smv_.ComputeSyntheticVarsFromBaseVars(base_ask_index_, kTradeTypeSell);
  smv_.syn_bestbid_size_ = buy_size;
  smv_.syn_bestask_size_ = sell_size;

  CheckIfL1Changed(t_security_id_);

  return true;
}

/*
* Function that sends a predictive trade notification for any incoming aggressive order.
* It also adds it as an order which caused a crossed book scenario.
*/
void IndexedNSEMarketViewManager::PredictTrade(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                               const int t_int_price_, const uint32_t t_size_,
                                               const bool t_is_intermediate_) {
  SecurityMarketView &smv_ = *market_view_ptr_;

  std::vector<MarketUpdateInfoLevelStruct> &base_levels_ =
      (t_buysell_ == kTradeTypeBuy) ? smv_.market_update_info_.asklevels_ : smv_.market_update_info_.bidlevels_;

  uint32_t remaining_size = t_size_;
  uint32_t base_index_ = 0;
  int32_t ticks_diff = 0;

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      base_index_ = smv_.syn_bestask_index_;
      ticks_diff = t_int_price_ - base_levels_[base_index_].limit_int_price_;
      if (smv_.syn_bestask_int_price_ == kInvalidIntPrice) return;
    } break;
    case kTradeTypeSell: {
      base_index_ = smv_.syn_bestbid_index_;
      ticks_diff = base_levels_[base_index_].limit_int_price_ - t_int_price_;
      if (smv_.syn_bestbid_int_price_ == kInvalidIntPrice) return;
    } break;
    default:
      return;
  }

  typedef struct {
    int int_price_;
    uint32_t trade_size;
    TradeType_t buysell_;
  } trade_struct;

  std::vector<trade_struct> trade_list;

  for (int i = 0; i <= ticks_diff; i++) {
    if (base_levels_[base_index_ - i].limit_size_ <= 0) continue;
    int best_int_price_ = 0;
    uint32_t level_size = 0;

    if (i == 0) {
      // Get the best syn size
      level_size = t_buysell_ == kTradeTypeSell ? smv_.syn_bestbid_size_ : smv_.syn_bestask_size_;
      best_int_price_ = t_buysell_ == kTradeTypeSell ? smv_.syn_bestbid_int_price_ : smv_.syn_bestask_int_price_;
    } else {
      level_size = base_levels_[base_index_ - i].limit_size_;
      best_int_price_ = base_levels_[base_index_ - i].limit_int_price_;
    }

    if (level_size >= remaining_size) {
      // Full aggress contained on this level
      trade_struct trade;
      trade.int_price_ = best_int_price_;
      trade.trade_size = remaining_size;
      trade.buysell_ = (t_buysell_ == kTradeTypeSell ? kTradeTypeSell : kTradeTypeBuy);
      trade_list.push_back(trade);
      remaining_size = 0;
      break;
    } else {
      trade_struct trade;
      trade.int_price_ = best_int_price_;
      trade.trade_size = level_size;
      trade.buysell_ = (t_buysell_ == kTradeTypeSell ? kTradeTypeSell : kTradeTypeBuy);
      trade_list.push_back(trade);
      remaining_size -= level_size;
    }
  }

  GetUncrossedSynVars(t_security_id_, t_is_intermediate_);

  for (uint32_t i = 0; i < trade_list.size(); i++) {
    trade_struct trade = trade_list[i];
    NotifyListenersOnTrade(t_security_id_, trade.int_price_, trade.trade_size, trade.buysell_);
  }

  trade_list.clear();
  NotifyListenersOnLevelChange(t_security_id_, t_buysell_, t_is_intermediate_);
}

void IndexedNSEMarketViewManager::OnOrderAdd(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                             const uint64_t order_id_, const double t_price_, const uint32_t t_size_,
                                             const bool t_is_intermediate_) {
  SecurityMarketView &smv_ = *(security_market_view_map_[t_security_id_]);
  market_view_ptr_ = &smv_;

  int int_price_ = smv_.GetIntPx(t_price_);

  if (!smv_.initial_book_constructed_) {
    BuildIndex(t_security_id_, t_buysell_, int_price_);
    smv_.ComputeSyntheticVarsFromBaseVars(smv_.base_bid_index_, kTradeTypeBuy);
    smv_.ComputeSyntheticVarsFromBaseVars(smv_.base_ask_index_, kTradeTypeSell);
    CheckIfL1Changed(t_security_id_);
  }

  int bid_index_ = 0;
  int ask_index_ = 0;

  bool is_crossing_index_ = false;

  bool is_order_seen = order_history_instance.IsOrderSeen(t_security_id_, order_id_);
  if (is_order_seen) {
    dbglogger_ << "Incorrect AddOrder state - existent orderid added " << order_id_ << '\n';
    return;
  }

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      if (smv_.market_update_info_.asklevels_[smv_.syn_bestask_index_].limit_int_price_ <= int_price_ &&
          smv_.market_update_info_.asklevels_[smv_.syn_bestask_index_].limit_size_ > 0) {
        is_crossing_index_ = true;
        sid_to_msecs_at_first_cross_[t_security_id_] = watch_.msecs_from_midnight();
      }

      bid_index_ = smv_.GetBidIndex(int_price_);

      // There are 0 levels on the bid side
      if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ <= 0) {
        if (bid_index_ < LOW_ACCESS_INDEX) {
          RebuildIndexLowAccess(t_security_id_, t_buysell_, int_price_);
          bid_index_ = smv_.base_bid_index_;
        } else if (bid_index_ >= (int)smv_.max_tick_range_) {
          RebuildIndexHighAccess(t_security_id_, t_buysell_, int_price_);
          bid_index_ = smv_.base_bid_index_;
        } else {
          // bid_index_ falls within the range. So, set base_bid_index_ as bid_index_
          smv_.base_bid_index_ = bid_index_;
        }
      } else if (bid_index_ < 0) {
        // TODO - fix it for NSE
        // Skip the message if we don't have any info for this level
        ReScaleBook(t_security_id_, t_buysell_, int_price_);
        bid_index_ = smv_.GetBidIndex(int_price_);
        if (bid_index_ < 0) {
          return;
        }
      }

      if (bid_index_ >= (int)smv_.max_tick_range_) {
        RebuildIndexHighAccess(t_security_id_, t_buysell_, int_price_);
        bid_index_ = smv_.base_bid_index_;
      }

      if (bid_index_ > (int)smv_.base_bid_index_) {
        // Crossed book scenario/New level add scenario
        smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ = t_size_;
        smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ = 1;

        smv_.base_bid_index_ = bid_index_;
        UpdateSanitizeVarsOnBaseL1Change(t_security_id_, t_buysell_);
      } else if (bid_index_ == (int)smv_.base_bid_index_) {
        smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ += t_size_;
        smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_++;
        UpdateSanitizeVarsOnBaseL1Change(t_security_id_, t_buysell_);
      } else {
        smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ += t_size_;
        smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_++;
      }

      if (is_crossing_index_) {
        // Send a pre emptive trade notification
        PredictTrade(t_security_id_, t_buysell_, int_price_, t_size_, t_is_intermediate_);
      }
    } break;
    case kTradeTypeSell: {
      // Queue aggressive messages
      if (smv_.market_update_info_.bidlevels_[smv_.syn_bestbid_index_].limit_int_price_ >= int_price_ &&
          smv_.market_update_info_.bidlevels_[smv_.syn_bestbid_index_].limit_size_ > 0) {
        is_crossing_index_ = true;
        sid_to_msecs_at_first_cross_[t_security_id_] = watch_.msecs_from_midnight();
      }

      ask_index_ = smv_.GetAskIndex(int_price_);

      // There are 0 levels on the ask side
      if (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ <= 0) {
        if (ask_index_ < LOW_ACCESS_INDEX) {
          RebuildIndexLowAccess(t_security_id_, t_buysell_, int_price_);
          ask_index_ = smv_.base_ask_index_;
        } else if (ask_index_ >= (int)smv_.max_tick_range_) {
          RebuildIndexHighAccess(t_security_id_, t_buysell_, int_price_);
          ask_index_ = smv_.base_ask_index_;
        } else {
          // ask_index_ falls within the range. So, set base_ask_index_ as ask_index_
          smv_.base_ask_index_ = ask_index_;
        }
      } else if (ask_index_ < 0) {
        ReScaleBook(t_security_id_, t_buysell_, int_price_);
        ask_index_ = smv_.GetAskIndex(int_price_);
        if (ask_index_ < 0) {
          return;
        }
      }

      if (ask_index_ >= (int)smv_.max_tick_range_) {
        RebuildIndexHighAccess(t_security_id_, t_buysell_, int_price_);
        ask_index_ = smv_.base_ask_index_;
      }

      if (ask_index_ > (int)smv_.base_ask_index_) {
        // Crossed book scenario/New level add scenario
        smv_.market_update_info_.asklevels_[ask_index_].limit_size_ = t_size_;
        smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ = 1;

        smv_.base_ask_index_ = ask_index_;
        UpdateSanitizeVarsOnBaseL1Change(t_security_id_, t_buysell_);
      } else if (ask_index_ == (int)smv_.base_ask_index_) {
        smv_.market_update_info_.asklevels_[ask_index_].limit_size_ += t_size_;
        smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_++;
        UpdateSanitizeVarsOnBaseL1Change(t_security_id_, t_buysell_);
      } else {
        smv_.market_update_info_.asklevels_[ask_index_].limit_size_ += t_size_;
        smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_++;
      }

      if (is_crossing_index_) {
        // Send a pre emptive trade notification
        PredictTrade(t_security_id_, t_buysell_, int_price_, t_size_, t_is_intermediate_);
      }

    } break;
    default:
      break;
  }

  GetUncrossedSynVars(t_security_id_, t_is_intermediate_);

  order_history_instance.AddOrderToOrderHistory(t_security_id_, t_buysell_, t_price_, t_size_, order_id_);

  if (!is_crossing_index_) NotifyListenersOnLevelChange(t_security_id_, t_buysell_, t_is_intermediate_);
}

// Notify listeners on Trade
void IndexedNSEMarketViewManager::NotifyListenersOnTrade(const uint32_t t_security_id_, const int t_trade_int_price_,
                                                         const int t_trade_size_, const TradeType_t t_buysell_) {
  SecurityMarketView &smv_ = *market_view_ptr_;

  smv_.StorePreTrade();

  double trade_price_ = smv_.GetDoublePx(t_trade_int_price_);

  smv_.trade_print_info_.trade_price_ = trade_price_;
  smv_.trade_print_info_.size_traded_ = t_trade_size_;
  smv_.trade_print_info_.int_trade_price_ = t_trade_int_price_;
  smv_.trade_print_info_.buysell_ = t_buysell_;
  smv_.SetTradeVarsForIndicatorsIfRequired();

  smv_.UpdateL1Prices();

  if (CheckValidTime(t_security_id_)) {
    if (smv_.is_ready_) {
      smv_.NotifyTradeListeners();
      smv_.NotifyOnReadyListeners();
    }
  }
}

void IndexedNSEMarketViewManager::SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_) {
  for (auto i = 0u; i < security_market_view_map_.size(); i++) {
    SecurityMarketView *smv_ = security_market_view_map_[i];
    smv_->set_skip_listener_notification_end_time(r_start_time_);
  }
}

// Notifying listeners on a level change. Happens after order add/modify or delete.
void IndexedNSEMarketViewManager::NotifyListenersOnLevelChange(const uint32_t t_security_id_,
                                                               const TradeType_t t_buysell_, bool t_is_intermediate_) {
  if (t_is_intermediate_) return;

  SecurityMarketView &smv_ = *market_view_ptr_;

  // l1 price changed and size changed are set only if syn variables are changed. They are set through calls to
  // CheckIfL1Changed()
  if (l1_price_changed_[t_security_id_] || l1_size_changed_[t_security_id_]) {
    smv_.UpdateL1Prices();
  }

  if (!smv_.is_ready_) {
    if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ > 0 &&
        smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ > 0 && smv_.ArePricesComputed()) {
      smv_.is_ready_ = true;
    }
  }

  // Skip notifying listeners if either the message is intermediate or smv is not in ready state
  // Also, skip if the message order exec message. Because this will undo the effect of exec_summary messages.
  if (!smv_.is_ready_) {
    return;
  }

  if (CheckValidTime(t_security_id_)) {
    // Notify relevant listeners about the update
    if (l1_price_changed_[t_security_id_]) {
      smv_.NotifyL1PriceListeners();
      // UpdateSanitizeVarsOnL1Change(t_security_id_);
      l1_price_changed_[t_security_id_] = false;
      l1_size_changed_[t_security_id_] = false;
    } else if (l1_size_changed_[t_security_id_]) {
      smv_.NotifyL1SizeListeners();
      // UpdateSanitizeVarsOnL1Change(t_security_id_);
      l1_size_changed_[t_security_id_] = false;
    } else {
      smv_.NotifyL2Listeners();
      smv_.NotifyL2OnlyListeners();
    }
  }
}

// Helper function used to decrease/increase order size. Called from OnOrderModify/OnOrderDelete/OnTrade
void IndexedNSEMarketViewManager::ModifyOrderSize(const uint32_t t_security_id_, const uint64_t order_id_,
                                                  const int size_diff, const bool t_is_intermediate_) {
  SecurityMarketView &smv_ = *market_view_ptr_;

  OrderDetailsStruct *t_order_ = order_history_instance.GetOrderDetails(t_security_id_, order_id_);
  TradeType_t t_buysell_ = kTradeTypeBuy;

  int level_index_ = 0;
  int int_price_ = smv_.GetIntPx(t_order_->order_price);

  if (t_order_->is_buy_order) {
    // Buy order
    level_index_ = smv_.GetBidIndex(int_price_);
    if (level_index_ < 0) {
      ReScaleBook(t_security_id_, t_buysell_, int_price_);
      level_index_ = smv_.GetBidIndex(int_price_);
      if (level_index_ < 0) return;
    }
  } else {
    // Sell order
    level_index_ = smv_.GetAskIndex(int_price_);
    if (level_index_ < 0) {
      ReScaleBook(t_security_id_, t_buysell_, int_price_);
      level_index_ = smv_.GetAskIndex(int_price_);
      if (level_index_ < 0) return;
    }

    t_buysell_ = kTradeTypeSell;
  }

  std::vector<MarketUpdateInfoLevelStruct> &base_levels_ =
      (t_buysell_ == kTradeTypeSell) ? smv_.market_update_info_.asklevels_ : smv_.market_update_info_.bidlevels_;
  uint32_t &base_index_ = (t_buysell_ == kTradeTypeSell) ? smv_.base_ask_index_ : smv_.base_bid_index_;

  int old_order_size = t_order_->order_size;
  t_order_->order_size += size_diff;

  if (t_order_->order_size < 0) {
    // Possible iceberg order
    base_levels_[level_index_].limit_size_ += -old_order_size;
  } else {
    base_levels_[level_index_].limit_size_ += size_diff;
  }

  if (t_order_->order_size <= 0) {
    base_levels_[level_index_].limit_ordercount_ -= 1;
    order_history_instance.DeleteOrderFromHistory(t_security_id_, order_id_);
  }

  if (base_levels_[level_index_].limit_size_ <= 0 || base_levels_[level_index_].limit_ordercount_ <= 0) {
    base_levels_[level_index_].limit_size_ = 0;
    base_levels_[level_index_].limit_ordercount_ = 0;
  }

  if (level_index_ == (int)base_index_) {
    UpdateSanitizeVarsOnBaseL1Change(t_security_id_, t_buysell_);
  }

  // If the level that is changed is the base level, update SMV.
  if (base_levels_[base_index_].limit_size_ <= 0) {
    int index_ = base_index_ - 1;
    for (; index_ >= 0 && base_levels_[index_].limit_size_ <= 0; index_--)
      ;

    if (index_ >= 0) {
      base_index_ = index_;
      // Check if we need to re-centre the index
      if (base_index_ < LOW_ACCESS_INDEX) {
        RebuildIndexLowAccess(t_security_id_, t_buysell_, base_levels_[base_index_].limit_int_price_);
      }
    } else {
      smv_.is_ready_ = false;
    }
  }
}

// Handling Order Modify message:
void IndexedNSEMarketViewManager::OnOrderModify(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                                const uint64_t order_id_, const double t_price_,
                                                const uint32_t t_size_) {
  bool is_order_seen = order_history_instance.IsOrderSeen(t_security_id_, order_id_);
  if (!is_order_seen) {
    // Order isn't present in our history. Simulate an OnOrderAdd
    OnOrderAdd(t_security_id_, t_buysell_, order_id_, t_price_, t_size_, false);
    return;
  }

  SecurityMarketView &smv_ = *(security_market_view_map_[t_security_id_]);
  market_view_ptr_ = &smv_;

  OrderDetailsStruct *t_order_ = order_history_instance.GetOrderDetails(t_security_id_, order_id_);

  if (smv_.DblPxCompare(t_order_->order_price, t_price_)) {
    // 1. If the prices are same, simply change the size
    int size_diff = t_size_ - t_order_->order_size;
    ModifyOrderSize(t_security_id_, order_id_, size_diff, false);
    GetUncrossedSynVars(t_security_id_, false);
    NotifyListenersOnLevelChange(t_security_id_, t_buysell_, false);
  } else {
    // since prices are different
    // delete the previous order and add a new one while keeping intermediate flag in delete on so that we don't notify
    // listeners upon delete message
    OnOrderDelete(t_security_id_, t_buysell_, order_id_, t_order_->order_price, true, true);
    OnOrderAdd(t_security_id_, t_buysell_, order_id_, t_price_, t_size_, false);
  }
}

void IndexedNSEMarketViewManager::OnOrderDelete(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                                const uint64_t order_id_, const double t_price_,
                                                const bool t_delete_order_, const bool t_is_intermediate_) {
  bool is_order_seen = order_history_instance.IsOrderSeen(t_security_id_, order_id_);
  if (!is_order_seen) {
    // Order is not seen. Delete call is an invalid one. Return
    return;
  }

  SecurityMarketView &smv_ = *(security_market_view_map_[t_security_id_]);

  OrderDetailsStruct *t_order_ = order_history_instance.GetOrderDetails(t_security_id_, order_id_);
  market_view_ptr_ = &smv_;

  uint32_t size_diff = -t_order_->order_size;
  ModifyOrderSize(t_security_id_, order_id_, size_diff, t_is_intermediate_);
  GetUncrossedSynVars(t_security_id_, t_is_intermediate_);
  NotifyListenersOnLevelChange(t_security_id_, t_buysell_, t_is_intermediate_);
}

void IndexedNSEMarketViewManager::OnTrade(const unsigned int t_security_id_, double const t_trade_price_,
                                          const int t_trade_size_, const uint64_t t_buy_order_num_,
                                          const uint64_t t_sell_order_num_) {
  SecurityMarketView &smv_ = *(security_market_view_map_[t_security_id_]);
  market_view_ptr_ = &smv_;

  if (!smv_.initial_book_constructed_) {
    return;
  }

  int t_trade_int_price_ = smv_.GetIntPx(t_trade_price_);

  if (t_trade_int_price_ > smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_) {
    bool was_sanitized_ = CheckCrossedBookInstance(
        t_security_id_, kTradeTypeSell, smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_,
        t_trade_int_price_, smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_);

    if (was_sanitized_) return;
  } else if (t_trade_int_price_ < smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_) {
    bool was_sanitized_ = CheckCrossedBookInstance(
        t_security_id_, kTradeTypeBuy, smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_,
        t_trade_int_price_, smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_);

    if (was_sanitized_) return;
  }

  bool is_buy_order_seen_ = order_history_instance.IsOrderSeen(t_security_id_, t_buy_order_num_);
  bool is_sell_order_seen_ = order_history_instance.IsOrderSeen(t_security_id_, t_sell_order_num_);

  bool is_invisible_trade = false;
  if (!IsBookCrossed()) {
    // This trade is not seen by our prediction. Send a trade notification.
    // Usually happens when one of the orders is not seen.
    is_invisible_trade = true;
  }

  // TODO: Check how to handle iceberg orders
  if (is_buy_order_seen_) {
    ModifyOrderSize(t_security_id_, t_buy_order_num_, -t_trade_size_, false);
  }

  if (is_sell_order_seen_) {
    ModifyOrderSize(t_security_id_, t_sell_order_num_, -t_trade_size_, false);
  }

  if (is_invisible_trade) {
    smv_.trade_print_info_.trade_price_ = t_trade_price_;
    smv_.trade_print_info_.size_traded_ = t_trade_size_;
    smv_.trade_print_info_.int_trade_price_ = t_trade_int_price_;

    if (is_buy_order_seen_) {
      smv_.trade_print_info_.buysell_ = kTradeTypeSell;
    }

    if (is_sell_order_seen_) {
      smv_.trade_print_info_.buysell_ = kTradeTypeBuy;
    }

#if NSE_SANITIZE_ON_SUB_BEST_TRADE
    // Underlying book is uncrossed
    if (!IsBookCrossed()) {
      // Sub best-bid trade
      if (t_trade_int_price_ < smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_) {
        sid_to_side_to_be_sanitized_[t_security_id_] = kTradeTypeBuy;
        DBGLOG_TIME_CLASS_FUNC_LINE << "Sanitize on sub best-bid trade" << DBGLOG_ENDL_FLUSH;
        SanitizeBook(t_security_id_, t_trade_int_price_ + 1);
      }
      // Sub best-ask trade
      else if (t_trade_int_price_ > smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_) {
        sid_to_side_to_be_sanitized_[t_security_id_] = kTradeTypeSell;
        DBGLOG_TIME_CLASS_FUNC_LINE << "Sanitize on sub best-ask trade" << DBGLOG_ENDL_FLUSH;
        SanitizeBook(t_security_id_, t_trade_int_price_ - 1);
      }
    }
#endif

    GetUncrossedSynVars(t_security_id_, false);

    if (CheckValidTime(t_security_id_)) {
      if (smv_.is_ready_) {
        smv_.NotifyTradeListeners();
        smv_.NotifyOnReadyListeners();
      }
    }

    NotifyListenersOnLevelChange(t_security_id_, smv_.trade_print_info_.buysell_, false);
  }
}

// Called in cases where atleast one of the ordrs involved in the trade is a stop-loss order
void IndexedNSEMarketViewManager::OnHiddenTrade(const unsigned int t_security_id_, double const t_trade_price_,
                                                const int t_trade_size_, const uint64_t t_buy_order_num_,
                                                const uint64_t t_sell_order_num_) {
  // Defunct - only called for historical NSE provided data in Stop Loss scenarios - won't occur in Live
  OnTrade(t_security_id_, t_trade_price_, t_trade_size_, t_buy_order_num_, t_sell_order_num_);
}

/* This function is called if we have determined that the book needs to be sanitized.
   We do a couple of things here
( a ) We delete underlying indexed book levels upto and including offending trade price
( b ) We delete any offending orders in live and queued map
( c ) We call simulate post these changes so that queued orders can be flushed if needed */
void IndexedNSEMarketViewManager::SanitizeBook(const uint32_t t_security_id_, const int t_sanitize_price_) {
  // First correct underlying indexed book [ (a) above ]
  // SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);
  SecurityMarketView &smv_ = *market_view_ptr_;

  if (!dbglogger_.CheckLoggingLevel(SKIP_PACKET_DROPS_INFO)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << "Book state before Sanitize: " << smv_.market_update_info_.bestbid_size_ << " @ "
                                << smv_.market_update_info_.bestbid_price_ << " --- "
                                << smv_.market_update_info_.bestask_price_ << " @ "
                                << smv_.market_update_info_.bestask_size_ << DBGLOG_ENDL_FLUSH;
  }
  // Side to sanitize is the side where orders will be deleted, i.e. kTradeTypeBuy means we delete bid levels

  TradeType_t side_to_sanitize_ = sid_to_side_to_be_sanitized_[t_security_id_];
  int t_index_to_sanitize_to_ = 0;
  switch (side_to_sanitize_) {
    case kTradeTypeBuy: {
      t_index_to_sanitize_to_ = std::max(0, smv_.GetBidIndex(t_sanitize_price_));
      int t_index_ = smv_.base_bid_index_;
      // delete values upto and including sub-best trade price
      for (; t_index_ >= t_index_to_sanitize_to_; t_index_--) {
        smv_.market_update_info_.bidlevels_[t_index_].limit_size_ = 0;
        smv_.market_update_info_.bidlevels_[t_index_].limit_ordercount_ = 0;
      }
      // find next valid level
      for (; t_index_ >= 0 && smv_.market_update_info_.bidlevels_[t_index_].limit_size_ <= 0; t_index_--)
        ;

      /// set best variables appropriately
      if (t_index_ > 0) {
        smv_.market_update_info_.bestbid_int_price_ = smv_.market_update_info_.bidlevels_[t_index_].limit_int_price_;
        smv_.market_update_info_.bestbid_size_ = smv_.market_update_info_.bidlevels_[t_index_].limit_size_;
        smv_.market_update_info_.bestbid_price_ = smv_.market_update_info_.bidlevels_[t_index_].limit_price_;
        smv_.market_update_info_.bestbid_ordercount_ = smv_.market_update_info_.bidlevels_[t_index_].limit_ordercount_;
      } else {
        smv_.is_ready_ = false;
      }
      // set base index values
      if (t_index_ > 0) {
        smv_.base_bid_index_ = t_index_;
      } else {
        smv_.base_bid_index_ = 0;
      }
    } break;
    case kTradeTypeSell: {
      t_index_to_sanitize_to_ = std::max(0, smv_.GetAskIndex(t_sanitize_price_));
      int t_index_ = smv_.base_ask_index_;
      // delete values upto and including sub-best trade price
      for (; t_index_ >= t_index_to_sanitize_to_; t_index_--) {
        smv_.market_update_info_.asklevels_[t_index_].limit_size_ = 0;
        smv_.market_update_info_.asklevels_[t_index_].limit_ordercount_ = 0;
      }
      // find next valid level
      for (; t_index_ >= 0 && smv_.market_update_info_.asklevels_[t_index_].limit_size_ <= 0; t_index_--)
        ;

      /// set best variables appropriately
      if (t_index_ > 0) {
        smv_.market_update_info_.bestask_int_price_ = smv_.market_update_info_.asklevels_[t_index_].limit_int_price_;
        smv_.market_update_info_.bestask_size_ = smv_.market_update_info_.asklevels_[t_index_].limit_size_;
        smv_.market_update_info_.bestask_price_ = smv_.market_update_info_.asklevels_[t_index_].limit_price_;
        smv_.market_update_info_.bestask_ordercount_ = smv_.market_update_info_.asklevels_[t_index_].limit_ordercount_;
      } else {
        smv_.is_ready_ = false;
      }
      // set base index values
      if (t_index_ > 0) {
        smv_.base_ask_index_ = t_index_;
      } else {
        smv_.base_ask_index_ = 0;
      }
    } break;
    default:
      break;
  }

  // Next delete offending orders in maps
  std::vector<OrderDetailsStruct *> &order_cache_ = order_history_instance.GetOrderCache(t_security_id_);
  ska::flat_hash_map<uint64_t, OrderDetailsStruct *> &order_history_ = order_history_instance.GetOrderMaps(t_security_id_);

  std::vector<OrderDetailsStruct *>::iterator t_vectiter;

  for (t_vectiter = order_cache_.begin(); t_vectiter != order_cache_.end();) {
    OrderDetailsStruct *t_live_order_ = *t_vectiter;
    if (t_live_order_ == NULL) {
      t_vectiter++;
      continue;
    }
    // live order contradicting bestprice is deleted
    if ((t_live_order_->is_buy_order && side_to_sanitize_ == kTradeTypeBuy &&
         (smv_.base_bid_index_ == 0 || t_live_order_->order_price > smv_.market_update_info_.bestbid_price_ + 1e-5)) ||
        (!t_live_order_->is_buy_order && side_to_sanitize_ == kTradeTypeSell &&
         (smv_.base_ask_index_ == 0 || t_live_order_->order_price < smv_.market_update_info_.bestask_price_ - 1e-5))) {
      t_vectiter++;
      order_history_instance.DeleteOrderFromHistory(t_security_id_, t_live_order_->order_id);
    } else {
      t_vectiter++;
    }
  }

  ska::flat_hash_map<uint64_t, OrderDetailsStruct *>::iterator t_mapiter_;

  for (t_mapiter_ = order_history_.begin(); t_mapiter_ != order_history_.end();) {
    OrderDetailsStruct *t_live_order_ = (*t_mapiter_).second;
    // live order contradicting bestprice is deleted
    if ((t_live_order_->is_buy_order && side_to_sanitize_ == kTradeTypeBuy &&
         (smv_.base_bid_index_ == 0 || t_live_order_->order_price > smv_.market_update_info_.bestbid_price_ + 1e-5)) ||
        (!t_live_order_->is_buy_order && side_to_sanitize_ == kTradeTypeSell &&
         (smv_.base_ask_index_ == 0 || t_live_order_->order_price < smv_.market_update_info_.bestask_price_ - 1e-5))) {
      t_mapiter_++;
      order_history_instance.DeleteOrderFromHistory(t_security_id_, t_live_order_->order_id);
    } else {
      t_mapiter_++;
    }
  }
}

/* This function is called on any occurence of a sub-best trade. Logic for deciding whether
   SanitizeBook should be called is implemented here. Return value is true if book was sanitized */
bool IndexedNSEMarketViewManager::CheckCrossedBookInstance(const uint32_t t_security_id_, const TradeType_t t_side_,
                                                           const int t_crossed_int_px_, const int t_sanitize_price_,
                                                           const int t_crossed_px_size_) {
  if (sid_to_side_to_be_sanitized_[t_security_id_] != t_side_ ||
      sid_to_int_px_to_sanitize_[t_security_id_] != t_crossed_int_px_ ||
      sid_to_size_at_sanitize_px_[t_security_id_] != t_crossed_px_size_) {
    sid_to_sanitize_count_[t_security_id_] = 1;
    sid_to_side_to_be_sanitized_[t_security_id_] = t_side_;
    sid_to_int_px_to_sanitize_[t_security_id_] = t_crossed_int_px_;
    sid_to_msecs_at_first_cross_[t_security_id_] = watch_.msecs_from_midnight();
    sid_to_size_at_sanitize_px_[t_security_id_] = t_crossed_px_size_;
  } else {
    sid_to_sanitize_count_[t_security_id_] = sid_to_sanitize_count_[t_security_id_] + 1;
  }

  // Metric for calling sanitized is repeated crossing of same threshold
  if ((sid_to_sanitize_count_[t_security_id_] > NSE_SANITIZE_COUNT_THRESHOLD) &&
      (watch_.msecs_from_midnight() - sid_to_msecs_at_first_cross_[t_security_id_] > NSE_SANITIZE_TIME_THRESHOLD)) {
    SanitizeBook(t_security_id_, t_sanitize_price_);
    sid_to_int_px_to_sanitize_[t_security_id_] = -1;
    sid_to_sanitize_count_[t_security_id_] = 0;
    sid_to_msecs_at_first_cross_[t_security_id_] = watch_.msecs_from_midnight();
  }
  return false;
}

/* Updates sanitization behaviour if a side is just updated. */
void IndexedNSEMarketViewManager::UpdateSanitizeVarsOnBaseL1Change(const uint32_t t_security_id_,
                                                                   const TradeType_t t_buysell_) {
  // SecurityMarketView &smv_ = *market_view_ptr_;

  if (t_buysell_ == kTradeTypeBuy && sid_to_side_to_be_sanitized_[t_security_id_] != kTradeTypeSell) {
    // Since buy side was updated just now, update vars to reflect that sell side might need sanitization
    sid_to_side_to_be_sanitized_[t_security_id_] = kTradeTypeSell;
  } else if (t_buysell_ == kTradeTypeSell && sid_to_side_to_be_sanitized_[t_security_id_] != kTradeTypeBuy) {
    sid_to_side_to_be_sanitized_[t_security_id_] = kTradeTypeBuy;
  }
}

// Checks and returns if current time is trading time for the product
bool IndexedNSEMarketViewManager::CheckValidTime(int sec_id) {
  currently_trading_[sec_id] = trade_time_manager_.isValidTimeToTrade(sec_id, watch_.tv().tv_sec % 86400);
  return currently_trading_[sec_id];
}
}
