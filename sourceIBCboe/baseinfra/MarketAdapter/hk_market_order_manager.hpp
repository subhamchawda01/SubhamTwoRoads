/**
   \file MarketAdapter/hk_market_order_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/MarketAdapter/base_market_order_manager.hpp"
#include "baseinfra/MarketAdapter/hk_market_per_security.hpp"

namespace HFSAT {

class HKMarketOrderManager : public BaseMarketOrderManager, public OrderLevelListenerHK {
 protected:
  SimpleMempool<HKOrder> hk_order_mempool_;
  std::vector<HKMarketPerSecurity *> hk_markets_;

 public:
  HKMarketOrderManager(DebugLogger &t_dbglogger_, const Watch &t_watch_, SecurityNameIndexer &t_sec_name_indexer_,
                       const std::vector<MarketOrdersView *> &t_market_orders_view_map_);

  ~HKMarketOrderManager() {}

  void ResetBook(const unsigned int t_security_id_);

  // HK-OMD specific

  void OnOrderAdd(const uint32_t t_security_id_, const TradeType_t t_buysell_, const int64_t t_order_id_,
                  const double t_price_, const uint32_t t_size_);

  void OnOrderModify(const uint32_t t_security_id_, const TradeType_t t_buysell_, const int64_t t_order_id_,
                     const double t_price_, const uint32_t t_size_);

  void OnOrderDelete(const uint32_t t_security_id_, const TradeType_t t_buysell_, const int64_t t_order_id_);

  void OnOrderExec(const uint32_t t_security_id_, const TradeType_t t_buysell_, const int64_t t_order_id_,
                   const double t_traded_price_, const uint32_t t_traded_size_);
};
}
