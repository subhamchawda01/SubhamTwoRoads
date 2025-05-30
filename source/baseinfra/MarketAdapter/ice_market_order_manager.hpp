/**
   \file MarketAdapter/ice_market_order_manager.hpp

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
#include "baseinfra/MarketAdapter/ice_market_per_security.hpp"

namespace HFSAT {

class IceMarketOrderManager : public BaseMarketOrderManager, public OrderLevelListenerICE {
 protected:
  std::vector<IceMarketPerSecurity *> ice_markets_;
  bool first_trade_message_;
  bool is_last_message_trade_message;
  int trade_sum_aggregate_;
  double last_trade_price_;
  unsigned int last_traded_sec_id_;
  TradeType_t last_buysell_;
  std::map<int64_t, uint32_t> order_execution_map_;
  std::map<int64_t, int> order_size_map_;
  SecurityNameIndexer &sec_name_indexer_;

 public:
  IceMarketOrderManager(DebugLogger &t_dbglogger_, const Watch &t_watch_, SecurityNameIndexer &t_sec_name_indexer_,
                        const std::vector<MarketOrdersView *> &t_market_orders_view_map_);

  ~IceMarketOrderManager() {}

  void ResetBook(const unsigned int t_security_id_);

  // ICE specific

  void OnOrderAdd(const uint32_t t_security_id_, const TradeType_t t_buysell_, const int64_t t_order_id_,
                  const double t_price_, const uint32_t t_size_, const int64_t priority_);

  void OnOrderModify(const uint32_t t_security_id_, const TradeType_t t_buysell_, const int64_t t_order_id_,
                     const double t_price_, const uint32_t t_size_);

  void OnOrderDelete(const uint32_t t_security_id_, const int64_t t_order_id_);

  void OnOrderExec(const uint32_t t_security_id_, const TradeType_t t_buysell_, const int64_t t_order_id_,
                   const double t_traded_price_, const uint32_t t_traded_size_);
  void ResetTradeVariable();
  void NotifyAggregateTrade(const int sum_aggregate_trade_size_, const double t_traded_price_,
                            const uint32_t t_security_id_, TradeType_t last_buysell_);
  void UpdateMov(const double t_traded_price_, const uint32_t t_security_id_, TradeType_t last_buysell_);
};
}
