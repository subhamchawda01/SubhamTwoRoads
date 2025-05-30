/**
   \file MarketAdapterCode/ose_market_order_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/MarketAdapter/ose_market_order_manager.hpp"

namespace HFSAT {

OseMarketOrderManager::OseMarketOrderManager(DebugLogger &t_dbglogger_, const Watch &t_watch_,
                                             SecurityNameIndexer &t_sec_name_indexer_,
                                             const std::vector<MarketOrdersView *> &t_market_orders_view_map_)
    : BaseMarketOrderManager(t_dbglogger_, t_watch_, t_sec_name_indexer_, t_market_orders_view_map_) {}

void OseMarketOrderManager::OnOrderAdd(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                       const int64_t t_order_id_, const double t_price_, const uint32_t t_size_) {
  MarketOrdersView *mov_ = market_orders_view_map_[t_security_id_];

  // Skip the weird priced orders - it won't make any difference on
  // our use cases if we ignore them and ignoring them makes the
  // tasks easier in market_orders_view
  if (mov_->GetIntPx(t_price_) == -214748365) {
    return;
  }

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

void OseMarketOrderManager::OnOrderModify(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                          const int64_t t_order_id_, const double t_price_, const int t_size_diff_) {
  MarketOrdersView *mov_ = market_orders_view_map_[t_security_id_];

  // Skip the weird priced orders - it won't make any difference on
  // our use cases if we ignore them and ignoring them makes the
  // tasks easier in market_orders_view
  if (mov_->GetIntPx(t_price_) == -214748365) {
    return;
  }

  // In OSE, price remains same in OrderModify, only the size changes
  // And also, the size only gets reduced due to partial exec

  if (t_size_diff_ >= 0) {
    DBGLOG_TIME_CLASS_FUNC << "ERROR. size_diff >= 0 in modify."
                           << " diff: " << t_size_diff_ << DBGLOG_ENDL_FLUSH;
  }

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      int current_size_ = mov_->GetBidSize(t_order_id_, t_price_);

      if (current_size_ <= 0 || (current_size_ + t_size_diff_ <= 0)) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR. B curr_size <= 0 modify."
                               << " curr_size: " << current_size_ << " size_diff: " << t_size_diff_
                               << " price: " << t_price_ << " OrderId: " << t_order_id_ << DBGLOG_ENDL_FLUSH;

        if (current_size_ > 0) {
          mov_->DeleteMarketBid(t_order_id_, t_price_, current_size_);
        }

        break;
      }

      mov_->ModifyMarketBid(t_order_id_, t_price_, current_size_, t_order_id_, t_price_, current_size_ + t_size_diff_);
    } break;
    case kTradeTypeSell: {
      int current_size_ = mov_->GetAskSize(t_order_id_, t_price_);

      if (current_size_ <= 0 || (current_size_ + t_size_diff_ <= 0)) {
        DBGLOG_TIME_CLASS_FUNC << "ERROR. S curr_size <= 0 modify."
                               << " curr_size: " << current_size_ << " size_diff: " << t_size_diff_
                               << " price: " << t_price_ << " OrderId: " << t_order_id_ << DBGLOG_ENDL_FLUSH;

        if (current_size_ > 0) {
          mov_->DeleteMarketAsk(t_order_id_, t_price_, current_size_);
        }

        break;
      }

      mov_->ModifyMarketAsk(t_order_id_, t_price_, current_size_, t_order_id_, t_price_, current_size_ + t_size_diff_);
    } break;
    default:
      break;
  }
}

void OseMarketOrderManager::OnOrderDelete(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                          const int64_t t_order_id_, const double t_price_, const int t_size_diff_) {
  MarketOrdersView *mov_ = market_orders_view_map_[t_security_id_];

  // Skip the weird priced orders - it won't make any difference on
  // our use cases if we ignore them and ignoring them makes the
  // tasks easier in market_orders_view
  if (mov_->GetIntPx(t_price_) == -214748365) {
    return;
  }

  // Since we get size_diff in OSE, the sign is negative for order delete
  int size_ = abs(t_size_diff_);

  switch (t_buysell_) {
    case kTradeTypeBuy: {
      mov_->DeleteMarketBid(t_order_id_, t_price_, size_);
    } break;
    case kTradeTypeSell: {
      mov_->DeleteMarketAsk(t_order_id_, t_price_, size_);
    } break;
    default:
      break;
  }
}

void OseMarketOrderManager::OnOrderExec(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                        const int64_t t_order_id_, const double t_traded_price_,
                                        const uint32_t t_traded_size_) {
  MarketOrdersView *mov_ = market_orders_view_map_[t_security_id_];

  // Skip the weird priced orders - it won't make any difference on
  // our use cases if we ignore them and ignoring them makes the
  // tasks easier in market_orders_view
  if (mov_->GetIntPx(t_traded_price_) == -214748365) {
    return;
  }

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

void OseMarketOrderManager::OnTrade(const uint32_t t_security_id_, TradeType_t t_aggressor_side_, const double t_price_,
                                    const uint32_t t_size_) {}
}
