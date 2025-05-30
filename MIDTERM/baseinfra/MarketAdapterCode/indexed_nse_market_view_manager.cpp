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

namespace HFSAT {

//#define INDEXED_BOOK_SIZE 1023
//#define INITIAL_BASE_INDEX 511
#define HIGH_ACCESS_INDEX 1023
#define LOW_ACCESS_INDEX 50
#define HISTORY_BUFF_LEN 5
// If there are these many sub-best trades against the same best price, and
// across atleast TIME_THRESHOLD we sanitize the book
#define NSE_SANITIZE_COUNT_THRESHOLD 5
#define NSE_SANITIZE_TIME_THRESHOLD 5000  // 5 seconds
// File listing days where there have been dropped packets in logged data.
// To avoid redundant emails
#define NSE_CROSSED_DATA_DAYS_FILE "NSE_Files/crossed_data_days.txt"
#define CCPROFILING_TRADEINIT 1

IndexedNSEMarketViewManager::IndexedNSEMarketViewManager(
    DebugLogger& t_dbglogger_, const Watch& t_watch_, const SecurityNameIndexer& t_sec_name_indexer_,
    const std::vector<SecurityMarketView*>& t_security_market_view_map_)
    : BaseMarketViewManager(t_dbglogger_, t_watch_, t_sec_name_indexer_, t_security_market_view_map_),
      l1_price_changed_(t_sec_name_indexer_.NumSecurityId(), false),
      l1_size_changed_(t_sec_name_indexer_.NumSecurityId(), false),
      sec_id_to_prev_update_was_quote_(t_sec_name_indexer_.NumSecurityId(), false),
      sec_id_to_mask_levels_above_(t_sec_name_indexer_.NumSecurityId(), false),
      skip_notifications_(t_sec_name_indexer_.NumSecurityId(), false),
      order_struct_mempool_(),
      sid_to_queued_orders_maps_vec_(t_sec_name_indexer_.NumSecurityId(), std::map<uint64_t, order_details_struct_*>()),
      sid_to_live_orders_maps_vec_(t_sec_name_indexer_.NumSecurityId(), std::map<uint64_t, order_details_struct_*>()),
      sid_to_side_to_be_sanitized_(t_sec_name_indexer_.NumSecurityId(), kTradeTypeNoInfo),
      sid_to_sanitize_count_(t_sec_name_indexer_.NumSecurityId(), 0),
      sid_to_int_px_to_sanitize_(t_sec_name_indexer_.NumSecurityId(), -1),
      sid_to_msecs_at_first_cross_(t_sec_name_indexer_.NumSecurityId(), 0),
      sid_to_size_at_sanitize_px_(t_sec_name_indexer_.NumSecurityId(), 0),
      crossed_data_days_(),
      trade_time_manager_(TradeTimeManager::GetUniqueInstance(t_sec_name_indexer_, t_watch_.YYYYMMDD())),
      currently_trading_(t_sec_name_indexer_.NumSecurityId(), false),
      last_trade_buy_order_num_(0),
      last_trade_buy_size_remaining_(0),
      last_trade_sell_order_num_(0),
      last_trade_sell_size_remaining_(0) {
  /// initialize sanitization related code
  is_live_ = false;
  checked_for_live_ = false;
  sent_alarm_in_hist_ = false;
}

// Only called in case of aggressive Order in OnOrderAdd. We assume this order is not
// present in any maps at that stage
void IndexedNSEMarketViewManager::AddOrderToQueuedOrdersMap(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                                            const double t_price_, const uint32_t t_size_,
                                                            const uint64_t t_order_id_) {
  order_details_struct_* t_que_order_ = order_struct_mempool_.Alloc();
  t_que_order_->is_buy_order_ = (t_buysell_ == kTradeTypeBuy ? true : false);
  t_que_order_->last_order_size_ = t_size_;
  t_que_order_->order_id_ = t_order_id_;
  t_que_order_->order_price_ = t_price_;
  (sid_to_queued_orders_maps_vec_[t_security_id_])
      .insert(std::pair<uint64_t, order_details_struct_*>(t_order_id_, t_que_order_));
}

// Only called in case of passive order in OnOrderAdd ( i.e. an order which is not causing crossed
// books. We assume this order is not present in any maps at this stage
void IndexedNSEMarketViewManager::AddOrderToLiveOrdersMap(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                                          const double t_price_, const uint32_t t_size_,
                                                          const uint64_t t_order_id_) {
  order_details_struct_* t_que_order_ = order_struct_mempool_.Alloc();
  t_que_order_->is_buy_order_ = (t_buysell_ == kTradeTypeBuy ? true : false);
  t_que_order_->last_order_size_ = t_size_;
  t_que_order_->order_id_ = t_order_id_;
  t_que_order_->order_price_ = t_price_;
  (sid_to_live_orders_maps_vec_[t_security_id_])
      .insert(std::pair<uint64_t, order_details_struct_*>(t_order_id_, t_que_order_));
}

// Move orders from queued map to live map and trigger onorderadd calls

// Returns true iff order is queued internally and is not reflected in the book
bool IndexedNSEMarketViewManager::OrderIsQueued(const uint32_t t_security_id_, const uint64_t t_order_id_) {
  std::map<uint64_t, order_details_struct_*>::iterator t_mapiter_ =
      sid_to_queued_orders_maps_vec_[t_security_id_].find(t_order_id_);
  return (t_mapiter_ != sid_to_queued_orders_maps_vec_[t_security_id_].end());
}

// Returns true iff order is queued internally and reflected in book. Price is set to order price in this case
bool IndexedNSEMarketViewManager::OrderIsLive(const uint32_t t_security_id_, const uint64_t t_order_id_,
                                              double* t_price_) {
  std::map<uint64_t, order_details_struct_*>::iterator t_mapiter_ =
      sid_to_live_orders_maps_vec_[t_security_id_].find(t_order_id_);
  if (t_mapiter_ != sid_to_live_orders_maps_vec_[t_security_id_].end()) {
    order_details_struct_* t_que_order_ = (*t_mapiter_).second;
    *t_price_ = (t_que_order_)->order_price_;
    return true;
  }
  return false;
}

// Updates order parameters for queued up order if orderid matches
void IndexedNSEMarketViewManager::UpdateOrderMapsOnModify(const uint32_t t_security_id_, const uint64_t t_order_id_,
                                                          const uint32_t t_size_) {
  std::map<uint64_t, order_details_struct_*>::iterator t_mapiter_ =
      sid_to_queued_orders_maps_vec_[t_security_id_].find(t_order_id_);
  if (t_mapiter_ != sid_to_queued_orders_maps_vec_[t_security_id_].end()) {
    order_details_struct_* t_que_order_ = (*t_mapiter_).second;
    t_que_order_->last_order_size_ = t_size_;
  }

  t_mapiter_ = sid_to_live_orders_maps_vec_[t_security_id_].find(t_order_id_);
  if (t_mapiter_ != sid_to_live_orders_maps_vec_[t_security_id_].end()) {
    order_details_struct_* t_que_order_ = (*t_mapiter_).second;
    t_que_order_->last_order_size_ = t_size_;
  }
}

void IndexedNSEMarketViewManager::UpdateOrderMapsOnTrade(const uint32_t t_security_id_, const uint64_t t_buy_order_id_,
                                                         const uint64_t t_sell_order_id_, const uint32_t t_size_,
                                                         const double t_price_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);
  std::map<uint64_t, order_details_struct_*>::iterator t_mapiter_;

  // process buy order if needed
  t_mapiter_ = sid_to_queued_orders_maps_vec_[t_security_id_].find(t_buy_order_id_);
  if (t_mapiter_ != sid_to_queued_orders_maps_vec_[t_security_id_].end()) {
    order_details_struct_* t_que_order_ = (*t_mapiter_).second;
    t_que_order_->last_order_size_ -= t_size_;
    if (t_que_order_->last_order_size_ <= 0) {
      (sid_to_queued_orders_maps_vec_[t_security_id_]).erase(t_mapiter_);
      order_struct_mempool_.DeAlloc(t_que_order_);
    }
  }
  t_mapiter_ = sid_to_live_orders_maps_vec_[t_security_id_].find(t_buy_order_id_);
  if (t_mapiter_ != sid_to_live_orders_maps_vec_[t_security_id_].end()) {
    order_details_struct_* t_que_order_ = (*t_mapiter_).second;
    if (!(smv_.DblPxCompare(t_price_, t_que_order_->order_price_))) {
      OnOrderDelete(t_security_id_, (t_que_order_->is_buy_order_ ? kTradeTypeBuy : kTradeTypeSell), 0,
                    t_que_order_->order_price_, t_size_, t_que_order_->last_order_size_ == (int)t_size_, false);
    }
    t_que_order_->last_order_size_ -= t_size_;
    if (t_que_order_->last_order_size_ <= 0) {
      (sid_to_live_orders_maps_vec_[t_security_id_]).erase(t_mapiter_);
      order_struct_mempool_.DeAlloc(t_que_order_);
    }
  }

  // process sell order if needed
  t_mapiter_ = sid_to_queued_orders_maps_vec_[t_security_id_].find(t_sell_order_id_);
  if (t_mapiter_ != sid_to_queued_orders_maps_vec_[t_security_id_].end()) {
    order_details_struct_* t_que_order_ = (*t_mapiter_).second;
    t_que_order_->last_order_size_ -= t_size_;
    if (t_que_order_->last_order_size_ <= 0) {
      (sid_to_queued_orders_maps_vec_[t_security_id_]).erase(t_mapiter_);
      order_struct_mempool_.DeAlloc(t_que_order_);
    }
  }
  t_mapiter_ = sid_to_live_orders_maps_vec_[t_security_id_].find(t_sell_order_id_);
  if (t_mapiter_ != sid_to_live_orders_maps_vec_[t_security_id_].end()) {
    order_details_struct_* t_que_order_ = (*t_mapiter_).second;
    if (!(smv_.DblPxCompare(t_price_, t_que_order_->order_price_))) {
      OnOrderDelete(t_security_id_, (t_que_order_->is_buy_order_ ? kTradeTypeBuy : kTradeTypeSell), 0,
                    t_que_order_->order_price_, t_size_, t_que_order_->last_order_size_ == (int)t_size_, false);
    }
    t_que_order_->last_order_size_ -= t_size_;
    if (t_que_order_->last_order_size_ <= 0) {
      (sid_to_live_orders_maps_vec_[t_security_id_]).erase(t_mapiter_);
      order_struct_mempool_.DeAlloc(t_que_order_);
    }
  }

  // add simulated orders if needed
  AddSimulatedOrders(t_security_id_);
}

void IndexedNSEMarketViewManager::AddSimulatedOrders(const uint32_t t_security_id_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  std::map<uint64_t, order_details_struct_*>::iterator t_mapiter_;
  std::map<uint64_t, order_details_struct_*>::iterator t_deliter_;

  for (t_mapiter_ = sid_to_queued_orders_maps_vec_[t_security_id_].begin();
       t_mapiter_ != sid_to_queued_orders_maps_vec_[t_security_id_].end();) {
    order_details_struct_* t_que_order_ = (*t_mapiter_).second;
    // check if order can be added
    if ((t_que_order_->is_buy_order_ &&
         t_que_order_->order_price_ < smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_price_ - 1e-5) ||
        (!t_que_order_->is_buy_order_ &&
         t_que_order_->order_price_ > smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_price_ + 1e-5)) {
      // The idea is to set the correct size of simulated order to be added. This is required as it was traded while it
      // was still in the queue.
      if (t_que_order_->is_buy_order_ && t_que_order_->order_id_ == last_trade_buy_order_num_) {
        t_que_order_->last_order_size_ = last_trade_buy_size_remaining_;
      } else if (!t_que_order_->is_buy_order_ && t_que_order_->order_id_ == last_trade_sell_order_num_) {
        t_que_order_->last_order_size_ = last_trade_sell_size_remaining_;
      }

      OnOrderAdd(t_security_id_, (t_que_order_->is_buy_order_ ? kTradeTypeBuy : kTradeTypeSell), 0,
                 t_que_order_->order_price_, t_que_order_->last_order_size_, false);
      t_deliter_ = t_mapiter_;
      t_mapiter_++;
      (sid_to_queued_orders_maps_vec_[t_security_id_]).erase(t_deliter_);
      (sid_to_live_orders_maps_vec_[t_security_id_])
          .insert(std::pair<uint64_t, order_details_struct_*>(t_que_order_->order_id_, t_que_order_));
    } else
      t_mapiter_++;
  }
}

// Called in cases where atleast one of the ordrs involved in the trade is a stop-loss order
void IndexedNSEMarketViewManager::OnHiddenTrade(const unsigned int t_security_id_, double const t_trade_price_,
                                                const int t_trade_size_, const uint64_t t_buy_order_num_,
                                                const uint64_t t_sell_order_num_, const int32_t t_bid_size_remaining_,
                                                const int32_t t_ask_size_remaining_) {
  // Defunct - only called for historical NSE provided data in Stop Loss scenarios - won't occur in Live
  OnTrade(t_security_id_, t_trade_price_, t_trade_size_, t_buy_order_num_, t_sell_order_num_, t_bid_size_remaining_,
          t_ask_size_remaining_);
}

void IndexedNSEMarketViewManager::OnOrderAdd(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                             const uint64_t order_id_, const double t_price_, const uint32_t t_size_,
                                             const bool t_is_intermediate_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  int int_price_ = smv_.GetIntPx(t_price_);

  if (!smv_.initial_book_constructed_) {
    BuildIndex(t_security_id_, t_buysell_, int_price_);
  }

  int bid_index_ = 0;
  int ask_index_ = 0;

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      // Queue aggressive messages
      if (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ <= int_price_ &&
          smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ > 0) {
        if (order_id_ > 0) {
          AddOrderToQueuedOrdersMap(t_security_id_, t_buysell_, t_price_, t_size_, order_id_);
        } else {
          DBGLOG_TIME_CLASS_FUNC_LINE << " FATAL Error .. Simulated Order entered as aggressive. Will exit \n";
          exit(-1);
        }
        return;
      }

      bid_index_ = (int)smv_.base_bid_index_ -
                   (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - int_price_);

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

        l1_price_changed_[t_security_id_] = true;
      } else if (bid_index_ < 0) {
        // TODO - fix it for NSE
        // Skip the message if we don't have any info for this level
        ReScaleBook(t_security_id_, t_buysell_, int_price_);
        bid_index_ = (int)smv_.base_bid_index_ -
                     (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - int_price_);
        if (bid_index_ < 0) {
          return;
        }
      }

      if (bid_index_ >= (int)smv_.max_tick_range_) {
        RebuildIndexHighAccess(t_security_id_, t_buysell_, int_price_);
        bid_index_ = smv_.base_bid_index_;

        l1_price_changed_[t_security_id_] = true;
      }

      if (bid_index_ > (int)smv_.base_bid_index_) {
        // Use assignment instead of increment while updating size and order count to make sure that we don't depend on
        // them being set to zero earlier.
        // But note that if this introduces holes in the book we do need the size to have been zeroed out at all invalid
        // levels
        smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ = t_size_;
        smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ = 1;

        smv_.base_bid_index_ = bid_index_;

        l1_price_changed_[t_security_id_] = true;
      } else if (bid_index_ == (int)smv_.base_bid_index_) {
        smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ += t_size_;
        smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_++;

        l1_size_changed_[t_security_id_] = true;
      } else {
        smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ += t_size_;
        smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_++;
      }
    } break;
    case kTradeTypeSell: {
      // Queue aggressive messages
      if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ >= int_price_ &&
          smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ > 0) {
        if (order_id_ > 0) {
          AddOrderToQueuedOrdersMap(t_security_id_, t_buysell_, t_price_, t_size_, order_id_);
        } else {
          DBGLOG_TIME_CLASS_FUNC_LINE << " FATAL Error .. Simulated Order entered as aggressive. Will exit \n";
          exit(-1);
        }
        return;
      }

      ask_index_ = (int)smv_.base_ask_index_ +
                   (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - int_price_);

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

        l1_price_changed_[t_security_id_] = true;
      } else if (ask_index_ < 0) {
        ReScaleBook(t_security_id_, t_buysell_, int_price_);
        ask_index_ = (int)smv_.base_ask_index_ +
                     (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - int_price_);
        if (ask_index_ < 0) {
          return;
        }
      }

      if (ask_index_ >= (int)smv_.max_tick_range_) {
        RebuildIndexHighAccess(t_security_id_, t_buysell_, int_price_);
        ask_index_ = smv_.base_ask_index_;

        l1_price_changed_[t_security_id_] = true;
      }

      if (ask_index_ > (int)smv_.base_ask_index_) {
        // Use assignment instead of increment while updating size and order count to make sure that they are set to
        // zero earlier.
        smv_.market_update_info_.asklevels_[ask_index_].limit_size_ = t_size_;
        smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ = 1;

        smv_.base_ask_index_ = ask_index_;

        l1_price_changed_[t_security_id_] = true;
      } else if (ask_index_ == (int)smv_.base_ask_index_) {
        smv_.market_update_info_.asklevels_[ask_index_].limit_size_ += t_size_;
        smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_++;

        l1_size_changed_[t_security_id_] = true;
      } else {
        smv_.market_update_info_.asklevels_[ask_index_].limit_size_ += t_size_;
        smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_++;
      }
    } break;
    default:
      break;
  }

  if (l1_price_changed_[t_security_id_] || l1_size_changed_[t_security_id_]) {
    if (t_buysell_ == kTradeTypeBuy) {
      smv_.market_update_info_.bestbid_int_price_ =
          smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_;
      smv_.market_update_info_.bestbid_price_ = smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_price_;
      smv_.market_update_info_.bestbid_size_ = smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_;
      smv_.market_update_info_.bestbid_ordercount_ =
          smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_ordercount_;
      UpdateBestBidVariablesUsingOurOrders(t_security_id_);
    } else {
      smv_.market_update_info_.bestask_int_price_ =
          smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_;
      smv_.market_update_info_.bestask_price_ = smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_price_;
      smv_.market_update_info_.bestask_size_ = smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_;
      smv_.market_update_info_.bestask_ordercount_ =
          smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_ordercount_;
      UpdateBestAskVariablesUsingOurOrders(t_security_id_);
    }

    smv_.UpdateL1Prices();
  }

  if (!smv_.is_ready_) {
    if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ > 0 &&
        smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ > 0 && smv_.ArePricesComputed()) {
      smv_.is_ready_ = true;
    }
  }

  if (smv_.market_update_info_.trade_update_implied_quote_) {
    smv_.market_update_info_.trade_update_implied_quote_ = false;
    smv_.trade_print_info_.num_trades_++;
  }

  sec_id_to_prev_update_was_quote_[t_security_id_] = true;

  // Skip notifying listeners if the message is intermediate
  if (!smv_.is_ready_ || t_is_intermediate_) {
    return;
  }

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(14);
#endif

  if (CheckValidTime(t_security_id_)) {
    if (smv_.pl_change_listeners_present_) {
      if (t_buysell_ == kTradeTypeBuy && (int)smv_.base_bid_index_ >= bid_index_ &&
          (int)smv_.base_bid_index_ - bid_index_ <= 10) {
        int level_ = smv_.base_bid_index_ - bid_index_;
        int int_price_level_ = smv_.base_bid_index_ - bid_index_;
        smv_.NotifyOnPLChangeListeners(
            t_security_id_, smv_.market_update_info_, t_buysell_, level_, int_price_, int_price_level_,
            smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ - t_size_,
            smv_.market_update_info_.bidlevels_[bid_index_].limit_size_,
            smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ - 1,
            smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_, t_is_intermediate_,
            (int)smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ > (int)t_size_ ? 'C' : 'N');
      } else if (t_buysell_ == kTradeTypeSell && (int)smv_.base_ask_index_ >= ask_index_ &&
                 (int)smv_.base_ask_index_ - ask_index_ <= 10) {
        int level_ = smv_.base_ask_index_ - ask_index_;
        int int_price_level_ = smv_.base_ask_index_ - ask_index_;
        smv_.NotifyOnPLChangeListeners(
            t_security_id_, smv_.market_update_info_, t_buysell_, level_, int_price_, int_price_level_,
            smv_.market_update_info_.asklevels_[ask_index_].limit_size_ - t_size_,
            smv_.market_update_info_.asklevels_[ask_index_].limit_size_,
            smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ - 1,
            smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_, t_is_intermediate_,
            (int)smv_.market_update_info_.asklevels_[ask_index_].limit_size_ > (int)t_size_ ? 'C' : 'N');
      }
    }

#if CCPROFILING_TRADEINIT
    HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(14);
#endif

    // Notify relevant listeners about the update
    if (l1_price_changed_[t_security_id_]) {
      smv_.NotifyL1PriceListeners();
      smv_.NotifyOnReadyListeners();
      UpdateSanitizeVarsOnL1Change(t_security_id_);

      l1_price_changed_[t_security_id_] = false;
      l1_size_changed_[t_security_id_] = false;
    } else if (l1_size_changed_[t_security_id_]) {
      smv_.NotifyL1SizeListeners();
      smv_.NotifyOnReadyListeners();
      UpdateSanitizeVarsOnL1Change(t_security_id_);

      l1_size_changed_[t_security_id_] = false;
    } else {
      smv_.NotifyL2Listeners();
      smv_.NotifyL2OnlyListeners();
      smv_.NotifyOnReadyListeners();
    }
  }

  // We ignore simulated OrderAdd calls from queued orders for the purposes of maintaining maps.
  if (order_id_ > 0) {
    AddOrderToLiveOrdersMap(t_security_id_, t_buysell_, t_price_, t_size_, order_id_);
  }
}

bool IndexedNSEMarketViewManager::UpdateOrderMapsOnDelete(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                                          const uint64_t t_order_id_, const int t_size_,
                                                          const bool t_delete_order_) {
  std::map<uint64_t, order_details_struct_*>::iterator t_mapiter_;
  t_mapiter_ = sid_to_queued_orders_maps_vec_[t_security_id_].find(t_order_id_);
  if (t_mapiter_ != sid_to_queued_orders_maps_vec_[t_security_id_].end()) {
    order_details_struct_* t_que_order_ = (*t_mapiter_).second;
    //    DBGLOG_TIME_CLASS_FUNC_LINE << " RARE Case Orderid deleted " << t_que_order_->order_id_ << " "
    //                                << (t_que_order_->is_buy_order_ ? "B" : "S") << "  @ " <<
    //                                t_que_order_->order_price_
    //                                << DBGLOG_ENDL_FLUSH;
    // SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);
    //    DBGLOG_TIME_CLASS_FUNC_LINE << smv_.bid_size(0) << " @ " << smv_.bid_price(0) << " --- " << smv_.ask_price(0)
    //                                << " @ " << smv_.ask_size(0) << '\n';
    DBGLOG_DUMP;
    t_que_order_->last_order_size_ -= t_size_;
    if (t_que_order_->last_order_size_ < 0) {
      DBGLOG_TIME_CLASS_FUNC_LINE << "Potential problem Orderid " << t_order_id_ << '\n';
    }
    if (t_delete_order_ || t_que_order_->last_order_size_ <= 0) {
      sid_to_queued_orders_maps_vec_[t_security_id_].erase(t_mapiter_);
      order_struct_mempool_.DeAlloc(t_que_order_);
    }
    return true;
  }

  t_mapiter_ = sid_to_live_orders_maps_vec_[t_security_id_].find(t_order_id_);
  if (t_mapiter_ != sid_to_live_orders_maps_vec_[t_security_id_].end()) {
    order_details_struct_* t_que_order_ = (*t_mapiter_).second;
    t_que_order_->last_order_size_ -= t_size_;
    if (t_que_order_->last_order_size_ <= 0 || t_delete_order_) {
      sid_to_live_orders_maps_vec_[t_security_id_].erase(t_mapiter_);
      order_struct_mempool_.DeAlloc(t_que_order_);
    }
  }
  return false;
}

void IndexedNSEMarketViewManager::OnOrderDelete(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                                const uint64_t order_id_, const double t_price_, const int t_size_,
                                                const bool t_delete_order_, const bool t_is_intermediate_) {
  // del_px = t_price_ unless order delete is received at a different price than order price. In that case del_px =
  // original order price
  double del_px_ = t_price_;
  if (order_id_ != 0) OrderIsLive(t_security_id_, order_id_, &del_px_);

  // return in case delete corresponds to a queued order not yet reflected in book - rare case but happens sometimes -
  // NSE dissemination problems
  if (order_id_ != 0) {
    bool is_found_ = false;
    is_found_ = UpdateOrderMapsOnDelete(t_security_id_, t_buysell_, order_id_, t_size_, t_delete_order_);
    if (is_found_) return;
  }

  if (t_size_ == 0) {
    return;
  }

  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  int int_price_ = smv_.GetIntPx(del_px_);

  if (!smv_.initial_book_constructed_) {
    BuildIndex(t_security_id_, t_buysell_, int_price_);
  }

  int bid_index_ = 0;
  int ask_index_ = 0;

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      bid_index_ = (int)smv_.base_bid_index_ -
                   (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - int_price_);

      // Skip the message if we don't have any info for this level
      if (bid_index_ < 0) {
        // TODO _ should not happen
        return;
      }

      if (bid_index_ > (int)smv_.base_bid_index_) {
        // Error case
      } else if (bid_index_ == (int)smv_.base_bid_index_) {
        smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ -= t_size_;

        if (t_delete_order_) {
          smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_--;
        }

        l1_size_changed_[t_security_id_] = true;

        if (smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ <= 0 ||
            smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ <= 0) {
          // Set both size and order count to 0
          smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ = 0;
          smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ = 0;

          // L1 has changed
          int index_ = bid_index_ - 1;
          for (; index_ >= 0 && smv_.market_update_info_.bidlevels_[index_].limit_size_ <= 0; index_--)
            ;

          if (index_ >= 0) {
            smv_.base_bid_index_ = index_;

            // Check if we need to re-centre the index
            if (smv_.base_bid_index_ < LOW_ACCESS_INDEX) {
              RebuildIndexLowAccess(t_security_id_, t_buysell_,
                                    smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_);
            }
          } else {
            // There are 0 bid levels in the book right now. So, change the is_ready_ to false.
            smv_.is_ready_ = false;
          }

          l1_price_changed_[t_security_id_] = true;
        }
      } else {
        smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ -= t_size_;

        if (t_delete_order_) {
          smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_--;
        }

        if (smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ <= 0 ||
            smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ <= 0) {
          smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ = 0;
          smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ = 0;
        }

        // We need to mask the levels above this index
        else if (sec_id_to_mask_levels_above_[t_security_id_]) {
          for (int index_ = (int)smv_.base_bid_index_; index_ > bid_index_; index_--) {
            smv_.market_update_info_.bidlevels_[index_].limit_size_ = 0;
            smv_.market_update_info_.bidlevels_[index_].limit_ordercount_ = 0;
          }
          smv_.base_bid_index_ = bid_index_;

          if (smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ <= 0) {
            // L1 has changed
            int index_ = bid_index_ - 1;
            for (; index_ >= 0 && smv_.market_update_info_.bidlevels_[index_].limit_size_ <= 0; index_--)
              ;

            if (index_ >= 0) {
              smv_.base_bid_index_ = index_;
            } else {
              smv_.is_ready_ = false;
            }
          }

          // Check if we need to re-centre the index
          if (smv_.base_bid_index_ < LOW_ACCESS_INDEX) {
            RebuildIndexLowAccess(t_security_id_, t_buysell_,
                                  smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_);
          }

          l1_price_changed_[t_security_id_] = true;
        }
      }
    } break;
    case kTradeTypeSell: {
      ask_index_ = (int)smv_.base_ask_index_ +
                   (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - int_price_);

      // Skip the message if we don't have any info for this level
      if (ask_index_ < 0) {
        // TODO - make robust
        return;
      }

      if (ask_index_ > (int)smv_.base_ask_index_) {
        // Error case
      } else if (ask_index_ == (int)smv_.base_ask_index_) {
        smv_.market_update_info_.asklevels_[ask_index_].limit_size_ -= t_size_;

        if (t_delete_order_) {
          smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_--;
        }

        l1_size_changed_[t_security_id_] = true;

        if (smv_.market_update_info_.asklevels_[ask_index_].limit_size_ <= 0 ||
            smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ <= 0) {
          // Set both size and order count to 0
          smv_.market_update_info_.asklevels_[ask_index_].limit_size_ = 0;
          smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ = 0;

          // L1 has changed
          int index_ = ask_index_ - 1;
          for (; (index_ >= 0) && (smv_.market_update_info_.asklevels_[index_].limit_size_ <= 0); index_--)
            ;

          if (index_ >= 0) {
            smv_.base_ask_index_ = index_;

            // Check if we need to re-centre the index
            if (smv_.base_ask_index_ < LOW_ACCESS_INDEX) {
              RebuildIndexLowAccess(t_security_id_, t_buysell_,
                                    smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_);
            }
          } else {
            // There are 0 ask levels in the book right now. So, change the is_ready to false.
            smv_.is_ready_ = false;
          }

          l1_price_changed_[t_security_id_] = true;
        }
      } else {
        smv_.market_update_info_.asklevels_[ask_index_].limit_size_ -= t_size_;

        if (t_delete_order_) {
          smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_--;
        }

        if (smv_.market_update_info_.asklevels_[ask_index_].limit_size_ <= 0 ||
            smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ <= 0) {
          smv_.market_update_info_.asklevels_[ask_index_].limit_size_ = 0;
          smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ = 0;
        }

        // We need to mask the levels above this index
        else if (sec_id_to_mask_levels_above_[t_security_id_]) {
          for (int index_ = (int)smv_.base_ask_index_; index_ > ask_index_; index_--) {
            smv_.market_update_info_.asklevels_[index_].limit_size_ = 0;
            smv_.market_update_info_.asklevels_[index_].limit_ordercount_ = 0;
          }
          smv_.base_ask_index_ = ask_index_;

          if (smv_.market_update_info_.asklevels_[ask_index_].limit_size_ <= 0) {
            // L1 has changed
            int index_ = ask_index_ - 1;
            for (; index_ >= 0 && smv_.market_update_info_.asklevels_[index_].limit_size_ <= 0; index_--)
              ;

            if (index_ >= 0) {
              smv_.base_ask_index_ = index_;
            } else {
              smv_.is_ready_ = false;
            }
          }

          // Check if we need to re-centre the index
          if (smv_.base_ask_index_ < LOW_ACCESS_INDEX) {
            RebuildIndexLowAccess(t_security_id_, t_buysell_,
                                  smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_);
          }

          l1_price_changed_[t_security_id_] = true;
        }
      }
    } break;
    default:
      break;
  }

  if (l1_price_changed_[t_security_id_] || l1_size_changed_[t_security_id_]) {
    if (t_buysell_ == kTradeTypeBuy) {
      smv_.market_update_info_.bestbid_int_price_ =
          smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_;
      smv_.market_update_info_.bestbid_price_ = smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_price_;
      smv_.market_update_info_.bestbid_size_ = smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_;
      smv_.market_update_info_.bestbid_ordercount_ =
          smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_ordercount_;
      UpdateBestBidVariablesUsingOurOrders(t_security_id_);
    } else {
      smv_.market_update_info_.bestask_int_price_ =
          smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_;
      smv_.market_update_info_.bestask_price_ = smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_price_;
      smv_.market_update_info_.bestask_size_ = smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_;
      smv_.market_update_info_.bestask_ordercount_ =
          smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_ordercount_;
      UpdateBestAskVariablesUsingOurOrders(t_security_id_);
    }

    smv_.UpdateL1Prices();
  }

  if (smv_.market_update_info_.trade_update_implied_quote_) {
    smv_.market_update_info_.trade_update_implied_quote_ = false;
    smv_.trade_print_info_.num_trades_++;
  }

  sec_id_to_prev_update_was_quote_[t_security_id_] = true;

  // Skip notifying listeners if either the message is intermediate or smv is not in ready state
  // Also, skip if the message order exec message. Because this will undo the effect of exec_summary messages.
  if (!smv_.is_ready_ || t_is_intermediate_ || skip_notifications_[t_security_id_]) {
    return;
  }

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(14);
#endif

  if (CheckValidTime(t_security_id_)) {
    if (smv_.pl_change_listeners_present_) {
      if (t_buysell_ == kTradeTypeBuy && (int)smv_.base_bid_index_ >= bid_index_ &&
          (int)smv_.base_bid_index_ - bid_index_ <= 10) {
        int level_ = smv_.base_bid_index_ - bid_index_;
        int int_price_level_ = smv_.base_bid_index_ - bid_index_;
        smv_.NotifyOnPLChangeListeners(
            t_buysell_, smv_.market_update_info_, t_buysell_, level_, int_price_, int_price_level_,
            smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ + t_size_,
            smv_.market_update_info_.bidlevels_[bid_index_].limit_size_,
            smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ + 1,
            smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_, t_is_intermediate_,
            smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ == 0 ? 'D' : 'C');
      } else if (t_buysell_ == kTradeTypeSell && (int)smv_.base_ask_index_ >= ask_index_ &&
                 (int)smv_.base_ask_index_ - ask_index_ <= 10) {
        int level_ = smv_.base_ask_index_ - ask_index_;
        int int_price_level_ = smv_.base_ask_index_ - ask_index_;
        smv_.NotifyOnPLChangeListeners(
            t_buysell_, smv_.market_update_info_, t_buysell_, level_, int_price_, int_price_level_,
            smv_.market_update_info_.asklevels_[ask_index_].limit_size_ + t_size_,
            smv_.market_update_info_.asklevels_[ask_index_].limit_size_,
            smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ + 1,
            smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_, t_is_intermediate_,
            smv_.market_update_info_.asklevels_[ask_index_].limit_size_ == 0 ? 'D' : 'C');
      }
    }

#if CCPROFILING_TRADEINIT
    HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(14);
#endif

    // Notify relevant listeners about the update
    if (l1_price_changed_[t_security_id_]) {
      smv_.NotifyL1PriceListeners();
      smv_.NotifyOnReadyListeners();
      UpdateSanitizeVarsOnL1Change(t_security_id_);

      l1_price_changed_[t_security_id_] = false;
      l1_size_changed_[t_security_id_] = false;
    } else if (l1_size_changed_[t_security_id_]) {
      smv_.NotifyL1SizeListeners();
      smv_.NotifyOnReadyListeners();
      UpdateSanitizeVarsOnL1Change(t_security_id_);

      l1_size_changed_[t_security_id_] = false;
    } else {
      smv_.NotifyL2Listeners();
      smv_.NotifyL2OnlyListeners();
      smv_.NotifyOnReadyListeners();
    }
  }
}

// Handling Order Modify message:
void IndexedNSEMarketViewManager::OnOrderModify(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                                const uint64_t order_id_, const double t_price_, const uint32_t t_size_,
                                                const double t_prev_price_, const uint32_t t_prev_size_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  if (smv_.DblPxCompare(t_prev_price_, t_price_)) {
    // 1. If the prices are same, simply change the size
    OnOrderDelete(t_security_id_, t_buysell_, order_id_, t_prev_price_, (int)t_prev_size_ - (int)t_size_, false, false);
    if (order_id_ > 0) UpdateOrderMapsOnModify(t_security_id_, order_id_, t_size_);
  } else {
    // since prices are different
    // delete the previous order and add a new one while keeping intermediate flag in delete on so that we don't notify
    // listeners upon delete message
    OnOrderDelete(t_security_id_, t_buysell_, order_id_, t_prev_price_, t_prev_size_, true, true);
    OnOrderAdd(t_security_id_, t_buysell_, order_id_, t_price_, t_size_, false);
  }
}

void IndexedNSEMarketViewManager::OnTrade(const unsigned int t_security_id_, double const t_trade_price_,
                                          const int t_trade_size_, const uint64_t t_buy_order_num_,
                                          const uint64_t t_sell_order_num_, const int32_t t_bid_size_remaining_,
                                          const int32_t t_ask_size_remaining_) {
  try {
  SecurityMarketView& smv_ = *(security_market_view_map_.at(t_security_id_));
  if (!smv_.initial_book_constructed_) {
    return;
  }
  // notify all rawtradelisteners of this tradeprint
  if (CheckValidTime(t_security_id_)) {
    if (smv_.is_ready_) {
      // set the primary variables, we dont care about market_update_info
      smv_.raw_trade_print_info_.trade_price_ = t_trade_price_;
      smv_.raw_trade_print_info_.size_traded_ = t_trade_size_;
      smv_.raw_trade_print_info_.int_trade_price_ = smv_.GetIntPx(t_trade_price_);

      smv_.NotifyRawTradeListeners();
    }
  }
  // Storing last trade info
  last_trade_buy_order_num_ = t_buy_order_num_;
  last_trade_buy_size_remaining_ = t_bid_size_remaining_;
  last_trade_sell_order_num_ = t_sell_order_num_;
  last_trade_sell_size_remaining_ = t_ask_size_remaining_;

  // If neither of these orders are reflected in the book then simply check if any queued order needs to be modified and
  // return. Size remaining -1 is only for orders which have not been seen by daemon/propagated by convertor.
  if ((t_bid_size_remaining_ == -1 || OrderIsQueued(t_security_id_, t_buy_order_num_)) &&
      (t_ask_size_remaining_ == -1 || OrderIsQueued(t_security_id_, t_sell_order_num_))) {
    UpdateOrderMapsOnTrade(t_security_id_, t_buy_order_num_, t_sell_order_num_, t_trade_size_, t_trade_price_);
    return;
  }
  int t_trade_int_price_ = smv_.GetIntPx(t_trade_price_);
  // If visible order gets executed at a different price, then visible order would have been very recently added and
  // will be in queue.
  // update queue entry and return - we don't delete twice
  {
    double t_temp_price_ = 0;
    if ((t_trade_int_price_ <= smv_.market_update_info_.bestbid_int_price_ &&
         OrderIsLive(t_security_id_, t_buy_order_num_, &t_temp_price_) &&
         !smv_.DblPxCompare(t_temp_price_, t_trade_price_)) ||
        (t_trade_int_price_ >= smv_.market_update_info_.bestask_int_price_ &&
         OrderIsLive(t_security_id_, t_sell_order_num_, &t_temp_price_) &&
         !smv_.DblPxCompare(t_temp_price_, t_trade_price_))) {
      UpdateOrderMapsOnTrade(t_security_id_, t_buy_order_num_, t_sell_order_num_, t_trade_size_, t_trade_price_);
      return;
    }
  }
  TradeType_t t_buysell_;

  if (smv_.trade_print_info_.computing_last_book_tdiff_) {
    if (sec_id_to_prev_update_was_quote_.at(t_security_id_)) {  // the difference between
                                                             // last_book_mkt_size_weighted_price_ and
                                                             // mkt_size_weighted_price_ is that
      // in case trade_before_quote exchanges  where we might have a number of
      // back to back trade messages, the last_book_mkt_size_weighted_price_ is the snapshot of mkt_size_weighted_price_
      // the last time the update was a book message
      smv_.market_update_info_.last_book_mkt_size_weighted_price_ = smv_.market_update_info_.mkt_size_weighted_price_;
    }
  }

  if (!smv_.market_update_info_.trade_update_implied_quote_) {
    smv_.market_update_info_.trade_update_implied_quote_ = true;
  }
  smv_.StorePreTrade();
  /// Case 1: aggressive sell @ best bid price
  if (t_trade_int_price_ == smv_.market_update_info_.bestbid_int_price_) {
    t_buysell_ = kTradeTypeSell;

    int trade_index_ =
        (int)smv_.base_bid_index_ -
        (smv_.market_update_info_.bidlevels_.at(smv_.base_bid_index_).limit_int_price_ - t_trade_int_price_);
    // check whether entire level is deleted or part thereof
    // If the trade size is big enough to clear this level
    if ((int)t_trade_size_ >= (int)smv_.market_update_info_.bidlevels_.at(smv_.base_bid_index_).limit_size_) {
      // This level is cleared now. So, find the next level
      int next_bid_index_ = smv_.base_bid_index_ - 1;
      for (; next_bid_index_ >= 0 && smv_.market_update_info_.bidlevels_.at(next_bid_index_).limit_size_ <= 0;
           next_bid_index_--)
        ;
      // If the next_ask_index_ is a valid one, update the best variables with that level
      if (next_bid_index_ >= 0 /*&& trade_index_ - next_bid_index_ <= 10*/) {
        smv_.market_update_info_.bestbid_int_price_ =
            smv_.market_update_info_.bidlevels_.at(next_bid_index_).limit_int_price_;
        smv_.market_update_info_.bestbid_size_ = smv_.market_update_info_.bidlevels_.at(next_bid_index_).limit_size_;
        smv_.market_update_info_.bestbid_price_ = smv_.market_update_info_.bidlevels_.at(next_bid_index_).limit_price_;
        smv_.market_update_info_.bestbid_ordercount_ =
            smv_.market_update_info_.bidlevels_.at(next_bid_index_).limit_ordercount_;

      } else {
        /// SMV should be set to not-ready as side is cleared
        smv_.is_ready_ = false;
      }

      // Set base_bid_index_ size and order count to 0
      smv_.market_update_info_.bidlevels_.at(smv_.base_bid_index_).limit_size_ = 0;
      smv_.market_update_info_.bidlevels_.at(smv_.base_bid_index_).limit_ordercount_ = 0;
      if (next_bid_index_ > 0) {
        smv_.base_bid_index_ = next_bid_index_;
      } else {
        smv_.base_bid_index_ = 0;
      }
    }
    // Level not cleared
    else {
      // Update the book here
      smv_.market_update_info_.bidlevels_.at(trade_index_).limit_size_ -= (int)t_trade_size_;
      double t_temp_px_;
      if (t_bid_size_remaining_ == 0 && OrderIsLive(t_security_id_, t_buy_order_num_, &t_temp_px_)) {
        smv_.market_update_info_.bidlevels_.at(trade_index_).limit_ordercount_ -= 1;
      }
      smv_.market_update_info_.bestbid_int_price_ = smv_.market_update_info_.bidlevels_.at(trade_index_).limit_int_price_;
      smv_.market_update_info_.bestbid_size_ = smv_.market_update_info_.bidlevels_.at(trade_index_).limit_size_;
      smv_.market_update_info_.bestbid_price_ = smv_.market_update_info_.bidlevels_.at(trade_index_).limit_price_;
      smv_.market_update_info_.bestbid_ordercount_ =
          smv_.market_update_info_.bidlevels_.at(trade_index_).limit_ordercount_;
    }
    UpdateBestBidVariablesUsingOurOrders(t_security_id_);
    smv_.UpdateL1Prices();

#if CCPROFILING_TRADEINIT
    HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(14);
#endif

    if (smv_.pl_change_listeners_present_) {
      bool level_cleared_ = ((int)smv_.base_bid_index_ != trade_index_);
      smv_.NotifyOnPLChangeListeners(
          t_security_id_, smv_.market_update_info_, kTradeTypeBuy, 0, t_trade_int_price_, 0,
          smv_.market_update_info_.bidlevels_.at(trade_index_).limit_size_ + t_trade_size_,
          smv_.market_update_info_.bidlevels_.at(trade_index_).limit_size_,
          level_cleared_ ? 0 : smv_.market_update_info_.bidlevels_.at(trade_index_).limit_ordercount_,
          smv_.market_update_info_.bidlevels_.at(trade_index_).limit_ordercount_, false, level_cleared_ ? 'D' : 'C');
    }
    // set the primary variables
    smv_.trade_print_info_.trade_price_ = t_trade_price_;
    smv_.trade_print_info_.size_traded_ = t_trade_size_;
    smv_.trade_print_info_.int_trade_price_ = t_trade_int_price_;
    smv_.trade_print_info_.buysell_ = t_buysell_;

    smv_.SetTradeVarsForIndicatorsIfRequired();
  }

  // Case 2: Aggressive buy at best ask price
  else if (t_trade_int_price_ == smv_.market_update_info_.bestask_int_price_) {
    t_buysell_ = kTradeTypeBuy;
    // Calculate level index for the trade price
    int trade_index_ =
        (int)smv_.base_ask_index_ +
        (smv_.market_update_info_.asklevels_.at(smv_.base_ask_index_).limit_int_price_ - t_trade_int_price_);
    // check whether entire level is deleted or part thereof
    // If the trade size is big enough to clear this level
    if ((int)t_trade_size_ >= (int)smv_.market_update_info_.asklevels_.at(smv_.base_ask_index_).limit_size_) {
      // This level is cleared now. So, find the next level
      int next_ask_index_ = smv_.base_ask_index_ - 1;
      for (; next_ask_index_ >= 0 && smv_.market_update_info_.asklevels_.at(next_ask_index_).limit_size_ <= 0;
           next_ask_index_--)
        ;
      // If the next_ask_index_ is a valid one, update the best variables with that level
      if (next_ask_index_ >= 0 /*&& trade_index_ - next_ask_index_ <= 10 */) {
        smv_.market_update_info_.bestask_int_price_ =
            smv_.market_update_info_.asklevels_.at(next_ask_index_).limit_int_price_;
        smv_.market_update_info_.bestask_size_ = smv_.market_update_info_.asklevels_.at(next_ask_index_).limit_size_;
        smv_.market_update_info_.bestask_price_ = smv_.market_update_info_.asklevels_.at(next_ask_index_).limit_price_;
        smv_.market_update_info_.bestask_ordercount_ =
            smv_.market_update_info_.asklevels_.at(next_ask_index_).limit_ordercount_;
      } else {
        /// SMV should be set to not-ready as side is cleared
        smv_.is_ready_ = false;
      }

      // Set base_ask_index_ size and order count to 0
      smv_.market_update_info_.asklevels_.at(smv_.base_ask_index_).limit_size_ = 0;
      smv_.market_update_info_.asklevels_.at(smv_.base_ask_index_).limit_ordercount_ = 0;

      if (next_ask_index_ > 0) {
        smv_.base_ask_index_ = next_ask_index_;
      } else {
        smv_.base_ask_index_ = 0;
      }
    }

    else  // level not cleared
    {
      // Update the book here
      smv_.market_update_info_.asklevels_.at(trade_index_).limit_size_ -= (int)t_trade_size_;
      // order is visible in book
      double t_temp_px_;
      if (t_ask_size_remaining_ == 0 && OrderIsLive(t_security_id_, t_sell_order_num_, &t_temp_px_)) {
        smv_.market_update_info_.asklevels_.at(trade_index_).limit_ordercount_ -= 1;
      }

      smv_.market_update_info_.bestask_int_price_ = smv_.market_update_info_.asklevels_.at(trade_index_).limit_int_price_;
      smv_.market_update_info_.bestask_size_ = smv_.market_update_info_.asklevels_.at(trade_index_).limit_size_;
      smv_.market_update_info_.bestask_price_ = smv_.market_update_info_.asklevels_.at(trade_index_).limit_price_;
      smv_.market_update_info_.bestask_ordercount_ =
          smv_.market_update_info_.asklevels_.at(trade_index_).limit_ordercount_;
    }
    UpdateBestAskVariablesUsingOurOrders(t_security_id_);
    smv_.UpdateL1Prices();

#if CCPROFILING_TRADEINIT
    HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(14);
#endif

    if (smv_.pl_change_listeners_present_) {
      bool level_cleared_ = ((int)smv_.base_ask_index_ != trade_index_);
      smv_.NotifyOnPLChangeListeners(
          t_security_id_, smv_.market_update_info_, kTradeTypeSell, 0, t_trade_int_price_, 0,
          smv_.market_update_info_.asklevels_.at(trade_index_).limit_size_ + t_trade_size_,
          smv_.market_update_info_.bidlevels_.at(trade_index_).limit_size_,
          level_cleared_ ? 0 : smv_.market_update_info_.asklevels_.at(trade_index_).limit_ordercount_,
          smv_.market_update_info_.asklevels_.at(trade_index_).limit_ordercount_, false, level_cleared_ ? 'D' : 'C');
    }

    // set the primary variables
    smv_.trade_print_info_.trade_price_ = t_trade_price_;
    smv_.trade_print_info_.size_traded_ = t_trade_size_;
    smv_.trade_print_info_.int_trade_price_ = t_trade_int_price_;
    smv_.trade_print_info_.buysell_ = t_buysell_;
    smv_.SetTradeVarsForIndicatorsIfRequired();
  }

  /// Case 3: trade between L1 prices - will happen occasionally due to non-standard order interplay
  else if (t_trade_int_price_ > smv_.market_update_info_.bestbid_int_price_ &&
           t_trade_int_price_ < smv_.market_update_info_.bestask_int_price_) {
    // set the primary variables - TODO set buysell appropriately
    smv_.trade_print_info_.trade_price_ = t_trade_price_;
    smv_.trade_print_info_.size_traded_ = t_trade_size_;
    smv_.trade_print_info_.int_trade_price_ = t_trade_int_price_;
    smv_.trade_print_info_.buysell_ = kTradeTypeBuy;
  }

  /// Case 4: Buy trade at higher than best ask price - we don't sanitize
  else if (t_trade_int_price_ > smv_.market_update_info_.bestask_int_price_) {
    //    DBGLOG_TIME_CLASS_FUNC_LINE << " sub-best buy trade " << t_trade_size_ << " @ " << t_trade_price_ << " \tBook
    //    "
    //                                << smv_.bid_size(0) << " @ " << smv_.bid_price(0) << " ---- " << smv_.ask_price(0)
    //                                << " @ " << smv_.ask_size(0) << DBGLOG_ENDL_FLUSH;
    //    DBGLOG_DUMP;
    t_buysell_ = kTradeTypeBuy;
    // Call sanitize function and return if book was sanitized.
    bool was_sanitized_ =
        CheckCrossedBookInstance(t_security_id_, kTradeTypeSell, smv_.market_update_info_.bestask_int_price_,
                                 t_trade_int_price_, smv_.market_update_info_.bestask_size_);
    if (was_sanitized_) return;

    int trade_index_ =
        (int)smv_.base_ask_index_ +
        (smv_.market_update_info_.asklevels_.at(smv_.base_ask_index_).limit_int_price_ - t_trade_int_price_);
    if (trade_index_ < 0) {
      DBGLOG_TIME_CLASS_FUNC_LINE << " Important Issue: tradeintpx " << t_trade_int_price_ << " ask limitpx "
                                  << smv_.market_update_info_.asklevels_.at(smv_.base_ask_index_).limit_int_price_
                                  << " base_index " << (int)smv_.base_ask_index_ << " trade_index "
                                  << DBGLOG_ENDL_FLUSH;
      return;
    }
    if (smv_.market_update_info_.asklevels_.at(trade_index_).limit_size_ <= t_trade_size_) {
      smv_.market_update_info_.asklevels_.at(trade_index_).limit_size_ = 0;

    } else {
      smv_.market_update_info_.asklevels_.at(trade_index_).limit_size_ -= t_trade_size_;
      double t_temp_px_;
      if (t_ask_size_remaining_ == 0 && OrderIsLive(t_security_id_, t_sell_order_num_, &t_temp_px_)) {
        smv_.market_update_info_.asklevels_.at(trade_index_).limit_ordercount_ -= 1;
      }
    }

    smv_.trade_print_info_.trade_price_ = t_trade_price_;
    smv_.trade_print_info_.size_traded_ = t_trade_size_;
    smv_.trade_print_info_.int_trade_price_ = t_trade_int_price_;
    smv_.trade_print_info_.buysell_ = kTradeTypeBuy;

    // TODO - put in support for PL callback
    smv_.SetTradeVarsForIndicatorsIfRequired();

  }
  /// Case 5 aggressive sell at sub L1 level
  else {
    //    DBGLOG_TIME_CLASS_FUNC_LINE << " sub-best sell trade " << t_trade_size_ << " @ " << t_trade_price_ << " \tBook
    //    "
    //                                << smv_.bid_size(0) << " @ " << smv_.bid_price(0) << " ---- " << smv_.ask_price(0)
    //                                << " @ " << smv_.ask_size(0) << DBGLOG_ENDL_FLUSH;
    //    DBGLOG_DUMP;
    t_buysell_ = kTradeTypeSell;
    bool was_sanitized_ =
        CheckCrossedBookInstance(t_security_id_, kTradeTypeBuy, smv_.market_update_info_.bestbid_int_price_,
                                 t_trade_int_price_, smv_.market_update_info_.bestbid_size_);
    if (was_sanitized_) return;

    int trade_index_ =
        (int)smv_.base_bid_index_ -
        (smv_.market_update_info_.bidlevels_.at(smv_.base_bid_index_).limit_int_price_ - t_trade_int_price_);
    if (trade_index_ <= 0) {
      DBGLOG_TIME_CLASS_FUNC_LINE << " Important Issue: tradeintpx " << t_trade_int_price_ << " limitpx "
                                  << smv_.market_update_info_.bidlevels_.at(smv_.base_bid_index_).limit_int_price_
                                  << " base_index " << (int)smv_.base_bid_index_ << " trade_index " << trade_index_
                                  << DBGLOG_ENDL_FLUSH;
      return;
    }
    if (smv_.market_update_info_.bidlevels_.at(trade_index_).limit_size_ <= t_trade_size_) {
      smv_.market_update_info_.bidlevels_.at(trade_index_).limit_size_ = 0;
    } else {
      smv_.market_update_info_.bidlevels_.at(trade_index_).limit_size_ -= t_trade_size_;
      double t_temp_px_;
      if (t_bid_size_remaining_ == 0 && OrderIsLive(t_security_id_, t_buy_order_num_, &t_temp_px_)) {
        smv_.market_update_info_.bidlevels_.at(trade_index_).limit_ordercount_ -= 1;
      }
    }

    smv_.trade_print_info_.trade_price_ = t_trade_price_;
    smv_.trade_print_info_.size_traded_ = t_trade_size_;
    smv_.trade_print_info_.int_trade_price_ = t_trade_int_price_;
    smv_.trade_print_info_.buysell_ = t_buysell_;
    // TODO - put in support for PL callback
    smv_.SetTradeVarsForIndicatorsIfRequired();

    if (smv_.base_bid_index_ < LOW_ACCESS_INDEX) {
      RebuildIndexLowAccess(t_security_id_, kTradeTypeBuy,
                            smv_.market_update_info_.bidlevels_.at(smv_.base_bid_index_).limit_int_price_);
    }
  }

  if (CheckValidTime(t_security_id_)) {
// Same as SecurityMarketView::OnTrade
#if CCPROFILING_TRADEINIT
    HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(14);
#endif
    if (smv_.is_ready_) {
      smv_.NotifyTradeListeners();
      smv_.NotifyOnReadyListeners();
      /// check if l1listener calls are needed as well
    }
  }
  UpdateOrderMapsOnTrade(t_security_id_, t_buy_order_num_, t_sell_order_num_, t_trade_size_, t_trade_price_);
 }
 catch (const std::out_of_range& oor) {
    std::cout << "ERRORMEM Out of Range error: " << oor.what() << std::endl;
  }
 catch (...) {
    std::cout << "ERRORMEM Default Exception occurred"<<std::endl;
 }
}

void IndexedNSEMarketViewManager::OnGlobalOrderChange(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                                      const int t_int_price_) {}

void IndexedNSEMarketViewManager::SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_) {
  for (auto i = 0u; i < security_market_view_map_.size(); i++) {
    SecurityMarketView* smv_ = security_market_view_map_[i];
    smv_->set_skip_listener_notification_end_time(r_start_time_);
  }
}

/* This function is called if we run across a sanitize condition in historical mode.
   It loads a list of bad days from static file in tradeinfo. */
void IndexedNSEMarketViewManager::LoadCrossedDataDays() {
  std::string nse_crossed_data_days_filepath_ = std::string("/spare/local/tradeinfo/") + NSE_CROSSED_DATA_DAYS_FILE;
  if (FileUtils::ExistsAndReadable(nse_crossed_data_days_filepath_)) {
    std::ifstream nse_crossed_data_days_file_;
    nse_crossed_data_days_file_.open(nse_crossed_data_days_filepath_.c_str(), std::ifstream::in);
    char readline_buffer_[1024];
    if (nse_crossed_data_days_file_.is_open()) {
      while (nse_crossed_data_days_file_.good()) {
        memset(readline_buffer_, 0, sizeof(readline_buffer_));
        nse_crossed_data_days_file_.getline(readline_buffer_, sizeof(readline_buffer_));
        PerishableStringTokenizer st_(readline_buffer_, sizeof(readline_buffer_));

        const std::vector<const char*>& tokens_ = st_.GetTokens();
        if (tokens_.size() == 1 && tokens_[0][0] != '#') {
          DBGLOG_TIME_CLASS_FUNC_LINE << " Pushing value in crossed data days " << tokens_[0] << DBGLOG_ENDL_FLUSH;
          crossed_data_days_.insert(atoi(tokens_[0]));
        }
      }
    }
    nse_crossed_data_days_file_.close();
  } else {
    DBGLOG_TIME_CLASS_FUNC_LINE << " ALERT: Could not read file " << nse_crossed_data_days_filepath_
                                << " will continue with empty crossed data days vector " << DBGLOG_ENDL_FLUSH;
  }
}

/* Returns true if running in live. Takes diff of current time ( gettimeofday ) and watch
   to figure that out */
bool IndexedNSEMarketViewManager::IsRunningInLive() {
  timeval live_tv_;
  gettimeofday(&live_tv_, NULL);
  DBGLOG_TIME_CLASS_FUNC_LINE << " Delay computed " << live_tv_.tv_sec - watch_.tv().tv_sec << DBGLOG_ENDL_FLUSH;
  return (fabs(live_tv_.tv_sec - watch_.tv().tv_sec) < 300);
}

/* This function sends an alert email and adds the current date to the crossed days file */
void IndexedNSEMarketViewManager::AlertInHist(const uint32_t t_security_id_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  std::ostringstream t_oss_;
  t_oss_ << "Sanitization Called for NSE Instrument " << smv_.shortcode() << " in HIST for " << watch_.YYYYMMDD() << ' '
         << "Packet Drops Suspected in Logging \n"
         << "Please Check and Validate\n";

  DBGLOG_TIME_CLASS_FUNC_LINE << t_oss_.str() << DBGLOG_ENDL_FLUSH;

  // add date to days file
  std::string nse_crossed_data_days_filepath_ = std::string("/spare/local/tradeinfo/") + NSE_CROSSED_DATA_DAYS_FILE;
  if (FileUtils::ExistsAndReadable(nse_crossed_data_days_filepath_)) {
    std::ofstream nse_crossed_data_days_file_(nse_crossed_data_days_filepath_, std::ios_base::app | std::ios_base::out);
    nse_crossed_data_days_file_ << watch_.YYYYMMDD() << '\n';
    nse_crossed_data_days_file_.close();
  } else {
    DBGLOG_TIME_CLASS_FUNC_LINE << " ALERT: Could not open the following file to add crossed day "
                                << nse_crossed_data_days_filepath_ << DBGLOG_ENDL_FLUSH;
  }
}

/* This function sends an alert email in Live
   TODO - add support for Slack or other more efficient alert mechanisms */
void IndexedNSEMarketViewManager::AlertInLive(const uint32_t t_security_id_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  DBGLOG_TIME_CLASS_FUNC_LINE << "CRITCAL: Sent Sanitize email for " << smv_.shortcode() << " in LIVE "
                              << DBGLOG_ENDL_FLUSH;
}

/* This function is called if we have determined that the book needs to be sanitized.
   We do a couple of things here
( a ) We delete underlying indexed book levels upto and including offending trade price
( b ) We delete any offending orders in live and queued map
( c ) We call simulate post these changes so that queued orders can be flushed if needed */
void IndexedNSEMarketViewManager::SanitizeBook(const uint32_t t_security_id_, const int t_sanitize_price_) {
  // First correct underlying indexed book [ (a) above ]
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);
  /*
  DBGLOG_TIME_CLASS_FUNC_LINE << "Book state before Sanitize: " << smv_.market_update_info_.bestbid_size_ << " @ "
                              << smv_.market_update_info_.bestbid_price_ << " --- "
                              << smv_.market_update_info_.bestask_price_ << " @ "
                              << smv_.market_update_info_.bestask_size_ << DBGLOG_ENDL_FLUSH;
  */
  // Side to sanitize is the side where orders will be deleted, i.e. kTradeTypeBuy means we delete bid levels
  TradeType_t side_to_sanitize_ = sid_to_side_to_be_sanitized_[t_security_id_];
  int t_index_to_sanitize_to_ = 0;
  switch (side_to_sanitize_) {
    case kTradeTypeBuy: {
      t_index_to_sanitize_to_ = std::max(
          0, (int)smv_.base_bid_index_ -
                 (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - t_sanitize_price_));
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
      t_index_to_sanitize_to_ = std::max(
          0, (int)smv_.base_ask_index_ +
                 (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - t_sanitize_price_));
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
  /*
  DBGLOG_TIME_CLASS_FUNC_LINE << "Book state after Sanitize: " << smv_.market_update_info_.bestbid_size_ << " @ "
                              << smv_.market_update_info_.bestbid_price_ << " --- "
                              << smv_.market_update_info_.bestask_price_ << " @ "
                              << smv_.market_update_info_.bestask_size_ << DBGLOG_ENDL_FLUSH;

  DBGLOG_TIME_CLASS_FUNC_LINE << " Prior to vectors sanitize: Queued vector size "
                              << sid_to_queued_orders_maps_vec_[t_security_id_].size() << " Live vector size "
                              << sid_to_live_orders_maps_vec_[t_security_id_].size() << DBGLOG_ENDL_FLUSH;
  */
  // Next delete offending orders in live and queued maps [ (b) above ]
  std::map<uint64_t, order_details_struct_*>::iterator t_mapiter_;
  std::map<uint64_t, order_details_struct_*>::iterator t_deliter_;

  for (t_mapiter_ = sid_to_queued_orders_maps_vec_[t_security_id_].begin();
       t_mapiter_ != sid_to_queued_orders_maps_vec_[t_security_id_].end();) {
    order_details_struct_* t_que_order_ = (*t_mapiter_).second;
    // queued order contradicting bestprice is deleted
    if ((t_que_order_->is_buy_order_ && side_to_sanitize_ == kTradeTypeBuy &&
         (smv_.base_bid_index_ == 0 || t_que_order_->order_price_ > smv_.market_update_info_.bestbid_price_ + 1e-5)) ||
        (!t_que_order_->is_buy_order_ && side_to_sanitize_ == kTradeTypeSell &&
         (smv_.base_ask_index_ == 0 || t_que_order_->order_price_ < smv_.market_update_info_.bestask_price_ - 1e-5))) {
      t_deliter_ = t_mapiter_;
      t_mapiter_++;
      sid_to_queued_orders_maps_vec_[t_security_id_].erase(t_deliter_);
    } else {
      t_mapiter_++;
    }
  }

  for (t_mapiter_ = sid_to_live_orders_maps_vec_[t_security_id_].begin();
       t_mapiter_ != sid_to_live_orders_maps_vec_[t_security_id_].end();) {
    order_details_struct_* t_live_order_ = (*t_mapiter_).second;
    // live order contradicting bestprice is deleted
    if ((t_live_order_->is_buy_order_ && side_to_sanitize_ == kTradeTypeBuy &&
         (smv_.base_bid_index_ == 0 || t_live_order_->order_price_ > smv_.market_update_info_.bestbid_price_ + 1e-5)) ||
        (!t_live_order_->is_buy_order_ && side_to_sanitize_ == kTradeTypeSell &&
         (smv_.base_ask_index_ == 0 || t_live_order_->order_price_ < smv_.market_update_info_.bestask_price_ - 1e-5))) {
      t_deliter_ = t_mapiter_;
      t_mapiter_++;
      sid_to_live_orders_maps_vec_[t_security_id_].erase(t_deliter_);
    } else {
      t_mapiter_++;
    }
  }
  DBGLOG_TIME_CLASS_FUNC_LINE << " Post vectors sanitize: Queued vector size "
                              << sid_to_queued_orders_maps_vec_[t_security_id_].size() << " Live vector size "
                              << sid_to_live_orders_maps_vec_[t_security_id_].size() << DBGLOG_ENDL_FLUSH;

  // Finally call simulate to flush correct orders that had been queued [ (c) above ]
  AddSimulatedOrders(t_security_id_);
  DBGLOG_TIME_CLASS_FUNC_LINE << "Book state after AddSimulated in  Sanitize: "
                              << smv_.market_update_info_.bestbid_size_ << " @ "
                              << smv_.market_update_info_.bestbid_price_ << " --- "
                              << smv_.market_update_info_.bestask_price_ << " @ "
                              << smv_.market_update_info_.bestask_size_ << DBGLOG_ENDL_FLUSH;
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
    if (!checked_for_live_) {
      is_live_ = IsRunningInLive();
      checked_for_live_ = true;
      if (!is_live_) {
        LoadCrossedDataDays();
      }
    }
    if (is_live_) {
      AlertInLive(t_security_id_);
    } else if (!sent_alarm_in_hist_) {
      if (crossed_data_days_.end() == crossed_data_days_.find(watch_.YYYYMMDD())) {
        AlertInHist(t_security_id_);
      }
      sent_alarm_in_hist_ = true;  // single email should be sent
    }
    return true;
  }
  return false;
}

/* Resets sanitization check if price or size of level tagged as potential sanitize case changes
   Minimizes occurences of false positives */
void IndexedNSEMarketViewManager::UpdateSanitizeVarsOnL1Change(const uint32_t t_security_id_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  if ((sid_to_int_px_to_sanitize_[t_security_id_] != -1) &&
      ((((sid_to_int_px_to_sanitize_[t_security_id_] != smv_.market_update_info_.bestbid_int_price_) ||
         (sid_to_size_at_sanitize_px_[t_security_id_] != smv_.market_update_info_.bestbid_size_)) &&
        sid_to_side_to_be_sanitized_[t_security_id_] == kTradeTypeBuy) ||
       (((sid_to_int_px_to_sanitize_[t_security_id_] != smv_.market_update_info_.bestask_int_price_) ||
         (sid_to_size_at_sanitize_px_[t_security_id_] != smv_.market_update_info_.bestask_size_)) &&
        sid_to_side_to_be_sanitized_[t_security_id_] == kTradeTypeSell))) {
    sid_to_int_px_to_sanitize_[t_security_id_] = -1;
  }
}

// Checks and returns if current time is trading time for the product
bool IndexedNSEMarketViewManager::CheckValidTime(int sec_id) {
  currently_trading_[sec_id] = trade_time_manager_.isValidTimeToTrade(sec_id, watch_.tv().tv_sec % 86400);
  return currently_trading_[sec_id];
}
}
