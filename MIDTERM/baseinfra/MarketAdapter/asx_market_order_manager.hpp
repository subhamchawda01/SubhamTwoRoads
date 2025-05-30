/**
   \file MarketAdapter/asx_market_order_manager.hpp

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
#include "baseinfra/MarketAdapter/asx_market_per_security.hpp"

namespace HFSAT {

class AsxMarketOrderManager : public BaseMarketOrderManager, public OrderLevelListener {
 protected:
  SimpleMempool<AsxOrder> asx_order_mempool_;
  std::vector<AsxMarketPerSecurity*> asx_markets_;
  bool is_ose_itch_;

 public:
  AsxMarketOrderManager(DebugLogger& dbglogger, const Watch& watch, SecurityNameIndexer& sec_name_indexer,
                        const std::vector<MarketOrdersView*>& mov_map, bool is_ose_itch = false);

  ~AsxMarketOrderManager() {}

  void SetTimeToSkipUntilFirstEvent(const ttime_t start_time) {}

  void OnOrderAdd(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                  const uint32_t priority, const double price, const uint32_t size);

  void OnOrderModify(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                     const uint32_t priority, const double price, const uint32_t size);

  void OnVolumeDelete(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                      const uint32_t size);

  void OnOrderDelete(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id);

  void OnOrderExec(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                   const double exec_price, const uint32_t size_exec, const uint32_t size_remaining);

  void OnOrderExecWithPx(const uint32_t security_id, const uint64_t bid_order_id, const uint64_t ask_order_id,
                         const double exec_price, const uint32_t size_exec, const uint32_t bid_remaining,
                         const uint32_t ask_remaining);

  // Used for ASX ITCH since the feed changed from march 2017
  void OnOrderExecWithPxU(const uint32_t security_id, const uint64_t order_id, const uint64_t opposite_order_id,
                          const char side, const double exec_price, const uint32_t size_exec,
                          const uint32_t size_remaining);

  void OnOrderSpExec(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                     const double exec_price, const uint32_t size_exec, const uint32_t size_remaining);

  void ResetBook(const unsigned int security_id);
  void OnResetBegin(const unsigned int security_id) override { ResetBook(security_id); }

  void OnResetEnd(const unsigned int security_id) override {}
};
}
