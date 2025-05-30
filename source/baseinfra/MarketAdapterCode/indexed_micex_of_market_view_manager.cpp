/**
   \file MarketAdapterCode/indexed_micex_of_market_view_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include <limits.h>
#include "baseinfra/MarketAdapter/indexed_micex_of_market_view_manager.hpp"

namespace HFSAT {

#define LOW_ACCESS_INDEX 50
#define INVALID_PRICE -100000.0

#define PRINT_ORDER_INFO_MODE_MICEX 0
#define MVM_PROFILING 0

IndexedMicexOFMarketViewManager::IndexedMicexOFMarketViewManager(
    DebugLogger& t_dbglogger_, const Watch& t_watch_, const SecurityNameIndexer& t_sec_name_indexer_,
    const std::vector<SecurityMarketView*>& t_security_market_view_map_, bool t_is_trade_prediction_on_)
    : BaseMarketViewManager(t_dbglogger_, t_watch_, t_sec_name_indexer_, t_security_market_view_map_),
      sec_id_to_prev_update_was_quote_(t_sec_name_indexer_.NumSecurityId(), false),
      bid_order_info_map_iter_(t_sec_name_indexer_.NumSecurityId()),
      order_id_to_bid_order_info_map_(t_sec_name_indexer_.NumSecurityId()),
      ask_order_info_map_iter_(t_sec_name_indexer_.NumSecurityId()),
      order_id_to_ask_order_info_map_(t_sec_name_indexer_.NumSecurityId()),
      sec_id_to_last_trade_notification_timestamp_(t_sec_name_indexer_.NumSecurityId(), 0),
      dbglogger_(t_dbglogger_),
      watch_(t_watch_),
      sec_name_indexer_(t_sec_name_indexer_),
      is_trade_prediction_on_(t_is_trade_prediction_on_) {
  aggregated_trades_buffer_[0].resize(t_sec_name_indexer_.NumSecurityId());
  aggregated_trades_buffer_[1].resize(t_sec_name_indexer_.NumSecurityId());
  if (is_trade_prediction_on_) {
    DBGLOG_CLASS_FUNC_LINE_INFO << "Trade Prediction is ON..." << '\n';
  } else {
    DBGLOG_CLASS_FUNC_LINE_INFO << "Trade Prediction is OFF..." << '\n';
  }
}

void IndexedMicexOFMarketViewManager::Process(const unsigned int security_id,
                                              MICEX_OF_MDS::MICEXOFCommonStruct* next_event) {
#if PRINT_ORDER_INFO_MODE_MICEX
  SecurityMarketView& smv_ = *(security_market_view_map_[security_id]);
  std::cout << "Before applying the update : " << std::endl;
  std::cout << "Is_ready : " << smv_.is_ready_ << std::endl;
  std::cout << next_event->ToString() << std::endl;
  std::cout << smv_.ShowMarket() << std::endl;

#endif

  MICEX_OF_MDS::MICEXOFMsgType msg_type = next_event->msg_type;

  switch (msg_type) {
    // Order Add
    case MICEX_OF_MDS::MICEXOFMsgType::kMICEXAdd: {
      OnOrderAdd(security_id, next_event->data.add.order_id, next_event->data.add.side, next_event->data.add.price,
                 next_event->data.add.size, next_event->intermediate_);
      break;
    }

    // Order delete
    case MICEX_OF_MDS::MICEXOFMsgType::kMICEXDelete: {
      OnOrderDelete(security_id, next_event->data.del.order_id, next_event->data.del.side, next_event->is_slower_,
                    next_event->intermediate_);
      break;
    }

    // Order modify
    case MICEX_OF_MDS::MICEXOFMsgType::kMICEXModify: {
      OnOrderModify(security_id, next_event->data.mod.order_id, next_event->data.mod.side, next_event->data.mod.size,
                    next_event->is_slower_, next_event->intermediate_);
      break;
    }

    // Order Exec
    case MICEX_OF_MDS::MICEXOFMsgType::kMICEXExec: {
      OnOrderExec(security_id, next_event->data.exec.order_id, next_event->data.exec.side, next_event->data.exec.price,
                  next_event->data.exec.size_exec, next_event->exchange_time_stamp, next_event->is_slower_,
                  next_event->intermediate_);
      break;
    }
    case MICEX_OF_MDS::MICEXOFMsgType::kMICEXBestLevelUpdate: {
      OnMSRL1Update(security_id, next_event);
    } break;

    case MICEX_OF_MDS::MICEXOFMsgType::kMICEXResetBegin: {
      OnOrderResetBegin(security_id);
    } break;
    case MICEX_OF_MDS::MICEXOFMsgType::kMICEXResetEnd: {
      OnOrderResetEnd(security_id);
    } break;

    default: { break; }
  }

#if PRINT_ORDER_INFO_MODE_MICEX
  std::cout << "After applying the update : " << std::endl;
  std::cout << "Is_ready : " << smv_.is_ready_ << std::endl;
  std::cout << "Best Ask variables : " << smv_.market_update_info_.bestask_price_ << " "
            << smv_.market_update_info_.bestask_int_price_ << " " << smv_.market_update_info_.bestask_size_
            << std::endl;
  std::cout << "Best Bid variables : " << smv_.market_update_info_.bestbid_price_ << " "
            << smv_.market_update_info_.bestbid_int_price_ << " " << smv_.market_update_info_.bestbid_size_
            << std::endl;
  std::cout << smv_.ShowMarket() << std::endl;
#endif
}

// Main Functions
void IndexedMicexOFMarketViewManager::OnOrderAdd(const unsigned int t_security_id_, uint64_t t_order_id_,
                                                 uint8_t t_side_, double t_price_, int t_size_, bool t_intermediate_) {
  if (t_price_ <= INVALID_PRICE) return;  // ignore market orders

  CheckToFlushAggregatedTrades(0, t_security_id_);
  CheckToFlushAggregatedTrades(1, t_security_id_);

#if MVM_PROFILING
  HFSAT::CpucycleProfiler::GetUniqueInstance().Start(29);
#endif

#if PRINT_ORDER_INFO_MODE_MICEX
  std::cout << typeid(*this).name() << ":" << __func__ << " " << t_order_id_ << " [" << t_price_ << "," << t_size_
            << "," << t_side_ << "]" << std::endl;
#endif

  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  int int_price = smv_.GetIntPx(t_price_);
  TradeType_t buysell = ((t_side_ == '0') ? kTradeTypeBuy : (t_side_ == '1' ? kTradeTypeSell : kTradeTypeNoInfo));

  if (!smv_.initial_book_constructed_) {
    BuildIndex(t_security_id_, buysell, int_price);
    smv_.initial_book_constructed_ = true;
  }

  int old_size = 0;
  int old_ordercount = 0;
  int level_added = 0;

  switch (buysell) {
    case kTradeTypeBuy: {
      bid_order_info_map_iter_[t_security_id_] = order_id_to_bid_order_info_map_[t_security_id_].find(t_order_id_);

      if (bid_order_info_map_iter_[t_security_id_] == order_id_to_bid_order_info_map_[t_security_id_].end()) {
        MICEX_OF_MDS::MicexOFOrderInfo* order_info_ptr = order_mempool_.Alloc();

        order_info_ptr->price = t_price_;
        order_info_ptr->size = t_size_;
        order_info_ptr->side = t_side_;
        order_info_ptr->best_level_updated_ = false;

        order_id_to_bid_order_info_map_[t_security_id_].insert(bid_order_info_map_iter_[t_security_id_],
                                                               std::make_pair(t_order_id_, order_info_ptr));

      } else {
        timeval tv;
        gettimeofday(&tv, NULL);
        DBGLOG_CLASS_FUNC_LINE_INFO << sec_name_indexer_.GetSecurityNameFromId(t_security_id_) << " watch@"
                                    << watch_.tv_ToString() << " "
                                    << "TimeOfDay@ " << tv.tv_sec << "." << tv.tv_usec
                                    << " Bid Order already present with order_id: " << t_order_id_ << "\n";
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
        ReScaleBook(t_security_id_, buysell, int_price);
        bid_index = (int)smv_.base_bid_index_ -
                    (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - int_price);
        if (bid_index < 0) {
          timeval tv;
          gettimeofday(&tv, NULL);
          DBGLOG_CLASS_FUNC_LINE_INFO
              << sec_name_indexer_.GetSecurityNameFromId(t_security_id_) << " watch@" << watch_.tv_ToString() << " "
              << "TimeOfDay@ " << tv.tv_sec << "." << tv.tv_usec
              << " Order added way below the best bid level, Ignoring this order with order_id :" << t_order_id_
              << "\n";
          return;
        }
      }

      if (bid_index >= (int)smv_.max_tick_range_) {
        RebuildIndexHighAccess(t_security_id_, buysell, int_price);
        bid_index = smv_.base_bid_index_;
      }

      if (smv_.market_update_info_.bidlevels_[bid_index].limit_size_ > 0)
        old_size = smv_.market_update_info_.bidlevels_[bid_index].limit_size_;
      if (smv_.market_update_info_.bidlevels_[bid_index].limit_ordercount_ > 0)
        old_ordercount = smv_.market_update_info_.bidlevels_[bid_index].limit_ordercount_;

      if ((smv_.market_update_info_.bidlevels_[bid_index].limit_size_ <= 0) ||
          (bid_index > (int)smv_.base_bid_index_)) {
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
      ask_order_info_map_iter_[t_security_id_] = order_id_to_ask_order_info_map_[t_security_id_].find(t_order_id_);

      if (ask_order_info_map_iter_[t_security_id_] == order_id_to_ask_order_info_map_[t_security_id_].end()) {
        MICEX_OF_MDS::MicexOFOrderInfo* order_info_ptr = order_mempool_.Alloc();

        order_info_ptr->price = t_price_;
        order_info_ptr->size = t_size_;
        order_info_ptr->side = t_side_;
        order_info_ptr->best_level_updated_ = false;

        order_id_to_ask_order_info_map_[t_security_id_].insert(ask_order_info_map_iter_[t_security_id_],
                                                               std::make_pair(t_order_id_, order_info_ptr));

      } else {
        timeval tv;
        gettimeofday(&tv, NULL);
        DBGLOG_CLASS_FUNC_LINE_INFO << sec_name_indexer_.GetSecurityNameFromId(t_security_id_) << " watch@"
                                    << watch_.tv_ToString() << " "
                                    << "TimeOfDay@ " << tv.tv_sec << "." << tv.tv_usec
                                    << " Ask Order already present with order_id: " << t_order_id_ << "\n";
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
        ReScaleBook(t_security_id_, buysell, int_price);
        ask_index = (int)smv_.base_ask_index_ +
                    (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - int_price);
        if (ask_index < 0) {
          timeval tv;
          gettimeofday(&tv, NULL);
          DBGLOG_CLASS_FUNC_LINE_INFO
              << sec_name_indexer_.GetSecurityNameFromId(t_security_id_) << " watch@" << watch_.tv_ToString() << " "
              << "TimeOfDay@ " << tv.tv_sec << "." << tv.tv_usec
              << " Order added way below the best ask level, Ignoring this order with order_id :" << t_order_id_
              << "\n";
          return;
        }
      }

      if (ask_index >= (int)smv_.max_tick_range_) {
        RebuildIndexHighAccess(t_security_id_, buysell, int_price);
        ask_index = smv_.base_ask_index_;
      }

      if (smv_.market_update_info_.asklevels_[ask_index].limit_size_ > 0)
        old_size = smv_.market_update_info_.asklevels_[ask_index].limit_size_;
      if (smv_.market_update_info_.asklevels_[ask_index].limit_ordercount_ > 0)
        old_ordercount = smv_.market_update_info_.asklevels_[ask_index].limit_ordercount_;

      if ((smv_.market_update_info_.asklevels_[ask_index].limit_size_ <= 0) ||
          (ask_index > (int)smv_.base_ask_index_)) {
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
      timeval tv;
      gettimeofday(&tv, NULL);
      DBGLOG_CLASS_FUNC_LINE_INFO << sec_name_indexer_.GetSecurityNameFromId(t_security_id_) << " watch@"
                                  << watch_.tv_ToString() << " "
                                  << "TimeOfDay@ " << tv.tv_sec << "." << tv.tv_usec
                                  << " Side is neither B or S: " << t_side_ << "\n";
      return;
    } break;
  }

#if PRINT_ORDER_INFO_MODE_MICEX
  std::cout << typeid(*this).name() << ":" << __func__ << "Order Added at level :" << level_added << "...."
            << std::endl;
#endif

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

#if MVM_PROFILING
  HFSAT::CpucycleProfiler::GetUniqueInstance().End(29);
#endif

  if (t_intermediate_) {
    return;
  }

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

void IndexedMicexOFMarketViewManager::OnOrderDelete(const unsigned int t_security_id_, uint64_t t_order_id_,
                                                    uint8_t t_side_, bool t_is_slower_, bool t_intermediate_,
                                                    bool t_is_set_order_info_map_iter_) {
  if (!t_intermediate_) {
    CheckToFlushAggregatedTrades(0, t_security_id_);
    CheckToFlushAggregatedTrades(1, t_security_id_);
  }

#if PRINT_ORDER_INFO_MODE_MICEX
  std::cout << typeid(*this).name() << ':' << __func__ << " " << t_order_id_;
#endif

#if MVM_PROFILING
  HFSAT::CpucycleProfiler::GetUniqueInstance().Start(30);
#endif

  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  bool are_best_variables_updated = false;

  if (!smv_.initial_book_constructed_) {
    return;
  }

  int order_size = 0;
  double order_price = 0.0;
  TradeType_t buysell = ((t_side_ == '0') ? kTradeTypeBuy : (t_side_ == '1' ? kTradeTypeSell : kTradeTypeNoInfo));

  int cumulative_old_size = 0;
  int cumulative_old_ordercount = 0;
  int int_price = 0;
  bool is_level_deleted = false;
  bool is_order_present_in_best_level = false;

  int level_changed = 0;

  switch (buysell) {
    case kTradeTypeBuy: {
      // searching the bid_order_map to retrieve the meta-data corresponding to @t_order_id
      if (!t_is_set_order_info_map_iter_) {  // checking if the iterator is set already before calling this function
        bid_order_info_map_iter_[t_security_id_] = order_id_to_bid_order_info_map_[t_security_id_].find(t_order_id_);
      }
      if (bid_order_info_map_iter_[t_security_id_] != order_id_to_bid_order_info_map_[t_security_id_].end()) {
        order_price = bid_order_info_map_iter_[t_security_id_]->second->price;
        order_size = bid_order_info_map_iter_[t_security_id_]->second->size;

        are_best_variables_updated = bid_order_info_map_iter_[t_security_id_]->second->best_level_updated_;

        order_mempool_.DeAlloc(bid_order_info_map_iter_[t_security_id_]->second);

        // bid_order_info_map_iter_[t_security_id_]->second = nullptr;

        order_id_to_bid_order_info_map_[t_security_id_].erase(bid_order_info_map_iter_[t_security_id_]);

      } else {
        timeval tv;
        gettimeofday(&tv, NULL);
        DBGLOG_CLASS_FUNC_LINE_INFO << sec_name_indexer_.GetSecurityNameFromId(t_security_id_) << " watch@"
                                    << watch_.tv_ToString() << " "
                                    << "TimeOfDay@ " << tv.tv_sec << "." << tv.tv_usec
                                    << " Error: Bid OrderId: " << t_order_id_ << " not present to delete."
                                    << "\n";
        return;
      }
#if PRINT_ORDER_INFO_MODE_MICEX
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

#if PRINT_ORDER_INFO_MODE_MICEX
      std::cout << "Before Deleting : [ Size - " << cumulative_old_size << " , OrderCount - "
                << cumulative_old_ordercount << " ] " << std::endl;
#endif

      // modify the data at level @bid_index
      smv_.market_update_info_.bidlevels_[bid_index].limit_size_ -= order_size;
      smv_.market_update_info_.bidlevels_[bid_index].limit_ordercount_--;

      if (smv_.market_update_info_.bidlevels_[bid_index].limit_size_ < 0 ||
          smv_.market_update_info_.bidlevels_[bid_index].limit_ordercount_ < 0) {
#if PRINT_ORDER_INFO_MODE_MICEX
        std::cerr << typeid(*this).name() << ':' << __func__
                  << " order_size/order_count is negative after deleting the order..." << std::endl;
#endif

        smv_.market_update_info_.bidlevels_[bid_index].limit_size_ = 0;
        smv_.market_update_info_.bidlevels_[bid_index].limit_ordercount_ = 0;
      }

      // checking if no more orders are left at @bid_index
      is_level_deleted = ((smv_.market_update_info_.bidlevels_[bid_index].limit_size_ <= 0) ||
                          (smv_.market_update_info_.bidlevels_[bid_index].limit_ordercount_ <= 0));

#if PRINT_ORDER_INFO_MODE_MICEX
      if (is_level_deleted) std::cout << "Level Deleted...." << std::endl;
#endif

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

      if (!t_is_set_order_info_map_iter_) {  // checking if the iterator is set already before calling this function
        ask_order_info_map_iter_[t_security_id_] = order_id_to_ask_order_info_map_[t_security_id_].find(t_order_id_);
      }
      if (ask_order_info_map_iter_[t_security_id_] != order_id_to_ask_order_info_map_[t_security_id_].end()) {
        order_price = ask_order_info_map_iter_[t_security_id_]->second->price;
        order_size = ask_order_info_map_iter_[t_security_id_]->second->size;

        are_best_variables_updated = ask_order_info_map_iter_[t_security_id_]->second->best_level_updated_;

        order_mempool_.DeAlloc(ask_order_info_map_iter_[t_security_id_]->second);
        //        ask_order_info_map_iter_[t_security_id_]->second = nullptr;

        order_id_to_ask_order_info_map_[t_security_id_].erase(ask_order_info_map_iter_[t_security_id_]);

      } else {
        timeval tv;
        gettimeofday(&tv, NULL);
        DBGLOG_CLASS_FUNC_LINE_INFO << sec_name_indexer_.GetSecurityNameFromId(t_security_id_) << " watch@"
                                    << watch_.tv_ToString() << " "
                                    << "TimeOfDay@ " << tv.tv_sec << "." << tv.tv_usec
                                    << " Error: Ask OrderId: " << t_order_id_ << " not present to delete."
                                    << "\n";
        return;
      }
#if PRINT_ORDER_INFO_MODE_MICEX
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

#if PRINT_ORDER_INFO_MODE_MICEX
      std::cout << "Before Deleting : [ Size - " << cumulative_old_size << " , OrderCount - "
                << cumulative_old_ordercount << " ] " << std::endl;
#endif

      // modify the data at level @ask_index
      smv_.market_update_info_.asklevels_[ask_index].limit_size_ -= order_size;
      smv_.market_update_info_.asklevels_[ask_index].limit_ordercount_--;

      if (smv_.market_update_info_.asklevels_[ask_index].limit_size_ < 0 ||
          smv_.market_update_info_.asklevels_[ask_index].limit_ordercount_ < 0) {
#if PRINT_ORDER_INFO_MODE_TMX_OBF
        std::cerr << typeid(*this).name() << ':' << __func__
                  << " order_size/order_count is negative after deleting the order..." << std::endl;
#endif
        smv_.market_update_info_.asklevels_[ask_index].limit_size_ = 0;
        smv_.market_update_info_.asklevels_[ask_index].limit_ordercount_ = 0;
      }

      // checking if no more orders are left at @ask_index
      is_level_deleted = ((smv_.market_update_info_.asklevels_[ask_index].limit_size_ <= 0) ||
                          (smv_.market_update_info_.asklevels_[ask_index].limit_ordercount_ <= 0));

#if PRINT_ORDER_INFO_MODE_MICEX
      if (is_level_deleted) std::cout << "Level Deleted...." << std::endl;
#endif

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
      timeval tv;
      gettimeofday(&tv, NULL);
      DBGLOG_CLASS_FUNC_LINE_INFO << sec_name_indexer_.GetSecurityNameFromId(t_security_id_) << " watch@"
                                  << watch_.tv_ToString() << " "
                                  << "TimeOfDay@ " << tv.tv_sec << "." << tv.tv_usec << " Invalid Side: " << t_side_
                                  << "\n";
      return;
    } break;
  }

#if PRINT_ORDER_INFO_MODE_MICEX
  std::cout << typeid(*this).name() << ":" << __func__ << "Order Deleted at level :" << level_changed << "...."
            << std::endl;
#endif

  are_best_variables_updated = (are_best_variables_updated || t_is_slower_);

  if (smv_.l1_changed_since_last_ && (!are_best_variables_updated)) {
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

#if MVM_PROFILING
  HFSAT::CpucycleProfiler::GetUniqueInstance().End(30);
#endif

  if (t_intermediate_ || are_best_variables_updated) {
    return;
  }

  if (smv_.pl_change_listeners_present_) {
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
 * but prices will remain same as before.
 */
void IndexedMicexOFMarketViewManager::OnOrderModify(const unsigned int t_security_id_, uint64_t t_order_id_,
                                                    uint8_t t_side_, int t_new_size_, bool t_is_slower_,
                                                    bool t_intermediate_, bool t_is_set_order_info_map_iter_) {
#if PRINT_ORDER_INFO_MODE_MICEX
  std::cout << typeid(*this).name() << ':' << __func__ << " " << t_order_id_;
  std::cout << "[" << t_new_size_ << "," << t_side_ << "]" << std::endl;
#endif

#if MVM_PROFILING
  HFSAT::CpucycleProfiler::GetUniqueInstance().Start(31);
#endif

  CheckToFlushAggregatedTrades(0, t_security_id_);
  CheckToFlushAggregatedTrades(1, t_security_id_);

  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  bool are_best_variables_updated = false;

  double old_order_price = 0.0;
  int old_order_size = 0;

  int int_price = 0;
  TradeType_t buysell = ((t_side_ == '0') ? kTradeTypeBuy : (t_side_ == '1' ? kTradeTypeSell : kTradeTypeNoInfo));

  int cumulative_old_size = 0;
  int cumulative_new_size = 0;
  int cumulative_old_ordercount = 0;
  int level_modified = 0;

  bool is_level_deleted = false;
  bool is_order_present_in_best_level = false;

  switch (buysell) {
    case kTradeTypeBuy: {
      if (!t_is_set_order_info_map_iter_) {  // checking if the iterator is set already before calling this function
        bid_order_info_map_iter_[t_security_id_] = order_id_to_bid_order_info_map_[t_security_id_].find(t_order_id_);
      }
      if (bid_order_info_map_iter_[t_security_id_] != order_id_to_bid_order_info_map_[t_security_id_].end()) {
        MICEX_OF_MDS::MicexOFOrderInfo* order_info = bid_order_info_map_iter_[t_security_id_]->second;
        old_order_price = order_info->price;
        old_order_size = order_info->size;

        are_best_variables_updated = order_info->best_level_updated_;

        ToggleBooleanValue(order_info->best_level_updated_);

        if (old_order_size == t_new_size_) {
          return;
        }

        // update the size
        order_info->size = t_new_size_;

      } else {
        timeval tv;
        gettimeofday(&tv, NULL);
        DBGLOG_CLASS_FUNC_LINE_INFO << sec_name_indexer_.GetSecurityNameFromId(t_security_id_) << " watch@"
                                    << watch_.tv_ToString() << " "
                                    << "TimeOfDay@ " << tv.tv_sec << "." << tv.tv_usec
                                    << " Error: BidOrderId: " << t_order_id_ << " not present to replace."
                                    << "\n";
        return;
      }

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
        ask_order_info_map_iter_[t_security_id_] = order_id_to_ask_order_info_map_[t_security_id_].find(t_order_id_);
      }
      if (ask_order_info_map_iter_[t_security_id_] != order_id_to_ask_order_info_map_[t_security_id_].end()) {
        MICEX_OF_MDS::MicexOFOrderInfo* order_info = ask_order_info_map_iter_[t_security_id_]->second;
        old_order_price = order_info->price;
        old_order_size = order_info->size;

        are_best_variables_updated = order_info->best_level_updated_;

        ToggleBooleanValue(order_info->best_level_updated_);

        if (old_order_size == t_new_size_) {
          return;
        }

        // update the size
        order_info->size = t_new_size_;
      } else {
        timeval tv;
        gettimeofday(&tv, NULL);
        DBGLOG_CLASS_FUNC_LINE_INFO << sec_name_indexer_.GetSecurityNameFromId(t_security_id_) << " watch@"
                                    << watch_.tv_ToString() << " "
                                    << "TimeOfDay@ " << tv.tv_sec << "." << tv.tv_usec
                                    << " Error: AskOrderId: " << t_order_id_ << " not present to replace."
                                    << "\n";
        return;
      }

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
      timeval tv;
      gettimeofday(&tv, NULL);
      DBGLOG_CLASS_FUNC_LINE_INFO << sec_name_indexer_.GetSecurityNameFromId(t_security_id_) << " watch@"
                                  << watch_.tv_ToString() << " "
                                  << "TimeOfDay@ " << tv.tv_sec << "." << tv.tv_usec << " Invalid Side : " << t_side_
                                  << "\n";
      return;
    } break;
  }

#if PRINT_ORDER_INFO_MODE_MICEX
  std::cout << typeid(*this).name() << ":" << __func__ << "Order modified at level :" << level_modified << "...."
            << std::endl;
#endif

  are_best_variables_updated = (are_best_variables_updated || t_is_slower_);

  if (smv_.l1_changed_since_last_ && (!are_best_variables_updated)) {
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

#if MVM_PROFILING
  HFSAT::CpucycleProfiler::GetUniqueInstance().End(31);
#endif

  if (t_intermediate_ || are_best_variables_updated) {
    return;
  }

  if (smv_.pl_change_listeners_present_) {
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
void IndexedMicexOFMarketViewManager::OnOrderExec(const unsigned int t_security_id_, uint64_t t_order_id_,
                                                  uint8_t t_aggressive_side_, double t_price_, int t_size_exec_,
                                                  uint64_t t_exchange_timestamp_, bool t_is_slower_,
                                                  bool t_intermediate_) {
#if PRINT_ORDER_INFO_MODE_MICEX
  std::cout << typeid(*this).name() << ':' << __func__ << " " << t_order_id_;
#endif

  uint8_t t_passive_side_ = GetOppositeTradeSide(t_aggressive_side_);

  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  // not till book is ready
  if ((smv_.market_update_info_.bestbid_int_price_ <= kInvalidIntPrice) ||
      (smv_.market_update_info_.bestask_int_price_ <= kInvalidIntPrice)) {
    return;
  }

#if MVM_PROFILING
  HFSAT::CpucycleProfiler::GetUniqueInstance().Start(33);
#endif

  int int_trade_price = smv_.GetIntPx(t_price_);

  if (!(t_passive_side_ == 'B' || t_passive_side_ == 'S')) {
    return;
  }

  bool are_best_variables_updated = false;

  // Find the order_id_ in the map, if not preset its an error case, return
  if (t_passive_side_ == 'B') {
    bid_order_info_map_iter_[t_security_id_] = order_id_to_bid_order_info_map_[t_security_id_].find(t_order_id_);

    if (bid_order_info_map_iter_[t_security_id_] != order_id_to_bid_order_info_map_[t_security_id_].end()) {
      are_best_variables_updated = bid_order_info_map_iter_[t_security_id_]->second->best_level_updated_;
      ToggleBooleanValue(bid_order_info_map_iter_[t_security_id_]->second->best_level_updated_);
    } else {
      are_best_variables_updated = true;
    }
  } else if (t_passive_side_ == 'S') {
    ask_order_info_map_iter_[t_security_id_] = order_id_to_ask_order_info_map_[t_security_id_].find(t_order_id_);

    if (ask_order_info_map_iter_[t_security_id_] != order_id_to_ask_order_info_map_[t_security_id_].end()) {
      are_best_variables_updated = ask_order_info_map_iter_[t_security_id_]->second->best_level_updated_;
      ToggleBooleanValue(ask_order_info_map_iter_[t_security_id_]->second->best_level_updated_);
    } else {
      are_best_variables_updated = true;
    }
  }

  if (smv_.trade_print_info_.computing_last_book_tdiff_) {
    smv_.market_update_info_.last_book_mkt_size_weighted_price_ = smv_.market_update_info_.mkt_size_weighted_price_;
  }

  if (!smv_.market_update_info_.trade_update_implied_quote_) {
    smv_.market_update_info_.trade_update_implied_quote_ = true;
  }

  TradeType_t aggressive_side = ((t_passive_side_ == 'B') ? kTradeTypeSell : kTradeTypeBuy);

  smv_.StorePreTrade();

  are_best_variables_updated = (are_best_variables_updated || t_is_slower_);

  if (!are_best_variables_updated) {
    switch (aggressive_side) {
      case kTradeTypeBuy:  // Aggressive buy
      {
#if PRINT_ORDER_INFO_MODE_MICEX
        std::cout << "Updating Best Ask variables..." << std::endl;
#endif

        int trade_ask_index_ =
            smv_.base_ask_index_ +
            (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - int_trade_price);

        if (trade_ask_index_ > (int)smv_.base_ask_index_ || trade_ask_index_ < 0) {
          break;
        }

        // Update best variables using already set best variables
        if (!sec_id_to_prev_update_was_quote_[t_security_id_]) {
          if ((int_trade_price == smv_.market_update_info_.bestask_int_price_) &&
              (t_size_exec_ < smv_.market_update_info_.bestask_size_)) {
            smv_.market_update_info_.bestask_size_ -= t_size_exec_;
          } else {
            int next_ask_index_ = trade_ask_index_ - 1;
            for (; next_ask_index_ >= 0 && smv_.market_update_info_.asklevels_[next_ask_index_].limit_size_ <= 0;
                 next_ask_index_--)
              ;

            if (next_ask_index_ < 0) {
              return;
            }

            UpdateBestAskVariables(t_security_id_, next_ask_index_);
          }

          break;
        }

        if (t_size_exec_ >= smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_) {
          int next_ask_index_ = smv_.base_ask_index_ - 1;
          for (; next_ask_index_ >= 0 && smv_.market_update_info_.asklevels_[next_ask_index_].limit_size_ <= 0;
               next_ask_index_--)
            ;
          if (next_ask_index_ < 0) {
            timeval tv;
            gettimeofday(&tv, NULL);
            DBGLOG_CLASS_FUNC_LINE_INFO << sec_name_indexer_.GetSecurityNameFromId(t_security_id_) << " watch@"
                                        << watch_.tv_ToString() << " "
                                        << "TimeOfDay@ " << tv.tv_sec << "." << tv.tv_usec
                                        << " Empty Book on Ask Side after trade"
                                        << "\n";
            smv_.is_ready_ = false;
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
#if PRINT_ORDER_INFO_MODE_MICEX
        std::cout << "Updating Best Bid variables..." << std::endl;
#endif

        int trade_bid_index_ =
            smv_.base_bid_index_ -
            (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - int_trade_price);

        if (trade_bid_index_ > (int)smv_.base_bid_index_ || trade_bid_index_ < 0) {
          break;
        }

        // Update best variables using already set best variables
        if (!sec_id_to_prev_update_was_quote_[t_security_id_]) {
          if ((int_trade_price == smv_.market_update_info_.bestbid_int_price_) &&
              (t_size_exec_ < smv_.market_update_info_.bestbid_size_)) {
            smv_.market_update_info_.bestbid_size_ -= t_size_exec_;
          } else {
            int next_bid_index_ = trade_bid_index_ - 1;
            for (; next_bid_index_ >= 0 && smv_.market_update_info_.bidlevels_[next_bid_index_].limit_size_ <= 0;
                 next_bid_index_--)
              ;

            if (next_bid_index_ < 0) {
              return;
            }

            UpdateBestBidVariables(t_security_id_, next_bid_index_);
          }

          break;
        }

        // At this point, trade_bid_index_ == smv_.base_bid_index_ has to hold.
        if (t_size_exec_ >= smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_) {
          int next_bid_index_ = smv_.base_bid_index_ - 1;
          for (; next_bid_index_ >= 0 && smv_.market_update_info_.bidlevels_[next_bid_index_].limit_size_ <= 0;
               next_bid_index_--)
            ;
          if (next_bid_index_ < 0) {
            timeval tv;
            gettimeofday(&tv, NULL);
            DBGLOG_CLASS_FUNC_LINE_INFO << sec_name_indexer_.GetSecurityNameFromId(t_security_id_) << " watch@"
                                        << watch_.tv_ToString() << " "
                                        << "TimeOfDay@ " << tv.tv_sec << "." << tv.tv_usec
                                        << " Empty Book on Bid Side after trade"
                                        << "\n";
            smv_.is_ready_ = false;
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
  }

  // check if we have already predicted these trades
  if (is_trade_prediction_on_ &&
      sec_id_to_last_trade_notification_timestamp_[t_security_id_] >= t_exchange_timestamp_) {
#if PRINT_ORDER_INFO_MODE_MICEX
    DBGLOG_CLASS_FUNC_LINE_INFO << "Supressed Trade : " << sec_name_indexer_.GetSecurityNameFromId(t_security_id_)
                                << " watch@" << watch_.tv_ToString() << " [ " << t_size_exec_ << " " << t_price_
                                << " ]\n";
#endif

    return;
  }

  smv_.UpdateL1Prices();

  if (aggressive_side == kTradeTypeSell) {
    if ((aggregated_trades_buffer_[0][t_security_id_].size() > 0) &&
        (aggregated_trades_buffer_[0][t_security_id_].back()->int_trade_price == int_trade_price)) {
      aggregated_trades_buffer_[0][t_security_id_].back()->size_traded += t_size_exec_;
    } else {
      HFSAT::MicexOFBufferedOrderExecData* t_cur = aggregated_trades_mempool_.Alloc();
      t_cur->aggressive_side = aggressive_side;
      t_cur->trade_price = t_price_;
      t_cur->size_traded = t_size_exec_;
      t_cur->int_trade_price = int_trade_price;

      aggregated_trades_buffer_[0][t_security_id_].push_back(t_cur);
    }
    if (!t_intermediate_) {
      CheckToFlushAggregatedTrades(0, t_security_id_);
    }
  } else {
    if ((aggregated_trades_buffer_[1][t_security_id_].size() > 0) &&
        (aggregated_trades_buffer_[1][t_security_id_].back()->int_trade_price == int_trade_price)) {
      aggregated_trades_buffer_[1][t_security_id_].back()->size_traded += t_size_exec_;
    } else {
      HFSAT::MicexOFBufferedOrderExecData* t_cur = aggregated_trades_mempool_.Alloc();
      t_cur->aggressive_side = aggressive_side;
      t_cur->trade_price = t_price_;
      t_cur->size_traded = t_size_exec_;
      t_cur->int_trade_price = int_trade_price;

      aggregated_trades_buffer_[1][t_security_id_].push_back(t_cur);
    }
    if (!t_intermediate_) {
      CheckToFlushAggregatedTrades(1, t_security_id_);
    }
  }

#if MVM_PROFILING
  HFSAT::CpucycleProfiler::GetUniqueInstance().End(33);
#endif

  sec_id_to_prev_update_was_quote_[t_security_id_] = false;
}

void IndexedMicexOFMarketViewManager::OnMSRL1Update(const unsigned int t_security_id_,
                                                    MICEX_OF_MDS::MICEXOFCommonStruct* t_msr_of_str_ptr_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  if (!smv_.initial_book_constructed_) {
    return;
  }

  // check if we have L1 bid side info
  if (t_msr_of_str_ptr_->data.best_level_update.HasL1BidSideInfo()) {
    int t_new_best_bid_int_price = smv_.GetIntPx(t_msr_of_str_ptr_->data.best_level_update.best_bid_price);
    int t_new_best_bid_size = t_msr_of_str_ptr_->data.best_level_update.best_bid_size;

    if ((smv_.market_update_info_.bestbid_int_price_ == t_new_best_bid_int_price) &&
        (smv_.market_update_info_.bestbid_size_ == t_new_best_bid_size)) {
      return;
    }

    // update best bid variables
    smv_.market_update_info_.bestbid_int_price_ = t_new_best_bid_int_price;
    smv_.market_update_info_.bestbid_price_ = t_msr_of_str_ptr_->data.best_level_update.best_bid_price;
    smv_.market_update_info_.bestbid_size_ = t_new_best_bid_size;
  }
  // check if we have L1 ask side info
  if (t_msr_of_str_ptr_->data.best_level_update.HasL1AskSideInfo()) {
    int t_new_best_ask_int_price = smv_.GetIntPx(t_msr_of_str_ptr_->data.best_level_update.best_ask_price);
    int t_new_best_ask_size = t_msr_of_str_ptr_->data.best_level_update.best_ask_size;

    if ((smv_.market_update_info_.bestask_int_price_ == t_new_best_ask_int_price) &&
        (smv_.market_update_info_.bestask_size_ == t_new_best_ask_size)) {
      return;
    }

    // update best bid variables
    smv_.market_update_info_.bestask_int_price_ = t_new_best_ask_int_price;
    smv_.market_update_info_.bestask_price_ = t_msr_of_str_ptr_->data.best_level_update.best_ask_price;
    smv_.market_update_info_.bestask_size_ = t_new_best_ask_size;
  }

  if (t_msr_of_str_ptr_->data.best_level_update.HasLastTradePriceInfo()) {
    SendFastTradeNotification(t_security_id_, t_msr_of_str_ptr_);
  } else {
    smv_.UpdateL1Prices();
    smv_.NotifyL1PriceListeners();
  }
  return;
}

void IndexedMicexOFMarketViewManager::SendFastTradeNotification(const unsigned int t_security_id_,
                                                                MICEX_OF_MDS::MICEXOFCommonStruct* t_msr_of_str_ptr_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  TradeType_t t_aggressive_side;

  if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_price_ >=
      t_msr_of_str_ptr_->data.best_level_update.last_trade_price) {
    t_aggressive_side = kTradeTypeSell;
  } else {
    t_aggressive_side = kTradeTypeBuy;
  }
  // Awaken Fast Trade Listeners first, then send OnMarketUpdate and then send OnTradePrint
  smv_.NotifyFastTradeListeners(t_security_id_, t_aggressive_side,
                                t_msr_of_str_ptr_->data.best_level_update.last_trade_price);
  smv_.UpdateL1Prices();
  smv_.NotifyL1PriceListeners();

  if (is_trade_prediction_on_) {
    sec_id_to_last_trade_notification_timestamp_[t_security_id_] = t_msr_of_str_ptr_->exchange_time_stamp;
    SendPredictedTrades(t_security_id_, t_msr_of_str_ptr_);
  }
  return;
}

void IndexedMicexOFMarketViewManager::SendPredictedTrades(const unsigned int t_security_id_,
                                                          MICEX_OF_MDS::MICEXOFCommonStruct* t_msr_of_str_ptr_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  // Check if trades are on Bid Side
  if (t_msr_of_str_ptr_->data.best_level_update.HasL1BidSideInfo()) {
    int t_old_best_bid_idx_ = smv_.base_bid_index_;

    int t_new_best_bid_idx =
        smv_.base_bid_index_ - (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ -
                                smv_.GetIntPx(t_msr_of_str_ptr_->data.best_level_update.best_bid_price));

    int t_last_trade_price_idx =
        smv_.base_bid_index_ - (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ -
                                smv_.GetIntPx(t_msr_of_str_ptr_->data.best_level_update.last_trade_price));

    // iterate over the levels in between and notify upon finding non-empty level
    for (int i = std::max(t_old_best_bid_idx_, 0); i >= std::max(t_last_trade_price_idx, t_new_best_bid_idx); i--) {
      if (smv_.market_update_info_.bidlevels_[i].limit_size_ <= 0) continue;
      // update best variables and notify
      smv_.trade_print_info_.int_trade_price_ = smv_.market_update_info_.bidlevels_[i].limit_int_price_;
      smv_.trade_print_info_.trade_price_ = smv_.market_update_info_.bidlevels_[i].limit_price_;
      smv_.trade_print_info_.size_traded_ = smv_.market_update_info_.bidlevels_[i].limit_size_;
      smv_.trade_print_info_.buysell_ = HFSAT::kTradeTypeSell;

      if (i == t_new_best_bid_idx) {
        if (t_last_trade_price_idx == t_new_best_bid_idx) {
          smv_.trade_print_info_.size_traded_ -= t_msr_of_str_ptr_->data.best_level_update.best_bid_size;
          if (smv_.trade_print_info_.size_traded_ < 0) {
            smv_.trade_print_info_.size_traded_ = smv_.market_update_info_.bidlevels_[i].limit_size_;
          }
        }
      }

      smv_.trade_print_info_.size_traded_ = std::max(1, smv_.trade_print_info_.size_traded_);

      smv_.SetTradeVarsForIndicatorsIfRequired();

#if PRINT_ORDER_INFO_MODE_MICEX
      DBGLOG_CLASS_FUNC_LINE_INFO << "Predicted Trade : " << sec_name_indexer_.GetSecurityNameFromId(t_security_id_)
                                  << " watch@" << watch_.tv_ToString() << " [ " << smv_.trade_print_info_.size_traded_
                                  << " " << smv_.trade_print_info_.trade_price_ << " ]\n";
#endif

      if (smv_.is_ready_) {
        smv_.NotifyTradeListeners();
        smv_.NotifyOnReadyListeners();
      }
    }
  }

  // Check if trades are on Ask Side
  if (t_msr_of_str_ptr_->data.best_level_update.HasL1AskSideInfo()) {
    int t_old_best_ask_idx_ = smv_.base_ask_index_;

    int t_new_best_ask_idx =
        smv_.base_ask_index_ + (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ -
                                smv_.GetIntPx(t_msr_of_str_ptr_->data.best_level_update.best_ask_price));

    int t_last_trade_price_idx =
        smv_.base_ask_index_ + (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ -
                                smv_.GetIntPx(t_msr_of_str_ptr_->data.best_level_update.last_trade_price));

    // iterate over the levels in between and notify upon finding non-empty level
    for (int i = std::max(t_old_best_ask_idx_, 0); i >= std::min(t_last_trade_price_idx, t_last_trade_price_idx); i--) {
      if (smv_.market_update_info_.asklevels_[i].limit_size_ <= 0) continue;
      // update best variables and notify
      smv_.trade_print_info_.int_trade_price_ = smv_.market_update_info_.asklevels_[i].limit_int_price_;
      smv_.trade_print_info_.trade_price_ = smv_.market_update_info_.asklevels_[i].limit_price_;
      smv_.trade_print_info_.size_traded_ = smv_.market_update_info_.asklevels_[i].limit_size_;
      smv_.trade_print_info_.buysell_ = HFSAT::kTradeTypeBuy;

      if (i == t_new_best_ask_idx) {
        if (t_last_trade_price_idx == t_new_best_ask_idx) {
          smv_.trade_print_info_.size_traded_ -= t_msr_of_str_ptr_->data.best_level_update.best_ask_size;
          if (smv_.trade_print_info_.size_traded_ < 0) {
            smv_.trade_print_info_.size_traded_ = smv_.market_update_info_.asklevels_[i].limit_size_;
          }
        }
      }

      smv_.trade_print_info_.size_traded_ = std::max(1, smv_.trade_print_info_.size_traded_);

#if PRINT_ORDER_INFO_MODE_MICEX
      DBGLOG_CLASS_FUNC_LINE_INFO << "Predicted Trade : " << sec_name_indexer_.GetSecurityNameFromId(t_security_id_)
                                  << " watch@" << watch_.tv_ToString() << " [ " << smv_.trade_print_info_.size_traded_
                                  << " " << smv_.trade_print_info_.trade_price_ << " ]\n";
#endif

      smv_.SetTradeVarsForIndicatorsIfRequired();
      if (smv_.is_ready_) {
        smv_.NotifyTradeListeners();
        smv_.NotifyOnReadyListeners();
      }
    }
  }

  return;
}

void IndexedMicexOFMarketViewManager::CheckToFlushAggregatedTrades(int t_idx_, const unsigned int t_security_id_) {
  if (aggregated_trades_buffer_[t_idx_][t_security_id_].size() == 0) return;

  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  while (aggregated_trades_buffer_[t_idx_][t_security_id_].size() > 0) {
    HFSAT::MicexOFBufferedOrderExecData* t_cur = aggregated_trades_buffer_[t_idx_][t_security_id_].front();
    aggregated_trades_buffer_[t_idx_][t_security_id_].pop_front();

    // Set the trade variables
    smv_.trade_print_info_.trade_price_ = t_cur->trade_price;
    smv_.trade_print_info_.size_traded_ = t_cur->size_traded;
    smv_.trade_print_info_.int_trade_price_ = t_cur->int_trade_price;
    smv_.trade_print_info_.buysell_ = t_cur->aggressive_side;

    smv_.SetTradeVarsForIndicatorsIfRequired();

    if (smv_.is_ready_) {
      smv_.NotifyTradeListeners();
      smv_.NotifyOnReadyListeners();
    }
    aggregated_trades_mempool_.DeAlloc(t_cur);
  }

  return;
}

void IndexedMicexOFMarketViewManager::OnOrderResetBegin(const unsigned int t_security_id_) {
  timeval tv;
  gettimeofday(&tv, NULL);
  DBGLOG_CLASS_FUNC_LINE_INFO << sec_name_indexer_.GetSecurityNameFromId(t_security_id_) << " watch@"
                              << watch_.tv_ToString() << " "
                              << "TimeOfDay@ " << tv.tv_sec << "." << tv.tv_usec
                              << " Resetting order book, flushing all the orders so far..."
                              << "\n";
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);
  smv_.InitializeSMVForIndexedBook();

  // De- Allocate all the order structs
  for (auto id_order_pair = order_id_to_bid_order_info_map_[t_security_id_].begin();
       id_order_pair != order_id_to_bid_order_info_map_[t_security_id_].end(); id_order_pair++) {
    order_mempool_.DeAlloc(id_order_pair->second);
  }

  // De- Allocate all the order structs
  for (auto id_order_pair = order_id_to_ask_order_info_map_[t_security_id_].begin();
       id_order_pair != order_id_to_ask_order_info_map_[t_security_id_].end(); id_order_pair++) {
    order_mempool_.DeAlloc(id_order_pair->second);
  }

  std::fill(sec_id_to_prev_update_was_quote_.begin(), sec_id_to_prev_update_was_quote_.end(), false);

  // flushing all the order pointers
  order_id_to_bid_order_info_map_[t_security_id_].clear();
  order_id_to_ask_order_info_map_[t_security_id_].clear();
}
void IndexedMicexOFMarketViewManager::OnOrderResetEnd(const unsigned int t_security_id_) {}

void IndexedMicexOFMarketViewManager::UpdateBaseBidIndex(const unsigned int t_security_id_, TradeType_t t_buysell_) {
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
    DBGLOG_TIME_CLASS_FUNC_LINE << " Bid Side Empty for Symbol : "
                                << sec_name_indexer_.GetSecurityNameFromId(t_security_id_) << DBGLOG_ENDL_FLUSH;
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

void IndexedMicexOFMarketViewManager::UpdateBaseAskIndex(const unsigned int t_security_id_, TradeType_t t_buysell_) {
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
    DBGLOG_TIME_CLASS_FUNC_LINE << " Ask side empty for Symbol : "
                                << sec_name_indexer_.GetSecurityNameFromId(t_security_id_) << DBGLOG_ENDL_FLUSH;
    return;
  }

  smv_.base_ask_index_ = next_ask_index_;  // updating the best ask level

  smv_.l1_changed_since_last_ = true;  // indicating that best ask level has been updated

  if (smv_.base_ask_index_ < LOW_ACCESS_INDEX) {
    RebuildIndexLowAccess(t_security_id_, t_buysell_,
                          smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_);
  }
}

void IndexedMicexOFMarketViewManager::SanitizeBidSide(const unsigned int t_security_id_, int t_int_price_) {
  DBGLOG_TIME_CLASS_FUNC_LINE << " Sanitizing Bid Side for Symbol : "
                              << sec_name_indexer_.GetSecurityNameFromId(t_security_id_) << DBGLOG_ENDL_FLUSH;
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  // Delete all the orders correspondingly

  for (auto id_order_pair = order_id_to_bid_order_info_map_[t_security_id_].begin(),
            tmp_itr = order_id_to_bid_order_info_map_[t_security_id_].begin();
       id_order_pair != order_id_to_bid_order_info_map_[t_security_id_].end();) {
    tmp_itr = id_order_pair;
    id_order_pair++;
    if (tmp_itr->second->side == 'B' && smv_.GetIntPx(tmp_itr->second->price) >= t_int_price_) {
      order_mempool_.DeAlloc(tmp_itr->second);
      // tmp_itr->second = nullptr;
      order_id_to_bid_order_info_map_[t_security_id_].erase(tmp_itr);
    }
  }

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
    DBGLOG_TIME_CLASS_FUNC_LINE << " Bid side empty after sanitisation for Symbol : "
                                << sec_name_indexer_.GetSecurityNameFromId(t_security_id_) << DBGLOG_ENDL_FLUSH;
    smv_.is_ready_ = false;
    return;
  }

  // Check if we need to re-align the center
  if (smv_.base_bid_index_ < LOW_ACCESS_INDEX) {
    RebuildIndexLowAccess(t_security_id_, kTradeTypeBuy,
                          smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_);
  }
}

void IndexedMicexOFMarketViewManager::SanitizeAskSide(const unsigned int t_security_id_, int t_int_price_) {
  DBGLOG_TIME_CLASS_FUNC_LINE << " Sanitizing Ask Side for Symbol : "
                              << sec_name_indexer_.GetSecurityNameFromId(t_security_id_) << DBGLOG_ENDL_FLUSH;
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  // Delete all the orders correspondingly
  for (auto id_order_pair = order_id_to_ask_order_info_map_[t_security_id_].begin(),
            tmp_itr = order_id_to_ask_order_info_map_[t_security_id_].begin();
       id_order_pair != order_id_to_ask_order_info_map_[t_security_id_].end();) {
    tmp_itr = id_order_pair;
    id_order_pair++;
    if (tmp_itr->second->side == 'S' && smv_.GetIntPx(tmp_itr->second->price) <= t_int_price_) {
      order_mempool_.DeAlloc(tmp_itr->second);
      // tmp_itr->second = nullptr;
      order_id_to_ask_order_info_map_[t_security_id_].erase(tmp_itr);
    }
  }
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
    DBGLOG_TIME_CLASS_FUNC_LINE << " Ask Side Empty after sanitisation for symbol : "
                                << sec_name_indexer_.GetSecurityNameFromId(t_security_id_) << DBGLOG_ENDL_FLUSH;
    return;
  }

  // Check if we need to re-align the center
  if (smv_.base_ask_index_ < LOW_ACCESS_INDEX) {
    RebuildIndexLowAccess(t_security_id_, kTradeTypeSell,
                          smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_);
  }
}
}
