/**
   \file MarketAdapterCode/indexed_ose_order_feed_market_view_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include <limits.h>
#include "baseinfra/MarketAdapter/indexed_ose_order_feed_market_view_manager.hpp"

namespace HFSAT {

#define LOW_ACCESS_INDEX 50
#define INVALID_PRICE -1

#define PRINT_ORDER_INFO_MODE_OSE_OF 0

IndexedOseOrderFeedMarketViewManager::IndexedOseOrderFeedMarketViewManager(
    DebugLogger& t_dbglogger_, const Watch& t_watch_, const SecurityNameIndexer& t_sec_name_indexer_,
    const std::vector<SecurityMarketView*>& t_security_market_view_map_)
    : BaseMarketViewManager(t_dbglogger_, t_watch_, t_sec_name_indexer_, t_security_market_view_map_),
      sec_id_to_prev_update_was_quote_(t_sec_name_indexer_.NumSecurityId(), false),
      sec_id_to_order_id_and_bid_order_info_map_(t_sec_name_indexer_.NumSecurityId()),
      sec_id_to_order_id_and_ask_order_info_map_(t_sec_name_indexer_.NumSecurityId()),
      sec_id_to_is_regular_session_(t_sec_name_indexer_.NumSecurityId(), false),
      sec_id_to_delete_on_cross_(t_sec_name_indexer_.NumSecurityId(), true),
      dbglogger_(t_dbglogger_),
      watch_(t_watch_),
      sec_name_indexer_(t_sec_name_indexer_),
      prev_PL_change_state_(),
      buffered_trades_(DEF_MAX_SEC_ID) {}

// Main Functions
void IndexedOseOrderFeedMarketViewManager::OnOrderAdd(const unsigned int t_security_id_, uint64_t t_order_id_,
                                                      uint8_t t_side_, double t_price_, int t_size_,
                                                      uint32_t t_priority_, bool t_intermediate_) {
  if (t_price_ < INVALID_PRICE) {
    return;
  }

#if PRINT_ORDER_INFO_MODE_OSE_OF
  std::cout << typeid(*this).name() << ":" << __func__ << " " << t_order_id_ << " [" << t_price_ << "," << t_size_
            << "," << t_side_ << "]" << std::endl;
#endif

  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  int int_price = smv_.GetIntPx(t_price_);
  TradeType_t buysell = t_side_ == 'B' ? kTradeTypeBuy : (t_side_ == 'S' ? kTradeTypeSell : kTradeTypeNoInfo);

  if (!smv_.initial_book_constructed_) {
    BuildIndex(t_security_id_, buysell, int_price);
    smv_.initial_book_constructed_ = true;
  }

  int old_size = 0;
  int old_ordercount = 0;
  int level_added = 0;

  switch (buysell) {
    case kTradeTypeBuy: {
      bid_order_info_map_iter_ = sec_id_to_order_id_and_bid_order_info_map_[t_security_id_].find(t_order_id_);

      if (bid_order_info_map_iter_ == sec_id_to_order_id_and_bid_order_info_map_[t_security_id_].end()) {
        OSE_ITCH_MDS::OSEOrderInfo* ose_order_info_ptr = order_mempool_.Alloc();

        ose_order_info_ptr->price = t_price_;
        ose_order_info_ptr->size = t_size_;
        ose_order_info_ptr->side = t_side_;
        ose_order_info_ptr->priority = t_priority_;

        sec_id_to_order_id_and_bid_order_info_map_[t_security_id_].insert(
            bid_order_info_map_iter_, std::make_pair(t_order_id_, ose_order_info_ptr));

      } else {
        DBGLOG_CLASS_FUNC_LINE_INFO << watch_.tv_ToString()
                                    << " SecName: " << sec_name_indexer_.GetSecurityNameFromId(t_security_id_)
                                    << " SecId: " << t_security_id_
                                    << " Bid Order already present with order_id: " << t_order_id_ << DBGLOG_ENDL_FLUSH;
        return;
      }

      int bid_index = smv_.base_bid_index_ -
                      (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - int_price);

      // There are 0 levels on bid side
      if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ <= 0) {
        if (bid_index < LOW_ACCESS_INDEX) {
          RebuildIndexLowAccess(t_security_id_, buysell, int_price);
          bid_index = smv_.base_bid_index_;
        } else if (bid_index >= (int)smv_.max_tick_range_) {
          RebuildIndexHighAccess(t_security_id_, buysell, int_price);
          bid_index = smv_.base_bid_index_;
        }
        smv_.base_bid_index_ = bid_index;
      } else if (bid_index < 0) {
        return;
      }

      if (bid_index >= (int)smv_.max_tick_range_) {
        RebuildIndexHighAccess(t_security_id_, buysell, int_price);
        bid_index = smv_.base_bid_index_;
      }

      // Store old size and order count for OnPL change listeners. ( Not sure if having a bool check will help )
      old_size = smv_.market_update_info_.bidlevels_[bid_index].limit_size_;
      old_ordercount = smv_.market_update_info_.bidlevels_[bid_index].limit_ordercount_;

      if (smv_.market_update_info_.bidlevels_[bid_index].limit_size_ <= 0) {
        smv_.market_update_info_.bidlevels_[bid_index].limit_size_ = t_size_;
        smv_.market_update_info_.bidlevels_[bid_index].limit_ordercount_ = 1;
      } else {
        smv_.market_update_info_.bidlevels_[bid_index].limit_size_ += t_size_;
        smv_.market_update_info_.bidlevels_[bid_index].limit_ordercount_++;
      }

      if (bid_index >= (int)smv_.base_bid_index_) {
        smv_.l1_changed_since_last_ = true;
        smv_.base_bid_index_ = bid_index;

        if (int_price >= smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_) {
          if (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ <= 0) {
            // We should not do sanitization, if the other side is not ready
            smv_.is_ready_ = false;
            break;
          }

          SanitizeAskSide(t_security_id_, int_price);
          UpdateBestAskVariables(t_security_id_, smv_.base_ask_index_);
        }
      } else {
        smv_.l2_changed_since_last_ = true;
      }

      for (int t_index = smv_.base_bid_index_ - 1; t_index >= bid_index; t_index--) {
        if (smv_.market_update_info_.bidlevels_[t_index].limit_size_ > 0 &&
            smv_.market_update_info_.bidlevels_[t_index].limit_ordercount_ > 0)
          level_added++;
      }
    } break;
    case kTradeTypeSell: {
      ask_order_info_map_iter_ = sec_id_to_order_id_and_ask_order_info_map_[t_security_id_].find(t_order_id_);

      if (ask_order_info_map_iter_ == sec_id_to_order_id_and_ask_order_info_map_[t_security_id_].end()) {
        OSE_ITCH_MDS::OSEOrderInfo* ose_order_info_ptr = order_mempool_.Alloc();

        ose_order_info_ptr->price = t_price_;
        ose_order_info_ptr->size = t_size_;
        ose_order_info_ptr->side = t_side_;
        ose_order_info_ptr->priority = t_priority_;

        sec_id_to_order_id_and_ask_order_info_map_[t_security_id_].insert(
            ask_order_info_map_iter_, std::make_pair(t_order_id_, ose_order_info_ptr));

      } else {
        DBGLOG_CLASS_FUNC_LINE_INFO << watch_.tv_ToString()
                                    << " SecName: " << sec_name_indexer_.GetSecurityNameFromId(t_security_id_)
                                    << " SecId: " << t_security_id_
                                    << " Ask Order already present with order_id: " << t_order_id_ << DBGLOG_ENDL_FLUSH;
        return;
      }

      int ask_index = smv_.base_ask_index_ +
                      (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - int_price);

      // There are 0 levels on ask side
      if (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ <= 0) {
        if (ask_index < LOW_ACCESS_INDEX) {
          RebuildIndexLowAccess(t_security_id_, buysell, int_price);
          ask_index = smv_.base_ask_index_;
        } else if (ask_index >= (int)smv_.max_tick_range_) {
          RebuildIndexHighAccess(t_security_id_, buysell, int_price);
          ask_index = smv_.base_ask_index_;
        }

        smv_.base_ask_index_ = ask_index;
      } else if (ask_index < 0) {
        return;
      }

      if (ask_index >= (int)smv_.max_tick_range_) {
        RebuildIndexHighAccess(t_security_id_, buysell, int_price);
        ask_index = smv_.base_ask_index_;
      }

      old_size = smv_.market_update_info_.asklevels_[ask_index].limit_size_;
      old_ordercount = smv_.market_update_info_.asklevels_[ask_index].limit_ordercount_;

      if (smv_.market_update_info_.asklevels_[ask_index].limit_size_ <= 0) {
        smv_.market_update_info_.asklevels_[ask_index].limit_size_ = t_size_;
        smv_.market_update_info_.asklevels_[ask_index].limit_ordercount_ = 1;
      } else {
        smv_.market_update_info_.asklevels_[ask_index].limit_size_ += t_size_;
        smv_.market_update_info_.asklevels_[ask_index].limit_ordercount_++;
      }

      if (ask_index >= (int)smv_.base_ask_index_) {
        smv_.l1_changed_since_last_ = true;
        smv_.base_ask_index_ = ask_index;

        if (int_price <= smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_) {
          if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ <= 0) {
            // We should not do sanitization, if the other side is not ready
            smv_.is_ready_ = false;
            break;
          }

          SanitizeBidSide(t_security_id_, int_price);
          UpdateBestBidVariables(t_security_id_, smv_.base_bid_index_);
        }
      } else {
        smv_.l2_changed_since_last_ = true;
      }

      for (int t_index = smv_.base_ask_index_ - 1; t_index >= ask_index; t_index--) {
        if (smv_.market_update_info_.asklevels_[t_index].limit_size_ > 0 &&
            smv_.market_update_info_.asklevels_[t_index].limit_ordercount_ > 0)
          level_added++;
      }

    } break;
    default: {
      DBGLOG_CLASS_FUNC_LINE_INFO << watch_.tv_ToString()
                                  << " SecName: " << sec_name_indexer_.GetSecurityNameFromId(t_security_id_)
                                  << " SecId: " << t_security_id_
                                  << " OSE OF OnOrderAdd: Side is neither B nor S: " << t_side_ << DBGLOG_ENDL_FLUSH;
      return;
    } break;
  }

  if (smv_.l1_changed_since_last_) {
    switch (buysell) {
      case kTradeTypeBuy: {
        UpdateBestBidVariables(t_security_id_, smv_.base_bid_index_);
        smv_.UpdateL1Prices();
      } break;
      case kTradeTypeSell: {
        UpdateBestAskVariables(t_security_id_, smv_.base_ask_index_);
        smv_.UpdateL1Prices();
      } break;
      default:
        break;
    }
  }

  sec_id_to_prev_update_was_quote_[t_security_id_] = true;

  if (!smv_.is_ready_) {
    if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ > 0 &&
        smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ > 0 && smv_.ArePricesComputed()) {
      smv_.is_ready_ = true;
    } else {
      return;
    }
  }

  if (smv_.market_update_info_.trade_update_implied_quote_) {
    smv_.market_update_info_.trade_update_implied_quote_ = false;
    smv_.trade_print_info_.num_trades_++;
  }

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(29);
#endif

  if (t_intermediate_) {
    return;
  }

  CheckToNotifyTradeMessage(t_security_id_, t_intermediate_);

  if (smv_.pl_change_listeners_present_) {
    smv_.NotifyOnPLChangeListeners(t_security_id_, smv_.market_update_info_, buysell, level_added, int_price, 0,
                                   old_size, old_size + t_size_, old_ordercount, old_ordercount + 1, t_intermediate_,
                                   old_size == 0 ? 'N' : 'C');
  }

  if (smv_.l1_changed_since_last_) {
    smv_.NotifyL1PriceListeners();
    smv_.l1_changed_since_last_ = false;
  } else {
    smv_.NotifyL2Listeners();
  }

  if (smv_.l2_changed_since_last_) {
    smv_.NotifyL2OnlyListeners();
    smv_.l2_changed_since_last_ = false;
  }

  smv_.NotifyOnReadyListeners();
}

void IndexedOseOrderFeedMarketViewManager::OnOrderDelete(const unsigned int t_security_id_, uint64_t t_order_id_,
                                                         uint8_t t_side_, bool t_intermediate_,
                                                         bool t_is_set_order_info_map_iter_) {
#if PRINT_ORDER_INFO_MODE_OSE_OF
  std::cout << typeid(*this).name() << ':' << __func__ << " " << t_order_id_;
#endif
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  if (!smv_.initial_book_constructed_) {
    return;
  }

  int order_size = 0;
  double order_price = 0.0;
  TradeType_t buysell = t_side_ == 'B' ? kTradeTypeBuy : (t_side_ == 'S' ? kTradeTypeSell : kTradeTypeNoInfo);

  int cumulative_old_size = 0;
  int cumulative_old_ordercount = 0;
  int int_price = 0;
  bool is_level_deleted = false;
  bool is_order_present_in_best_level = false;

  int level_changed = 0;

  switch (buysell) {
    case kTradeTypeBuy: {
      // searching the bid_order_map to retrieve the meta-data corresponding to @t_order_id
      bid_order_info_map_iter_ = sec_id_to_order_id_and_bid_order_info_map_[t_security_id_].find(t_order_id_);
      if (bid_order_info_map_iter_ != sec_id_to_order_id_and_bid_order_info_map_[t_security_id_].end()) {
        order_price = bid_order_info_map_iter_->second->price;
        order_size = bid_order_info_map_iter_->second->size;

        order_mempool_.DeAlloc(bid_order_info_map_iter_->second);
        sec_id_to_order_id_and_bid_order_info_map_[t_security_id_].erase(bid_order_info_map_iter_);
      } else {
        DBGLOG_CLASS_FUNC_LINE_INFO << watch_.tv_ToString()
                                    << " SecName: " << sec_name_indexer_.GetSecurityNameFromId(t_security_id_)
                                    << " SecId: " << t_security_id_ << " Error: Bid OrderId: " << t_order_id_
                                    << " not present to delete." << DBGLOG_ENDL_FLUSH;
        return;
      }
#if PRINT_ORDER_INFO_MODE_OSE_OF
      std::cout << " [" << order_price << "," << order_size << "," << t_side_ << "]" << std::endl;
#endif
      int_price = smv_.GetIntPx(order_price);

      // find the index in bid_level vector to which this order belongs
      int bid_index = smv_.base_bid_index_ -
                      (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - int_price);

      if (bid_index == (int)smv_.base_bid_index_) is_order_present_in_best_level = true;
      // store the old information corresponding to the above found level @bid_index
      cumulative_old_size = smv_.market_update_info_.bidlevels_[bid_index].limit_size_;
      cumulative_old_ordercount = smv_.market_update_info_.bidlevels_[bid_index].limit_ordercount_;

      // modify the data at level @bid_index
      smv_.market_update_info_.bidlevels_[bid_index].limit_size_ -= order_size;
      smv_.market_update_info_.bidlevels_[bid_index].limit_ordercount_--;

      if (smv_.market_update_info_.bidlevels_[bid_index].limit_size_ < 0 ||
          smv_.market_update_info_.bidlevels_[bid_index].limit_ordercount_ < 0) {
#if PRINT_ORDER_INFO_MODE_OSE_OF
        std::cout << " order_size/order_count is negative after deleting the order..." << std::endl;
#endif
        smv_.market_update_info_.bidlevels_[bid_index].limit_size_ = 0;
        smv_.market_update_info_.bidlevels_[bid_index].limit_ordercount_ = 0;
      }

      // checking if no more orders are left at @bid_index
      is_level_deleted = ((smv_.market_update_info_.bidlevels_[bid_index].limit_size_ <= 0) ||
                          (smv_.market_update_info_.bidlevels_[bid_index].limit_ordercount_ <= 0));

      if (is_level_deleted) {
        // checking if the level deleted is the best_bid_level
        if (is_order_present_in_best_level) {
          UpdateBaseBidIndex(t_security_id_, buysell);
        } else {
          smv_.l2_changed_since_last_ = true;  // indicating that level other than best bid level has been changed
        }
      } else {
        if (is_order_present_in_best_level) {
          smv_.l1_changed_since_last_ = true;  // indicating that best bid level has been changed
        } else {
          smv_.l2_changed_since_last_ = true;  // indicating that level other than best bid level has been changed
        }
      }
      for (int t_index = smv_.base_bid_index_ - 1; t_index >= bid_index; t_index--) {
        if (smv_.market_update_info_.bidlevels_[t_index].limit_size_ > 0 &&
            smv_.market_update_info_.bidlevels_[t_index].limit_ordercount_ > 0)
          level_changed++;
      }
    } break;
    case kTradeTypeSell: {
      // searching the ask_order_map to retrieve the meta-data corresponding to @t_order_id
      ask_order_info_map_iter_ = sec_id_to_order_id_and_ask_order_info_map_[t_security_id_].find(t_order_id_);
      if (ask_order_info_map_iter_ != sec_id_to_order_id_and_ask_order_info_map_[t_security_id_].end()) {
        order_price = ask_order_info_map_iter_->second->price;
        order_size = ask_order_info_map_iter_->second->size;

        order_mempool_.DeAlloc(ask_order_info_map_iter_->second);
        sec_id_to_order_id_and_ask_order_info_map_[t_security_id_].erase(ask_order_info_map_iter_);
      } else {
        DBGLOG_CLASS_FUNC_LINE_INFO << watch_.tv_ToString()
                                    << " SecName: " << sec_name_indexer_.GetSecurityNameFromId(t_security_id_)
                                    << " SecId: " << t_security_id_ << " Error: Ask OrderId: " << t_order_id_
                                    << " not present to delete." << DBGLOG_ENDL_FLUSH;
        return;
      }
#if PRINT_ORDER_INFO_MODE_OSE_OF
      std::cout << " [" << order_price << "," << order_size << "," << t_side_ << "]" << std::endl;
#endif
      int_price = smv_.GetIntPx(order_price);

      // find the index in ask_level vector to which this order belongs
      int ask_index = smv_.base_ask_index_ +
                      (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - int_price);

      if (ask_index == (int)smv_.base_ask_index_) is_order_present_in_best_level = true;

      // store the old information corresponding to the above fund level @ask_index
      cumulative_old_size = smv_.market_update_info_.asklevels_[ask_index].limit_size_;
      cumulative_old_ordercount = smv_.market_update_info_.asklevels_[ask_index].limit_ordercount_;

      // modify the data at level @ask_index
      smv_.market_update_info_.asklevels_[ask_index].limit_size_ -= order_size;
      smv_.market_update_info_.asklevels_[ask_index].limit_ordercount_--;

      if (smv_.market_update_info_.asklevels_[ask_index].limit_size_ < 0 ||
          smv_.market_update_info_.asklevels_[ask_index].limit_ordercount_ < 0) {
        DBGLOG_CLASS_FUNC_LINE_INFO << watch_.tv_ToString()
                                    << " SecName: " << sec_name_indexer_.GetSecurityNameFromId(t_security_id_)
                                    << " SecId: " << t_security_id_
                                    << " order_size/order_count is negative after deleting the order_id: "
                                    << t_order_id_ << ". Setting to 0" << DBGLOG_ENDL_FLUSH;

        smv_.market_update_info_.asklevels_[ask_index].limit_size_ = 0;
        smv_.market_update_info_.asklevels_[ask_index].limit_ordercount_ = 0;
      }

      // checking if no more orders are left at @ask_index
      is_level_deleted = ((smv_.market_update_info_.asklevels_[ask_index].limit_size_ <= 0) ||
                          (smv_.market_update_info_.asklevels_[ask_index].limit_ordercount_ <= 0));

      if (is_level_deleted) {
        // checking if the level deleted is the best_ask_level
        if (is_order_present_in_best_level) {
          UpdateBaseAskIndex(t_security_id_, buysell);
        } else {
          smv_.l2_changed_since_last_ = true;  // indicating that level other than best ask level has been changed
        }
      } else {
        if (is_order_present_in_best_level) {
          smv_.l1_changed_since_last_ = true;  // indicating that best ask level has been updated
        } else {
          smv_.l2_changed_since_last_ = true;  // indicating that level other than best ask level has been changed
        }
      }
      for (int t_index = smv_.base_ask_index_ - 1; t_index >= ask_index; t_index--) {
        if (smv_.market_update_info_.asklevels_[t_index].limit_size_ > 0 &&
            smv_.market_update_info_.asklevels_[t_index].limit_ordercount_ > 0)
          level_changed++;
      }
    } break;
    default: {
      DBGLOG_CLASS_FUNC_LINE_INFO << watch_.tv_ToString()
                                  << " SecName: " << sec_name_indexer_.GetSecurityNameFromId(t_security_id_)
                                  << " SecId: " << t_security_id_ << " Invalid Side (Neither B or S): " << t_side_
                                  << DBGLOG_ENDL_FLUSH;
      return;
    } break;
  }

  if (smv_.l1_changed_since_last_) {
    switch (buysell) {
      case kTradeTypeBuy: {
        UpdateBestBidVariables(t_security_id_, smv_.base_bid_index_);
        smv_.UpdateL1Prices();
      } break;
      case kTradeTypeSell: {
        UpdateBestAskVariables(t_security_id_, smv_.base_ask_index_);
        smv_.UpdateL1Prices();
      } break;
      default:
        break;
    }
  }

  if (smv_.market_update_info_.trade_update_implied_quote_) {
    smv_.market_update_info_.trade_update_implied_quote_ = false;
    smv_.trade_print_info_.num_trades_++;
  }

  sec_id_to_prev_update_was_quote_[t_security_id_] = true;

  if (!smv_.is_ready_) {
    if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ > 0 &&
        smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ > 0 && smv_.ArePricesComputed()) {
      smv_.is_ready_ = true;
    } else {
      return;
    }
  }

  // store price level change state if called from delta for trade
  if (t_is_set_order_info_map_iter_ && smv_.pl_change_listeners_present_) {
    cumulative_old_size = std::max(cumulative_old_size, prev_PL_change_state_.old_size_);
    cumulative_old_ordercount = std::max(cumulative_old_ordercount, prev_PL_change_state_.old_ordercount_);
    if (is_level_deleted) {
      prev_PL_change_state_.SetState(t_security_id_, buysell, level_changed, int_price, 0, cumulative_old_size, 0,
                                     cumulative_old_ordercount, 0, t_intermediate_, 'D');
    } else {
      prev_PL_change_state_.SetState(t_security_id_, buysell, level_changed, int_price, 0, cumulative_old_size,
                                     cumulative_old_size - order_size, cumulative_old_ordercount,
                                     cumulative_old_ordercount - 1, t_intermediate_, 'C');
    }
  }

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(29);
#endif

  if (t_intermediate_) {
    return;
  }

  CheckToNotifyTradeMessage(t_security_id_, t_intermediate_);

  if (!t_is_set_order_info_map_iter_ && smv_.pl_change_listeners_present_) {
    if (is_level_deleted) {
      smv_.NotifyOnPLChangeListeners(t_security_id_, smv_.market_update_info_, buysell, level_changed, int_price, 0,
                                     cumulative_old_size, 0, cumulative_old_ordercount, 0, t_intermediate_, 'D');
    } else {
      smv_.NotifyOnPLChangeListeners(t_security_id_, smv_.market_update_info_, buysell, level_changed, int_price, 0,
                                     cumulative_old_size, cumulative_old_size - order_size, cumulative_old_ordercount,
                                     cumulative_old_ordercount - 1, t_intermediate_, 'C');
    }
  }

  if (smv_.l1_changed_since_last_) {
    smv_.NotifyL1PriceListeners();
    smv_.l1_changed_since_last_ = false;
  } else {
    smv_.NotifyL2Listeners();
  }

  if (smv_.l2_changed_since_last_) {
    smv_.NotifyL2OnlyListeners();
    smv_.l2_changed_since_last_ = false;
  }

  smv_.NotifyOnReadyListeners();
}

/*
 * This function assumes that the order to modify will only have its size changed
 * but prices will remain same as before.The "Replace Order" where both prices and
 * size can change has been implemented in OrderReplace as OrderDelete + OrderAdd
 */
void IndexedOseOrderFeedMarketViewManager::OnOrderModify(const unsigned int t_security_id_, uint64_t t_order_id_,
                                                         uint8_t t_side_, int t_new_size_, uint64_t t_new_order_id_,
                                                         bool t_intermediate_, bool t_is_set_order_info_map_iter_) {
#if PRINT_ORDER_INFO_MODE_OSE_OF
  std::cout << typeid(*this).name() << ':' << __func__ << " " << t_order_id_;
  std::cout << " NEW : [" << t_new_order_id_ << "," << t_new_size_ << "," << t_side_ << "]";
#endif

  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  double old_order_price = 0.0;
  int old_order_size = 0;
  // uint8_t old_side = '-';

  int int_price = 0;
  TradeType_t buysell = t_side_ == 'B' ? kTradeTypeBuy : (t_side_ == 'S' ? kTradeTypeSell : kTradeTypeNoInfo);

  int cumulative_old_size = 0;
  int cumulative_new_size = 0;
  int cumulative_old_ordercount = 0;
  int level_modified = 0;

  bool is_level_deleted = false;
  bool is_order_present_in_best_level = false;

  switch (buysell) {
    case kTradeTypeBuy: {
      if (!t_is_set_order_info_map_iter_) {  // checking if the iterator is set already before calling this function
        bid_order_info_map_iter_ = sec_id_to_order_id_and_bid_order_info_map_[t_security_id_].find(t_order_id_);
      }
      if (bid_order_info_map_iter_ != sec_id_to_order_id_and_bid_order_info_map_[t_security_id_].end()) {
        OSE_ITCH_MDS::OSEOrderInfo* order_info = bid_order_info_map_iter_->second;
        old_order_price = order_info->price;
        old_order_size = order_info->size;
        // old_side = order_info->side;
        // update the size
        order_info->size = t_new_size_;

        if (t_order_id_ != t_new_order_id_) {
          // bid_order_info_map_iter_->second = nullptr;
          sec_id_to_order_id_and_bid_order_info_map_[t_security_id_].erase(bid_order_info_map_iter_);
          sec_id_to_order_id_and_bid_order_info_map_[t_security_id_][t_new_order_id_] = order_info;
        }
      } else {
        DBGLOG_CLASS_FUNC_LINE_INFO << watch_.tv_ToString()
                                    << " SecName: " << sec_name_indexer_.GetSecurityNameFromId(t_security_id_)
                                    << " SecId: " << t_security_id_ << " Error: BidOrderId: " << t_order_id_
                                    << " not present to modify." << DBGLOG_ENDL_FLUSH;
        return;
      }

#if PRINT_ORDER_INFO_MODE_OSE_OF
      std::cout << " OLD : [" << t_order_id_ << "," << old_order_size << "," 
                << "] PRICE: " << old_order_price << std::endl;
#endif

      int_price = smv_.GetIntPx(old_order_price);

      if (!smv_.initial_book_constructed_) {
        BuildIndex(t_security_id_, buysell, int_price);
        smv_.initial_book_constructed_ = true;
      }

      int bid_index = smv_.base_bid_index_ -
                      (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - int_price);

      is_order_present_in_best_level = (bid_index == (int)smv_.base_bid_index_);

      if (smv_.market_update_info_.bidlevels_[bid_index].limit_size_ > 0)
        cumulative_old_size = smv_.market_update_info_.bidlevels_[bid_index].limit_size_;
      if (smv_.market_update_info_.bidlevels_[bid_index].limit_ordercount_ > 0)
        cumulative_old_ordercount = smv_.market_update_info_.bidlevels_[bid_index].limit_ordercount_;

      smv_.market_update_info_.bidlevels_[bid_index].limit_size_ -= old_order_size;

      if (smv_.market_update_info_.bidlevels_[bid_index].limit_size_ >= 0)
        smv_.market_update_info_.bidlevels_[bid_index].limit_size_ += t_new_size_;
      else
        smv_.market_update_info_.bidlevels_[bid_index].limit_size_ = t_new_size_;

      is_level_deleted = ((smv_.market_update_info_.bidlevels_[bid_index].limit_size_ <= 0) ||
                          (smv_.market_update_info_.bidlevels_[bid_index].limit_ordercount_ <= 0));

      if (!is_level_deleted) {
        cumulative_new_size = smv_.market_update_info_.bidlevels_[bid_index].limit_size_;
      }

      if (is_level_deleted) {
        // checking if the level deleted is the best_bid_level
        if (is_order_present_in_best_level) {
          UpdateBaseBidIndex(t_security_id_, buysell);
        } else {
          smv_.l2_changed_since_last_ = true;  // indicating that level other than best bid level has been changed
        }
      } else {
        if (is_order_present_in_best_level) {
          smv_.l1_changed_since_last_ = true;
        } else {
          smv_.l2_changed_since_last_ = true;
        }
      }

      for (int t_index = smv_.base_bid_index_ - 1; t_index >= bid_index; t_index--) {
        if (smv_.market_update_info_.bidlevels_[t_index].limit_size_ > 0 &&
            smv_.market_update_info_.bidlevels_[t_index].limit_ordercount_ > 0)
          level_modified++;
      }
    } break;
    case kTradeTypeSell: {
      if (!t_is_set_order_info_map_iter_) {  // checking if the iterator is set already before calling this function
        ask_order_info_map_iter_ = sec_id_to_order_id_and_ask_order_info_map_[t_security_id_].find(t_order_id_);
      }
      if (ask_order_info_map_iter_ != sec_id_to_order_id_and_ask_order_info_map_[t_security_id_].end()) {
        OSE_ITCH_MDS::OSEOrderInfo* order_info = ask_order_info_map_iter_->second;
        old_order_price = order_info->price;
        old_order_size = order_info->size;
        // old_side = order_info->side;
        // update the size
        order_info->size = t_new_size_;
        if (t_order_id_ != t_new_order_id_) {
          // ask_order_info_map_iter_->second = nullptr;
          sec_id_to_order_id_and_ask_order_info_map_[t_security_id_].erase(ask_order_info_map_iter_);
          sec_id_to_order_id_and_ask_order_info_map_[t_security_id_][t_new_order_id_] = order_info;
        }
      } else {
        DBGLOG_CLASS_FUNC_LINE_INFO << watch_.tv_ToString()
                                    << " SecName: " << sec_name_indexer_.GetSecurityNameFromId(t_security_id_)
                                    << " SecId: " << t_security_id_ << " Error: AskOrderId: " << t_order_id_
                                    << " not present to modify." << DBGLOG_ENDL_FLUSH;
        return;
      }

#if PRINT_ORDER_INFO_MODE_OSE_OF
      std::cout << " OLD : [" << t_order_id_ << "," << old_order_size << "," 
                << "] PRICE: " << old_order_price << std::endl;
#endif

      int_price = smv_.GetIntPx(old_order_price);

      if (!smv_.initial_book_constructed_) {
        BuildIndex(t_security_id_, buysell, int_price);
        smv_.initial_book_constructed_ = true;
      }

      int ask_index = smv_.base_ask_index_ +
                      (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - int_price);

      is_order_present_in_best_level = (ask_index == (int)smv_.base_ask_index_);

      if (smv_.market_update_info_.asklevels_[ask_index].limit_size_ > 0)
        cumulative_old_size = smv_.market_update_info_.asklevels_[ask_index].limit_size_;
      if (smv_.market_update_info_.asklevels_[ask_index].limit_ordercount_ > 0)
        cumulative_old_ordercount = smv_.market_update_info_.asklevels_[ask_index].limit_ordercount_;

      smv_.market_update_info_.asklevels_[ask_index].limit_size_ -= old_order_size;

      if (smv_.market_update_info_.asklevels_[ask_index].limit_size_ >= 0)
        smv_.market_update_info_.asklevels_[ask_index].limit_size_ += t_new_size_;
      else
        smv_.market_update_info_.asklevels_[ask_index].limit_size_ = t_new_size_;

      is_level_deleted = ((smv_.market_update_info_.asklevels_[ask_index].limit_size_ <= 0) ||
                          (smv_.market_update_info_.asklevels_[ask_index].limit_ordercount_ <= 0));

      if (!is_level_deleted) {
        cumulative_new_size = smv_.market_update_info_.asklevels_[ask_index].limit_size_;
      }

      if (is_level_deleted) {
        // checking if the level deleted is the best_ask_level
        if (is_order_present_in_best_level) {
          UpdateBaseAskIndex(t_security_id_, buysell);
        } else {
          smv_.l2_changed_since_last_ = true;  // indicating that level other than best ask level has been changed
        }
      } else {
        if (is_order_present_in_best_level) {
          smv_.l1_changed_since_last_ = true;  // indicating that best ask level has been updated
        } else {
          smv_.l2_changed_since_last_ = true;  // indicating that level other than best ask level has been changed
        }
      }

      for (int t_index = smv_.base_ask_index_ - 1; t_index >= ask_index; t_index--) {
        if (smv_.market_update_info_.asklevels_[t_index].limit_size_ > 0 &&
            smv_.market_update_info_.asklevels_[t_index].limit_ordercount_ > 0)
          level_modified++;
      }
    } break;
    default: {
      DBGLOG_CLASS_FUNC_LINE_INFO << watch_.tv_ToString()
                                  << " SecName: " << sec_name_indexer_.GetSecurityNameFromId(t_security_id_)
                                  << " SecId: " << t_security_id_ << " Invalid Side (Niether B or S): " << t_side_
                                  << DBGLOG_ENDL_FLUSH;
      return;
    } break;
  }

  if (smv_.l1_changed_since_last_) {
    switch (buysell) {
      case kTradeTypeBuy: {
        UpdateBestBidVariables(t_security_id_, smv_.base_bid_index_);
        smv_.UpdateL1Prices();
      } break;
      case kTradeTypeSell: {
        UpdateBestAskVariables(t_security_id_, smv_.base_ask_index_);
        smv_.UpdateL1Prices();
      } break;
      default:
        break;
    }
  }

  sec_id_to_prev_update_was_quote_[t_security_id_] = true;

  if (!smv_.is_ready_) {
    if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ > 0 &&
        smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ > 0 && smv_.ArePricesComputed()) {
      smv_.is_ready_ = true;
    } else {
      return;
    }
  }

  if (smv_.market_update_info_.trade_update_implied_quote_) {
    smv_.market_update_info_.trade_update_implied_quote_ = false;
    smv_.trade_print_info_.num_trades_++;
  }

  // store price level change state if called from delta for trade
  if (t_is_set_order_info_map_iter_ && smv_.pl_change_listeners_present_) {
    cumulative_old_size = std::max(cumulative_old_size, prev_PL_change_state_.old_size_);
    cumulative_old_ordercount = std::max(cumulative_old_ordercount, prev_PL_change_state_.old_ordercount_);
    if (is_level_deleted) {
      prev_PL_change_state_.SetState(t_security_id_, buysell, level_modified, int_price, 0, cumulative_old_size, 0,
                                     cumulative_old_ordercount, 0, t_intermediate_, 'D');
    } else {
      prev_PL_change_state_.SetState(t_security_id_, buysell, level_modified, int_price, 0, cumulative_old_size,
                                     cumulative_new_size, cumulative_old_ordercount, cumulative_old_ordercount,
                                     t_intermediate_, 'C');
    }
  }

  if (t_intermediate_) {
    return;
  }

  CheckToNotifyTradeMessage(t_security_id_, t_intermediate_);

  if (!t_is_set_order_info_map_iter_ && smv_.pl_change_listeners_present_) {
    if (is_level_deleted) {
      smv_.NotifyOnPLChangeListeners(t_security_id_, smv_.market_update_info_, buysell, level_modified, int_price, 0,
                                     cumulative_old_size, 0, cumulative_old_ordercount, 0, t_intermediate_, 'D');
    } else {
      smv_.NotifyOnPLChangeListeners(t_security_id_, smv_.market_update_info_, buysell, level_modified, int_price, 0,
                                     cumulative_old_size, cumulative_new_size, cumulative_old_ordercount,
                                     cumulative_old_ordercount, t_intermediate_, 'C');
    }
  }

  if (smv_.l1_changed_since_last_) {
    smv_.NotifyL1PriceListeners();
    smv_.l1_changed_since_last_ = false;
  } else {
    smv_.NotifyL2Listeners();
  }

  if (smv_.l2_changed_since_last_) {
    smv_.NotifyL2OnlyListeners();
    smv_.l2_changed_since_last_ = false;
  }

  smv_.NotifyOnReadyListeners();
}

/*
 * This function assumes that the order exec received has been for a resting order,we
 * simulate it as Delete ( if size is 0 ) or Modify ( if still has some size )
 */
void IndexedOseOrderFeedMarketViewManager::OnOrderExec(const unsigned int t_security_id_, uint64_t t_order_id_,
                                                       uint8_t t_side_, double t_price_, int t_size_exec_,
                                                       bool t_intermediate_) {
#if PRINT_ORDER_INFO_MODE_OSE_OF
  std::cout << typeid(*this).name() << ':' << __func__ << " " << t_order_id_ << " ";
#endif

  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  // not till book is ready
  if ((smv_.market_update_info_.bestbid_int_price_ <= kInvalidIntPrice) ||
      (smv_.market_update_info_.bestask_int_price_ <= kInvalidIntPrice)) {
#if PRINT_ORDER_INFO_MODE_OSE_OF
    std::cout << "BEST PRICE <= kInvalidIntPrice" << std::endl;
#endif
    return;
  }

  if (t_side_ != 'B' && t_side_ != 'S') {
    DBGLOG_CLASS_FUNC_LINE_INFO << watch_.tv_ToString()
                                << " SecName: " << sec_name_indexer_.GetSecurityNameFromId(t_security_id_)
                                << " SecId: " << t_security_id_ << " Invalid side (Niether B or S): " << t_side_
                                << DBGLOG_ENDL_FLUSH;
#if PRINT_ORDER_INFO_MODE_OSE_OF
    std::cout << "INVALID SIDE (NIETHER 'B' NOR 'S'): " << t_side_ << std::endl;
#endif
    return;
  }

  int old_order_size = 0;
  double trade_px = 0.0;
  // Find the order_id_ in the map, if not preset its an error case, return
  if (t_side_ == 'B') {
    bid_order_info_map_iter_ = sec_id_to_order_id_and_bid_order_info_map_[t_security_id_].find(t_order_id_);

    if (bid_order_info_map_iter_ != sec_id_to_order_id_and_bid_order_info_map_[t_security_id_].end()) {
      old_order_size = bid_order_info_map_iter_->second->size;
      if (sec_id_to_delete_on_cross_[t_security_id_]) {
        trade_px = bid_order_info_map_iter_->second->price;
        sec_id_to_is_regular_session_[t_security_id_] = true;
      } else {  // if called from OnOrderExecWithTradeInfo
        trade_px = t_price_;
      }
    } else {
      DBGLOG_CLASS_FUNC_LINE_INFO << watch_.tv_ToString()
                                  << " SecName: " << sec_name_indexer_.GetSecurityNameFromId(t_security_id_)
                                  << " SecId: " << t_security_id_ << " Error: BidOrderId: " << t_order_id_
                                  << " not present for Exec." << DBGLOG_ENDL_FLUSH;
#if PRINT_ORDER_INFO_MODE_OSE_OF
      std::cout << "BID ORDER " << t_order_id_ << "NOT PRESENT FOR EXEC" << std::endl;
#endif
      return;
    }
  } else if (t_side_ == 'S') {
    ask_order_info_map_iter_ = sec_id_to_order_id_and_ask_order_info_map_[t_security_id_].find(t_order_id_);

    if (ask_order_info_map_iter_ != sec_id_to_order_id_and_ask_order_info_map_[t_security_id_].end()) {
      old_order_size = ask_order_info_map_iter_->second->size;
      if (sec_id_to_delete_on_cross_[t_security_id_]) {
        trade_px = ask_order_info_map_iter_->second->price;
        sec_id_to_is_regular_session_[t_security_id_] = true;
      } else {  // if called from OnOrderExecWithTradeInfo
        trade_px = t_price_;
      }

    } else {
      DBGLOG_CLASS_FUNC_LINE_INFO << watch_.tv_ToString()
                                  << " SecName: " << sec_name_indexer_.GetSecurityNameFromId(t_security_id_)
                                  << " SecId: " << t_security_id_ << " Error: AskOrderId: " << t_order_id_
                                  << " not present to Exec." << DBGLOG_ENDL_FLUSH;
#if PRINT_ORDER_INFO_MODE_OSE_OF
      std::cout << "ASK ORDER " << t_order_id_ << "NOT PRESENT FOR EXEC" << std::endl;
#endif
      return;
    }
  }
  int order_size_remained = old_order_size - t_size_exec_;
  int int_trade_price = smv_.GetIntPx(trade_px);

#if PRINT_ORDER_INFO_MODE_OSE_OF
  std::cout << "[" << old_order_size << "," << order_size_remained << "," << t_side_ << "," << trade_px << "]"
            << std::endl;
#endif

  if (int_trade_price < smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_) {
    SanitizeBidSide(t_security_id_, int_trade_price + 1);
    UpdateBestBidVariables(t_security_id_, smv_.base_bid_index_);
  }

  if (int_trade_price > smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_) {
    SanitizeAskSide(t_security_id_, int_trade_price - 1);
    UpdateBestAskVariables(t_security_id_, smv_.base_ask_index_);
  }

  if (smv_.trade_print_info_.computing_last_book_tdiff_) {
    smv_.market_update_info_.last_book_mkt_size_weighted_price_ = smv_.market_update_info_.mkt_size_weighted_price_;
  }

  if (!smv_.market_update_info_.trade_update_implied_quote_) {
    smv_.market_update_info_.trade_update_implied_quote_ = true;
  }

  TradeType_t aggressive_side = ((t_side_ == 'B') ? kTradeTypeSell : kTradeTypeBuy);

  smv_.StorePreTrade();

  switch (aggressive_side) {
    case kTradeTypeBuy:  // Aggressive buy
    {
      int trade_ask_index_ =
          smv_.base_ask_index_ +
          (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - int_trade_price);

      if (trade_ask_index_ > (int)smv_.base_ask_index_ || trade_ask_index_ < 0) {
        break;
      }

      if (t_size_exec_ == smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_) {
        int next_ask_index_ = smv_.base_ask_index_ - 1;
        for (; next_ask_index_ >= 0 && smv_.market_update_info_.asklevels_[next_ask_index_].limit_size_ <= 0;
             next_ask_index_--)
          ;
        if (next_ask_index_ < 0) {
          return;
        }

        UpdateBestAskVariables(t_security_id_, next_ask_index_);
      } else {
        smv_.market_update_info_.bestask_int_price_ =
            smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_;
        smv_.market_update_info_.bestask_ordercount_ =
            smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_ordercount_;
        smv_.market_update_info_.bestask_price_ =
            smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_price_;
        smv_.market_update_info_.bestask_size_ =
            smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ - t_size_exec_;
      }
    } break;
    case kTradeTypeSell:  // Aggressive sell
    {
      int trade_bid_index_ =
          smv_.base_bid_index_ -
          (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - int_trade_price);

      if (trade_bid_index_ > (int)smv_.base_bid_index_ || trade_bid_index_ < 0) {
        break;
      }

      // At this point, trade_bid_index_ == smv_.base_bid_index_ has to hold.
      if (t_size_exec_ == smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_) {
        int next_bid_index_ = smv_.base_bid_index_ - 1;
        for (; next_bid_index_ >= 0 && smv_.market_update_info_.bidlevels_[next_bid_index_].limit_size_ <= 0;
             next_bid_index_--)
          ;
        if (next_bid_index_ < 0) {
          return;
        }

        UpdateBestBidVariables(t_security_id_, next_bid_index_);

      } else {
        smv_.market_update_info_.bestbid_int_price_ =
            smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_;
        smv_.market_update_info_.bestbid_ordercount_ =
            smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_ordercount_;
        smv_.market_update_info_.bestbid_price_ =
            smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_price_;
        smv_.market_update_info_.bestbid_size_ =
            smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ - t_size_exec_;
      }
    } break;
    default:
      break;
  }

  smv_.UpdateL1Prices();

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(29);
#endif

  // Even if this exec is an intermediate message, we accumulate execs and wait for trade notification.
  CheckAndBufferTrades(t_security_id_, t_side_, trade_px, t_size_exec_, int_trade_price);

  sec_id_to_prev_update_was_quote_[t_security_id_] = false;

  if (order_size_remained > 0) {  // delta for trade
    OnOrderModify(t_security_id_, t_order_id_, t_side_, order_size_remained, t_order_id_, t_intermediate_, true);
  } else {
    OnOrderDelete(t_security_id_, t_order_id_, t_side_, t_intermediate_, true);
  }
}

void IndexedOseOrderFeedMarketViewManager::OnOrderExecWithTradeInfo(const uint32_t security_id, const uint64_t order_id,
                                                                    const uint8_t t_side_, const double exec_price,
                                                                    const uint32_t size_exec, bool intermediate) {
#if PRINT_ORDER_INFO_MODE_OSE_OF
  std::cout << typeid(*this).name() << ':' << __func__ << " ";
#endif

  sec_id_to_delete_on_cross_[security_id] = false;
  OnOrderExec(security_id, order_id, t_side_, exec_price, size_exec, intermediate);
  sec_id_to_delete_on_cross_[security_id] = true;
}

void IndexedOseOrderFeedMarketViewManager::UpdateBaseBidIndex(const unsigned int t_security_id_,
                                                              TradeType_t t_buysell_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);
  int next_bid_index_ = smv_.base_bid_index_ - 1;

  // finding the next best bid index
  for (; next_bid_index_ >= 0; next_bid_index_--) {
    if ((smv_.market_update_info_.bidlevels_[next_bid_index_].limit_size_ <= 0) ||
        (smv_.market_update_info_.bidlevels_[next_bid_index_].limit_ordercount_ <= 0)) {
      smv_.market_update_info_.bidlevels_[next_bid_index_].limit_size_ = 0;
      smv_.market_update_info_.bidlevels_[next_bid_index_].limit_ordercount_ = 0;
    } else {
      break;
    }
  }

  if (next_bid_index_ < 0) {
    smv_.is_ready_ = false;
    return;
  }

  smv_.base_bid_index_ = next_bid_index_;  // updating the best bid level

  smv_.l1_changed_since_last_ = true;  // indicating that best bid level has been updated

  if (smv_.base_bid_index_ < LOW_ACCESS_INDEX) {
    RebuildIndexLowAccess(t_security_id_, t_buysell_,
                          smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_);
  }
}

void IndexedOseOrderFeedMarketViewManager::UpdateBaseAskIndex(const unsigned int t_security_id_,
                                                              TradeType_t t_buysell_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);
  int next_ask_index_ = smv_.base_ask_index_ - 1;
  // finding the next best ask bid index
  for (; next_ask_index_ >= 0; next_ask_index_--) {
    if ((smv_.market_update_info_.asklevels_[next_ask_index_].limit_size_ <= 0) ||
        (smv_.market_update_info_.asklevels_[next_ask_index_].limit_ordercount_ <= 0)) {
      smv_.market_update_info_.asklevels_[next_ask_index_].limit_size_ = 0;
      smv_.market_update_info_.asklevels_[next_ask_index_].limit_ordercount_ = 0;
    } else {
      break;
    }
  }

  if (next_ask_index_ < 0) {
    smv_.is_ready_ = false;
    return;
  }

  smv_.base_ask_index_ = next_ask_index_;  // updating the best ask level

  smv_.l1_changed_since_last_ = true;  // indicating that best ask level has been updated

  if (smv_.base_ask_index_ < LOW_ACCESS_INDEX) {
    RebuildIndexLowAccess(t_security_id_, t_buysell_,
                          smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_);
  }
}

void IndexedOseOrderFeedMarketViewManager::SanitizeBidSide(const unsigned int t_security_id_, int t_int_price_) {
  if (sec_id_to_delete_on_cross_[t_security_id_] == false || sec_id_to_is_regular_session_[t_security_id_] == false) {
    return;
  }

#if PRINT_ORDER_INFO_MODE_OSE_OF
  std::cout << watch_.tv() << " " << typeid(*this).name() << ":" << __func__ << " ";
#endif

  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

// Delete all the orders correspondingly
#if PRINT_ORDER_INFO_MODE_OSE_OF
  std::cout << "DELETING BID ORDER_IDs: ";
#endif

  for (auto id_order_pair = sec_id_to_order_id_and_bid_order_info_map_[t_security_id_].begin(),
            tmp_itr = sec_id_to_order_id_and_bid_order_info_map_[t_security_id_].begin();
       id_order_pair != sec_id_to_order_id_and_bid_order_info_map_[t_security_id_].end();) {
    tmp_itr = id_order_pair;
    id_order_pair++;
    if (tmp_itr->second->side == 'B' && smv_.GetIntPx(tmp_itr->second->price) >= t_int_price_) {
#if PRINT_ORDER_INFO_MODE_OSE_OF
      std::cout << tmp_itr->first << " ";
#endif
      order_mempool_.DeAlloc(tmp_itr->second);
      sec_id_to_order_id_and_bid_order_info_map_[t_security_id_].erase(tmp_itr);
    }
  }
#if PRINT_ORDER_INFO_MODE_OSE_OF
  std::cout << std::endl;
#endif

  int index_ = smv_.base_bid_index_;

  for (; index_ >= 0; index_--) {
    if (smv_.market_update_info_.bidlevels_[index_].limit_int_price_ < t_int_price_ &&
        smv_.market_update_info_.bidlevels_[index_].limit_size_ > 0) {
      smv_.base_bid_index_ = index_;
      break;
    }

    smv_.market_update_info_.bidlevels_[index_].limit_size_ = 0;
    smv_.market_update_info_.bidlevels_[index_].limit_ordercount_ = 0;
  }

  // bid side is empty
  if (index_ < 0) {
    smv_.is_ready_ = false;
    return;
  }

  // Check if we need to re-align the center
  if (smv_.base_bid_index_ < LOW_ACCESS_INDEX) {
    RebuildIndexLowAccess(t_security_id_, kTradeTypeBuy,
                          smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_);
  }
}

void IndexedOseOrderFeedMarketViewManager::SanitizeAskSide(const unsigned int t_security_id_, int t_int_price_) {
  if (sec_id_to_delete_on_cross_[t_security_id_] == false || sec_id_to_is_regular_session_[t_security_id_] == false) {
    return;
  }
#if PRINT_ORDER_INFO_MODE_OSE_OF
  std::cout << watch_.tv() << " " << typeid(*this).name() << ":" << __func__ << " ";
#endif

  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

// Delete all the orders correspondingly
#if PRINT_ORDER_INFO_MODE_OSE_OF
  std::cout << "DELETING ASK ORDER_IDs: ";
#endif

  for (auto id_order_pair = sec_id_to_order_id_and_ask_order_info_map_[t_security_id_].begin(),
            tmp_itr = sec_id_to_order_id_and_ask_order_info_map_[t_security_id_].begin();
       id_order_pair != sec_id_to_order_id_and_ask_order_info_map_[t_security_id_].end();) {
    tmp_itr = id_order_pair;
    id_order_pair++;
    if (tmp_itr->second->side == 'S' && smv_.GetIntPx(tmp_itr->second->price) <= t_int_price_) {
#if PRINT_ORDER_INFO_MODE_OSE_OF
      std::cout << tmp_itr->first << " ";
#endif
      order_mempool_.DeAlloc(tmp_itr->second);
      sec_id_to_order_id_and_ask_order_info_map_[t_security_id_].erase(tmp_itr);
    }
  }
#if PRINT_ORDER_INFO_MODE_OSE_OF
  std::cout << std::endl;
#endif

  int t_index = smv_.base_ask_index_;

  for (; t_index >= 0; t_index--) {
    if (smv_.market_update_info_.asklevels_[t_index].limit_int_price_ > t_int_price_ &&
        smv_.market_update_info_.asklevels_[t_index].limit_size_ > 0) {
      smv_.base_ask_index_ = t_index;
      break;
    }
    smv_.market_update_info_.asklevels_[t_index].limit_size_ = 0;
    smv_.market_update_info_.asklevels_[t_index].limit_ordercount_ = 0;
  }

  // The ask side is empty
  if (t_index < 0) {
    smv_.is_ready_ = false;
    return;
  }

  // Check if we need to re-align the center
  if (smv_.base_ask_index_ < LOW_ACCESS_INDEX) {
    RebuildIndexLowAccess(t_security_id_, kTradeTypeSell,
                          smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_);
  }
}

void IndexedOseOrderFeedMarketViewManager::OnTradingStatus(const uint32_t security_id, std::string status) {
#if PRINT_ORDER_INFO_MODE_OSE_OF
  std::cout << typeid(*this).name() << ":" << __func__ << " SecId: " << security_id << " Status: " << status
            << std::endl;
#endif

  sec_id_to_is_regular_session_[security_id] = (status.find("ZARABA") != std::string::npos);
}

void IndexedOseOrderFeedMarketViewManager::OnOrderResetBegin(const unsigned int security_id) {
#if PRINT_ORDER_INFO_MODE_OSE_OF
  std::cout << typeid(*this).name() << ":" << __func__ << " SecId: " << security_id << std::endl;
#endif

  SecurityMarketView& smv_ = *(security_market_view_map_[security_id]);
  smv_.InitializeSMVForIndexedBook();

  // De- Allocate all the order structs
  for (auto id_order_pair = sec_id_to_order_id_and_bid_order_info_map_[security_id].begin();
       id_order_pair != sec_id_to_order_id_and_bid_order_info_map_[security_id].end(); id_order_pair++) {
    order_mempool_.DeAlloc(id_order_pair->second);
  }

  // De- Allocate all the order structs
  for (auto id_order_pair = sec_id_to_order_id_and_ask_order_info_map_[security_id].begin();
       id_order_pair != sec_id_to_order_id_and_ask_order_info_map_[security_id].end(); id_order_pair++) {
    order_mempool_.DeAlloc(id_order_pair->second);
  }

  // flushing all the order pointers
  sec_id_to_order_id_and_bid_order_info_map_[security_id].clear();
  sec_id_to_order_id_and_ask_order_info_map_[security_id].clear();
}

void IndexedOseOrderFeedMarketViewManager::CheckAndBufferTrades(const unsigned int t_security_id_, uint8_t side,
                                                                double price, int size, int int_price) {
  // get this trade aggressive side
  HFSAT::TradeType_t aggressive_side = ((side == 'B') ? kTradeTypeSell : kTradeTypeBuy);

  if (buffered_trades_[t_security_id_].size() > 0 &&
      buffered_trades_[t_security_id_].back().int_trade_price_ == int_price &&
      buffered_trades_[t_security_id_].back().buysell_ == aggressive_side) {
    buffered_trades_[t_security_id_].back().size_traded_ += size;
#if PRINT_ORDER_INFO_MODE_OSE_OF
    std::cout << typeid(*this).name() << ':' << __func__ << " Cumulate trade: " << price << " "
              << buffered_trades_[t_security_id_].back().size_traded_ << " " << (int)side << std::endl;
#endif
  } else {
    BufferTradeDetails t_cur;
    t_cur.buysell_ = aggressive_side;
    t_cur.price_ = price;
    t_cur.size_traded_ = size;
    t_cur.int_trade_price_ = int_price;
#if PRINT_ORDER_INFO_MODE_OSE_OF
    std::cout << typeid(*this).name() << ':' << __func__ << " Exec at new px: " << price << " " << size << " "
              << (int)side << " Num buffered execs: " << buffered_trades_[t_security_id_].size() << std::endl;
#endif
    buffered_trades_[t_security_id_].push_back(t_cur);
  }
  return;
}

// Notifying all buffered trades which couldn't be notified(intermediate msgs)
// This will be called after processing any order update [Add, Exec, Delete]
void IndexedOseOrderFeedMarketViewManager::CheckToNotifyTradeMessage(const unsigned int t_security_id_,
                                                                     bool is_intermediate_msg) {
  if (is_intermediate_msg || buffered_trades_[t_security_id_].size() == 0) return;

  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  while (buffered_trades_[t_security_id_].size() > 0) {
    BufferTradeDetails t_cur = buffered_trades_[t_security_id_].front();
    buffered_trades_[t_security_id_].pop_front();

    // Set the trade variables
    smv_.trade_print_info_.trade_price_ = t_cur.price_;
    smv_.trade_print_info_.size_traded_ = t_cur.size_traded_;
    smv_.trade_print_info_.int_trade_price_ = t_cur.int_trade_price_;
    smv_.trade_print_info_.buysell_ = t_cur.buysell_;

    smv_.SetTradeVarsForIndicatorsIfRequired();

    // Notifying trade
    if (smv_.is_ready_) {
#if PRINT_ORDER_INFO_MODE_OSE_OF
      std::cout << typeid(*this).name() << ':' << __func__ << " Notifying Trades at " << t_cur.price_
                << " size: " << t_cur.size_traded_ << " side " << (int)t_cur.buysell_ << std::endl;
#endif
      smv_.NotifyTradeListeners();
      smv_.NotifyOnReadyListeners();
    }
  }

  // Notified all buffered trades
  // Notifying delta in case pl change listener is present
  if (smv_.pl_change_listeners_present_) {
#if PRINT_ORDER_INFO_MODE_OSE_OF
    std::cout << typeid(*this).name() << ':' << __func__ << " Notifying trade and delta "
              << prev_PL_change_state_.sec_id_ << " " << prev_PL_change_state_.int_price_ << " "
              << prev_PL_change_state_.old_size_ << " " << prev_PL_change_state_.new_size_ << " "
              << (int)prev_PL_change_state_.buysell_ << std::endl;
#endif
    smv_.NotifyOnPLChangeListeners(prev_PL_change_state_.sec_id_, smv_.market_update_info_,
                                   prev_PL_change_state_.buysell_, prev_PL_change_state_.level_changed_,
                                   prev_PL_change_state_.int_price_, prev_PL_change_state_.int_price_level_,
                                   prev_PL_change_state_.old_size_, prev_PL_change_state_.new_size_,
                                   prev_PL_change_state_.old_ordercount_, prev_PL_change_state_.new_ordercount_,
                                   prev_PL_change_state_.is_intermediate_message_, prev_PL_change_state_.pl_notif_);
  }
  // reset delta
  prev_PL_change_state_.old_ordercount_ = -1;
  prev_PL_change_state_.old_size_ = -1;

  return;
}
}
