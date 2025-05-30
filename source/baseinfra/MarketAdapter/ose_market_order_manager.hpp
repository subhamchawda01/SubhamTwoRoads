/**
   \file MarketAdapter/ose_market_order_manager.hpp

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

namespace HFSAT {

class OseMarketOrderManager : public BaseMarketOrderManager, public OrderLevelListenerSim {
 protected:
 public:
  OseMarketOrderManager(DebugLogger &t_dbglogger_, const Watch &t_watch_, SecurityNameIndexer &t_sec_name_indexer_,
                        const std::vector<MarketOrdersView *> &t_market_orders_view_map_);

  ~OseMarketOrderManager() {}

  void SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_) {}

  void OnOrderAdd(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_,
                  const int64_t t_order_id_, const double t_price_, const int t_size_) {
    OnOrderAdd(t_security_id_, t_buysell_, t_order_id_, t_price_, t_size_);
  }

  void OnOrderModify(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_,
                     const int64_t t_old_order_id_, const double t_old_price_, const int t_old_size_,
                     const int64_t t_new_order_id_, const double t_new_price_, const int t_new_size_) {
    OnOrderModify(t_security_id_, t_buysell_, t_old_order_id_, t_old_price_, t_old_size_);
  }

  void OnOrderDelete(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_,
                     const int64_t t_order_id_, const double t_price_, const int t_size_) {
    OnOrderDelete(t_security_id_, t_buysell_, t_order_id_, t_price_, t_size_);
  }

  void OnOrderExec(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_,
                   const int64_t t_order_id_, const double t_price_, const int t_size_executed_,
                   const int t_size_remaining_) {
    OnOrderExec(t_security_id_, t_buysell_, t_order_id_, t_price_, t_size_executed_);
  }

  void OnTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_,
               const TradeType_t t_buysell_) {
    OnTrade(t_security_id_, t_buysell_, t_trade_price_, t_trade_size_);
  }

  void ResetBook(const unsigned int t_security_id_) {}

  // TODO: get rid of the above functions
  // OSE specific

  void OnOrderAdd(const uint32_t t_security_id_, const TradeType_t t_buysell_, const int64_t t_order_id_,
                  const double t_price_, const uint32_t t_size_);

  void OnOrderModify(const uint32_t t_security_id_, const TradeType_t t_buysell_, const int64_t t_order_id_,
                     const double t_price_, const int t_size_diff_);

  void OnOrderDelete(const uint32_t t_security_id_, const TradeType_t t_buysell_, const int64_t t_order_id_,
                     const double t_price_, const int t_size_diff_);

  void OnOrderExec(const uint32_t t_security_id_, const TradeType_t t_buysell_, const int64_t t_order_id_,
                   const double t_traded_price_, const uint32_t t_traded_size_);

  void OnTrade(const uint32_t t_security_id_, TradeType_t t_aggressor_side_, const double t_price_,
               const uint32_t t_size_);
};
}
