/**
   \file MarketAdapter/indexed_tmx_obf_of_market_view_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include <map>
#include <sys/time.h>
#include <unordered_map>

#include "dvccode/CommonDataStructures/simple_mempool.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/tmx_obf_mds_defines.hpp"
#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/assumptions.hpp"

namespace HFSAT {

#define AUCTION_PERIOD_SECONDS 0
#define AUCTION_PERIOD_MICROSECONDS 1000

struct TMXOBFOFBufferedOrderExecData {
  TradeType_t aggressive_side;
  double trade_price;
  int size_traded;
  int int_trade_price;

  TMXOBFOFBufferedOrderExecData()
      : aggressive_side(kTradeTypeNoInfo), trade_price(0.0), size_traded(0), int_trade_price(0) {}

  TMXOBFOFBufferedOrderExecData(TradeType_t t_aggressive_side, double t_trade_price, int t_size_traded,
                                int t_int_trade_price) {
    aggressive_side = t_aggressive_side;
    trade_price = t_trade_price;
    size_traded = t_size_traded;
    int_trade_price = t_int_trade_price;
  }
};

class IndexedTMXOBFOFMarketViewManager : public OrderFeedGlobalListener, public BaseMarketViewManager {
 private:
  std::vector<bool> sec_id_to_prev_update_was_quote_;  // for setting of a variable in ontrade

  std::vector<std::unordered_map<uint64_t, TMX_OBF_MDS::TMXOBFOFOrderInfo*>::iterator> bid_order_info_map_iter_;
  std::vector<std::unordered_map<uint64_t, TMX_OBF_MDS::TMXOBFOFOrderInfo*> > order_id_to_bid_order_info_map_;

  std::vector<std::unordered_map<uint64_t, TMX_OBF_MDS::TMXOBFOFOrderInfo*>::iterator> ask_order_info_map_iter_;
  std::vector<std::unordered_map<uint64_t, TMX_OBF_MDS::TMXOBFOFOrderInfo*> > order_id_to_ask_order_info_map_;

  SimpleMempool<TMX_OBF_MDS::TMXOBFOFOrderInfo> order_mempool_;
  SimpleMempool<HFSAT::TMXOBFOFBufferedOrderExecData> aggregated_trades_mempool_;

  std::vector<bool> is_auction_period_over_;
  std::vector<bool> is_first_trade_received_;
  std::vector<ttime_t> first_trade_timestamp_;

  DebugLogger& dbglogger_;
  const Watch& watch_;
  const SecurityNameIndexer& sec_name_indexer_;

  std::vector<std::deque<HFSAT::TMXOBFOFBufferedOrderExecData*> > aggregated_trades_buffer_;

 public:
  IndexedTMXOBFOFMarketViewManager(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                                   const SecurityNameIndexer& t_sec_name_indexer_,
                                   const std::vector<SecurityMarketView*>& t_security_market_view_map_);

  // Main Functions
  void OnOrderAdd(const unsigned int t_security_id_, uint64_t t_order_id_, uint8_t t_side_, double t_price_,
                  int t_size_, uint32_t t_priority_, bool t_intermediate_);
  void OnOrderDelete(const unsigned int t_security_id_, uint64_t t_order_id_, uint8_t t_side_, bool t_intermediate_,
                     bool t_is_set_order_info_map_iter_ = false);
  void OnOrderModify(const unsigned int t_security_id_, uint64_t t_order_id_, uint8_t t_side_, int t_new_size_,
                     uint64_t t_new_order_id_, bool t_intermediate_, bool t_is_set_order_info_map_iter_ = false);
  void OnOrderReplace(const unsigned int t_security_id_, uint64_t t_order_id_, uint8_t t_side_, double t_new_price_,
                      int t_new_size_, uint64_t t_new_order_id_, bool t_intermediate_);
  void OnOrderExec(const unsigned int t_security_id_, uint64_t t_order_id_, uint8_t t_side_, double t_price_,
                   int t_size_exec_, bool t_intermediate_);
  void OnOrderResetBegin(const unsigned int t_security_id_);
  void OnOrderResetEnd(const unsigned int t_security_id_);

  // helper functions
  void CheckToFlushAggregatedTrades(const unsigned int t_security_id_);
  void UpdateBaseBidIndex(const unsigned int t_security_id_, TradeType_t t_buysell_);
  void UpdateBaseAskIndex(const unsigned int t_security_id_, TradeType_t t_buysell_);
  void SanitizeBidSide(const unsigned int t_security_id_, int t_int_price_);
  void SanitizeAskSide(const unsigned int t_security_id_, int t_int_price_);

  // functions to bypass some checks
  void SetAuctionPeriodStateVec(bool t_state_) {
    std::fill(is_auction_period_over_.begin(), is_auction_period_over_.end(), t_state_);
    return;
  }
};
}
