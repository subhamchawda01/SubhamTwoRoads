// =====================================================================================
//
//       Filename:  indexed_hkomd_market_view_namager.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  Monday 05 May 2014 09:22:09  GMT
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================
#include "baseinfra/MarketAdapter/indexed_hkomd_market_view_manager.hpp"
/*Add Order 330 :
  OrderID
  Price
  Quantity
  Side 0(Bid)/1(Offer)
  OrderBookPosition

  Modify Order 331 :
  OrderID
  Price
  Quantity
  Side 0/1
  OrderBookPosition

  Delete Order 332 :
  OrderID
  Side 0/1

  Trade 350
  OrderID
  Price
  Side 0/1/2/3
  DealType 0/1/2/4
  TradeCondition 1/2/8/16
  Quantity
 */

namespace HFSAT {
#define HIGH_ACCESS_INDEX 1023
#define LOW_ACCESS_INDEX 50

HKOMDIndexedMarketViewManager::HKOMDIndexedMarketViewManager(
    DebugLogger& t_dbglogger_, const Watch& t_watch_, const SecurityNameIndexer& t_sec_name_indexer_,
    const std::vector<SecurityMarketView*>& t_security_market_view_map_)
    : BaseMarketViewManager(t_dbglogger_, t_watch_, t_sec_name_indexer_, t_security_market_view_map_),
      bidside_oid_to_qty_map_vec_(t_sec_name_indexer_.NumSecurityId()),
      askside_oid_to_qty_map_vec_(t_sec_name_indexer_.NumSecurityId()),
      l1_price_changed_(t_sec_name_indexer_.NumSecurityId(), false),
      l1_size_changed_(t_sec_name_indexer_.NumSecurityId(), false)
// sec_id_to_mask_levels_above_ ( t_sec_name_indexer_.NumSecurityId ( ), false )
{}

void HKOMDIndexedMarketViewManager::OnOrderAdd(const uint32_t t_security_id_, const uint64_t t_order_id_,
                                               const double t_price_,  // sent as int32_t changed to double using ref
                                               const uint32_t t_size_, const TradeType_t t_buysell_,
                                               const bool t_is_intermediate_) {
  if (t_buysell_ == kTradeTypeBuy)  // bid
  {
    if (bidside_oid_to_qty_map_vec_[t_security_id_].find(t_order_id_) !=
        bidside_oid_to_qty_map_vec_[t_security_id_].end()) {
      DBGLOG_TIME_CLASS_FUNC << "Duplicate security_id: " << t_security_id_ << "order_id: " << t_order_id_
                             << " side: " << t_buysell_ << DBGLOG_ENDL_FLUSH;
    }
    bidside_oid_to_qty_map_vec_[t_security_id_][t_order_id_].size_ = t_size_;
    bidside_oid_to_qty_map_vec_[t_security_id_][t_order_id_].price_ = t_price_;
  } else {
    if (askside_oid_to_qty_map_vec_[t_security_id_].find(t_order_id_) !=
        askside_oid_to_qty_map_vec_[t_security_id_].end()) {
      DBGLOG_TIME_CLASS_FUNC << "Duplicate security_id: " << t_security_id_ << "order_id: " << t_order_id_
                             << " side: " << t_buysell_ << DBGLOG_ENDL_FLUSH;
    }
    askside_oid_to_qty_map_vec_[t_security_id_][t_order_id_].size_ = t_size_;
    askside_oid_to_qty_map_vec_[t_security_id_][t_order_id_].price_ = t_price_;
  }

  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);
  int int_price_ = smv_.GetIntPx(t_price_);

  if (!smv_.initial_book_constructed_) {
    BuildIndex(t_security_id_, t_buysell_, int_price_);
  }

  int bid_index_ = 0;
  int ask_index_ = 0;

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      bid_index_ = (int)smv_.base_bid_index_ -
                   (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - int_price_);

      if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ <= 0) {
        if (bid_index_ < LOW_ACCESS_INDEX) {
          RebuildIndexLowAccess(t_security_id_, t_buysell_, int_price_);
          bid_index_ = smv_.base_bid_index_;
        } else if (bid_index_ >= (int)smv_.max_tick_range_) {
          RebuildIndexHighAccess(t_security_id_, t_buysell_, int_price_);
          bid_index_ = smv_.base_bid_index_;
        } else {
          smv_.base_bid_index_ = bid_index_;
        }
        l1_price_changed_[t_security_id_] = true;
      } else if (bid_index_ < 0) {
        DBGLOG_TIME_CLASS_FUNC << "BidIndex < 0 for some reason. sec_id: " << t_security_id_
                               << " bid_index: " << bid_index_ << DBGLOG_ENDL_FLUSH;
        return;
      }

      if (bid_index_ > HIGH_ACCESS_INDEX) {
        RebuildIndexHighAccess(t_security_id_, t_buysell_, int_price_);
        bid_index_ = smv_.base_bid_index_;
        l1_price_changed_[t_security_id_] = true;
      }

      if (bid_index_ > (int)smv_.base_bid_index_) {
        smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ = t_size_;
        smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ = 1;
        smv_.base_bid_index_ = bid_index_;

        if (SanitizeAskSide(t_security_id_) == kBookManagerReturn) {
          return;
        }

        l1_price_changed_[t_security_id_] = true;
      } else if (bid_index_ == (int)smv_.base_bid_index_)  // l1 size changes
      {
        smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ += t_size_;
        smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_++;
        l1_size_changed_[t_security_id_] = true;
      } else  // l2 changes
      {
        smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ += t_size_;
        smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_++;
      }
    } break;
    case kTradeTypeSell: {
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
        DBGLOG_TIME_CLASS_FUNC << "AskIndex < 0 for some reason. sec_id: " << t_security_id_
                               << " ask_index: " << ask_index_ << DBGLOG_ENDL_FLUSH;
        // Skip the message if we don't have info for this level
        return;
      }
      if (ask_index_ > HIGH_ACCESS_INDEX) {
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

        if (SanitizeBidSide(t_security_id_) == kBookManagerReturn) {
          return;
        }
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
    } else {
      smv_.market_update_info_.bestask_int_price_ =
          smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_;
      smv_.market_update_info_.bestask_price_ = smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_price_;
      smv_.market_update_info_.bestask_size_ = smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_;
      smv_.market_update_info_.bestask_ordercount_ =
          smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_ordercount_;
    }

    smv_.UpdateL1Prices();
  }

  if (!smv_.is_ready_) {
    if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ > 0 &&
        smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ > 0 && smv_.ArePricesComputed()) {
      smv_.is_ready_ = true;
    }
  }

  // Skip notifying listeners if the message is intermediate
  if (!smv_.is_ready_ || t_is_intermediate_) {
    // DBGLOG_TIME_CLASS_FUNC << "OnOrderAdd Intermediate skipping OnMarketUpdate: " << t_order_id_ <<
    // DBGLOG_ENDL_FLUSH;
    return;
  }

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

  // Notify relevant listeners about the update
  if (l1_price_changed_[t_security_id_]) {
    DBGLOG_DUMP;
    smv_.NotifyL1PriceListeners();
    smv_.NotifyOnReadyListeners();

    l1_price_changed_[t_security_id_] = false;
    l1_size_changed_[t_security_id_] = false;
  } else if (l1_size_changed_[t_security_id_]) {
    smv_.NotifyL1SizeListeners();
    smv_.NotifyOnReadyListeners();

    l1_size_changed_[t_security_id_] = false;
  } else {
    smv_.NotifyL2Listeners();
    smv_.NotifyL2OnlyListeners();
    smv_.NotifyOnReadyListeners();
  }
}

void HKOMDIndexedMarketViewManager::OnOrderModify(const uint32_t t_security_id_, const uint64_t t_order_id_,
                                                  const double t_price_,  // sent as int32_t changed to double using ref
                                                  const uint32_t t_size_, const TradeType_t t_buysell_,
                                                  const bool t_intermediate_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);
  int int_price_ = smv_.GetIntPx(t_price_);

  if (t_buysell_ == kTradeTypeBuy) {
    if (bidside_oid_to_qty_map_vec_[t_security_id_].find(t_order_id_) !=
        bidside_oid_to_qty_map_vec_[t_security_id_].end()) {
      if (smv_.GetIntPx(bidside_oid_to_qty_map_vec_[t_security_id_][t_order_id_].price_) ==
          int_price_)  // if prices are same
      {
        int bid_index_ = (int)smv_.base_bid_index_ -
                         (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - int_price_);
        smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ +=
            (t_size_ - bidside_oid_to_qty_map_vec_[t_security_id_][t_order_id_].size_);
        bidside_oid_to_qty_map_vec_[t_security_id_][t_order_id_].size_ = t_size_;
      } else {
        OnOrderDelete(t_security_id_, t_order_id_, t_buysell_, false, true);
        OnOrderAdd(t_security_id_, t_order_id_, t_price_, t_size_, t_buysell_, t_intermediate_);
        // bidside_oid_to_qty_map_vec_[ t_security_id_ ][ t_order_id_ ].size_ = t_size_;
        // bidside_oid_to_qty_map_vec_[ t_security_id_ ][ t_order_id_ ].price_ = t_price_;
      }
    } else {
      DBGLOG_TIME_CLASS_FUNC << "OrderId missing security_id: " << t_security_id_ << "order_id: " << t_order_id_
                             << " side: " << t_buysell_ << DBGLOG_ENDL_FLUSH;
    }
  } else if (t_buysell_ == kTradeTypeSell) {
    if (askside_oid_to_qty_map_vec_[t_security_id_].find(t_order_id_) !=
        askside_oid_to_qty_map_vec_[t_security_id_].end()) {
      if (smv_.GetIntPx(askside_oid_to_qty_map_vec_[t_security_id_][t_order_id_].price_) == t_price_) {
        int ask_index_ = (int)smv_.base_ask_index_ +
                         (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - int_price_);
        smv_.market_update_info_.asklevels_[ask_index_].limit_size_ +=
            (t_size_ - askside_oid_to_qty_map_vec_[t_security_id_][t_order_id_].size_);
        askside_oid_to_qty_map_vec_[t_security_id_][t_order_id_].size_ = t_size_;
      } else {
        OnOrderDelete(t_security_id_, t_order_id_, t_buysell_, false, true);
        OnOrderAdd(t_security_id_, t_order_id_, t_price_, t_size_, t_buysell_, false);
        // askside_oid_to_qty_map_vec_[ t_security_id_ ][ t_order_id_ ].size_ = t_size_;
        // askside_oid_to_qty_map_vec_[ t_security_id_ ][ t_order_id_ ].price_ = t_price_;
      }
    } else {
      DBGLOG_TIME_CLASS_FUNC << "OrderId missing security_id: " << t_security_id_ << "order_id: " << t_order_id_
                             << " side: " << t_buysell_ << DBGLOG_ENDL_FLUSH;
    }
  }
}

void HKOMDIndexedMarketViewManager::OnOrderDelete(const uint32_t t_security_id_, const uint64_t t_order_id_,
                                                  const TradeType_t t_buysell_, const bool t_trade_delete_,
                                                  const bool t_intermediate_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  double t_price_ = kInvalidIntPrice;
  uint32_t t_size_ = 0;

  if (t_buysell_ == kTradeTypeBuy &&
      bidside_oid_to_qty_map_vec_[t_security_id_].find(t_order_id_) !=
          bidside_oid_to_qty_map_vec_[t_security_id_].end()) {
    t_price_ = bidside_oid_to_qty_map_vec_[t_security_id_][t_order_id_].price_;
    t_size_ = bidside_oid_to_qty_map_vec_[t_security_id_][t_order_id_].size_;
    bidside_oid_to_qty_map_vec_[t_security_id_].erase(t_order_id_);
  } else if (t_buysell_ == kTradeTypeSell &&
             askside_oid_to_qty_map_vec_[t_security_id_].find(t_order_id_) !=
                 askside_oid_to_qty_map_vec_[t_security_id_].end()) {
    t_price_ = askside_oid_to_qty_map_vec_[t_security_id_][t_order_id_].price_;
    t_size_ = askside_oid_to_qty_map_vec_[t_security_id_][t_order_id_].size_;
    askside_oid_to_qty_map_vec_[t_security_id_].erase(t_order_id_);
  } else {
    return;
  }

  int int_price_ = smv_.GetIntPx(t_price_);

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
        return;
      }

      if (bid_index_ > (int)smv_.base_bid_index_) {
        // Error case
      } else if (bid_index_ == (int)smv_.base_bid_index_) {
        smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ -= t_size_;
        smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_--;
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
        if (t_trade_delete_)  // remove all levels ( bids above this trade price )best_index > bid_index
        {
          DBGLOG_TIME_CLASS_FUNC << "Trade Delete @nonbest clearing Top levels: " << t_security_id_
                                 << "order_id: " << t_order_id_ << " side: " << t_buysell_ << " price: " << t_price_
                                 << DBGLOG_ENDL_FLUSH;
          int index_ = (int)smv_.base_bid_index_;
          for (; index_ > bid_index_; index_--) {
            smv_.market_update_info_.bidlevels_[index_].limit_size_ = 0;
            smv_.market_update_info_.bidlevels_[index_].limit_ordercount_ = 0;
          }
          smv_.base_bid_index_ = index_;
          l1_price_changed_[t_security_id_] = true;
          // Check if we need to re-centre the index
          if (smv_.base_bid_index_ < LOW_ACCESS_INDEX) {
            RebuildIndexLowAccess(t_security_id_, t_buysell_,
                                  smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_);
          }
        }
        smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ -= t_size_;
        smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_--;

        if (smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ <= 0 ||
            smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ <= 0) {
          smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ = 0;
          smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ = 0;
        }
      }
    } break;
    case kTradeTypeSell: {
      ask_index_ = (int)smv_.base_ask_index_ +
                   (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - int_price_);

      // Skip the message if we don't have any info for this level
      if (ask_index_ < 0) {
        return;
      }
      if (ask_index_ > (int)smv_.base_ask_index_) {
        // Error case
      } else if (ask_index_ == (int)smv_.base_ask_index_) {
        smv_.market_update_info_.asklevels_[ask_index_].limit_size_ -= t_size_;
        smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_--;
        l1_size_changed_[t_security_id_] = true;

        if (smv_.market_update_info_.asklevels_[ask_index_].limit_size_ <= 0 ||
            smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ <= 0) {
          smv_.market_update_info_.asklevels_[ask_index_].limit_size_ = 0;
          smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ = 0;
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
        if (t_trade_delete_)  // remove all
        {
          DBGLOG_TIME_CLASS_FUNC << "Trade Delete @nonbest clearing Top levels: " << t_security_id_
                                 << "order_id: " << t_order_id_ << " side: " << t_buysell_ << " price: " << t_price_
                                 << DBGLOG_ENDL_FLUSH;
          int index_ = (int)smv_.base_ask_index_;
          for (; index_ > ask_index_; index_--) {
            smv_.market_update_info_.asklevels_[index_].limit_size_ = 0;
            smv_.market_update_info_.asklevels_[index_].limit_ordercount_ = 0;
          }
          smv_.base_ask_index_ = index_;
          l1_price_changed_[t_security_id_] = true;
          // Check if we need to re-centre the index
          if (smv_.base_ask_index_ < LOW_ACCESS_INDEX) {
            RebuildIndexLowAccess(t_security_id_, t_buysell_,
                                  smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_);
          }
        }

        smv_.market_update_info_.asklevels_[ask_index_].limit_size_ -= t_size_;
        smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_--;

        if (smv_.market_update_info_.asklevels_[ask_index_].limit_size_ <= 0 ||
            smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ <= 0) {
          smv_.market_update_info_.asklevels_[ask_index_].limit_size_ = 0;
          smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ = 0;
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
    } else {
      smv_.market_update_info_.bestask_int_price_ =
          smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_;
      smv_.market_update_info_.bestask_price_ = smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_price_;
      smv_.market_update_info_.bestask_size_ = smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_;
      smv_.market_update_info_.bestask_ordercount_ =
          smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_ordercount_;
    }

    smv_.UpdateL1Prices();
  }

  // Skip notifying listeners if either the message is intermediate or smv is not in ready state
  // Also, skip if the message order exec message. Because this will undo the effect of exec_summary messages.
  if (!smv_.is_ready_ || t_intermediate_) {
    return;
  }

  if (smv_.pl_change_listeners_present_) {
    if (t_buysell_ == kTradeTypeBuy && (int)smv_.base_bid_index_ >= bid_index_ &&
        (int)smv_.base_bid_index_ - bid_index_ <= 10) {
      int level_ = smv_.base_bid_index_ - bid_index_;
      int int_price_level_ = smv_.base_bid_index_ - bid_index_;
      smv_.NotifyOnPLChangeListeners(t_buysell_, smv_.market_update_info_, t_buysell_, level_, int_price_,
                                     int_price_level_,
                                     smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ + t_size_,
                                     smv_.market_update_info_.bidlevels_[bid_index_].limit_size_,
                                     smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_ + 1,
                                     smv_.market_update_info_.bidlevels_[bid_index_].limit_ordercount_, t_intermediate_,
                                     smv_.market_update_info_.bidlevels_[bid_index_].limit_size_ == 0 ? 'D' : 'C');
    } else if (t_buysell_ == kTradeTypeSell && (int)smv_.base_ask_index_ >= ask_index_ &&
               (int)smv_.base_ask_index_ - ask_index_ <= 10) {
      int level_ = smv_.base_ask_index_ - ask_index_;
      int int_price_level_ = smv_.base_ask_index_ - ask_index_;
      smv_.NotifyOnPLChangeListeners(t_buysell_, smv_.market_update_info_, t_buysell_, level_, int_price_,
                                     int_price_level_,
                                     smv_.market_update_info_.asklevels_[ask_index_].limit_size_ + t_size_,
                                     smv_.market_update_info_.asklevels_[ask_index_].limit_size_,
                                     smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_ + 1,
                                     smv_.market_update_info_.asklevels_[ask_index_].limit_ordercount_, t_intermediate_,
                                     smv_.market_update_info_.asklevels_[ask_index_].limit_size_ == 0 ? 'D' : 'C');
    }
  }

  // Notify relevant listeners about the update
  if (l1_price_changed_[t_security_id_]) {
    smv_.NotifyL1PriceListeners();
    smv_.NotifyOnReadyListeners();

    l1_price_changed_[t_security_id_] = false;
    l1_size_changed_[t_security_id_] = false;
  } else if (l1_size_changed_[t_security_id_]) {
    smv_.NotifyL1SizeListeners();
    smv_.NotifyOnReadyListeners();

    l1_size_changed_[t_security_id_] = false;
  } else {
    smv_.NotifyL2Listeners();
    smv_.NotifyL2OnlyListeners();
    smv_.NotifyOnReadyListeners();
  }
}

void HKOMDIndexedMarketViewManager::OnTrade(const uint32_t t_security_id_,
                                            const uint64_t t_order_id_,   // passive order's id ( call modify / delete )
                                            const double t_trade_price_,  // sent as int32_t changed to double using ref
                                            const uint32_t t_size_, const TradeType_t t_buysell_) {
  // aggressive side of the trade ( the order wont be in the book ) for masking though
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  // not till book is ready
  if ((smv_.market_update_info_.bestbid_int_price_ <= kInvalidIntPrice) ||
      (smv_.market_update_info_.bestask_int_price_ <= kInvalidIntPrice)) {
    return;
  }

  smv_.StorePreTrade();

  if (t_order_id_ > 0) {
    // book changes  [ order corresponds to passive/resting order ]
    if (t_buysell_ == kTradeTypeBuy &&
        bidside_oid_to_qty_map_vec_[t_security_id_].find(t_order_id_) !=
            bidside_oid_to_qty_map_vec_[t_security_id_].end()) {
      if (bidside_oid_to_qty_map_vec_[t_security_id_][t_order_id_].size_ >= t_size_) {
        OnOrderDelete(t_security_id_, t_order_id_, t_buysell_, true, true);
      } else {
        OnOrderModify(t_security_id_, t_order_id_, t_trade_price_,
                      bidside_oid_to_qty_map_vec_[t_security_id_][t_order_id_].size_ - t_size_, t_buysell_, true);
      }
    } else if (t_buysell_ == kTradeTypeSell &&
               askside_oid_to_qty_map_vec_[t_security_id_].find(t_order_id_) !=
                   askside_oid_to_qty_map_vec_[t_security_id_].end()) {
      if (askside_oid_to_qty_map_vec_[t_security_id_][t_order_id_].size_ >= t_size_) {
        OnOrderDelete(t_security_id_, t_order_id_, t_buysell_, true, true);
      } else {
        OnOrderModify(t_security_id_, t_order_id_, t_trade_price_,
                      askside_oid_to_qty_map_vec_[t_security_id_][t_order_id_].size_ - t_size_, t_buysell_, true);
      }
    }
  }
  // l1 prices are updated

  // trade variables
  // set the primary variables
  smv_.trade_print_info_.trade_price_ = t_trade_price_;
  smv_.trade_print_info_.size_traded_ = t_size_;
  smv_.trade_print_info_.int_trade_price_ = smv_.GetIntPx(t_trade_price_);
  smv_.trade_print_info_.buysell_ =
      (t_buysell_ == kTradeTypeBuy ? kTradeTypeSell : kTradeTypeBuy);  // since we get passive one
  smv_.SetTradeVarsForIndicatorsIfRequired();

  if (smv_.is_ready_) {
    smv_.NotifyTradeListeners();
    smv_.NotifyOnReadyListeners();
  }
}
}
