#ifndef _INDEXED_RTS_OF_MARKET_VIEW_MANAGER_HPP_
#define _INDEXED_RTS_OF_MARKET_VIEW_MANAGER_HPP_

#include <map>
#include <vector>

#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvccode/CDef/rts_mds_defines.hpp"

#define RTS_OFv2_DBG 0
#define RTS_OFv2_DBG_PREDICTION 0
#define RTS_OF_USE_ORDER_MAPS 0

namespace HFSAT {

struct OrderDetails {
  uint64_t order_id;
  double price;
  uint32_t size;
  char side;
  bool is_ioc;
};

struct TradeDetails {
  double price_;
  int size_traded_;
  int int_trade_price_;
  TradeType_t buysell_;
  int order_count;
  TradeDetails() : price_(0), size_traded_(0), int_trade_price_(0), buysell_(kTradeTypeNoInfo), order_count(0) {}
  TradeDetails(double price, int size_traded, int int_trade_price, TradeType_t buysell, int order_count = 1)
      : price_(price),
        size_traded_(size_traded),
        int_trade_price_(int_trade_price),
        buysell_(buysell),
        order_count(order_count) {}
};

struct PredictionInfo {
  OrderDetails order;
  int32_t total_predicted_trade_size;
  uint32_t last_agg_order_actual_size_executed;
  bool ongoing_agg_add_event_;

  PredictionInfo() {
    total_predicted_trade_size = 0;
    last_agg_order_actual_size_executed = 0;
    ongoing_agg_add_event_ = false;
  }
};

class OrdersPool {
 public:
  OrdersPool();
  void Process(RTS_MDS::RTSOFCommonStructv2 order);
  bool IsOrderPresent(uint64_t order_id, char side);
  std::vector<std::map<uint64_t, OrderDetails> >& GetOrders();

 private:
  std::vector<std::map<uint64_t, OrderDetails> > order_id_to_details_;
};

// Not using this pool for now.
class DayOrdersPool {
 public:
  DayOrdersPool();
  void Process(uint64_t order_id_, RTS_MDS::RTSOFMsgType msg_type, char side);
  bool IsOrderPresent(uint64_t order_id, char side);
  std::vector<std::map<uint64_t, bool> >& GetOrders();

 private:
  std::vector<std::map<uint64_t, bool> > order_id_to_details_;
};

struct PLChangeState {
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

  PLChangeState()
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
#if RTS_OFv2_DBG
    std::cout << " SetState " << sec_id << " side: " << (int)buysell << "  px " << int_price << " old_sz " << old_size
              << "  new_sz " << new_size << " " << old_ordercount << " " << new_ordercount << std::endl;
#endif
    sec_id_ = sec_id;
    buysell_ = buysell;
    level_changed_ = level_changed;
    int_price_ = int_price;
    int_price_level_ = int_price_level;
    old_size_ = (old_size_ == -1) ? old_size : old_size_;  // initialised to 0, update just once
    new_size_ = new_size;
    old_ordercount_ = (old_ordercount_ == -1) ? old_ordercount : old_ordercount_;  // initialised to 0, update just once
    new_ordercount_ = new_ordercount;
    is_intermediate_message_ = is_intermediate_message;
    pl_notif_ = pl_notif;
  }
};

class IndexedRtsOFMarketViewManager : public OrderGlobalListener<RTS_MDS::RTSOFCommonStructv2>,
                                      public BaseMarketViewManager {
 public:
  IndexedRtsOFMarketViewManager(DebugLogger& dbglogger, const Watch& watch, const SecurityNameIndexer& sec_name_indexer,
                                const std::vector<SecurityMarketView*>& security_market_view_map);

  virtual void Process(const unsigned int security_id, RTS_MDS::RTSOFCommonStructv2* next_event);

  void DropIndexedBookForSource(HFSAT::ExchSource_t exch_source, const int security_id);

 private:
  // Members for updating the book

  // Called when a new order is added to the book
  void OnOrderAdd(const unsigned int security_id, double price, char side, uint32_t size, uint64_t order_id,
                  bool is_intermediate, bool is_ioc, int order_count = 1);
  // Called when an order is deleted from the book, normal as well as due to complete exec
  void OnOrderDelete(const unsigned int security_id, double price, char side, uint32_t size, uint64_t order_id,
                     bool is_intermediate, bool is_ioc, bool isCompleteExec = false);
  // Called when an order is partially executed from the book
  void OnOrderExec(const unsigned int security_id, double price, char side, uint32_t size, uint64_t order_id,
                   bool is_intermediate, bool is_ioc);
  // Clear book state on this synthetic RESET_BEGIN
  void OnResetBegin(const unsigned int t_security_id_);
  void OnResetEnd(const unsigned int t_security_id_);

  // On Trade prediction, we do not change the order count. If we predict entire level to be deleted, there isn't any
  // issue. If predicted trade needs to be added back to the book, we update the correct order count based on
  // number of passive legs for the aggress add.
  void UpdateOrderCountForNonEmptyLevels(const unsigned int t_security_id, int t_int_price, uint8_t t_side,
                                         int t_order_count);
  // If this order crosses the book and is aggressing
  bool IsAgressiveOrder(const unsigned int security_id, double price, char side);

  // helper functions: corrects book state, makes it uncrossing
  void SanitizeBidSide(const unsigned int t_security_id_, uint64_t order_id, int t_int_price_);
  void SanitizeAskSide(const unsigned int t_security_id_, uint64_t order_id, int t_int_price_);

  // Member variables for storing state while cumulating trades
  PLChangeState prev_PL_change_state_;

  // Members functions for handling trade predictions
  void FlushPassiveOrdersInAggressPool(const unsigned int security_id, bool is_intermediate_msg);
  void CheckToNotifyTradeMessage(const unsigned int t_security_id_, bool is_intermediate_msg);
  void CheckAndBufferTrades(const unsigned int t_security_id_, uint8_t side, double price, int size, int t_int_price_);

  // optimise aggress add handling
  void PredictTradesOnAggressAdd(const unsigned int t_security_id_, double price, int size, uint8_t side,
                                 uint64_t order_id, bool is_intermediate_msg, bool is_ioc);
  void VerifyTradePredictionOnAggressCancel(const unsigned int t_security_id_, RTS_MDS::RTSOFMsgType msg_type,
                                            uint64_t order_id, int size, uint8_t side, double price, bool is_ioc);

  // matches the passive execs against the predicted trades. Calls sanitization if we missed something
  void ProcessPassiveExecs(const unsigned int security_id, double price, char side, uint32_t size, uint64_t order_id,
                           bool is_intermediate, bool is_ioc, bool is_complete_exec);
  void ProcessEndOfAggressAddEvents(const unsigned int security_id, bool is_ioc);
  void ProcessAggressiveOrderDelta(const unsigned int t_security_id, RTS_MDS::RTSOFCommonStructv2* order);

 private:
#if RTS_OF_USE_ORDER_MAPS
  std::vector<DayOrdersPool> day_order_pool_;
#endif
  // Members for trade predictions on aggressive add
  std::vector<std::deque<TradeDetails> > buffered_trades_;
  std::vector<std::deque<TradeDetails> > predicted_trades_;
  std::vector<PredictionInfo> trade_prediction_info_;
};
}

#endif
