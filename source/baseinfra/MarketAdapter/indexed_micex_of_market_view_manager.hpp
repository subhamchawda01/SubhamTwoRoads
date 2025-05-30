/**
   \file MarketAdapter/indexed_micex_of_market_view_manager.hpp

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
#include "dvccode/CDef/micex_mds_defines.hpp"
#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/assumptions.hpp"

namespace HFSAT {

struct MicexOFBufferedOrderExecData {
  TradeType_t aggressive_side;
  double trade_price;
  int size_traded;
  int int_trade_price;

  MicexOFBufferedOrderExecData()
      : aggressive_side(kTradeTypeNoInfo), trade_price(0.0), size_traded(0), int_trade_price(0) {}

  MicexOFBufferedOrderExecData(TradeType_t t_aggressive_side, double t_trade_price, int t_size_traded,
                               int t_int_trade_price) {
    aggressive_side = t_aggressive_side;
    trade_price = t_trade_price;
    size_traded = t_size_traded;
    int_trade_price = t_int_trade_price;
  }
};

class IndexedMicexOFMarketViewManager : public OrderGlobalListener<MICEX_OF_MDS::MICEXOFCommonStruct>,
                                        public BaseMarketViewManager {
 private:
  std::vector<bool> sec_id_to_prev_update_was_quote_;  // for setting of a variable in ontrade

  std::vector<std::unordered_map<uint64_t, MICEX_OF_MDS::MicexOFOrderInfo*>::iterator> bid_order_info_map_iter_;
  std::vector<std::unordered_map<uint64_t, MICEX_OF_MDS::MicexOFOrderInfo*> > order_id_to_bid_order_info_map_;

  std::vector<std::unordered_map<uint64_t, MICEX_OF_MDS::MicexOFOrderInfo*>::iterator> ask_order_info_map_iter_;
  std::vector<std::unordered_map<uint64_t, MICEX_OF_MDS::MicexOFOrderInfo*> > order_id_to_ask_order_info_map_;

  std::vector<uint64_t> sec_id_to_last_trade_notification_timestamp_;

  SimpleMempool<MICEX_OF_MDS::MicexOFOrderInfo> order_mempool_;
  SimpleMempool<HFSAT::MicexOFBufferedOrderExecData> aggregated_trades_mempool_;

  DebugLogger& dbglogger_;
  const Watch& watch_;
  const SecurityNameIndexer& sec_name_indexer_;

  bool is_trade_prediction_on_;

  std::vector<std::deque<HFSAT::MicexOFBufferedOrderExecData*> >
      aggregated_trades_buffer_[2];  // 0 for bid side execs, 1 for ask side execs

 public:
  IndexedMicexOFMarketViewManager(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                                  const SecurityNameIndexer& t_sec_name_indexer_,
                                  const std::vector<SecurityMarketView*>& t_security_market_view_map_,
                                  bool t_is_trade_prediction_on_ = true);

  void Process(const unsigned int security_id, MICEX_OF_MDS::MICEXOFCommonStruct* next_event);

 private:
  // Main Functions
  void OnOrderAdd(const unsigned int t_security_id_, uint64_t t_order_id_, uint8_t t_side_, double t_price_,
                  int t_size_, bool t_intermediate_);
  void OnOrderDelete(const unsigned int t_security_id_, uint64_t t_order_id_, uint8_t t_side_, bool t_is_slower_,
                     bool t_intermediate_, bool t_is_set_order_info_map_iter_ = false);
  void OnOrderModify(const unsigned int t_security_id_, uint64_t t_order_id_, uint8_t t_side_, int t_new_size_,
                     bool t_is_slower_, bool t_intermediate_, bool t_is_set_order_info_map_iter_ = false);
  void OnOrderExec(const unsigned int t_security_id_, uint64_t t_order_id_, uint8_t t_side_, double t_price_,
                   int t_size_exec_, uint64_t t_exchange_timestamp_, bool t_is_slower_, bool t_intermediate_);
  void OnMSRL1Update(const unsigned int t_security_id_, MICEX_OF_MDS::MICEXOFCommonStruct* t_msr_of_str_ptr_);
  void OnOrderResetBegin(const unsigned int t_security_id_);
  void OnOrderResetEnd(const unsigned int t_security_id_);

  // helper functions
  void SendFastTradeNotification(const unsigned int t_security_id_,
                                 MICEX_OF_MDS::MICEXOFCommonStruct* t_msr_of_str_ptr_);
  void SendPredictedTrades(const unsigned int t_security_id_, MICEX_OF_MDS::MICEXOFCommonStruct* t_msr_of_str_ptr_);
  void CheckToFlushAggregatedTrades(int t_idx_, const unsigned int t_security_id_);
  void UpdateBaseBidIndex(const unsigned int t_security_id_, TradeType_t t_buysell_);
  void UpdateBaseAskIndex(const unsigned int t_security_id_, TradeType_t t_buysell_);
  void SanitizeBidSide(const unsigned int t_security_id_, int t_int_price_);
  void SanitizeAskSide(const unsigned int t_security_id_, int t_int_price_);
  inline void ToggleBooleanValue(bool& t_bool_var_) { t_bool_var_ ^= true; }
  uint8_t GetOppositeTradeSide(uint8_t side) {
    if (side == 'B') {
      return 'S';
    } else if (side == 'S') {
      return 'B';
    } else if (side == '0') {
      return '1';
    } else if (side == '1') {
      return '0';
    }
    return '-';
  }
};
}
