/**
   \file MarketAdapter/nse_market_order_manager.hpp

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
#include "baseinfra/MarketAdapter/nse_market_per_security.hpp"
#include "dvccode/CDef/error_utils.hpp"

#define USECS_BETWEEN_HIDDEN_MODIFY_TRADE 100

namespace HFSAT {

class NSEMarketOrderManager : public BaseMarketOrderManager, public OrderGlobalListenerNSE {
 protected:
  SimpleMempool<NSEOrder> nse_order_mempool_;
  std::vector<NSEMarketPerSecurity*> nse_markets_;
  bool is_ose_itch_;

  bool is_hidden_order_available_;
  uint64_t modified_order_id_;
  int modified_int_price_;
  int modified_cur_size_;
  int modified_prev_size_;
  ttime_t modified_time_;

 public:
  NSEMarketOrderManager(DebugLogger& dbglogger, const Watch& watch, SecurityNameIndexer& sec_name_indexer,
                        const std::vector<MarketOrdersView*>& mov_map, bool _is_hidden_size_avialable,
                        bool is_ose_itch = false);

  ~NSEMarketOrderManager() {}

  void SetTimeToSkipUntilFirstEvent(const ttime_t start_time) {}

  void OnOrderAdd(const uint32_t t_security_id_, const TradeType_t t_buysell_, const uint64_t t_order_id_,
                  const double t_price_, const uint32_t t_size_, const bool t_is_intermediate_);
  void OnOrderModify(const uint32_t t_security_id_, const TradeType_t t_buysell_, const uint64_t t_order_id_,
                     const double t_price_, const uint32_t t_size_);
  void OnOrderDelete(const uint32_t t_security_id_, const TradeType_t t_buysell_, const uint64_t t_order_id_,
                     const double t_price_, const bool t_delete_order_, const bool t_is_intermediate_);
  void OnTrade(const unsigned int t_security_id_, double const t_trade_price_, const int t_trade_size_,
               const uint64_t t_buy_order_num_, const uint64_t t_sell_order_num_);
  void OnHiddenTrade(const unsigned int t_security_id_, double const t_trade_price_, const int t_trade_size_,
                     const uint64_t t_buy_order_num_, const uint64_t t_sell_order_num_);
  void OnTradeExecRange(const unsigned int t_security_id_, double const t_low_exec_band_, double const t_high_exec_band_) {}
  void AddSimulatedOrders(const unsigned int security_id);
  void ResetBook(const unsigned int security_id);
  void DeleteOrders(const unsigned int t_security_id_, int t_start_int_price_, int t_end_int_price_,
                    TradeType_t t_buysell_);
};
}
