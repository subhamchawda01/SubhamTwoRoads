#include "baseinfra/MarketAdapter/indexed_rts_of_market_view_manager.hpp"

namespace HFSAT {

// FIXME: Validate LOW_ACCESS_INDEX value
#define LOW_ACCESS_INDEX 50
#define CCPROFILING_TRADEINIT 1

IndexedRtsOFMarketViewManager::IndexedRtsOFMarketViewManager(
    DebugLogger& dbglogger, const Watch& watch, const SecurityNameIndexer& sec_name_indexer,
    const std::vector<SecurityMarketView*>& security_market_view_map)
    : BaseMarketViewManager(dbglogger, watch, sec_name_indexer, security_market_view_map),
      prev_PL_change_state_(),
#if RTS_OF_USE_ORDER_MAPS
      day_order_pool_(DEF_MAX_SEC_ID),
#endif
      buffered_trades_(DEF_MAX_SEC_ID),
      predicted_trades_(DEF_MAX_SEC_ID),
      trade_prediction_info_(DEF_MAX_SEC_ID) {
}

void IndexedRtsOFMarketViewManager::DropIndexedBookForSource(HFSAT::ExchSource_t exch_source, const int security_id) {
  SecurityMarketView& smv_ = *(security_market_view_map_[security_id]);

  HFSAT::ExchSource_t this_exch_source =
      HFSAT::SecurityDefinitions::GetContractExchSource(smv_.shortcode(), watch_.YYYYMMDD());

  if (this_exch_source != exch_source) return;

  smv_.market_update_info_.bidlevels_.clear();
  smv_.market_update_info_.asklevels_.clear();
}

bool IndexedRtsOFMarketViewManager::IsAgressiveOrder(const unsigned int security_id, double price, char side) {
  SecurityMarketView& smv_ = *(security_market_view_map_[security_id]);

  int int_price = smv_.GetIntPx(price);
  TradeType_t buy_sell = (TradeType_t)side;

  switch (buy_sell) {
    case kTradeTypeBuy: {
      if (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ > 0 &&
          int_price >= smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_) {
        return true;
      }

    } break;
    case kTradeTypeSell: {
      if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ > 0 &&
          int_price <= smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_) {
        return true;
      }

    } break;
    default:
      break;
  }

  return false;
}

void IndexedRtsOFMarketViewManager::UpdateOrderCountForNonEmptyLevels(const unsigned int t_security_id, int t_int_price,
                                                                      uint8_t t_side, int t_order_count) {
  if (t_order_count <= 0) return;
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id]);

  TradeType_t buy_sell = (TradeType_t)t_side;
  switch (buy_sell) {
    case kTradeTypeBuy: {
      uint32_t bid_index = smv_.base_bid_index_ -
                           (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - t_int_price);

      if (smv_.market_update_info_.bidlevels_[bid_index].limit_size_ > 0)
        smv_.market_update_info_.bidlevels_[bid_index].limit_ordercount_ = t_order_count;
      if (bid_index == smv_.base_bid_index_) smv_.market_update_info_.bestbid_ordercount_ = t_order_count;
    } break;
    case kTradeTypeSell: {
      uint32_t ask_index = smv_.base_ask_index_ +
                           (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - t_int_price);

      if (smv_.market_update_info_.asklevels_[ask_index].limit_size_ > 0)
        smv_.market_update_info_.asklevels_[ask_index].limit_ordercount_ = t_order_count;
      if (ask_index == smv_.base_ask_index_) smv_.market_update_info_.bestask_ordercount_ = t_order_count;
    } break;
    default: {}
  }
}

// On aggressive order add, we predict the trades that would be triggered based on the current state of the book.
void IndexedRtsOFMarketViewManager::PredictTradesOnAggressAdd(const unsigned int t_security_id_, double price, int size,
                                                              uint8_t side, uint64_t order_id, bool is_intermediate_msg,
                                                              bool is_ioc) {
  // Reset earlier predicted trades
  trade_prediction_info_[t_security_id_].total_predicted_trade_size = 0;
  trade_prediction_info_[t_security_id_].last_agg_order_actual_size_executed = 0;
  predicted_trades_[t_security_id_].clear();

  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  int int_price = smv_.GetIntPx(price);  // aggress order add price
  TradeType_t aggressor_side = (TradeType_t)side;
  int index_ = -1;

  switch (aggressor_side) {
    case kTradeTypeSell: {
      index_ = smv_.base_bid_index_;
      for (; index_ >= 0 && size > 0; index_--) {
        int this_level_int_px = smv_.market_update_info_.bidlevels_[index_].limit_int_price_;
        double this_level_price = smv_.GetDoublePx(this_level_int_px);
        int this_level_limit_size = smv_.market_update_info_.bidlevels_[index_].limit_size_;
        if (this_level_limit_size <= 0) continue;  // hole in the book

        if (this_level_int_px >= int_price) {
          // aggressed from sell side at a price lower than this. we should delete the levels

          if (this_level_limit_size - size > 0) {
// Expecting trade of 'size' = size at 'px' = this_level_int_px on BUY side
#if RTS_OFv2_DBG_PREDICTION
            std::cout << watch_.tv() << " "
                      << "OTP [ S " << size << " @ " << this_level_int_px << " ]" << std::endl;
#endif
            trade_prediction_info_[t_security_id_].total_predicted_trade_size += size;
            TradeDetails predict_trade;
            predict_trade.size_traded_ = size;
            predict_trade.int_trade_price_ = this_level_int_px;
            predict_trade.price_ = this_level_price;
            predict_trade.order_count = smv_.market_update_info_.bidlevels_[index_].limit_ordercount_;
            predicted_trades_[t_security_id_].push_back(predict_trade);
            // dummy orderID, not updating order count
            OnOrderExec(t_security_id_, this_level_price, !side, size, -1, true, false);
            size = 0;

          } else {  // this level is cleared
                    // Expecting trade of 'size' = this_level_limit_size at 'px' = price on BUY side
#if RTS_OFv2_DBG_PREDICTION
            std::cout << watch_.tv() << " "
                      << "OTP [ S " << this_level_limit_size << " @ " << this_level_int_px << " ]" << std::endl;
#endif
            trade_prediction_info_[t_security_id_].total_predicted_trade_size += this_level_limit_size;
            TradeDetails predict_trade;
            predict_trade.size_traded_ = this_level_limit_size;
            predict_trade.int_trade_price_ = this_level_int_px;
            predict_trade.price_ = smv_.GetDoublePx(this_level_int_px);
            predict_trade.order_count = smv_.market_update_info_.bidlevels_[index_].limit_ordercount_;
            predicted_trades_[t_security_id_].push_back(predict_trade);
            // dummy orderID, not updating order count
            OnOrderExec(t_security_id_, this_level_price, !side, this_level_limit_size, -1, true, false);
            size -= this_level_limit_size;
          }
        } else {
          break;
        }
      }
    } break;
    case kTradeTypeBuy: {
      index_ = smv_.base_ask_index_;
      for (; index_ >= 0 && size > 0; index_--) {
        int this_level_int_px = smv_.market_update_info_.asklevels_[index_].limit_int_price_;
        double this_level_price = smv_.GetDoublePx(this_level_int_px);
        int this_level_limit_size = smv_.market_update_info_.asklevels_[index_].limit_size_;
        if (this_level_limit_size <= 0) continue;  // hole in the book

        if (this_level_int_px <= int_price) {
          // aggressed from buy side at a price higher than this.
          if (this_level_limit_size - size > 0) {
// Expecting trade of 'size' = size at 'px' = price on SELL side
#if RTS_OFv2_DBG_PREDICTION
            std::cout << watch_.tv() << " "
                      << "OTP [ B " << size << " @ " << this_level_int_px << " ]" << std::endl;
#endif
            trade_prediction_info_[t_security_id_].total_predicted_trade_size += size;
            TradeDetails predict_trade;
            predict_trade.size_traded_ = size;
            predict_trade.int_trade_price_ = this_level_int_px;
            predict_trade.price_ = smv_.GetDoublePx(this_level_int_px);
            predict_trade.order_count = smv_.market_update_info_.asklevels_[index_].limit_ordercount_;
            predicted_trades_[t_security_id_].push_back(predict_trade);
            // dummy orderID, not updating order count
            OnOrderExec(t_security_id_, this_level_price, !side, size, -1, true, false);
            size = 0;
          } else {  // this level is cleared
                    // Expecting trade of 'size' = this_level_limit_size at 'px' = price on SELL side
#if RTS_OFv2_DBG_PREDICTION
            std::cout << watch_.tv() << " "
                      << "OTP [ B " << this_level_limit_size << " @ " << this_level_int_px << " ]" << std::endl;
#endif
            trade_prediction_info_[t_security_id_].total_predicted_trade_size += this_level_limit_size;
            TradeDetails predict_trade;
            predict_trade.size_traded_ = this_level_limit_size;
            predict_trade.int_trade_price_ = this_level_int_px;
            predict_trade.price_ = smv_.GetDoublePx(this_level_int_px);
            predict_trade.order_count = smv_.market_update_info_.asklevels_[index_].limit_ordercount_;
            predicted_trades_[t_security_id_].push_back(predict_trade);
            // dummy orderID, not updating order count
            OnOrderExec(t_security_id_, this_level_price, !side, this_level_limit_size, -1, true, false);
            size -= this_level_limit_size;
          }
        } else {
          break;
        }
      }
    } break;

    default: {
      std::cout << typeid(*this).name() << ':' << __func__ << "  Incorrect trade side " << price << " " << size
                << std::endl;
    } break;
  }

  trade_prediction_info_[t_security_id_].ongoing_agg_add_event_ = true;

  // If aggress order is not completely executed after prediction, we add to book if its non ioc
  if (size > 0 && !is_ioc) {
#if RTS_OFv2_DBG_PREDICTION
    std::cout << typeid(*this).name() << ':' << __func__ << "  Agg order to book " << price << " " << size << std::endl;
#endif
    OnOrderAdd(t_security_id_, price, side, size, -1, true, is_ioc);
  }

  // Notify the predicted trades right away
  CheckToNotifyTradeMessage(t_security_id_, false);
}

// Processes all updates for aggressive orders. In case of executions, we do not change order size,
// because we need original Aggressive order add size for book correction in case of packet drops.
void IndexedRtsOFMarketViewManager::ProcessAggressiveOrderDelta(const unsigned int t_security_id,
                                                                RTS_MDS::RTSOFCommonStructv2* order) {
  switch (order->msg_type) {
    case RTS_MDS::RTSOFMsgType::kRTSAdd: {
#if RTS_OFv2_DBG
      std::cout << "Process agg add: order_id " << order.order_id << " px: " << order.price << " sz: " << order.size
                << std::endl;
#endif

      OrderDetails& details = trade_prediction_info_[t_security_id].order;
      details.order_id = order->order_id;
      details.price = order->price;
      details.side = order->side - '0';
      details.size = order->size;
      details.is_ioc = order->md_flags & 0x02;  // IOC order, to filter any ioc order;

    } break;
    case RTS_MDS::RTSOFMsgType::kRTSDelete: {
      trade_prediction_info_[t_security_id].order.order_id =
          0;  // order_id =0 means no pending aggressive order for this security

    } break;
    case RTS_MDS::RTSOFMsgType::kRTSExec: {
      if (order->is_full_exec) {
        trade_prediction_info_[t_security_id].order.order_id =
            0;  // order_id =0 means no pending aggressive order for this security
      }
      // IMPORTANT: not updating agg add size. It always has the original size added.
      // We need the actual add size of aggress order to verify prediction on agg order cancel
    } break;
    case RTS_MDS::RTSOFMsgType::kRTSDeleteAll: {
    } break;
    default:
      break;
  }
}

// Processes the RTS_OF msgs.
void IndexedRtsOFMarketViewManager::Process(const unsigned int security_id, RTS_MDS::RTSOFCommonStructv2* next_event) {
  int64_t md_flags = next_event->md_flags;  // OTC Order
  bool is_ioc = md_flags & 0x02;            // IOC order, to filter any ioc passive order

  if ((md_flags & 0x04) == 0x04) {
#if RTS_OFv2_DBG
    std::cout << typeid(*this).name() << ':' << __func__ << " OTC Order. Returning. OrderID:  " << next_event->order_id
              << std::endl;
#endif
    return;
  }

  // If order is already present in aggressive pool, process it in the pool itself before adding to book
  if (next_event->order_id > 0 && trade_prediction_info_[security_id].order.order_id == next_event->order_id) {
    ProcessAggressiveOrderDelta(security_id, next_event);
    VerifyTradePredictionOnAggressCancel(security_id, next_event->msg_type, next_event->order_id, next_event->size,
                                         next_event->side - '0', next_event->price, is_ioc);
    // A packet may end with Aggress Exec or Delete
    CheckToNotifyTradeMessage(security_id, next_event->is_intermediate);
    return;
  }

  // Aggressive order add or Passive order update
  switch (next_event->msg_type) {
    case RTS_MDS::RTSOFMsgType::kRTSAdd: {
      ProcessEndOfAggressAddEvents(security_id, is_ioc);
      if (IsAgressiveOrder(security_id, next_event->price, next_event->side - '0')) {
        ProcessAggressiveOrderDelta(security_id, next_event);
        PredictTradesOnAggressAdd(security_id, next_event->price, next_event->size, next_event->side - '0',
                                  next_event->order_id, next_event->is_intermediate, is_ioc);
      } else {
        OnOrderAdd(security_id, next_event->price, next_event->side - '0', next_event->size, next_event->order_id,
                   next_event->is_intermediate, is_ioc);
      }

    } break;

    case RTS_MDS::RTSOFMsgType::kRTSDelete: {
      ProcessEndOfAggressAddEvents(security_id, is_ioc);
      OnOrderDelete(security_id, next_event->price, next_event->side - '0', next_event->size, next_event->order_id,
                    next_event->is_intermediate, is_ioc);

    } break;

    case RTS_MDS::RTSOFMsgType::kRTSExec: {
      // Negotiated Trade
      if ((md_flags & 0x4000000) == 0x4000000) {
#if RTS_OFv2_DBG
        std::cout << typeid(*this).name() << ':' << __func__
                  << " Negotiated Exec. Returning. OrderID:  " << next_event->order_id << std::endl;
#endif
        return;
      }
      // We make use of passive execs to verify our predictions and handle book state that require sanitization
      ProcessPassiveExecs(security_id, next_event->price, next_event->side - '0', next_event->size,
                          next_event->order_id, next_event->is_intermediate, is_ioc, next_event->is_full_exec);

    } break;
    case RTS_MDS::RTSOFMsgType::kRTSDeleteAll: {
    } break;
    case RTS_MDS::RTSOFMsgType::kRTSResetBegin: {
      OnResetBegin(security_id);  // We entered recovery, reset the book state for this security
      return;
    } break;
    default: { return; } break;
  }

  // If there are buffered execs, we will flush it now. Flush in case of non intermediate ADD both aggress or passive.
  // If this msg is non-intermediate exec: denotes single exec in this trade or pkt end and this trade will
  // continue in next packet. In latter case, we are basically notifying partial accumulated exec here.

  CheckToNotifyTradeMessage(security_id, next_event->is_intermediate);

  // After notifying the trades, we flush passive orders from aggress pool and notify if required
  FlushPassiveOrdersInAggressPool(security_id, next_event->is_intermediate);
}

// Extract Aggressive Orders from aggressive_order_pool_ which have now become passive and insert into book.
// Notify if L1 has changed
void IndexedRtsOFMarketViewManager::FlushPassiveOrdersInAggressPool(const unsigned int security_id,
                                                                    bool is_intermediate_msg) {
  // Do not look to notify updates, if this was an intermediate update
  if (is_intermediate_msg) return;

  // If some orders became passive: Flush All Notifications here including all the last OnOrderAdd() from aggressive
  // pool as they were all intermediate
  SecurityMarketView& smv_ = *(security_market_view_map_[security_id]);

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(9);
#endif

  // If any of the above aggress pool order changed L1, the flag would be set now as well.
  if (smv_.l1_changed_since_last_) {
#if RTS_OFv2_DBG
    std::cout << typeid(*this).name() << ':' << __func__ << " Notifying L1 Px change listeners" << std::endl;
#endif
    smv_.NotifyL1PriceListeners();

    smv_.l1_changed_since_last_ = false;
  } else {
#if RTS_OFv2_DBG
    std::cout << typeid(*this).name() << ':' << __func__ << " Notifying L2  listeners" << std::endl;
#endif
    smv_.NotifyL2Listeners();
  }

  if (smv_.l2_changed_since_last_) {
#if RTS_OFv2_DBG
    std::cout << typeid(*this).name() << ':' << __func__ << " Notifying L2 only listeners" << std::endl;
#endif
    smv_.NotifyL2OnlyListeners();

    smv_.l2_changed_since_last_ = false;
  }

  smv_.NotifyOnReadyListeners();
}

// On Aggressive DELETE, check if the predicted trades actually were true
// If not, lets add the extra predicted trade, back to the book
void IndexedRtsOFMarketViewManager::VerifyTradePredictionOnAggressCancel(const unsigned int t_security_id_,
                                                                         RTS_MDS::RTSOFMsgType msg_type,
                                                                         uint64_t order_id, int size, uint8_t side,
                                                                         double price, bool is_ioc) {
  PredictionInfo& trade_prediction_info = trade_prediction_info_[t_security_id_];
  OrderDetails& last_agg_order = trade_prediction_info.order;

  if (RTS_MDS::RTSOFMsgType::kRTSDelete != msg_type || !trade_prediction_info.ongoing_agg_add_event_)
    return;  // Only triggered on agg delete

  int size_executed = last_agg_order.size - size;
  trade_prediction_info.last_agg_order_actual_size_executed += size;  // IMPORTANT: accounting for deletes of agg order.

  // Aggress order is deleted. If we had added it to the passive book, we delete it now
  if (last_agg_order.size - trade_prediction_info.total_predicted_trade_size > 0 && !is_ioc) {
#if RTS_OFv2_DBG_PREDICTION
    std::cout << typeid(*this).name() << ':' << __func__
              << "  Passive exec isn't added to book. Deleting: " << last_agg_order.price << " "
              << last_agg_order.size - trade_prediction_info.total_predicted_trade_size << " " << (int)side
              << std::endl;
#endif
    OnOrderDelete(t_security_id_, last_agg_order.price, side,
                  last_agg_order.size - trade_prediction_info.total_predicted_trade_size, -1, true, false, false);
    FlushPassiveOrdersInAggressPool(t_security_id_, false);
  }

  if (size_executed == trade_prediction_info.total_predicted_trade_size) {
    return;  // correct trade prediction. Any incorrectly added passive order would be removed above
  } else if (size_executed <
             trade_prediction_info.total_predicted_trade_size) {  // need to add extra predicted trades back to book

    for (auto itr : predicted_trades_[t_security_id_]) {
#if RTS_OFv2_DBG_PREDICTION
      std::cout << "AddToBook " << itr.size_traded_ << " @ " << itr.price_ << " " << itr.order_count << std::endl;
#endif
      OnOrderAdd(t_security_id_, itr.price_, !side, itr.size_traded_, -1, true, false, itr.order_count);
      predicted_trades_[t_security_id_].pop_front();  // remove this predicted trade from the pool
      UpdateOrderCountForNonEmptyLevels(t_security_id_, itr.int_trade_price_, !side, itr.order_count);
      FlushPassiveOrdersInAggressPool(t_security_id_, false);
    }

  } else {
#if RTS_OFv2_DBG_PREDICTION
    std::cout << typeid(*this).name() << ':' << __func__ << " Shouldn't be here. Predicted size "
              << trade_prediction_info.total_predicted_trade_size << "  is less than Executed size " << size_executed
              << std::endl;
#endif
  }
}

// Match passive legs of this execution with the predicted trades. Handle sanitization in case of crossed books
// due to packet drops
void IndexedRtsOFMarketViewManager::ProcessPassiveExecs(const unsigned int security_id, double price, char side,
                                                        uint32_t size, uint64_t order_id, bool is_intermediate,
                                                        bool is_ioc, bool is_complete_exec) {
  // We did not predict this trade. We should update the book now for this exec.
  if (predicted_trades_[security_id].size() <= 0) {
    OnOrderExec(security_id, price, side, size, order_id, is_intermediate, is_ioc);
    return;
  }

  TradeDetails& expected_trade = predicted_trades_[security_id].front();
  SecurityMarketView& smv_ = *(security_market_view_map_[security_id]);
  int int_exec_price = smv_.GetIntPx(price);
  TradeType_t passive_side = (TradeType_t)side;
  trade_prediction_info_[security_id].last_agg_order_actual_size_executed += size;

  if (expected_trade.int_trade_price_ == int_exec_price) {
#if RTS_OFv2_DBG_PREDICTION
    std::cout << typeid(*this).name() << ':' << __func__ << " Correct exec  sec " << security_id << " price " << price
              << " sz " << size << " full exec: " << is_complete_exec << " odr_count " << expected_trade.order_count
              << std::endl;
#endif

    expected_trade.size_traded_ -= size;
    expected_trade.order_count = expected_trade.order_count - is_complete_exec;  // implicit bool to int conversion

    if (expected_trade.size_traded_ <= 0) {
#if RTS_OFv2_DBG_PREDICTION
      std::cout << typeid(*this).name() << ':' << __func__ << " Complete passive.  sec " << security_id
                << " Removing from predicted pool. price " << price << " oid " << order_id << " sz " << size
                << " odr_count " << expected_trade.order_count << std::endl;
#endif
      UpdateOrderCountForNonEmptyLevels(security_id, int_exec_price, side, expected_trade.order_count);
      predicted_trades_[security_id].pop_front();  // remove this predicted trade from the pool
    }

  } else if (expected_trade.int_trade_price_ > int_exec_price) {
#if RTS_OFv2_DBG_PREDICTION
    std::cout << typeid(*this).name() << ':' << __func__ << "  ERROR: sec " << security_id << " Exec price " << price
              << " < predicted px " << expected_trade.price_ << " passive side " << (int)passive_side << std::endl;
#endif
    DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " ERROR: sec " << security_id << " Exec price " << price
                                << " < predicted px " << expected_trade.price_ << " passive side " << (int)passive_side
                                << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;

    switch (passive_side) {
      case kTradeTypeBuy: {
        // Trade at sub-best level on buy. We need to check for sanitize in book and also remove this expected trade
        // entry from this predicted pool.

        if (int_exec_price < smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_) {
          SanitizeBidSide(security_id, order_id, int_exec_price + 1);
          UpdateBestBidVariables(security_id, smv_.base_bid_index_);
        }
        // delete all predicted trades till int_exec_price
        for (auto itr : predicted_trades_[security_id]) {
          if (itr.int_trade_price_ > int_exec_price) {
            predicted_trades_[security_id].pop_front();
          }
        }
      } break;
      case kTradeTypeSell: {
#if RTS_OFv2_DBG_PREDICTION
        std::cout << typeid(*this).name() << ':' << __func__ << " ERROR:  sec " << security_id
                  << " Ignoring. Exec price " << price << " < predicted px " << expected_trade.price_ << std::endl;
#endif
      } break;
      default:
        break;
    }
  } else {
#if RTS_OFv2_DBG_PREDICTION
    std::cout << typeid(*this).name() << ':' << __func__ << "  ERROR:  sec " << security_id << " Exec price " << price
              << " > predicted px " << expected_trade.price_ << " passive side " << (int)passive_side << std::endl;
#endif

    DBGLOG_TIME_CLASS_FUNC_LINE << smv_.secname() << "ERROR:  sec " << security_id << " Exec price " << price
                                << " > predicted px " << expected_trade.price_ << " passive side " << (int)passive_side
                                << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;

    switch (passive_side) {
      case kTradeTypeBuy: {
#if RTS_OFv2_DBG_PREDICTION
        std::cout << typeid(*this).name() << ':' << __func__ << " ERROR:  sec " << security_id
                  << " Ignoring. Exec price " << price << " > predicted px " << expected_trade.price_ << std::endl;
#endif
      } break;
      case kTradeTypeSell: {
        // Trade at sub-best level on sell side. We need to check for sanitize in book and also remove this expected
        // trade entry from this predicted pool.

        if (int_exec_price > smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_) {
          SanitizeAskSide(security_id, order_id, int_exec_price - 1);
          UpdateBestAskVariables(security_id, smv_.base_ask_index_);
        }
        // delete all predicted trades till int_exec_price
        for (auto itr : predicted_trades_[security_id]) {
          if (itr.int_trade_price_ < int_exec_price) {
            predicted_trades_[security_id].pop_front();
          }
        }
      } break;
      default:
        break;
    }
  }
}

// All the updates for the aggressing order has ended. Ideally(in absence of pkt drops and correct predictions),
// we should do nothing here. We handle error cases here because of packet drops.
void IndexedRtsOFMarketViewManager::ProcessEndOfAggressAddEvents(const unsigned int security_id, bool is_ioc) {
  PredictionInfo& trade_prediction_info = trade_prediction_info_[security_id];
  OrderDetails& last_agg_order = trade_prediction_info.order;

  trade_prediction_info.ongoing_agg_add_event_ = false;

  // Reset the aggressive order id. By now, it should be passive and added to book
  if (last_agg_order.order_id != 0) last_agg_order.order_id = 0;

  // Lets check if prediction pool is non empty. May have dropped packets leading to incorrect trade prediction
  if (predicted_trades_[security_id].size() > 0) {
#if RTS_OFv2_DBG_PREDICTION
    std::cout << typeid(*this).name() << ':' << __func__ << " ERROR:  sec " << security_id
              << " Predicted Trades pool non empty after agg order processed. " << std::endl;
#endif

    DBGLOG_TIME_CLASS_FUNC_LINE << " ERROR : sec " << security_id
                                << " Predicted Trades pool non empty after agg order processed. "
                                << trade_prediction_info.last_agg_order_actual_size_executed << " "
                                << last_agg_order.size << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;

    TradeType_t aggressor_side = (TradeType_t)last_agg_order.side;
    SecurityMarketView& smv_ = *(security_market_view_map_[security_id]);
    int agg_int_price = smv_.GetIntPx(last_agg_order.price);  // aggress order add price

    // If we predicted incorrectly and added something to aggressor side, delete it
    if (last_agg_order.size - trade_prediction_info.total_predicted_trade_size > 0 && !is_ioc) {
#if RTS_OFv2_DBG_PREDICTION
      std::cout << typeid(*this).name() << ':' << __func__ << "  ERROR:  sec " << security_id
                << " Incorrect trade prediction. Del agg order(turned passive) from book: " << last_agg_order.price
                << " " << last_agg_order.size - trade_prediction_info.total_predicted_trade_size << " "
                << (int)aggressor_side << std::endl;
#endif
      OnOrderDelete(security_id, last_agg_order.price, last_agg_order.side,
                    last_agg_order.size - trade_prediction_info.total_predicted_trade_size, -1, true, false, false);
    }

    // Based on passive legs of execs, aggress order didn't get executed or got partially executed
    if ((int)(trade_prediction_info.last_agg_order_actual_size_executed - last_agg_order.size) < 0) {
// Sanitize the non aggressor side and delete if anything was added to book on aggressor side.
#if RTS_OFv2_DBG_PREDICTION
      std::cout << typeid(*this).name() << ':' << __func__ << "  sec " << security_id
                << " Agg order id didn't get executed or partially executed. " << last_agg_order.order_id << " px "
                << agg_int_price << " side " << (int)aggressor_side << std::endl;
#endif
      if (aggressor_side == HFSAT::kTradeTypeBuy) {
        SanitizeAskSide(security_id, last_agg_order.order_id, agg_int_price);
        UpdateBestAskVariables(security_id, smv_.base_ask_index_);
      } else {
        SanitizeBidSide(security_id, last_agg_order.order_id, agg_int_price);
        UpdateBestBidVariables(security_id, smv_.base_bid_index_);
      }

      // Add the correct passive order size to aggressor side
      OnOrderAdd(security_id, last_agg_order.price, aggressor_side,
                 last_agg_order.size - trade_prediction_info.last_agg_order_actual_size_executed, -1, true, false);
    }
    // Aggress order full exec.
    else if (trade_prediction_info.last_agg_order_actual_size_executed == last_agg_order.size) {
      // Add back whatever is left in prediction pool to the book on non aggressor side
      for (auto itr : predicted_trades_[security_id]) {
        OnOrderAdd(security_id, itr.price_, !aggressor_side, itr.size_traded_, -1, true, false, itr.order_count);
        predicted_trades_[security_id].pop_front();  // remove this predicted trade from the pool
      }
    }

    // clear the prediction pool and flush notifications
    predicted_trades_[security_id].clear();
    FlushPassiveOrdersInAggressPool(security_id, false);
  }
}

void IndexedRtsOFMarketViewManager::OnOrderAdd(const unsigned int security_id, double price, char side, uint32_t size,
                                               uint64_t order_id, bool is_intermediate, bool is_ioc, int order_count) {
  // Not adding ioc orders to book
  if (is_ioc) {
#if RTS_OFv2_DBG
    std::cout << "IOC order in passive book. Returning." << security_id << std::endl;
#endif
    return;
  }

#if RTS_OF_USE_ORDER_MAPS
  if (day_order_pool_[security_id].IsOrderPresent(order_id, side)) {
#if RTS_OFv2_DBG
    std::cout << " Order already added. Id: " << order_id << " side: " << (int)side << "  sec " << security_id
              << std::endl;
#endif
    return;
  } else {
#if RTS_OFv2_DBG
    std::cout << typeid(*this).name() << ':' << __func__ << " Adding the OrderID:  " << order_id << std::endl;
#endif
    day_order_pool_[security_id].Process(order_id, RTS_MDS::RTSOFMsgType::kRTSAdd, side);
  }
#endif

  SecurityMarketView& smv_ = *(security_market_view_map_[security_id]);

  int int_price = smv_.GetIntPx(price);
  TradeType_t buy_sell = (TradeType_t)side;

  if (!smv_.initial_book_constructed_) {
    BuildIndex(security_id, buy_sell, int_price);
  }

  int old_size = 0;
  int old_ordercount = 0;
  int new_size = 0;

  int new_ordercount = 0;
  int level_added = 0;

  switch (buy_sell) {
    case kTradeTypeBuy: {
      int bid_index = smv_.base_bid_index_ -
                      (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - int_price);

      // There are 0 levels on bid side
      if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ <= 0) {
        if (bid_index < LOW_ACCESS_INDEX) {
          RebuildIndexLowAccess(security_id, buy_sell, int_price);
          bid_index = smv_.base_bid_index_;
        } else if (bid_index >= (int)smv_.max_tick_range_) {
          RebuildIndexHighAccess(security_id, buy_sell, int_price);
          bid_index = smv_.base_bid_index_;
        }

        smv_.base_bid_index_ = bid_index;
      } else if (bid_index < 0) {
        return;
      }

      if (bid_index >= (int)smv_.max_tick_range_) {
        RebuildIndexHighAccess(security_id, buy_sell, int_price);
        bid_index = smv_.base_bid_index_;
      }

      // Store old size and order count for OnPL change listeners.
      old_size = smv_.market_update_info_.bidlevels_[bid_index].limit_size_;
      old_ordercount = smv_.market_update_info_.bidlevels_[bid_index].limit_ordercount_;

      smv_.market_update_info_.bidlevels_[bid_index].limit_size_ += size;
      smv_.market_update_info_.bidlevels_[bid_index].limit_ordercount_ += order_count;

      new_size = smv_.market_update_info_.bidlevels_[bid_index].limit_size_;
      new_ordercount = smv_.market_update_info_.bidlevels_[bid_index].limit_ordercount_;

      if (bid_index >= (int)smv_.base_bid_index_) {
        smv_.l1_changed_since_last_ = true;

        smv_.base_bid_index_ = bid_index;

        if (int_price >= smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_) {
          if (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ <= 0) {
            // We should not do sanitization, if the other side is not ready
            smv_.is_ready_ = false;
            break;
          }

          SanitizeAskSide(security_id, order_id, int_price);
          UpdateBestAskVariables(security_id, smv_.base_ask_index_);
        }
      } else {
        smv_.l2_changed_since_last_ = true;
      }

      // Get the level for this update considering holes in the book.
      // Level supplied is 0 for L1 and so on.
      for (int t_index = smv_.base_bid_index_ - 1; t_index >= bid_index; t_index--) {
        if (smv_.market_update_info_.bidlevels_[t_index].limit_size_ > 0 &&
            smv_.market_update_info_.bidlevels_[t_index].limit_ordercount_ > 0)
          level_added++;
      }

    } break;
    case kTradeTypeSell: {
      int ask_index = smv_.base_ask_index_ +
                      (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - int_price);

      // There are 0 levels on ask side
      if (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ <= 0) {
        if (ask_index < LOW_ACCESS_INDEX) {
          RebuildIndexLowAccess(security_id, buy_sell, int_price);
          ask_index = smv_.base_ask_index_;
        } else if (ask_index >= (int)smv_.max_tick_range_) {
          RebuildIndexHighAccess(security_id, buy_sell, int_price);
          ask_index = smv_.base_ask_index_;
        }

        smv_.base_ask_index_ = ask_index;
      } else if (ask_index < 0) {
        return;
      }

      if (ask_index >= (int)smv_.max_tick_range_) {
        RebuildIndexHighAccess(security_id, buy_sell, int_price);
        ask_index = smv_.base_ask_index_;
      }

      old_size = smv_.market_update_info_.asklevels_[ask_index].limit_size_;
      old_ordercount = smv_.market_update_info_.asklevels_[ask_index].limit_ordercount_;

      smv_.market_update_info_.asklevels_[ask_index].limit_size_ += size;
      smv_.market_update_info_.asklevels_[ask_index].limit_ordercount_ += order_count;

      new_size = smv_.market_update_info_.asklevels_[ask_index].limit_size_;
      new_ordercount = smv_.market_update_info_.asklevels_[ask_index].limit_ordercount_;

      if (ask_index >= (int)smv_.base_ask_index_) {
        smv_.l1_changed_since_last_ = true;

        smv_.base_ask_index_ = ask_index;

        if (int_price <= smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_) {
          if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ <= 0) {
            // We should not do sanitization, if the other side is not ready
            smv_.is_ready_ = false;
            break;
          }

          SanitizeBidSide(security_id, order_id, int_price);
          UpdateBestBidVariables(security_id, smv_.base_bid_index_);
        }
      } else {
        smv_.l2_changed_since_last_ = true;
      }

      // Get the level for this update considering holes in the book.
      // Level supplied is 0 for L1 and so on.
      for (int t_index = smv_.base_ask_index_ - 1; t_index >= ask_index; t_index--) {
        if (smv_.market_update_info_.asklevels_[t_index].limit_size_ > 0 &&
            smv_.market_update_info_.asklevels_[t_index].limit_ordercount_ > 0)
          level_added++;
      }

    } break;
    default:
      break;
  }

  if (smv_.l1_changed_since_last_) {
    switch (buy_sell) {
      case kTradeTypeBuy: {
        UpdateBestBidVariables(security_id, smv_.base_bid_index_);

        smv_.UpdateL1Prices();
      } break;
      case kTradeTypeSell: {
        UpdateBestAskVariables(security_id, smv_.base_ask_index_);
        smv_.UpdateL1Prices();
      } break;
      default:
        break;
    }
  }

  if (!smv_.is_ready_) {
    if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ > 0 &&
        smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ > 0 && smv_.ArePricesComputed()) {
      smv_.is_ready_ = true;
    } else {
      return;
    }
  }

  // FIXME: no idea about this, Abhishek
  if (smv_.market_update_info_.trade_update_implied_quote_) {
    smv_.market_update_info_.trade_update_implied_quote_ = false;
    smv_.trade_print_info_.num_trades_++;
  }

  if (is_intermediate) {
    return;
  }

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(9);
#endif

  if (smv_.pl_change_listeners_present_) {
    // FIXME: Abhishek: why do we have t_int_price_level_ set to 0
    smv_.NotifyOnPLChangeListeners(security_id, smv_.market_update_info_, buy_sell, level_added, int_price, 0, old_size,
                                   new_size, old_ordercount, new_ordercount, is_intermediate,
                                   old_size == 0 ? 'N' : 'C');
  }

  // will notify listeners after applying passive orders from aggress pool
}

void IndexedRtsOFMarketViewManager::OnOrderDelete(const unsigned int security_id, double price, char side,
                                                  uint32_t size, uint64_t order_id, bool is_intermediate, bool is_ioc,
                                                  bool is_complete_exec) {
  // Not adding ioc orders to book
  if (is_ioc) {
#if RTS_OFv2_DBG
    std::cout << "IOC order in passive book. Returning." << security_id << std::endl;
#endif
    return;
  }

#if RTS_OF_USE_ORDER_MAPS
  if (!day_order_pool_[security_id].IsOrderPresent(order_id, side)) {
#if RTS_OFv2_DBG
    std::cout << " Order not present to delete. Id: " << order_id << " side: " << side << "  sec " << security_id
              << std::endl;
#endif
    return;
  } else {
    day_order_pool_[security_id].Process(order_id, RTS_MDS::RTSOFMsgType::kRTSDelete, side);
  }
#endif

  SecurityMarketView& smv_ = *(security_market_view_map_[security_id]);

  int int_price = smv_.GetIntPx(price);
  TradeType_t buy_sell = (TradeType_t)side;

  if (!smv_.initial_book_constructed_) {
    BuildIndex(security_id, buy_sell, int_price);

    smv_.initial_book_constructed_ = true;
  }

  int old_size = 0;
  int old_ordercount = 0;
  int new_size = 0;
  int new_ordercount = 0;
  int level_changed = 0;

  switch (buy_sell) {
    case kTradeTypeBuy: {
      int bid_index = smv_.base_bid_index_ -
                      (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - int_price);

      // if 0 levels on bid size, delete doesn't make sense, we can simply return? Indicates
      // inconsistent book.
      if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ <= 0) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " Delete called for empty buy side book : orderid "
                                    << order_id << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
        return;
      }

      if (bid_index > (int)smv_.base_bid_index_) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << "Ignoring Delete above best bid for order " << order_id
                                    << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
        return;
      }

      // Store old size and order count for OnPL change listeners.
      old_size = smv_.market_update_info_.bidlevels_[bid_index].limit_size_;
      old_ordercount = smv_.market_update_info_.bidlevels_[bid_index].limit_ordercount_;

      smv_.market_update_info_.bidlevels_[bid_index].limit_size_ -= size;
      smv_.market_update_info_.bidlevels_[bid_index].limit_ordercount_--;

      new_size = smv_.market_update_info_.bidlevels_[bid_index].limit_size_;
      new_ordercount = smv_.market_update_info_.bidlevels_[bid_index].limit_ordercount_;

      // Level deleted
      if (new_size <= 0 || new_ordercount <= 0) {
        new_size = smv_.market_update_info_.bidlevels_[bid_index].limit_size_ = 0;
        new_ordercount = smv_.market_update_info_.bidlevels_[bid_index].limit_ordercount_ = 0;
      }

      // If Best Bid Level Changed
      if (bid_index == (int)smv_.base_bid_index_) {
        // If Best Bid Level Deleted, then reset base_bid_index_ to next bid level
        if (new_size <= 0 || new_ordercount <= 0) {
          int index = smv_.base_bid_index_;

          for (; index >= 0; index--) {
            if (smv_.market_update_info_.bidlevels_[index].limit_ordercount_ > 0 &&
                smv_.market_update_info_.bidlevels_[index].limit_size_ > 0) {
              smv_.base_bid_index_ = index;
              break;
            }

            smv_.market_update_info_.bidlevels_[index].limit_size_ = 0;
            smv_.market_update_info_.bidlevels_[index].limit_ordercount_ = 0;
          }

          if (index < 0) {
            smv_.is_ready_ = false;
            DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " Bid side empty after Delete" << DBGLOG_ENDL_FLUSH;
            DBGLOG_DUMP;
            return;
          }
        }

        smv_.l1_changed_since_last_ = true;
      } else {
        smv_.l2_changed_since_last_ = true;
      }

      // Get the level for this update considering holes in the book.
      // Level supplied is 0 for L1 and so on.
      for (int t_index = smv_.base_bid_index_ - 1; t_index >= bid_index; t_index--) {
        if (smv_.market_update_info_.bidlevels_[t_index].limit_size_ > 0 &&
            smv_.market_update_info_.bidlevels_[t_index].limit_ordercount_ > 0)
          level_changed++;
      }

    } break;
    case kTradeTypeSell: {
      int ask_index = smv_.base_ask_index_ +
                      (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - int_price);

      // There are 0 levels on ask side
      if (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ <= 0) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " Delete called for empty ask side book : orderid "
                                    << order_id << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
        return;
      }

      if (ask_index >= (int)smv_.max_tick_range_) {
        RebuildIndexHighAccess(security_id, buy_sell, int_price);
        ask_index = smv_.base_ask_index_;
      }

      if (ask_index > (int)smv_.base_ask_index_) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " Delete below best ask  for order " << order_id
                                    << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
        return;
      }

      old_size = smv_.market_update_info_.asklevels_[ask_index].limit_size_;
      old_ordercount = smv_.market_update_info_.asklevels_[ask_index].limit_ordercount_;

      smv_.market_update_info_.asklevels_[ask_index].limit_size_ -= size;
      smv_.market_update_info_.asklevels_[ask_index].limit_ordercount_--;

      new_size = smv_.market_update_info_.asklevels_[ask_index].limit_size_;
      new_ordercount = smv_.market_update_info_.asklevels_[ask_index].limit_ordercount_;

      if (new_size <= 0 || new_ordercount <= 0) {
        new_size = smv_.market_update_info_.asklevels_[ask_index].limit_size_ = 0;
        new_ordercount = smv_.market_update_info_.asklevels_[ask_index].limit_ordercount_ = 0;
      }

      // If Best Ask Level Changed
      if (ask_index == (int)smv_.base_ask_index_) {
        // If Best Bid Level Deleted, then reset base_bid_index_ to next bid level
        if (new_size <= 0 || new_ordercount <= 0) {
          int index = smv_.base_ask_index_;

          for (; index >= 0; index--) {
            if (smv_.market_update_info_.asklevels_[index].limit_ordercount_ > 0 &&
                smv_.market_update_info_.asklevels_[index].limit_size_ > 0) {
              smv_.base_ask_index_ = index;
              break;
            }

            smv_.market_update_info_.asklevels_[index].limit_size_ = 0;
            smv_.market_update_info_.asklevels_[index].limit_ordercount_ = 0;
          }

          if (index < 0) {
            smv_.is_ready_ = false;
            DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " Ask side empty after Delete" << DBGLOG_ENDL_FLUSH;
            DBGLOG_DUMP;
            return;
          }
        }

        smv_.l1_changed_since_last_ = true;
      } else {
        smv_.l2_changed_since_last_ = true;
      }

      // Get the level for this update considering holes in the book.
      // Level supplied is 0 for L1 and so on.
      for (int t_index = smv_.base_ask_index_ - 1; t_index >= ask_index; t_index--) {
        if (smv_.market_update_info_.asklevels_[t_index].limit_size_ > 0 &&
            smv_.market_update_info_.asklevels_[t_index].limit_ordercount_ > 0)
          level_changed++;
      }

    } break;
    default:
      break;
  }

  if (smv_.l1_changed_since_last_) {
    switch (buy_sell) {
      case kTradeTypeBuy: {
        UpdateBestBidVariables(security_id, smv_.base_bid_index_);
        smv_.UpdateL1Prices();
      } break;
      case kTradeTypeSell: {
        UpdateBestAskVariables(security_id, smv_.base_ask_index_);
        smv_.UpdateL1Prices();
      } break;
      default:
        break;
    }
  }

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

  // Lets notify the listeners if this is normal delete
  // Incase this is a delete due to complete exec, we will accumulate this delta to be notified with trade together
  if (!is_complete_exec) {
    if (is_intermediate) {
      return;
    }

#if CCPROFILING_TRADEINIT
    HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(9);
#endif

    if (smv_.pl_change_listeners_present_) {
      smv_.NotifyOnPLChangeListeners(security_id, smv_.market_update_info_, buy_sell, level_changed - 1, int_price, 0,
                                     old_size, new_size, old_ordercount, new_ordercount, is_intermediate,
                                     new_size == 0 ? 'D' : 'C');
    }

  } else {
    // Even if this exec is an intermediate message, we accumulate execs and wait for trade notification.
    CheckAndBufferTrades(security_id, buy_sell, price, size, int_price);

    if (smv_.pl_change_listeners_present_) {  // complete exec & PL change listeners present

      prev_PL_change_state_.SetState(security_id, buy_sell, level_changed, int_price, 0, old_size, new_size,
                                     old_ordercount, new_ordercount, is_intermediate, new_size == 0 ? 'D' : 'C');
    }
  }
}

// Called in case of partial execution for an order_id. Incase of complete exec, OnOrderDelete is called
void IndexedRtsOFMarketViewManager::OnOrderExec(const unsigned int security_id, double price, char side, uint32_t size,
                                                uint64_t order_id, bool is_intermediate, bool is_ioc) {
  // Not adding ioc orders to book
  if (is_ioc) {
#if RTS_OFv2_DBG
    std::cout << "IOC order in passive book. Returning." << security_id << std::endl;
#endif
    return;
  }

#if RTS_OF_USE_ORDER_MAPS
  if (!day_order_pool_[security_id].IsOrderPresent(order_id, side)) {
#if RTS_OFv2_DBG
    std::cout << " Order not added for partial exec processing. Id: " << order_id << " side: " << side << "  sec "
              << security_id << std::endl;
#endif
    return;
  }
#endif

  SecurityMarketView& smv_ = *(security_market_view_map_[security_id]);

  int int_price = smv_.GetIntPx(price);
  TradeType_t buy_sell = (TradeType_t)side;

  if (!smv_.initial_book_constructed_) {
    BuildIndex(security_id, buy_sell, int_price);

    smv_.initial_book_constructed_ = true;
  }

  int old_size = 0;
  int old_ordercount = 0;
  int new_size = 0;
  int new_ordercount = 0;
  int level_changed = 0;

  switch (buy_sell) {
    case kTradeTypeBuy: {
      int bid_index = smv_.base_bid_index_ -
                      (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_ - int_price);

      // There are 0 levels on bid side
      if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ <= 0) {
        return;
      }

      if (bid_index >= (int)smv_.max_tick_range_) {
        RebuildIndexHighAccess(security_id, buy_sell, int_price);
        bid_index = smv_.base_bid_index_;
      }
      if (bid_index > (int)smv_.base_bid_index_) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " Exec above best bid for order " << order_id
                                    << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
        return;
      }

      // Store old size and order count for OnPL change listeners.
      old_size = smv_.market_update_info_.bidlevels_[bid_index].limit_size_;
      old_ordercount = smv_.market_update_info_.bidlevels_[bid_index].limit_ordercount_;

      smv_.market_update_info_.bidlevels_[bid_index].limit_size_ -= size;

      // orderCount remains same in partial execution
      new_size = smv_.market_update_info_.bidlevels_[bid_index].limit_size_;
      new_ordercount = smv_.market_update_info_.bidlevels_[bid_index].limit_ordercount_;

      if (new_size <= 0 || new_ordercount <= 0) {
        new_size = smv_.market_update_info_.bidlevels_[bid_index].limit_size_ = 0;
        new_ordercount = smv_.market_update_info_.bidlevels_[bid_index].limit_ordercount_ = 0;
      }

      // check for sanitization
      if (int_price < smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_) {
        SanitizeBidSide(security_id, order_id, int_price + 1);
        UpdateBestBidVariables(security_id, smv_.base_bid_index_);
      }

      // If Best Bid Level Changed
      // If best bid deleted, adjust best bid index
      if (bid_index == (int)smv_.base_bid_index_) {
        // If Best Bid Level Deleted, then reset base_bid_index_ to next bid level
        if (new_size <= 0 || new_ordercount <= 0) {
          int index = smv_.base_bid_index_;

          for (; index >= 0; index--) {
            if (smv_.market_update_info_.bidlevels_[index].limit_ordercount_ > 0 &&
                smv_.market_update_info_.bidlevels_[index].limit_size_ > 0) {
              smv_.base_bid_index_ = index;
              break;
            }

            smv_.market_update_info_.bidlevels_[index].limit_size_ = 0;
            smv_.market_update_info_.bidlevels_[index].limit_ordercount_ = 0;
          }

          if (index < 0) {
            smv_.is_ready_ = false;
            DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname()
                                        << " Bid side empty after Partial exec on L1 for oid: " << order_id
                                        << DBGLOG_ENDL_FLUSH;
            DBGLOG_DUMP;
            return;
          }
        }

        smv_.l1_changed_since_last_ = true;
        UpdateBestBidVariables(security_id, smv_.base_bid_index_);
      } else {
        smv_.l2_changed_since_last_ = true;
      }

      // Get the level for this update considering holes in the book.
      // Level supplied is 0 for L1 and so on.
      for (int t_index = smv_.base_bid_index_ - 1; t_index >= bid_index; t_index--) {
        if (smv_.market_update_info_.bidlevels_[t_index].limit_size_ > 0 &&
            smv_.market_update_info_.bidlevels_[t_index].limit_ordercount_ > 0)
          level_changed++;
      }

    } break;
    case kTradeTypeSell: {
      int ask_index = smv_.base_ask_index_ +
                      (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_ - int_price);

      // There are 0 levels on ask side
      if (smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ <= 0) {
        return;
      }

      if (ask_index >= (int)smv_.max_tick_range_) {
        RebuildIndexHighAccess(security_id, buy_sell, int_price);

        ask_index = smv_.base_ask_index_;
      }

      if (ask_index > (int)smv_.base_ask_index_) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " Exec below best ask for order " << order_id
                                    << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
        return;
      }

      old_size = smv_.market_update_info_.asklevels_[ask_index].limit_size_;
      old_ordercount = smv_.market_update_info_.asklevels_[ask_index].limit_ordercount_;

      smv_.market_update_info_.asklevels_[ask_index].limit_size_ -= size;

      new_size = smv_.market_update_info_.asklevels_[ask_index].limit_size_;
      new_ordercount = smv_.market_update_info_.asklevels_[ask_index].limit_ordercount_;

      if (new_size <= 0 || new_ordercount <= 0) {
        new_size = smv_.market_update_info_.asklevels_[ask_index].limit_size_ = 0;
        new_ordercount = smv_.market_update_info_.asklevels_[ask_index].limit_ordercount_ = 0;
      }

      // check for ask side sanitization
      if (int_price > smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_) {
        SanitizeAskSide(security_id, order_id, int_price - 1);
        UpdateBestAskVariables(security_id, smv_.base_ask_index_);
      }

      // If Best Ask Level Changed
      if (ask_index == (int)smv_.base_ask_index_) {
        // If Best Bid Level Deleted, then reset base_bid_index_ to next bid level
        if (new_size <= 0 || new_ordercount <= 0) {
          int index = smv_.base_ask_index_;

          for (; index >= 0; index--) {
            if (smv_.market_update_info_.asklevels_[index].limit_ordercount_ > 0 &&
                smv_.market_update_info_.asklevels_[index].limit_size_ > 0) {
              smv_.base_ask_index_ = index;
              break;
            }

            smv_.market_update_info_.asklevels_[index].limit_size_ = 0;
            smv_.market_update_info_.asklevels_[index].limit_ordercount_ = 0;
          }

          if (index < 0) {
            smv_.is_ready_ = false;
            DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " Ask side empty after Delete" << DBGLOG_ENDL_FLUSH;
            DBGLOG_DUMP;
            return;
          }
        }

        smv_.l1_changed_since_last_ = true;
        UpdateBestAskVariables(security_id, smv_.base_ask_index_);
      } else {
        smv_.l2_changed_since_last_ = true;
      }

      // Get the level for this update considering holes in the book.
      // Level supplied is 0 for L1 and so on.
      for (int t_index = smv_.base_ask_index_ - 1; t_index >= ask_index; t_index--) {
        if (smv_.market_update_info_.asklevels_[t_index].limit_size_ > 0 &&
            smv_.market_update_info_.asklevels_[t_index].limit_ordercount_ > 0)
          level_changed++;
      }

    } break;
    default:
      break;
  }

  if (smv_.l1_changed_since_last_) {
    switch (buy_sell) {
      case kTradeTypeBuy: {
        UpdateBestBidVariables(security_id, smv_.base_bid_index_);

        smv_.UpdateL1Prices();
      } break;
      case kTradeTypeSell: {
        UpdateBestAskVariables(security_id, smv_.base_ask_index_);
        smv_.UpdateL1Prices();
      } break;
      default:
        break;
    }
  }

  if (!smv_.is_ready_) {
    if (smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_size_ > 0 &&
        smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_size_ > 0 && smv_.ArePricesComputed()) {
      smv_.is_ready_ = true;
    } else {
      return;
    }
  }

  // Even if this exec is an intermediate message, we accumulate execs and wait for trade notification.
  CheckAndBufferTrades(security_id, buy_sell, price, size, int_price);

  // Even if this exec is an intermediate message. We accumulate the delta for this exec as well.
  if (smv_.pl_change_listeners_present_) {
    //  accumulate the delta here and notify when entire trade is notified
    prev_PL_change_state_.SetState(security_id, buy_sell, level_changed, int_price, 0, old_size, new_size,
                                   old_ordercount, new_ordercount, is_intermediate, new_size == 0 ? 'D' : 'C');
  }
}

void IndexedRtsOFMarketViewManager::OnResetBegin(const unsigned int t_security_id_) {
  timeval tv;
  gettimeofday(&tv, NULL);
  DBGLOG_CLASS_FUNC_LINE_INFO << sec_name_indexer_.GetSecurityNameFromId(t_security_id_) << " watch@"
                              << watch_.tv_ToString() << " "
                              << "TimeOfDay@ " << tv.tv_sec << "." << tv.tv_usec
                              << " Resetting order book, flushing all the orders so far..."
                              << "\n";
#if RTS_OFv2_DBG
  std::cout << sec_name_indexer_.GetSecurityNameFromId(t_security_id_) << " watch@" << watch_.tv_ToString() << " "
            << "TimeOfDay@ " << tv.tv_sec << "." << tv.tv_usec
            << " Resetting order book, flushing all the orders so far..." << std::endl;
#endif
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);
  smv_.InitializeSMVForIndexedBook();

  // clear the aggress order and day order pool(if used)
  trade_prediction_info_.clear();

#if RTS_OF_USE_ORDER_MAPS
  std::vector<std::map<uint64_t, bool> >& day_order_ids = day_order_pool_[t_security_id_].GetOrders();
  day_order_ids[0].clear();
  day_order_ids[1].clear();
#endif
}
void IndexedRtsOFMarketViewManager::OnResetEnd(const unsigned int t_security_id_) {}

void IndexedRtsOFMarketViewManager::SanitizeBidSide(const unsigned int t_security_id_, uint64_t order_id,
                                                    int t_int_price_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  // Log sanitisation calls
  DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " " << smv_.base_bid_index_ << " sec " << t_security_id_
                              << " px " << t_int_price_ << " OID: " << order_id << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;

#if RTS_OFv2_DBG
  std::cout << " Sanitize bid side " << smv_.secname() << " " << smv_.base_bid_index_ << " sec " << t_security_id_
            << " px " << t_int_price_ << std::endl;
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

    DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " Bid side empty after sanitization " << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    return;
  }

  // Check if we need to re-align the center
  if (smv_.base_bid_index_ < LOW_ACCESS_INDEX) {
    RebuildIndexLowAccess(t_security_id_, kTradeTypeBuy,
                          smv_.market_update_info_.bidlevels_[smv_.base_bid_index_].limit_int_price_);
  }
}

void IndexedRtsOFMarketViewManager::SanitizeAskSide(const unsigned int t_security_id_, uint64_t order_id,
                                                    int t_int_price_) {
  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  // Log sanitisation calls
  DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " " << smv_.base_ask_index_ << " sec " << t_security_id_
                              << " px " << t_int_price_ << "  OID: " << order_id << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;

#if RTS_OFv2_DBG
  std::cout << " Sanitize ask side " << smv_.secname() << " " << smv_.base_ask_index_ << " sec " << t_security_id_
            << " px " << t_int_price_ << std::endl;
#endif

  // Sanitise ASK side
  int index = smv_.base_ask_index_;

  for (; index >= 0; index--) {
    if (smv_.market_update_info_.asklevels_[index].limit_int_price_ > t_int_price_ &&
        smv_.market_update_info_.asklevels_[index].limit_size_ > 0) {
      smv_.base_ask_index_ = index;
      break;
    }
    smv_.market_update_info_.asklevels_[index].limit_size_ = 0;
    smv_.market_update_info_.asklevels_[index].limit_ordercount_ = 0;
  }

  // The ask side is empty
  if (index < 0) {
    smv_.is_ready_ = false;
    DBGLOG_TIME_CLASS_FUNC_LINE << " " << smv_.secname() << " Ask side empty after sanitization " << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    return;
  }

  // Check if we need to re-align the index
  if (smv_.base_ask_index_ < LOW_ACCESS_INDEX) {
    RebuildIndexLowAccess(t_security_id_, kTradeTypeSell,
                          smv_.market_update_info_.asklevels_[smv_.base_ask_index_].limit_int_price_);
  }
}

void IndexedRtsOFMarketViewManager::CheckAndBufferTrades(const unsigned int t_security_id_, uint8_t side, double price,
                                                         int size, int int_price) {
#if RTS_OFv2_DBG
  std::cout << typeid(*this).name() << ':' << __func__ << t_security_id_ << " " << price << " " << size << std::endl;
#endif

  // get this trade aggress side
  HFSAT::TradeType_t agg_side = ((side == 0) ? kTradeTypeSell : kTradeTypeBuy);

  if (buffered_trades_[t_security_id_].size() > 0 &&
      buffered_trades_[t_security_id_].back().int_trade_price_ == int_price &&
      buffered_trades_[t_security_id_].back().buysell_ == agg_side) {
    buffered_trades_[t_security_id_].back().size_traded_ += size;
#if RTS_OFv2_DBG
    std::cout << typeid(*this).name() << ':' << __func__ << " Cumulate trade: " << price << " "
              << buffered_trades_[t_security_id_].back().size_traded_ << " " << (int)side << std::endl;
#endif
  } else {
    TradeDetails t_cur;
    t_cur.buysell_ = ((side == 0) ? kTradeTypeSell : kTradeTypeBuy);
    t_cur.price_ = price;
    t_cur.size_traded_ = size;
    t_cur.int_trade_price_ = int_price;
#if RTS_OFv2_DBG
    std::cout << typeid(*this).name() << ':' << __func__ << " Exec at new px: " << price << " " << size << " "
              << (int)side << " Num buffered execs: " << buffered_trades_[t_security_id_].size() << std::endl;
#endif
    buffered_trades_[t_security_id_].push_back(t_cur);
  }
  return;
}

// Notifying all buffered trades which couldn't be notified(intermediate msgs)
// This will be called after processing any order update [Add, Exec, Delete]
void IndexedRtsOFMarketViewManager::CheckToNotifyTradeMessage(const unsigned int t_security_id_,
                                                              bool is_intermediate_msg) {
#if RTS_OFv2_DBG
  std::cout << typeid(*this).name() << ':' << __func__ << t_security_id_ << " " << is_intermediate_msg << " "
            << buffered_trades_[t_security_id_].size() << std::endl;
#endif

  if (is_intermediate_msg || buffered_trades_[t_security_id_].size() == 0) return;

  SecurityMarketView& smv_ = *(security_market_view_map_[t_security_id_]);

  while (buffered_trades_[t_security_id_].size() > 0) {
    TradeDetails t_cur = buffered_trades_[t_security_id_].front();
    buffered_trades_[t_security_id_].pop_front();

    // Set the trade variables
    smv_.trade_print_info_.trade_price_ = t_cur.price_;
    smv_.trade_print_info_.size_traded_ = t_cur.size_traded_;
    smv_.trade_print_info_.int_trade_price_ = t_cur.int_trade_price_;
    smv_.trade_print_info_.buysell_ = t_cur.buysell_;

    smv_.SetTradeVarsForIndicatorsIfRequired();

    // Notifying trade
    if (smv_.is_ready_) {
#if RTS_OFv2_DBG
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
#if RTS_OFv2_DBG
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

OrdersPool::OrdersPool() : order_id_to_details_(2) {}
std::vector<std::map<uint64_t, OrderDetails> >& OrdersPool::GetOrders() { return order_id_to_details_; }

bool OrdersPool::IsOrderPresent(uint64_t order_id, char side) {
  return (order_id_to_details_[side - '0'].find(order_id) != order_id_to_details_[side - '0'].end());
}

void OrdersPool::Process(RTS_MDS::RTSOFCommonStructv2 order) {
  switch (order.msg_type) {
    case RTS_MDS::RTSOFMsgType::kRTSAdd: {
#if RTS_OFv2_DBG
      std::cout << "Process agg add: order_id " << order.order_id << " px: " << order.price << " sz: " << order.size
                << std::endl;
#endif

      OrderDetails details;
      details.order_id = order.order_id;
      details.price = order.price;
      details.side = order.side;
      details.size = order.size;
      details.is_ioc = order.md_flags & 0x02;  // IOC order, to filter any ioc order;
      order_id_to_details_[order.side - '0'][order.order_id] = details;
    } break;
    case RTS_MDS::RTSOFMsgType::kRTSDelete: {
#if RTS_OFv2_DBG
      std::cout << "Process agg orders delete: order_id " << order.order_id << " px: " << order.price
                << " sz: " << order.size << std::endl;
#endif

      if (order_id_to_details_[order.side - '0'].find(order.order_id) != order_id_to_details_[order.side - '0'].end()) {
        order_id_to_details_[order.side - '0'].erase(order.order_id);
      }
    } break;
    case RTS_MDS::RTSOFMsgType::kRTSExec: {
      if (order.is_full_exec) {
#if RTS_OFv2_DBG
        std::cout << "Agg orders full exec. Delete from agg pool, order_id " << order.order_id << " px: " << order.price
                  << " sz: " << order.size << std::endl;
#endif
        if (order_id_to_details_[order.side - '0'].find(order.order_id) !=
            order_id_to_details_[order.side - '0'].end()) {
          order_id_to_details_[order.side - '0'].erase(order.order_id);
        }
      } else {
#if RTS_OFv2_DBG
        std::cout << "Agg orders partial exec: order_id " << order.order_id << " px: " << order.price
                  << " sz: " << order.size << std::endl;
#endif
        if (order_id_to_details_[order.side - '0'].find(order.order_id) !=
            order_id_to_details_[order.side - '0'].end()) {
          order_id_to_details_[order.side - '0'][order.order_id].size -= order.size;
#if RTS_OFv2_DBG
          std::cout << "Agg orders partial exec: order_id: " << order.order_id << " new_size "
                    << order_id_to_details_[order.side - '0'][order.order_id].size << std::endl;
#endif
          if (order_id_to_details_[order.side - '0'][order.order_id].size <= 0) {
            order_id_to_details_[order.side - '0'].erase(order.order_id);
          }
        }
      }

    } break;
    case RTS_MDS::RTSOFMsgType::kRTSDeleteAll: {
      order_id_to_details_[0].clear();
      order_id_to_details_[1].clear();
    } break;
    default:
      break;
  }
}

DayOrdersPool::DayOrdersPool() : order_id_to_details_(2) {}
std::vector<std::map<uint64_t, bool> >& DayOrdersPool::GetOrders() { return order_id_to_details_; }
bool DayOrdersPool::IsOrderPresent(uint64_t order_id, char side) {
  return (order_id_to_details_[(int)side].find(order_id) != order_id_to_details_[(int)side].end());
}
void DayOrdersPool::Process(uint64_t order_id, RTS_MDS::RTSOFMsgType msg_type, char side) {
  switch (msg_type) {
    case RTS_MDS::RTSOFMsgType::kRTSAdd: {
#if RTS_OFv2_DBG
      std::cout << " Adding Order id  " << order_id << " side: " << (int)side << std::endl;
#endif
      order_id_to_details_[side][order_id] = true;
    } break;
    case RTS_MDS::RTSOFMsgType::kRTSDelete: {
      if (order_id_to_details_[side].find(order_id) != order_id_to_details_[side].end()) {
        order_id_to_details_[side].erase(order_id);
      } else {
#if RTS_OFv2_DBG
        std::cout << " Order already added. Id: " << order_id << " side: " << side << std::endl;
#endif
      }
    } break;
    default:
      break;
  }
}
}
