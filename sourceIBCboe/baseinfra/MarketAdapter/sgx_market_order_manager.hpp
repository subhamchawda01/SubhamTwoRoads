#pragma once

#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/MarketAdapter/base_market_order_manager.hpp"
#include "baseinfra/MarketAdapter/sgx_market_per_security.hpp"

namespace HFSAT {

class SGXMarketOrderManager : public BaseMarketOrderManager, public OrderLevelListener {
 public:
  SGXMarketOrderManager(DebugLogger& dbglogger, const Watch& watch, SecurityNameIndexer& sec_name_indexer,
                        const std::vector<MarketOrdersView*>& mov_map);

  ~SGXMarketOrderManager() {}

  int previous_delete_position;
  uint64_t previous_delete_order_id;
  uint32_t previous_delete_size;

  void SetTimeToSkipUntilFirstEvent(const ttime_t start_time) {}

  void OnOrderAdd(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                  const uint32_t priority, const double price, const uint32_t size);

  void OnOrderExec(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                   const double exec_price, const uint32_t size_exec, const uint32_t size_remaining);

  void OnOrderDelete(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id);
  void OnVolumeDelete(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                      const uint32_t size_remaining);
  void OnOrderModify(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                     const uint32_t priority, const double price, const uint32_t size);
  void OnResetBegin(const unsigned int security_id) override { ResetBook(security_id); }
  void ResetBook(const unsigned int security_id);
  // Empty functions that need to be defined due to virtual function definition in OrderLevelListener
  void OnResetEnd(const unsigned int security_id) {}
  void OnOrderSpExec(const uint32_t security_id, const TradeType_t buysell, const uint64_t order_id,
                     const double exec_price, const uint32_t size_exec, const uint32_t size_remaining) {}
  void OnOrderExecWithPx(const uint32_t security_id, const uint64_t bid_order_id, const uint64_t ask_order_id,
                         const double exec_price, const uint32_t size_exec, const uint32_t bid_remaining,
                         const uint32_t ask_remaining) {}
  void OnOrderExecWithPxU(const uint32_t security_id, const uint64_t order_id, const uint64_t opposite_order_id,
                          const char side, const double exec_price, const uint32_t size_exec,
                          const uint32_t size_remaining) {}

 protected:
  // assigning memory for the SGXOrder
  SimpleMempool<SGXOrder> sgx_order_mempool_;

  // A vector that has map of bid and ask order for a given security, the security is indexed via the security id
  std::vector<SGXMarketPerSecurity*> sgx_markets_;
};
}
