// =====================================================================================
//
//       Filename:  execution_logic.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  Friday 22 January 2016 01:33:45  GMT
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#pragma once
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "midterm/MidTerm/mid_term_order_listener.hpp"
#include "midterm/MidTerm/sim_trader.hpp"
#include "midterm/MidTerm/live_trader.hpp"
#include "midterm/MidTerm/base_algo_manager.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "midterm/MidTerm/mid_term_order_routing_defines.hpp"
#define VOLUME_PROFILE_GRANULARITY 30
#define VOLUME_PROFILE_FILEPATH "/spare/local/PeriodicVolume/"
#define LINEBUFFER 1024

namespace MIDTERM {

// Used in synthetic algos
enum class Order_Status {
  kInvalid = 0,
  kNSEOrderCancelled,
  kNSEOrderPartiallyExecuted,
  kNSEOrderExecuted,
  kNSEOrderRejected,
  kNSEOrderInProcess,
  kNSEOrderConfirmed
};

// Used in vanilla algos
struct OrderStatus {
  Order_Status status;
  int timestamp;
  int quantity;
  char side;

  OrderStatus() {
    status = Order_Status::kNSEOrderInProcess;
    quantity = 0;
    timestamp = -1;
    side = 'I';
  }
};

class ExecutionLogic : public MidTermOrderListener,
                       public HFSAT::TimePeriodListener,
                       public HFSAT::SecurityMarketViewChangeListener {
public:
  ExecutionLogic(HFSAT::DebugLogger &dbglogger, HFSAT::Watch &watch,
                 std::string shortcode, int lotsize, BaseTrader &trader,
                 BaseAlgoManager &algo_manager, Mode mode)
      : dbglogger_(dbglogger), watch_(watch),
        sec_name_indexer_(HFSAT::SecurityNameIndexer::GetUniqueInstance()),
        mkt_lock_(), market_(new LastSeenMarketSnapshot[DEF_MAX_SEC_ID]),
        shortcode_(shortcode), lotsize_(lotsize), orders_sent_(0),
        orders_received_(0), total_orders_to_process_(0), is_executing_(false),
        trader_(trader), algo_manager_(algo_manager), operating_mode_(mode) {
    side_ = 'B';                       // Just initialization
    watch_.subscribe_TimePeriod(this); // Time period update every 1 second
    security_id_ = sec_name_indexer_.GetIdFromString(shortcode);
  }

  virtual ~ExecutionLogic() {}
  HFSAT::DebugLogger &dbglogger_;
  HFSAT::Watch &watch_;
  HFSAT::SecurityNameIndexer &sec_name_indexer_;
  HFSAT::Lock mkt_lock_;
  // Every product has its own market, shortcode, lotsize
  // In case of synthetic instrument, shortcode is defined in the class
  LastSeenMarketSnapshot *market_;
  std::string shortcode_;
  int lotsize_;
  int orders_sent_;
  int orders_received_;
  int total_orders_to_process_;
  bool is_executing_;
  BaseTrader &trader_;
  BaseAlgoManager &algo_manager_;
  Mode operating_mode_;
  char side_;
  int security_id_;
  static int order_id_; // Initialized in cpp

  // vector of security_id to a pair of (timestamp, quantity)
  std::vector<std::pair<int, double>> schedule_;
  // vector of security_id to a pair of (quantity, price)
  std::vector<std::pair<int, double>> executions;

  // TODO::Let's try to remove this
  map<int, ResponseStatus> rejects_;

  // Map from last sent time to pair( OrderResponse, STATUS )
  // Status can be [ "UNDER_PROCESS", "CANCELLED", "CANCEL_REJECTED",  ]
  // Set to false when executed
  // We can choose to delete executed orders from map but keeping for more info
  std::map<int, OrderStatus> order_status_info_;

  virtual void AddOrder(int x) = 0;
  void OnOrderConfirmed(OrderResponse);
  void OnOrderExecuted(OrderResponse);
  void OnOrderCancelled(OrderResponse);
  void OnOrderCancelRejected(OrderResponse);
  void OnOrderRejected(OrderResponse);
  virtual void OnTimePeriodUpdate(const int num_pages_to_add_) = 0;

  // TODO::Let's try to remove this
  void RespondToOrderResponse(OrderResponse response_, OrderType ordertype_);

  // Not used anywhere but can be used to see order status info vector
  void PrintOrderStatusInfo() {
    typedef map<int, OrderStatus>::iterator it_;
    for (it_ iterator = order_status_info_.begin();
         iterator != order_status_info_.end(); iterator++) {
      dbglogger_ << "PRINTING..\n";
      dbglogger_ << iterator->first << "\t" << iterator->second.quantity << "\t"
                 << iterator->second.timestamp << "\n";
    }
  }

  void OnMarketUpdate(unsigned int const security_id,
                      HFSAT::MarketUpdateInfo const &market_update_info);

  // No need to implement this in base as in most cases we won't be using
  // OnTrade
  void OnTradePrint(unsigned int const security_id,
                    HFSAT::TradePrintInfo const &trade_print_info,
                    HFSAT::MarketUpdateInfo const &market_update_info){};

  void SendOrder(int32_t &order_id_, string shortcode_, char side_,
                 int32_t size_, double price_, OrderType order_type_);

  // get timestamp of the first order in the vector of execution schedule
  inline int GetTimeOfFirstOrder() {
    if (schedule_.size() == 0)
      return -1;
    return schedule_[0].first;
  }

  // get quantity of the first order in the vector of execution schedule
  inline int GetQuantityOfFirstOrder() {
    if (schedule_.size() == 0)
      return -1;
    return schedule_[0].second;
  }

  // get the first pair of time, qty of the execution schedule
  inline std::pair<int, int> GetFirstOrder() {
    if (schedule_.size() == 0)
      return std::make_pair(-1, -1);
    return schedule_[0];
  }

  // delete the first order from the execution schedule
  inline void DeleteFirstOrder() {
    if (schedule_.size() != 0)
      schedule_.erase(schedule_.begin());
  }

  // get the total quantity that has not been executed
  inline int GetTotalQuantity() {
    int total = 0;
    for (uint32_t i = 0; i < schedule_.size(); i++)
      total += schedule_[i].second;

    return total;
  }

  inline void MarketLock() {
    if (operating_mode_ == Mode::kNSEServerMode) {
      mkt_lock_.LockMutex();
    }
  }
  inline void MarketUnlock() {
    if (operating_mode_ == Mode::kNSEServerMode) {
      mkt_lock_.UnlockMutex();
    }
  }
  inline void Sleep(int n) {
    if (operating_mode_ == Mode::kNSEServerMode) {
      sleep(n);
    }
  }

  inline std::string PrintSchedule() {
    std::ostringstream tostring_;
    tostring_ << "-------PRINTING SCHEDULE--------"
              << "\n";
    tostring_ << "TimeStamp"
              << "\t"
              << "Quantity"
              << "\n";
    for (uint32_t i = 0; i < schedule_.size(); i++) {
      tostring_ << i << "\t" << schedule_[i].first << "\t"
                << schedule_[i].second << "\n";
    }
    tostring_ << "-------------DONE---------------"
              << "\n";
    return tostring_.str();
  }

  std::map<int, int> GetAverageVolumeProfile(std::string product);
};
}
