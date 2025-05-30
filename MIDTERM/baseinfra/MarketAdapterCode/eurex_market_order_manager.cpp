/**
   \file MarketAdapterCode/eurex_market_order_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/MarketAdapter/eurex_market_order_manager.hpp"

namespace HFSAT {

EurexMarketOrderManager::EurexMarketOrderManager(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                                                 SecurityNameIndexer& t_sec_name_indexer_,
                                                 const std::vector<MarketOrdersView*>& t_market_orders_view_map_)
    : BaseMarketOrderManager(t_dbglogger_, t_watch_, t_sec_name_indexer_, t_market_orders_view_map_) {}

void EurexMarketOrderManager::OnOrderAdd(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                         const int64_t t_order_id_, const double t_price_, const uint32_t t_size_) {
  MarketOrdersView* mov_ = market_orders_view_map_[t_security_id_];

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      mov_->AddMarketBidBack(t_order_id_, t_price_, t_size_);
    } break;
    case kTradeTypeSell: {
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
 * @param t_new_order_id_
 * @param t_price_
 * @param t_size_
 * @param t_prev_order_id_
 * @param t_prev_price_
 * @param t_prev_size_
 */
void EurexMarketOrderManager::OnOrderModify(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                            const int64_t t_new_order_id_, const double t_price_,
                                            const uint32_t t_size_, const int64_t t_prev_order_id_,
                                            const double t_prev_price_, const uint32_t t_prev_size_) {
  MarketOrdersView* mov_ = market_orders_view_map_[t_security_id_];

  // If the order gets changed, simply delete and add the order
  // instead of leaving this decision to market_orders_view
  if (t_new_order_id_ != t_prev_order_id_) {
    OnOrderDelete(t_security_id_, t_buysell_, t_prev_order_id_, t_prev_price_, t_prev_size_);

    OnOrderAdd(t_security_id_, t_buysell_, t_new_order_id_, t_price_, t_size_);
  } else {
    switch (t_buysell_) {
      case kTradeTypeBuy: {
        mov_->ModifyMarketBid(t_prev_order_id_, t_prev_price_, t_prev_size_, t_new_order_id_, t_price_, t_size_);
      } break;
      case kTradeTypeSell: {
        mov_->ModifyMarketAsk(t_prev_order_id_, t_prev_price_, t_prev_size_, t_new_order_id_, t_price_, t_size_);
      } break;
      default:
        break;
    }
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
void EurexMarketOrderManager::OnOrderDelete(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                            const int64_t t_order_id_, const double t_price_, const int t_size_) {
  MarketOrdersView* mov_ = market_orders_view_map_[t_security_id_];

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      mov_->DeleteMarketBid(t_order_id_, t_price_, t_size_);
    } break;
    case kTradeTypeSell: {
      mov_->DeleteMarketAsk(t_order_id_, t_price_, t_size_);
    } break;
    default:
      break;
  }
}

void EurexMarketOrderManager::OnOrderMassDelete(const uint32_t t_security_id_) {
  MarketOrdersView* mov_ = market_orders_view_map_[t_security_id_];

  // Delete everything
  mov_->Reset();
}

/**
 *
 * @param t_security_id_
 * @param t_buysell_
 * @param t_order_id_
 * @param t_traded_price_
 * @param t_traded_size_
 */
void EurexMarketOrderManager::OnPartialOrderExecution(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                                      const int64_t t_order_id_, const double t_traded_price_,
                                                      const uint32_t t_traded_size_) {
  MarketOrdersView* mov_ = market_orders_view_map_[t_security_id_];

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      int size_remaining_ = mov_->ExecMarketBid(t_order_id_, t_traded_price_, t_traded_size_, false);

      // Check if the size_remaining is 0
      if (size_remaining_ <= 0) {
        //              DBGLOG_TIME_CLASS_FUNC << "size_remaining "
        //                  << size_remaining_
        //                  << " for partial bid exec case"
        //                  << DBGLOG_ENDL_FLUSH;
        mov_->DeleteMarketBid(t_order_id_, t_traded_price_, t_traded_size_);
      } else {
        mov_->ModifyMarketBid(t_order_id_, t_traded_price_, size_remaining_ + t_traded_size_, t_order_id_,
                              t_traded_price_, size_remaining_);
      }
    } break;
    case kTradeTypeSell: {
      int size_remaining_ = mov_->ExecMarketAsk(t_order_id_, t_traded_price_, t_traded_size_, false);

      // Check if the size_remaining is 0
      if (size_remaining_ <= 0) {
        //              DBGLOG_TIME_CLASS_FUNC << "size_remaining "
        //                  << size_remaining_
        //                  << " for partial ask exec case"
        //                  << DBGLOG_ENDL_FLUSH;
        mov_->DeleteMarketAsk(t_order_id_, t_traded_price_, t_traded_size_);
      } else {
        mov_->ModifyMarketAsk(t_order_id_, t_traded_price_, size_remaining_ + t_traded_size_, t_order_id_,
                              t_traded_price_, size_remaining_);
      }
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
void EurexMarketOrderManager::OnFullOrderExecution(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                                   const int64_t t_order_id_, const double t_traded_price_,
                                                   const uint32_t t_traded_size_) {
  MarketOrdersView* mov_ = market_orders_view_map_[t_security_id_];

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      mov_->ExecMarketBid(t_order_id_, t_traded_price_, t_traded_size_, false);

      mov_->DeleteMarketBid(t_order_id_, t_traded_price_, t_traded_size_);
    } break;
    case kTradeTypeSell: {
      mov_->ExecMarketAsk(t_order_id_, t_traded_price_, t_traded_size_, false);

      mov_->DeleteMarketAsk(t_order_id_, t_traded_price_, t_traded_size_);
    } break;
    default:
      break;
  }
}

// This is not really needed here and hence not
// implementing this for now.
void EurexMarketOrderManager::OnExecutionSummary(const uint32_t t_security_id_, TradeType_t t_aggressor_side_,
                                                 const double t_price_, const uint32_t t_size_) {}
}
