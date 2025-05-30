/**
   \file MarketAdapterCode/ice_market_order_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/MarketAdapter/ice_market_order_manager.hpp"

namespace HFSAT {

IceMarketOrderManager::IceMarketOrderManager(DebugLogger &t_dbglogger_, const Watch &t_watch_,
                                             SecurityNameIndexer &t_sec_name_indexer_,
                                             const std::vector<MarketOrdersView *> &t_market_orders_view_map_)
    : BaseMarketOrderManager(t_dbglogger_, t_watch_, t_sec_name_indexer_, t_market_orders_view_map_),
      ice_markets_(t_sec_name_indexer_.NumSecurityId(), new IceMarketPerSecurity()),
      first_trade_message_(true),
      is_last_message_trade_message(false),
      trade_sum_aggregate_(0),
      last_trade_price_(0.0),
      last_traded_sec_id_(-1),
      last_buysell_(kTradeTypeNoInfo),
      sec_name_indexer_(t_sec_name_indexer_) {}

void IceMarketOrderManager::OnOrderAdd(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                       const int64_t t_order_id_, const double t_price_, const uint32_t t_size_,
                                       const int64_t priority_) {
  MarketOrdersView *mov_ = market_orders_view_map_[t_security_id_];
  IceMarketPerSecurity *mkt_ = ice_markets_[t_security_id_];

  // There are cases in ICE which results in which order is added
  // either in front of the queue or somewhere in the middle,
  // So, the best way to handle it is to always insert new order
  // based on its priority

  // check that the aggregation logic is only for LFI and LFL

  if (is_last_message_trade_message == true &&
      (sec_name_indexer_.GetShortcodeFromId(t_security_id_).substr(0, 3) == std::string("LFI") ||
       sec_name_indexer_.GetShortcodeFromId(t_security_id_).substr(0, 3) == std::string("LFL"))) {
    is_last_message_trade_message = false;
    NotifyAggregateTrade(trade_sum_aggregate_, last_trade_price_, last_traded_sec_id_, last_buysell_);
    UpdateMov(last_trade_price_, last_traded_sec_id_, last_buysell_);
    ResetTradeVariable();
  }

  if (priority_ <= 0) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR"
                           << " priority: " << priority_ << " for OrderId: " << t_order_id_ << DBGLOG_ENDL_FLUSH;
    return;
  }

  if (mkt_->ord_map_.find(t_order_id_) != mkt_->ord_map_.end()) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR"
                           << " OrderId: " << t_order_id_ << " already present in map" << DBGLOG_ENDL_FLUSH;
  }

  if (t_order_id_ > mkt_->last_add_order_) {
    mkt_->last_add_order_ = t_order_id_;
  }

  std::tr1::shared_ptr<IceOrder> order_(new IceOrder());
  order_->price_ = t_price_;
  order_->size_ = t_size_;
  order_->buysell_ = t_buysell_;
  mkt_->ord_map_[t_order_id_] = order_;

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      mov_->AddMarketBidPriority(t_order_id_, t_price_, t_size_, priority_);
    } break;
    case kTradeTypeSell: {
      mov_->AddMarketAskPriority(t_order_id_, t_price_, t_size_, priority_);
    } break;
    default:
      break;
  }
}

void IceMarketOrderManager::OnOrderModify(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                          const int64_t t_order_id_, const double t_price_, const uint32_t t_size_) {
  MarketOrdersView *mov_ = market_orders_view_map_[t_security_id_];
  IceMarketPerSecurity *mkt_ = ice_markets_[t_security_id_];

  if (is_last_message_trade_message == true &&
      (sec_name_indexer_.GetShortcodeFromId(t_security_id_).substr(0, 3) == std::string("LFI") ||
       sec_name_indexer_.GetShortcodeFromId(t_security_id_).substr(0, 3) == std::string("LFL"))) {
    is_last_message_trade_message = false;
    NotifyAggregateTrade(trade_sum_aggregate_, last_trade_price_, last_traded_sec_id_, last_buysell_);
    UpdateMov(last_trade_price_, last_traded_sec_id_, last_buysell_);
    ResetTradeVariable();
  }

  if (mkt_->ord_map_.find(t_order_id_) == mkt_->ord_map_.end()) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR"
                           << " OrderId: " << t_order_id_ << " not present in map"
                           << " " << mov_->GetMarketString() << DBGLOG_ENDL_FLUSH;

    return;
  }

  auto order_ = mkt_->ord_map_[t_order_id_];
  double prev_price_ = order_->price_;
  int prev_size_ = order_->size_;

  if (order_->buysell_ != t_buysell_) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR" << DBGLOG_ENDL_FLUSH;
  }

  // Assign new price/size to the order
  order_->price_ = t_price_;
  order_->size_ = t_size_;

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      mov_->ModifyMarketBid(t_order_id_, prev_price_, prev_size_, t_order_id_, t_price_, t_size_);
    } break;
    case kTradeTypeSell: {
      mov_->ModifyMarketAsk(t_order_id_, prev_price_, prev_size_, t_order_id_, t_price_, t_size_);
    } break;
    default:
      break;
  }
}

void IceMarketOrderManager::OnOrderDelete(const uint32_t t_security_id_, const int64_t t_order_id_) {
  MarketOrdersView *mov_ = market_orders_view_map_[t_security_id_];
  IceMarketPerSecurity *mkt_ = ice_markets_[t_security_id_];

  if (is_last_message_trade_message == true &&
      (sec_name_indexer_.GetShortcodeFromId(t_security_id_).substr(0, 3) == std::string("LFI") ||
       sec_name_indexer_.GetShortcodeFromId(t_security_id_).substr(0, 3) == std::string("LFL"))) {
    is_last_message_trade_message = false;
    NotifyAggregateTrade(trade_sum_aggregate_, last_trade_price_, last_traded_sec_id_, last_buysell_);
    UpdateMov(last_trade_price_, last_traded_sec_id_, last_buysell_);
    ResetTradeVariable();
  }

  // We don't get price/size information in order delete message,
  // So, fetch that information first.

  if (mkt_->ord_map_.find(t_order_id_) == mkt_->ord_map_.end()) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR"
                           << " OrderId: " << t_order_id_ << " not present in map"
                           << " " << mov_->GetMarketString() << DBGLOG_ENDL_FLUSH;

    return;
  }

  auto order_ = mkt_->ord_map_[t_order_id_];
  double price_ = order_->price_;
  int size_ = order_->size_;
  TradeType_t buysell_ = order_->buysell_;

  // Remove from the map and dealloc the order
  mkt_->ord_map_.erase(t_order_id_);

  switch (buysell_) {
    case kTradeTypeBuy: {
      mov_->DeleteMarketBid(t_order_id_, price_, size_);
    } break;
    case kTradeTypeSell: {
      mov_->DeleteMarketAsk(t_order_id_, price_, size_);
    } break;
    default:
      break;
  }
}

void IceMarketOrderManager::OnOrderExec(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                        const int64_t t_order_id_, const double t_traded_price_,
                                        const uint32_t t_traded_size_) {
  MarketOrdersView *mov_ = market_orders_view_map_[t_security_id_];
  IceMarketPerSecurity *mkt_ = ice_markets_[t_security_id_];

  // Since order delete/modify messages are implicit for exec,
  // delete them here

  // ICE also has weird behavior on partial exec.
  // On partial exec, we need to delete the whole order and ICE
  // will send the remaining order as a new order.
  // So, it's best to always rely on entry time based priority for ordering

  if (t_order_id_ > mkt_->last_add_order_) {
    // If the order id is more than the last order id added,
    // then this is a spurious trade
    DBGLOG_TIME_CLASS_FUNC << "WARNING " << t_order_id_ << " > " << mkt_->last_add_order_ << DBGLOG_ENDL_FLUSH;
    if (is_last_message_trade_message == true &&
        (sec_name_indexer_.GetShortcodeFromId(t_security_id_).substr(0, 3) == std::string("LFI") ||
         sec_name_indexer_.GetShortcodeFromId(t_security_id_).substr(0, 3) == std::string("LFL"))) {
      is_last_message_trade_message = false;
      NotifyAggregateTrade(trade_sum_aggregate_, last_trade_price_, last_traded_sec_id_, last_buysell_);
      UpdateMov(last_trade_price_, last_traded_sec_id_, last_buysell_);
      ResetTradeVariable();
    }
    return;
  }
  if (mkt_->ord_map_.find(t_order_id_) == mkt_->ord_map_.end()) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR"
                           << " OrderId: " << t_order_id_ << " not present in map"
                           << " " << mov_->GetMarketString() << DBGLOG_ENDL_FLUSH;
    if (is_last_message_trade_message == true &&
        (sec_name_indexer_.GetShortcodeFromId(t_security_id_).substr(0, 3) == std::string("LFI") ||
         sec_name_indexer_.GetShortcodeFromId(t_security_id_).substr(0, 3) == std::string("LFL"))) {
      is_last_message_trade_message = false;
      NotifyAggregateTrade(trade_sum_aggregate_, last_trade_price_, last_traded_sec_id_, last_buysell_);
      UpdateMov(last_trade_price_, last_traded_sec_id_, last_buysell_);
      ResetTradeVariable();
    }
    return;
  }

  auto order_ = mkt_->ord_map_[t_order_id_];
  int size_ = order_->size_;

  if (t_buysell_ != order_->buysell_ && t_buysell_ != kTradeTypeNoInfo) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR"
                           << " TradeType_t for OrderId: " << t_order_id_ << " is: " << t_buysell_
                           << " map: " << order_->buysell_ << " " << mov_->GetMarketString() << DBGLOG_ENDL_FLUSH;
  }

  TradeType_t buysell_ = order_->buysell_;

  if ((sec_name_indexer_.GetShortcodeFromId(t_security_id_).substr(0, 3) == std::string("LFI") ||
       sec_name_indexer_.GetShortcodeFromId(t_security_id_).substr(0, 3) == std::string("LFL"))) {
    if (is_last_message_trade_message == false) {
      is_last_message_trade_message = true;
    }

    if (first_trade_message_) {
      if (is_last_message_trade_message) {
        trade_sum_aggregate_ = trade_sum_aggregate_ + t_traded_size_;
        last_trade_price_ = t_traded_price_;
        last_traded_sec_id_ = t_security_id_;
        last_buysell_ = t_buysell_;
        order_execution_map_[t_order_id_] = t_traded_size_;
        order_size_map_[t_order_id_] = size_;
      }
      first_trade_message_ = false;
    } else {
      // check that trade is happening on the same side for the same security and same price , if yes then aggregate
      if (is_last_message_trade_message && last_trade_price_ == t_traded_price_ &&
          last_traded_sec_id_ == t_security_id_ && last_buysell_ == t_buysell_) {
        trade_sum_aggregate_ = trade_sum_aggregate_ + t_traded_size_;
        order_execution_map_[t_order_id_] = t_traded_size_;
        order_size_map_[t_order_id_] = size_;
      } else {
        is_last_message_trade_message = false;
        NotifyAggregateTrade(trade_sum_aggregate_, last_trade_price_, last_traded_sec_id_, last_buysell_);
        UpdateMov(last_trade_price_, last_traded_sec_id_, last_buysell_);
        ResetTradeVariable();
      }
    }
  } else {
    // Only for non LFI and non LFL securities
    // Remove from the map and dealloc the order
    mkt_->ord_map_.erase(t_order_id_);
    switch (buysell_) {
      case kTradeTypeBuy: {
        mov_->ExecMarketBid(t_order_id_, t_traded_price_, t_traded_size_, false);

        mov_->DeleteMarketBid(t_order_id_, t_traded_price_, size_);
      } break;
      case kTradeTypeSell: {
        mov_->ExecMarketAsk(t_order_id_, t_traded_price_, t_traded_size_, false);

        mov_->DeleteMarketAsk(t_order_id_, t_traded_price_, size_);
      } break;
      default:
        break;
    }
  }
}

void IceMarketOrderManager::NotifyAggregateTrade(const int sum_aggregate_trade_size_, const double t_traded_price_,
                                                 const uint32_t t_security_id_, TradeType_t last_buysell_) {
  MarketOrdersView *mov_ = market_orders_view_map_[t_security_id_];
  switch (last_buysell_) {
    case kTradeTypeBuy: {
      mov_->ExecMarketBid(0, t_traded_price_, sum_aggregate_trade_size_, true);
    } break;
    case kTradeTypeSell: {
      mov_->ExecMarketAsk(0, t_traded_price_, sum_aggregate_trade_size_, true);
    } break;
    default:
      break;
  }
}

void IceMarketOrderManager::ResetTradeVariable() {
  trade_sum_aggregate_ = 0;
  last_trade_price_ = 0.0;
  last_traded_sec_id_ = -1;
  first_trade_message_ = true;
  last_buysell_ = kTradeTypeNoInfo;
  order_execution_map_.clear();
  order_size_map_.clear();
}

void IceMarketOrderManager::UpdateMov(const double t_traded_price_, const uint32_t t_security_id_,
                                      TradeType_t last_buysell_) {
  std::map<int64_t, uint32_t>::iterator it;
  MarketOrdersView *mov_ = market_orders_view_map_[t_security_id_];
  IceMarketPerSecurity *mkt_ = ice_markets_[t_security_id_];
  // remove the order that has already been executed from the mov

  if (last_buysell_ == kTradeTypeBuy) {
    for (it = order_execution_map_.begin(); it != order_execution_map_.end(); it++) {
    }
  } else {
    for (it = order_execution_map_.begin(); it != order_execution_map_.end(); it++) {
    }
  }

  for (it = order_execution_map_.begin(); it != order_execution_map_.end(); it++) {
    if (last_buysell_ == kTradeTypeBuy) {
      mkt_->ord_map_.erase(it->first);
      mov_->ExecMarketBid(it->first, t_traded_price_, it->second, false);
      mov_->DeleteMarketBid(it->first, t_traded_price_, order_size_map_[it->first]);
    } else {
      mkt_->ord_map_.erase(it->first);
      mov_->ExecMarketAsk(it->first, t_traded_price_, it->second, false);
      mov_->DeleteMarketAsk(it->first, t_traded_price_, order_size_map_[it->first]);
    }
  }
}

void IceMarketOrderManager::ResetBook(const unsigned int t_security_id_) {
  MarketOrdersView *mov_ = market_orders_view_map_[t_security_id_];
  IceMarketPerSecurity *mkt_ = ice_markets_[t_security_id_];

  mkt_->Clear();
  mov_->Reset();
}
}
