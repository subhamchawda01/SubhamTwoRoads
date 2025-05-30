/**
   \file MarketAdapter/eobi_market_order_manager.hpp

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
#include "baseinfra/MarketAdapter/eobi_market_per_security.hpp"

namespace HFSAT {

class EOBIMarketOrderManager : public BaseMarketOrderManager, public OrderLevelListener {
 protected:
  SimpleMempool<EOBIOrder> eobi_order_mempool_;
  std::vector<EOBIMarketPerSecurity*> eobi_markets_;

 public:
  EOBIMarketOrderManager(DebugLogger& dbglogger, const Watch& watch, SecurityNameIndexer& sec_name_indexer,
                         const std::vector<MarketOrdersView*>& mov_map);

  ~EOBIMarketOrderManager() {}

  void SetTimeToSkipUntilFirstEvent(const ttime_t start_time) {}

  void OnOrderAdd(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                  const uint32_t priority, const double price, const uint32_t size);

  void OnOrderModify(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                     const uint32_t priority, const double price, const uint32_t size) {}

  void OnVolumeDelete(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                      const uint32_t size);

  void OnOrderDelete(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id);

  void OnOrderExec(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                   const double exec_price, const uint32_t size_exec, const uint32_t size_remaining);

  void OnOrderExecWithPx(const uint32_t security_id, const uint64_t bid_order_id, const uint64_t ask_order_id,
                         const double exec_price, const uint32_t size_exec, const uint32_t bid_remaining,
                         const uint32_t ask_remaining) {}

  void OnOrderSpExec(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                     const double exec_price, const uint32_t size_exec, const uint32_t size_remaining) {}

  void ResetBook(const unsigned int security_id);

  void OnResetBegin(const unsigned int security_id){};

  void OnResetEnd(const unsigned int security_id){};
  void OnOrderModifyWithPrevOrderId(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                    const int64_t t_new_order_id_, const double t_price_, const uint32_t t_size_,
                                    const int64_t t_prev_order_id_, const double t_prev_price_,
                                    const uint32_t t_prev_size_);
};
}
