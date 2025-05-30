// =====================================================================================
//
//       Filename:  sim_market_maker.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  Thursday 07 January 2016 02:50:58  GMT
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
#include <time.h>
#include "midterm/MidTerm/mid_term_order_routing_defines.hpp"

using namespace std;

namespace MIDTERM {

class SimMarketMaker : public HFSAT::SecurityMarketViewChangeListener {
public:
  HFSAT::DebugLogger &dbglogger_;
  HFSAT::Watch &watch_;
  HFSAT::SecurityNameIndexer &sec_name_indexer_;
  vector<MarketMakerInfo> mm_maker_;

public:
  SimMarketMaker(HFSAT::DebugLogger &dbglogger, HFSAT::Watch &watch)
      : dbglogger_(dbglogger), watch_(watch),
        sec_name_indexer_(HFSAT::SecurityNameIndexer::GetUniqueInstance()) {}

  SimMarketMaker(SimMarketMaker const &disabled_copy_constructor) = delete;

  static SimMarketMaker &GetUniqueInstance(HFSAT::DebugLogger &dbglogger,
                                           HFSAT::Watch &watch) {
    static SimMarketMaker unique_instance(dbglogger, watch);
    return unique_instance;
  }
  void OnMarketUpdate(unsigned int const security_id,
                      HFSAT::MarketUpdateInfo const &market_update_info);
  void SendResponseToAlgo(char *, HFSAT::ORRType_t, char, int32_t, double,
                          int32_t, int32_t);
  void OnTradePrint(unsigned int const security_id,
                    HFSAT::TradePrintInfo const &trade_print_info,
                    HFSAT::MarketUpdateInfo const &market_update_info){};
  void ReceiveOrder(OrderRequest order_);
};
}
