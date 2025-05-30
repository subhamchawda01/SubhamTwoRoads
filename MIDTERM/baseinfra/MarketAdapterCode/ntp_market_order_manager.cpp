/**
   \file MarketAdapterCode/ntp_market_order_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/MarketAdapter/ntp_market_order_manager.hpp"

namespace HFSAT {

NtpMarketOrderManager::NtpMarketOrderManager(DebugLogger &t_dbglogger_, const Watch &t_watch_,
                                             SecurityNameIndexer &t_sec_name_indexer_,
                                             const std::vector<MarketOrdersView *> &t_market_orders_view_map_)
    : BaseMarketOrderManager(t_dbglogger_, t_watch_, t_sec_name_indexer_, t_market_orders_view_map_),
      bmf_order_mempool_(),
      bmf_markets_(t_sec_name_indexer_.NumSecurityId(), new BmfMarketPerSecurity()) {}

/**
 *
 * @param t_security_id_
 * @param t_buysell_
 * @param t_order_id_
 * @param t_price_
 * @param t_size_
 */
void NtpMarketOrderManager::OnOrderAdd(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                       const int64_t t_order_id_, const double t_price_, const uint32_t t_size_) {
  MarketOrdersView *mov_ = market_orders_view_map_[t_security_id_];
  BmfMarketPerSecurity *mkt_ = bmf_markets_[t_security_id_];

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      if (mkt_->order_id_bids_map_.find(t_order_id_) != mkt_->order_id_bids_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR"
                               << " OrderId: " << t_order_id_ << " already present in map" << DBGLOG_ENDL_FLUSH;
      }

      BmfOrder *order_ = bmf_order_mempool_.Alloc();
      order_->price_ = t_price_;
      order_->size_ = t_size_;
      mkt_->order_id_bids_map_[t_order_id_] = order_;

      mov_->AddMarketBidBack(t_order_id_, t_price_, t_size_);
    } break;
    case kTradeTypeSell: {
      if (mkt_->order_id_asks_map_.find(t_order_id_) != mkt_->order_id_asks_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR"
                               << " OrderId: " << t_order_id_ << " already present in map" << DBGLOG_ENDL_FLUSH;
      }

      BmfOrder *order_ = bmf_order_mempool_.Alloc();
      order_->price_ = t_price_;
      order_->size_ = t_size_;
      mkt_->order_id_asks_map_[t_order_id_] = order_;

      mov_->AddMarketAskBack(t_order_id_, t_price_, t_size_);
    } break;
    default:
      break;
  }
}

/**
 *
 * @param t_security_id_
 * @param t_buysell_
 * @param t_order_id_
 * @param t_price_
 * @param t_size_
 */
void NtpMarketOrderManager::OnOrderModify(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                          const int64_t t_order_id_, const double t_price_, const uint32_t t_size_) {
  MarketOrdersView *mov_ = market_orders_view_map_[t_security_id_];
  BmfMarketPerSecurity *mkt_ = bmf_markets_[t_security_id_];

  // Since we don't know the previous size/price, fetch the
  // previous size/price

  // Ideally, we don't expect the price to change in OrderModify

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      if (mkt_->order_id_bids_map_.find(t_order_id_) == mkt_->order_id_bids_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR"
                               << " OrderId: " << t_order_id_ << " not present in map" << DBGLOG_ENDL_FLUSH;

        return;
      }

      BmfOrder *order_ = mkt_->order_id_bids_map_[t_order_id_];
      double prev_price_ = order_->price_;
      int prev_size_ = order_->size_;

      // Assign new price/size to the order
      order_->price_ = t_price_;
      order_->size_ = t_size_;

      mov_->ModifyMarketBid(t_order_id_, prev_price_, prev_size_, t_order_id_, t_price_, t_size_);
    } break;
    case kTradeTypeSell: {
      if (mkt_->order_id_asks_map_.find(t_order_id_) == mkt_->order_id_asks_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR"
                               << " OrderId: " << t_order_id_ << " not present in map" << DBGLOG_ENDL_FLUSH;

        return;
      }

      BmfOrder *order_ = mkt_->order_id_asks_map_[t_order_id_];
      double prev_price_ = order_->price_;
      int prev_size_ = order_->size_;

      // Assign new price/size to the order
      order_->price_ = t_price_;
      order_->size_ = t_size_;

      mov_->ModifyMarketAsk(t_order_id_, prev_price_, prev_size_, t_order_id_, t_price_, t_size_);
    } break;
    default:
      break;
  }
}

/**
 *
 * @param t_security_id_
 * @param t_buysell_
 * @param t_order_id_
 */
void NtpMarketOrderManager::OnOrderDelete(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                          const int64_t t_order_id_) {
  MarketOrdersView *mov_ = market_orders_view_map_[t_security_id_];
  BmfMarketPerSecurity *mkt_ = bmf_markets_[t_security_id_];

  // We don't get price/size information in order delete message,
  // So, fetch that information first.

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      if (mkt_->order_id_bids_map_.find(t_order_id_) == mkt_->order_id_bids_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR"
                               << " OrderId: " << t_order_id_ << " not present in map" << DBGLOG_ENDL_FLUSH;

        return;
      }

      BmfOrder *order_ = mkt_->order_id_bids_map_[t_order_id_];
      double price_ = order_->price_;
      int size_ = order_->size_;

      // Remove from the map and dealloc the order
      mkt_->order_id_bids_map_.erase(t_order_id_);
      bmf_order_mempool_.DeAlloc(order_);

      mov_->DeleteMarketBid(t_order_id_, price_, size_);
    } break;
    case kTradeTypeSell: {
      if (mkt_->order_id_asks_map_.find(t_order_id_) == mkt_->order_id_asks_map_.end()) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR"
                               << " OrderId: " << t_order_id_ << " not present in map" << DBGLOG_ENDL_FLUSH;

        return;
      }

      BmfOrder *order_ = mkt_->order_id_asks_map_[t_order_id_];
      double price_ = order_->price_;
      int size_ = order_->size_;

      // Remove from the map and dealloc the order
      mkt_->order_id_asks_map_.erase(t_order_id_);
      bmf_order_mempool_.DeAlloc(order_);

      mov_->DeleteMarketAsk(t_order_id_, price_, size_);
    } break;
    default:
      break;
  }
}

/**
 *
 * @param t_security_id_
 * @param t_buysell_
 * @param t_order_id_
 * @param t_traded_price_
 * @param t_traded_size_
 */
void NtpMarketOrderManager::OnOrderExec(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                        const int64_t t_order_id_, const double t_traded_price_,
                                        const uint32_t t_traded_size_) {
  MarketOrdersView *mov_ = market_orders_view_map_[t_security_id_];
  // BmfMarketPerSecurity * mkt_ = bmf_markets_[ t_security_id_ ];

  // Since order delete/modify messages are explicit for exec,
  // do not delete/modify order here

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      mov_->ExecMarketBid(t_order_id_, t_traded_price_, t_traded_size_, false);
    } break;
    case kTradeTypeSell: {
      mov_->ExecMarketAsk(t_order_id_, t_traded_price_, t_traded_size_, false);
    } break;
    default:
      break;
  }
}

void NtpMarketOrderManager::ResetBook(const unsigned int t_security_id_) {
  MarketOrdersView *mov_ = market_orders_view_map_[t_security_id_];
  BmfMarketPerSecurity *mkt_ = bmf_markets_[t_security_id_];
  for (auto it_ = mkt_->order_id_bids_map_.begin(); it_ != mkt_->order_id_bids_map_.end(); it_++) {
    bmf_order_mempool_.DeAlloc(it_->second);
  }

  for (auto it_ = mkt_->order_id_asks_map_.begin(); it_ != mkt_->order_id_asks_map_.end(); it_++) {
    bmf_order_mempool_.DeAlloc(it_->second);
  }

  mkt_->Clear();
  mov_->Reset();
}
}
