/**
   \file MarketAdapterCode/nse_market_order_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/MarketAdapter/nse_market_order_manager.hpp"

namespace HFSAT {

NSEMarketOrderManager::NSEMarketOrderManager(DebugLogger& dbglogger, const Watch& t_watch_,
                                             SecurityNameIndexer& t_sec_name_indexer_,
                                             const std::vector<MarketOrdersView*>& t_market_orders_view_map_,
                                             bool _is_hidden_order_avialable_, bool is_ose_itch)
    : BaseMarketOrderManager(dbglogger, t_watch_, t_sec_name_indexer_, t_market_orders_view_map_),
      nse_order_mempool_(),
      nse_markets_(t_sec_name_indexer_.NumSecurityId(), new NSEMarketPerSecurity()),
      is_ose_itch_(is_ose_itch),
      is_hidden_order_available_(_is_hidden_order_avialable_),
      modified_order_id_(0),
      modified_int_price_(0),
      modified_cur_size_(0),
      modified_prev_size_(0),
      modified_time_(ttime_t(0, 0)) {}

void NSEMarketOrderManager::OnOrderAdd(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                       const uint64_t t_order_id_, const double t_price_, const uint32_t t_size_,
                                       const bool t_is_intermediate_) {
  MarketOrdersView* mov = market_orders_view_map_[t_security_id_];
  NSEMarketPerSecurity* mkt = nse_markets_[t_security_id_];

  if (mkt->order_id_map_.find(t_order_id_) != mkt->order_id_map_.end()) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR"
                           << " OrderId: " << t_order_id_ << " already present in order map as unqueued"
                           << DBGLOG_ENDL_FLUSH;
    if (!t_is_intermediate_) {
      modified_order_id_ = 0;
    }
    return;
  }

  NSEOrder* order = nse_order_mempool_.Alloc();
  order->price = t_price_;
  order->size = t_size_;
  order->buysell_ = t_buysell_;

  int int_price_ = mov->GetIntPx(t_price_);

  // Check for crossed book
  if (((t_buysell_ == kTradeTypeBuy) && (int_price_ >= mov->GetBestAskIntPrice())) ||
      ((t_buysell_ == kTradeTypeSell) && (int_price_ <= mov->GetBestBidIntPrice()))) {
    mkt->queued_order_id_map_[t_order_id_] = order;
  } else {
    mkt->order_id_map_[t_order_id_] = order;
    if (t_buysell_ == kTradeTypeBuy)
      mov->AddMarketBidPriority(t_order_id_, t_price_, t_size_, 0, is_ose_itch_);
    else
      mov->AddMarketAskPriority(t_order_id_, t_price_, t_size_, 0, is_ose_itch_);
  }

  if (!t_is_intermediate_) {
    modified_order_id_ = 0;
  }

  /*if((!t_is_intermediate_))
    std::cout << watch_.tv() << " OrderAdd " << std::setprecision(7) << t_price_ << " " << t_size_ << "  Mkt: " <<
  mov->GetBidSize(mov->GetBestBidIntPrice())  << " " << std::setprecision(7) << mov->GetBestBidPrice() << " X  "  <<
  std::setprecision(7) << mov->GetBestAskPrice() << " " << mov->GetAskSize(mov->GetBestAskIntPrice())  << std::endl;
  else
    std::cout << watch_.tv() << " SimulatedOrderAdd "  << std::setprecision(7) << t_price_ << " " << t_size_ << "  Mkt:
  " << mov->GetBidSize(mov->GetBestBidIntPrice())  << " "  << std::setprecision(7) << mov->GetBestBidPrice() << " X  "
  << std::setprecision(7) << mov->GetBestAskPrice() << " " << mov->GetAskSize(mov->GetBestAskIntPrice())  << std::endl;
  */

}

void NSEMarketOrderManager::OnOrderModify(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                          const uint64_t t_order_id_, const double t_price_, const uint32_t t_size_) {
  // This changes priority as well
  // So, do a delete followed by add

  // Delete

  MarketOrdersView* mov = market_orders_view_map_[t_security_id_];
  NSEMarketPerSecurity* mkt = nse_markets_[t_security_id_];

  NSEOrder* order = nullptr;
  if (mkt->order_id_map_.find(t_order_id_) != mkt->order_id_map_.end()) {
    order = mkt->order_id_map_[t_order_id_];

    // We need to keep track of modified state for hidden orders
    // only when following conditions hold:
    // 1) Price is not changed
    // 2) The order is passive
    // 3) Hidden orders are allowed

    if (is_hidden_order_available_) {
      int int_trade_price_ = mov->GetIntPx(t_price_);
      int int_order_price_ = mov->GetIntPx(order->price);

      if ((int_order_price_ == int_trade_price_) && ((uint32_t)order->size < t_size_)) {
        modified_order_id_ = t_order_id_;
        modified_int_price_ = int_trade_price_;
        modified_cur_size_ = t_size_;
        // modified_prev_size_ = t_prev_size_;
        modified_time_ = watch_.tv();
      } else {
        modified_order_id_ = 0;
      }
    }

    if (order->buysell_ == kTradeTypeBuy)
      mov->DeleteMarketBid(t_order_id_, order->price, order->size);
    else
      mov->DeleteMarketAsk(t_order_id_, order->price, order->size);
    mkt->order_id_map_.erase(t_order_id_);
    nse_order_mempool_.DeAlloc(order);
  } else if (mkt->queued_order_id_map_.find(t_order_id_) != mkt->queued_order_id_map_.end()) {
    order = mkt->queued_order_id_map_[t_order_id_];
    mkt->queued_order_id_map_.erase(t_order_id_);
    nse_order_mempool_.DeAlloc(order);
  } else {
    DBGLOG_TIME_CLASS_FUNC << "ERROR"
                           << " OrderId: " << t_order_id_ << " is not present in order map" << DBGLOG_ENDL_FLUSH;
  }

  // Add the new order
  OnOrderAdd(t_security_id_, t_buysell_, t_order_id_, t_price_, t_size_, true);
}

void NSEMarketOrderManager::OnOrderDelete(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                          const uint64_t t_order_id_, const double t_price_, const bool t_delete_order_,
                                          const bool t_is_intermediate_) {
  MarketOrdersView* mov = market_orders_view_map_[t_security_id_];
  NSEMarketPerSecurity* mkt = nse_markets_[t_security_id_];

  NSEOrder* order = nullptr;
  if (mkt->order_id_map_.find(t_order_id_) != mkt->order_id_map_.end()) {
    order = mkt->order_id_map_[t_order_id_];
    if (order->buysell_ == kTradeTypeBuy)
      mov->DeleteMarketBid(t_order_id_, order->price, order->size);
    else
      mov->DeleteMarketAsk(t_order_id_, order->price, order->size);
    mkt->order_id_map_.erase(t_order_id_);
  } else if (mkt->queued_order_id_map_.find(t_order_id_) != mkt->queued_order_id_map_.end()) {
    order = mkt->queued_order_id_map_[t_order_id_];
    mkt->queued_order_id_map_.erase(t_order_id_);
  } else {
    DBGLOG_TIME_CLASS_FUNC << "ERROR"
                           << " OrderId: " << t_order_id_ << " is not present in order map" << DBGLOG_ENDL_FLUSH;
    if (!t_is_intermediate_) {
      modified_order_id_ = 0;
    }
    return;
  }
  // Dealloc order
  nse_order_mempool_.DeAlloc(order);

  if (!t_is_intermediate_) {
    AddSimulatedOrders(t_security_id_);
    modified_order_id_ = 0;
  }
}

void NSEMarketOrderManager::OnTrade(const unsigned int t_security_id_, double const t_trade_price_,
                                    const int t_trade_size_, const uint64_t t_buy_order_num_,
                                    const uint64_t t_sell_order_num_) {
  MarketOrdersView* mov = market_orders_view_map_[t_security_id_];
  NSEMarketPerSecurity* mkt = nse_markets_[t_security_id_];
  double exec_price = t_trade_price_;
  bool is_bid_hidden_ = false;
  bool is_ask_hidden_ = false;

  int int_trade_price_ = mov->GetIntPx(t_trade_price_);

  bool bid_order_queued_ = false;
  bool ask_order_queued_ = false;

  NSEOrder* bid_order_ = nullptr;
  NSEOrder* ask_order_ = nullptr;

  // Finding the order on buy side
  if (mkt->order_id_map_.find(t_buy_order_num_) != mkt->order_id_map_.end()) {
    bid_order_ = mkt->order_id_map_[t_buy_order_num_];
  } else if (mkt->queued_order_id_map_.find(t_buy_order_num_) != mkt->queued_order_id_map_.end()) {
    bid_order_ = mkt->queued_order_id_map_[t_buy_order_num_];
    bid_order_queued_ = true;
  }

  if (bid_order_ != nullptr) {
    if (bid_order_->buysell_ != kTradeTypeBuy) {
      DBGLOG_TIME_CLASS_FUNC << "ERROR"
                             << " BuyOrderId: " << t_buy_order_num_ << " trade type differ from Buy"
                             << DBGLOG_ENDL_FLUSH;
      ExitVerbose(kExitErrorCodeGeneral, "BuyOrder trade type differ from Buy");
    }
  }

  // Finding the order on sell side
  if (mkt->order_id_map_.find(t_sell_order_num_) != mkt->order_id_map_.end()) {
    ask_order_ = mkt->order_id_map_[t_sell_order_num_];
  } else if (mkt->queued_order_id_map_.find(t_sell_order_num_) != mkt->queued_order_id_map_.end()) {
    ask_order_ = mkt->queued_order_id_map_[t_sell_order_num_];
    ask_order_queued_ = true;
  }

  if (ask_order_ != nullptr) {
    if (ask_order_->buysell_ != kTradeTypeSell) {
      DBGLOG_TIME_CLASS_FUNC << "ERROR"
                             << " SellOrderId: " << t_sell_order_num_ << " trade type differ from Sell"
                             << DBGLOG_ENDL_FLUSH;
      ExitVerbose(kExitErrorCodeGeneral, "SellOrder trade type differ from Sell");
    }
  }

  if ((bid_order_ == nullptr) && ((ask_order_ == nullptr))) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR:"
                           << " BuyOrderId: " << t_buy_order_num_ << " and "
                           << " SellOrderId: " << t_sell_order_num_ << " both not present in order map"
                           << DBGLOG_ENDL_FLUSH;
    modified_order_id_ = 0;
    return;
  }

  if (int_trade_price_ == mov->GetBestBidIntPrice()) {  // Best bid trade

    if (bid_order_ == nullptr) {
      DBGLOG_TIME_CLASS_FUNC << "ERROR:"
                             << "Passive BuyOrderId: " << t_buy_order_num_ << " not present in order map"
                             << DBGLOG_ENDL_FLUSH;
    } else {
      if (bid_order_queued_) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR:"
                               << "Passive BuyOrderId: " << t_buy_order_num_ << " is present in queued order map"
                               << DBGLOG_ENDL_FLUSH;
      }
      if (bid_order_->price != t_trade_price_) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR:"
                               << "Passive BuyOrderId: " << t_buy_order_num_ << " price differ from trade price"
                               << DBGLOG_ENDL_FLUSH;
      }
    }

    if ((modified_order_id_ != 0) && (bid_order_ != nullptr) && (t_buy_order_num_ == modified_order_id_) &&
        (modified_int_price_ == int_trade_price_) && (t_trade_size_ >= modified_prev_size_)) {
      ttime_t time_diff_ = watch_.tv() - modified_time_;

      //      DBGLOG_TIME_CLASS_FUNC << "INFO:" << "ProbableBuyHiddenOrder " << t_buy_order_num_ << " TimeDiff: " <<
      //      time_diff_ << DBGLOG_ENDL_FLUSH;

      if ((time_diff_.tv_sec == 0) && (time_diff_.tv_usec < USECS_BETWEEN_HIDDEN_MODIFY_TRADE)) {
        mov->ExecMarketBidHidden(t_buy_order_num_, exec_price, modified_prev_size_, t_trade_size_);
        is_bid_hidden_ = true;
      } else {
        mov->ExecMarketBid(t_buy_order_num_, exec_price, t_trade_size_, false);
      }
    } else {
      if (bid_order_ != nullptr) {
        mov->ExecMarketBid(t_buy_order_num_, exec_price, t_trade_size_, false);
      }
    }

  } else if (int_trade_price_ == mov->GetBestAskIntPrice()) {  // Best ask trade

    if (ask_order_ == nullptr) {
      DBGLOG_TIME_CLASS_FUNC << "ERROR:"
                             << "Passive AskOrderId: " << t_sell_order_num_ << " not present in order map"
                             << DBGLOG_ENDL_FLUSH;
    } else {
      if (ask_order_queued_) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR:"
                               << "Passive AskOrderId: " << t_sell_order_num_ << " is present in queued order map"
                               << DBGLOG_ENDL_FLUSH;
      }

      if (ask_order_->price != t_trade_price_) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR:"
                               << "Passive AskOrderId: " << t_sell_order_num_ << " price differ from trade price"
                               << DBGLOG_ENDL_FLUSH;
      }
    }

    if ((modified_order_id_ != 0) && (ask_order_ != nullptr) && (t_sell_order_num_ == modified_order_id_) &&
        (modified_int_price_ == int_trade_price_) && (t_trade_size_ >= modified_prev_size_)) {
      ttime_t time_diff_ = watch_.tv() - modified_time_;

      //      DBGLOG_TIME_CLASS_FUNC << "INFO:" << "ProbableAskHiddenOrder " << t_sell_order_num_ << " TimeDiff: " <<
      //      time_diff_ << DBGLOG_ENDL_FLUSH;

      if ((time_diff_.tv_sec == 0) && (time_diff_.tv_usec < USECS_BETWEEN_HIDDEN_MODIFY_TRADE)) {
        mov->ExecMarketAskHidden(t_sell_order_num_, exec_price, modified_prev_size_, t_trade_size_);
        is_ask_hidden_ = true;
      } else {
        mov->ExecMarketAsk(t_sell_order_num_, exec_price, t_trade_size_, false);
      }
    } else {
      if (ask_order_ != nullptr) {
        mov->ExecMarketAsk(t_sell_order_num_, exec_price, t_trade_size_, false);
      }
    }

  } else if (int_trade_price_ < mov->GetBestBidIntPrice()) {  // Sub-best bid trade

    if (bid_order_ == nullptr) {
      DBGLOG_TIME_CLASS_FUNC << "ERROR:"
                             << "Passive BuyOrderId: " << t_buy_order_num_ << " not present in order map"
                             << DBGLOG_ENDL_FLUSH;
    } else {
      if (bid_order_queued_) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR:"
                               << "Passive BuyOrderId: " << t_buy_order_num_ << " is present in queued order map"
                               << DBGLOG_ENDL_FLUSH;
      }
      if (bid_order_->price != t_trade_price_) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR:"
                               << "Passive BuyOrderId: " << t_buy_order_num_ << " price differ from trade price"
                               << DBGLOG_ENDL_FLUSH;
      }
    }

    DBGLOG_TIME_CLASS_FUNC << "Sub best bid trade :"
                           << " Mkt " << mov->GetBidSize(mov->GetBestBidIntPrice()) << " " << mov->GetBestBidPrice()
                           << " X  " << mov->GetBestAskPrice() << " " << mov->GetAskSize(mov->GetBestAskIntPrice())
                           << " while trade price : " << exec_price << DBGLOG_ENDL_FLUSH;

    DeleteOrders(t_security_id_, int_trade_price_ + 1, mov->GetBestBidIntPrice(), kTradeTypeBuy);

    // It might happen while deleting orders we may dealloc the current bid order
    // which may cause all kind of memory corruption issues, so after deleting orders
    // we check whether the current order exists in map, failing which we assign nullptr to it

    if (mkt->order_id_map_.find(t_buy_order_num_) != mkt->order_id_map_.end()) {
      bid_order_ = mkt->order_id_map_[t_buy_order_num_];
    } else if (mkt->queued_order_id_map_.find(t_buy_order_num_) != mkt->queued_order_id_map_.end()) {
      bid_order_ = mkt->queued_order_id_map_[t_buy_order_num_];
      bid_order_queued_ = true;
    } else {
      bid_order_ = nullptr;
    }

    mov->ExecMarketBid(t_buy_order_num_, exec_price, t_trade_size_, false);

  } else if (int_trade_price_ > mov->GetBestAskIntPrice()) {  // Sub-best ask trade

    if (ask_order_ == nullptr) {
      DBGLOG_TIME_CLASS_FUNC << "ERROR:"
                             << "Passive AskOrderId: " << t_sell_order_num_ << " not present in order map"
                             << DBGLOG_ENDL_FLUSH;
    } else {
      if (ask_order_queued_) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR:"
                               << "Passive AskOrderId: " << t_sell_order_num_ << " is present in queued order map"
                               << DBGLOG_ENDL_FLUSH;
      }
      if (ask_order_->price != t_trade_price_) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR:"
                               << "Passive AskOrderId: " << t_sell_order_num_ << " price differ from trade price"
                               << DBGLOG_ENDL_FLUSH;
      }
    }

    DBGLOG_TIME_CLASS_FUNC << "Sub best ask trade :"
                           << " Mkt " << mov->GetBidSize(mov->GetBestBidIntPrice()) << " " << mov->GetBestBidPrice()
                           << " X  " << mov->GetBestAskPrice() << " " << mov->GetAskSize(mov->GetBestAskIntPrice())
                           << " while trade price : " << exec_price << DBGLOG_ENDL_FLUSH;

    DeleteOrders(t_security_id_, mov->GetBestAskIntPrice(), int_trade_price_ - 1, kTradeTypeSell);

    // It might happen while deleting orders we may dealloc the current ask order
    // which may cause all kind of memory corruption issues, so after deleting orders
    // we check whether the current order exists in map, failing which we assign nullptr to it

    if (mkt->order_id_map_.find(t_sell_order_num_) != mkt->order_id_map_.end()) {
      ask_order_ = mkt->order_id_map_[t_sell_order_num_];
    } else if (mkt->queued_order_id_map_.find(t_sell_order_num_) != mkt->queued_order_id_map_.end()) {
      ask_order_ = mkt->queued_order_id_map_[t_sell_order_num_];
      ask_order_queued_ = true;
    } else {
      ask_order_ = nullptr;
    }

    mov->ExecMarketAsk(t_sell_order_num_, exec_price, t_trade_size_, false);

  } else {  // Trade between bid and ask
    if ((!bid_order_queued_) && (bid_order_ != nullptr)) {
      mov->ExecMarketBid(t_buy_order_num_, exec_price, t_trade_size_, false);
    }

    if ((!ask_order_queued_) && (ask_order_ != nullptr)) {
      mov->ExecMarketAsk(t_sell_order_num_, exec_price, t_trade_size_, false);
    }
  }

  if (bid_order_ != nullptr) {
    if (bid_order_->size == (int)t_trade_size_) {
      // Remove/Dealloc order
      if (bid_order_queued_) {
        mkt->queued_order_id_map_.erase(t_buy_order_num_);
      } else {
        // If the trade size is same as order size, order should be deleted
        // In case of hidden order, already deleted
        if (!is_bid_hidden_) {
          mov->DeleteMarketBid(t_buy_order_num_, bid_order_->price, bid_order_->size);
        }
        mkt->order_id_map_.erase(t_buy_order_num_);
      }
      nse_order_mempool_.DeAlloc(bid_order_);
    } else if (bid_order_->size > (int)t_trade_size_) {
      if ((!bid_order_queued_) && (!is_bid_hidden_)) {
        mov->ModifyMarketBid(t_buy_order_num_, bid_order_->price, bid_order_->size, t_buy_order_num_, bid_order_->price,
                             bid_order_->size - t_trade_size_);
      }
      bid_order_->size = bid_order_->size - t_trade_size_;  // Update size
    } else {
      DBGLOG_TIME_CLASS_FUNC << "ERROR: "
                             << " OrderId: " << t_buy_order_num_
                             << " volume deleted is more than order-size, deleting the order" << DBGLOG_ENDL_FLUSH;
      // Remove/Dealloc order
      if (bid_order_queued_) {
        mkt->queued_order_id_map_.erase(t_buy_order_num_);
      } else {
        // If the trade size is same as oder size, order should be deleted
        // In case of hidden order, already deleted
        if (!is_bid_hidden_) {
          mov->DeleteMarketBid(t_buy_order_num_, bid_order_->price, bid_order_->size);
        }
        mkt->order_id_map_.erase(t_buy_order_num_);
      }
      nse_order_mempool_.DeAlloc(bid_order_);
    }
  }

  if (ask_order_ != nullptr) {
    if (ask_order_->size == (int)t_trade_size_) {
      // Remove/Dealloc order
      if (ask_order_queued_) {
        mkt->queued_order_id_map_.erase(t_sell_order_num_);
      } else {
        // If the trade size is same as order size, order should be deleted
        // In case of hidden order, already deleted
        if (!is_ask_hidden_) {
          mov->DeleteMarketAsk(t_sell_order_num_, ask_order_->price, ask_order_->size);
        }
        mkt->order_id_map_.erase(t_sell_order_num_);
      }
      nse_order_mempool_.DeAlloc(ask_order_);
    } else if (ask_order_->size > (int)t_trade_size_) {
      if ((!ask_order_queued_) && (!is_ask_hidden_)) {
        mov->ModifyMarketAsk(t_sell_order_num_, ask_order_->price, ask_order_->size, t_sell_order_num_,
                             ask_order_->price, ask_order_->size - t_trade_size_);
      }
      ask_order_->size = ask_order_->size - t_trade_size_;  // Update size
    } else {
      DBGLOG_TIME_CLASS_FUNC << "ERROR: "
                             << " OrderId: " << t_sell_order_num_
                             << " volume deleted is more than order-size, deleting the order" << DBGLOG_ENDL_FLUSH;
      // Remove/Dealloc order
      if (ask_order_queued_) {
        mkt->queued_order_id_map_.erase(t_sell_order_num_);
      } else {
        // If the trade size is same as order size, order should be deleted
        // In case of hidden order, already deleted
        if (!is_ask_hidden_) {
          mov->DeleteMarketAsk(t_sell_order_num_, ask_order_->price, ask_order_->size);
        }
        mkt->order_id_map_.erase(t_sell_order_num_);
      }
      nse_order_mempool_.DeAlloc(ask_order_);
    }
  }

  modified_order_id_ = 0;
  AddSimulatedOrders(t_security_id_);
}

void NSEMarketOrderManager::OnHiddenTrade(const unsigned int t_security_id_, double const t_trade_price_,
                                          const int t_trade_size_, const uint64_t t_buy_order_num_,
                                          const uint64_t t_sell_order_num_) {
  OnTrade(t_security_id_, t_trade_price_, t_trade_size_, t_buy_order_num_, t_sell_order_num_);
}

void NSEMarketOrderManager::AddSimulatedOrders(const unsigned int t_security_id_) {
  NSEMarketPerSecurity* mkt = nse_markets_[t_security_id_];
  MarketOrdersView* mov = market_orders_view_map_[t_security_id_];
  for (auto iter = mkt->queued_order_id_map_.begin(); iter != mkt->queued_order_id_map_.end();) {
    const uint64_t order_id_ = iter->first;
    NSEOrder* order = iter->second;
    int int_price_ = mov->GetIntPx(order->price);
    if (((order->buysell_ == kTradeTypeBuy) && (int_price_ < mov->GetBestAskIntPrice())) ||
        ((order->buysell_ == kTradeTypeSell) && (int_price_ > mov->GetBestBidIntPrice()))) {
      mkt->queued_order_id_map_.erase(iter++);
      OnOrderAdd(t_security_id_, order->buysell_, order_id_, order->price, order->size, true);
      nse_order_mempool_.DeAlloc(order);
    } else {
      ++iter;
    }
  }
}

// Deletes Orders from start price to end price in both maps
void NSEMarketOrderManager::DeleteOrders(const unsigned int t_security_id_, int t_start_int_price_,
                                         int t_end_int_price_, TradeType_t t_buysell_) {
  NSEMarketPerSecurity* mkt = nse_markets_[t_security_id_];
  MarketOrdersView* mov = market_orders_view_map_[t_security_id_];

  // Remove from live orders map
  for (auto t_int_price_ = t_start_int_price_; t_int_price_ <= t_end_int_price_; t_int_price_++) {
    std::vector<ExchMarketOrder*> mkt_order_vec_;
    if (t_buysell_ == kTradeTypeBuy) {
      mkt_order_vec_ = mov->GetBidOrders(t_int_price_);
    } else {
      mkt_order_vec_ = mov->GetAskOrders(t_int_price_);
    }

    for (auto iter = mkt_order_vec_.begin(); iter != mkt_order_vec_.end();) {
      int64_t t_order_id_ = (*iter)->order_id_;

      if (mkt->order_id_map_.find(t_order_id_) == mkt->order_id_map_.end()) {
        // If our logic reaches here, there is definitely some issue in market order manager.
        DBGLOG_TIME_CLASS_FUNC << "WEIRD ERROR"
                               << " OrderId: " << t_order_id_ << " not present in map while present in market book"
                               << DBGLOG_ENDL_FLUSH;
        auto t_iter = iter;
        iter++;
        if (t_buysell_ == kTradeTypeBuy)
          mov->DeleteMarketBid(t_order_id_, (*t_iter)->price_, (*t_iter)->size_);
        else
          mov->DeleteMarketAsk(t_order_id_, (*t_iter)->price_, (*t_iter)->size_);
        ExitVerbose(kExitErrorCodeGeneral, "Inconsistency in market book and market order manager");
      }

      NSEOrder* order = mkt->order_id_map_[t_order_id_];
      iter++;
      if (order->buysell_ == kTradeTypeBuy)
        mov->DeleteMarketBid(t_order_id_, order->price, order->size);
      else
        mov->DeleteMarketAsk(t_order_id_, order->price, order->size);
      mkt->order_id_map_.erase(t_order_id_);
      nse_order_mempool_.DeAlloc(order);
    }
  }

  // Remove from queued maps
  for (auto iter = mkt->queued_order_id_map_.begin(); iter != mkt->queued_order_id_map_.end();) {
    NSEOrder* order = iter->second;
    int int_price_ = mov->GetIntPx(order->price);
    if ((order->buysell_ == t_buysell_) && (((t_buysell_ == kTradeTypeBuy) && (int_price_ >= t_start_int_price_)) ||
                                            ((t_buysell_ == kTradeTypeSell) && (int_price_ <= t_end_int_price_)))) {
      mkt->queued_order_id_map_.erase(iter++);
      nse_order_mempool_.DeAlloc(order);
    } else {
      ++iter;
    }
  }
}

void NSEMarketOrderManager::ResetBook(const unsigned int security_id) {
  MarketOrdersView* mov = market_orders_view_map_[security_id];
  NSEMarketPerSecurity* mkt = nse_markets_[security_id];

  for (auto order : mkt->order_id_map_) {
    nse_order_mempool_.DeAlloc(order.second);
  }

  for (auto order : mkt->queued_order_id_map_) {
    nse_order_mempool_.DeAlloc(order.second);
  }

  mkt->Clear();
  mov->Reset();
}
}
