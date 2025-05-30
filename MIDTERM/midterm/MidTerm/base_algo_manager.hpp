// =====================================================================================
//
//       Filename:  trade_bar_generator.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  01/05/2016 12:10:02 PM
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
#include "midterm/MidTerm/mid_term_order_routing_defines.hpp"
#include "midterm/MidTerm/mid_term_order_listener.hpp"

using namespace std;

namespace MIDTERM {

class BaseAlgoManager {
public:
  vector<MidTermOrderListener *> order_listeners;
  HFSAT::Lock mkt_lock_;
  Mode operating_mode_;
  HFSAT::DebugLogger &dbglogger_;
  HFSAT::Watch &watch_;
  HFSAT::SecurityNameIndexer &sec_name_indexer_;
  LastSeenMarketSnapshot *market_;
  std::vector<HFSAT::FastPriceConvertor *> fast_price_convertor_vec_;
  OrderResponse order_response_;
  char data_buffer_[MAX_ORDER_RESPONSE_BUFFER_SIZE];
  int32_t read_offset_;
  int32_t order_id_;

public:
  BaseAlgoManager(Mode mode, HFSAT::DebugLogger &dbglogger, HFSAT::Watch &watch)
      : mkt_lock_(), operating_mode_(mode), dbglogger_(dbglogger),
        watch_(watch),
        sec_name_indexer_(HFSAT::SecurityNameIndexer::GetUniqueInstance()),
        market_(new LastSeenMarketSnapshot[DEF_MAX_SEC_ID]), data_buffer_(),
        read_offset_(0), order_id_(150000) {
    if (mode != Mode::kNSEServerMode && mode != Mode::kNSESimMode)
      return;

    dbglogger_ << "Algo Manager ready..." << DBGLOG_ENDL_FLUSH;
  }
  BaseAlgoManager(BaseAlgoManager const &disabled_copy_constructor) = delete;

  static BaseAlgoManager &GetUniqueInstance(Mode mode,
                                            HFSAT::DebugLogger &dbglogger,
                                            HFSAT::Watch &watch) {
    static BaseAlgoManager unique_instance(mode, dbglogger, watch);
    return unique_instance;
  }
  void SubscribeOrderListener(MidTermOrderListener *);
  void NotifyOrderConfirmedListeners(OrderResponse);
  void NotifyOrderExecutedListeners(OrderResponse);
  void NotifyOrderCancelledListeners(OrderResponse);
  void NotifyOrderCancelRejectedListeners(OrderResponse);
  void NotifyOrderRejectedListeners(OrderResponse);

  void ProcessSingleResponse();
  void RespondToOrderResponse(OrderResponse, OrderType);
};
}
