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
#include <unordered_map>

#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvccode/CommonDataStructures/simple_mempool.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/ose_itch_mds_defines.hpp"

namespace HFSAT {

struct BufferTradeDetails {
  double price_;
  int size_traded_;
  int int_trade_price_;
  TradeType_t buysell_;

  BufferTradeDetails() : price_(0), size_traded_(0), int_trade_price_(0), buysell_(kTradeTypeNoInfo) {}
  BufferTradeDetails(double price, int size_traded, int int_trade_price, TradeType_t buysell)
      : price_(price), size_traded_(size_traded), int_trade_price_(int_trade_price), buysell_(buysell) {}
};

struct BufferPLChangeState {
  unsigned int sec_id_;
  TradeType_t buysell_;
  int level_changed_;
  int int_price_;
  int int_price_level_;
  int old_size_;
  int new_size_;
  int old_ordercount_;
  int new_ordercount_;
  bool is_intermediate_message_;
  char pl_notif_;

  BufferPLChangeState()
      : sec_id_(0),
        buysell_(kTradeTypeNoInfo),
        level_changed_(-1),
        int_price_(-1),
        int_price_level_(-1),
        old_size_(-1),
        new_size_(-1),
        old_ordercount_(-1),
        new_ordercount_(-1),
        is_intermediate_message_(false),
        pl_notif_('-') {}

  void SetState(unsigned int sec_id, TradeType_t buysell, int level_changed, int int_price, int int_price_level,
                int old_size, int new_size, int old_ordercount, int new_ordercount, bool is_intermediate_message,
                char pl_notif) {
    sec_id_ = sec_id;
    buysell_ = buysell;
    level_changed_ = level_changed;
    int_price_ = int_price;
    int_price_level_ = int_price_level;
    old_size_ = old_size;
    new_size_ = new_size;
    old_ordercount_ = old_ordercount;
    new_ordercount_ = new_ordercount;
    is_intermediate_message_ = is_intermediate_message;
    pl_notif_ = pl_notif;
  }
};

class IndexedOseOrderFeedMarketViewManager : public OrderFeedGlobalListener, public BaseMarketViewManager {
 private:
  std::vector<bool> sec_id_to_prev_update_was_quote_;  // for setting of a variable in ontrade

  std::unordered_map<uint64_t, OSE_ITCH_MDS::OSEOrderInfo*>::iterator bid_order_info_map_iter_;
  std::vector<std::unordered_map<uint64_t, OSE_ITCH_MDS::OSEOrderInfo*>> sec_id_to_order_id_and_bid_order_info_map_;

  std::unordered_map<uint64_t, OSE_ITCH_MDS::OSEOrderInfo*>::iterator ask_order_info_map_iter_;
  std::vector<std::unordered_map<uint64_t, OSE_ITCH_MDS::OSEOrderInfo*>> sec_id_to_order_id_and_ask_order_info_map_;

  SimpleMempool<OSE_ITCH_MDS::OSEOrderInfo> order_mempool_;

  std::vector<bool> sec_id_to_is_regular_session_;
  std::vector<bool> sec_id_to_delete_on_cross_;

  DebugLogger& dbglogger_;
  const Watch& watch_;
  const SecurityNameIndexer& sec_name_indexer_;

  // Member variables for cumulating trades and for storing state
  BufferPLChangeState prev_PL_change_state_;
  std::vector<std::deque<BufferTradeDetails>> buffered_trades_;
  void CheckToNotifyTradeMessage(const unsigned int t_security_id_, bool is_intermediate_msg);
  void CheckAndBufferTrades(const unsigned int t_security_id_, uint8_t side, double price, int size, int t_int_price_);

 public:
  IndexedOseOrderFeedMarketViewManager(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                                       const SecurityNameIndexer& t_sec_name_indexer_,
                                       const std::vector<SecurityMarketView*>& t_security_market_view_map_);

  // Main Functions from OrderFeedGlobalListener
  void OnOrderAdd(const unsigned int t_security_id_, uint64_t t_order_id_, uint8_t t_side_, double t_price_,
                  int t_size_, uint32_t t_priority_, bool t_intermediate_) override;
  void OnOrderDelete(const unsigned int t_security_id_, uint64_t t_order_id_, uint8_t t_side_, bool t_intermediate_,
                     bool t_is_set_order_info_map_iter_ = false) override;
  void OnOrderModify(const unsigned int t_security_id_, uint64_t t_order_id_, uint8_t t_side_, int t_new_size_,
                     uint64_t t_new_order_id_, bool t_intermediate_,
                     bool t_is_set_order_info_map_iter_ = false) override;
  void OnOrderExec(const unsigned int t_security_id_, uint64_t t_order_id_, uint8_t t_side_, double t_price_,
                   int t_size_exec_, bool t_intermediate_) override;
  void OnOrderExecWithTradeInfo(const uint32_t security_id, const uint64_t order_id, const uint8_t t_side_,
                                const double exec_price, const uint32_t size_exec, bool intermediate) override;
  void OnOrderResetBegin(const unsigned int security_id) override;
  void OnTradingStatus(const uint32_t security_id, std::string status) override;

  // unimplemented virtual functions
  void OnOrderReplace(const unsigned int t_security_id_, uint64_t t_order_id_, uint8_t t_side_, double t_new_price_,
                      int t_new_size_, uint64_t t_new_order_id_, bool t_intermediate_) override {}
  void OnOrderResetEnd(const unsigned int t_security_id_) override {}

  // meta-functions
  void UpdateBaseBidIndex(const unsigned int t_security_id_, TradeType_t t_buysell_);
  void UpdateBaseAskIndex(const unsigned int t_security_id_, TradeType_t t_buysell_);
  void SanitizeBidSide(const unsigned int t_security_id_, int t_int_price_);
  void SanitizeAskSide(const unsigned int t_security_id_, int t_int_price_);
};
}
