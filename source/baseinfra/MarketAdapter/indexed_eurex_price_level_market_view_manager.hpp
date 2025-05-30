// =====================================================================================
//
//       Filename:  indexed_eurex_price_level_market_view_manager.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  12/24/2012 07:10:03 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 353, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#pragma once

#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "baseinfra/OrderRouting/prom_order_manager.hpp"

namespace HFSAT {

/// Assumption PLChange never changes price .. used to detect missing pkts
class IndexedEUREXPriceLevelMarketViewManager : public BaseMarketViewManager,
                                                public PriceLevelGlobalListener,
                                                public GlobalOrderChangeListener {
 public:
  IndexedEUREXPriceLevelMarketViewManager(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                                          const SecurityNameIndexer& t_sec_name_indexer_,
                                          const std::vector<SecurityMarketView*>& t_security_market_view_map_);

  void OnPriceLevelNew(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_added_,
                       const double t_price_, const int t_new_size_, const int t_new_ordercount_,
                       const bool t_is_intermediate_message_);
  void OnPriceLevelDelete(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_removed_,
                          const double t_price_, const bool t_is_intermediate_message_);
  void OnPriceLevelChange(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_changed_,
                          const double t_price_, const int t_new_size_, const int t_new_ordercount_,
                          const bool t_is_intermediate_message_);

  void OnPriceLevelDeleteFrom(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                              const int t_min_level_deleted_, const bool t_is_intermediate_message_) {}
  void OnPriceLevelDeleteThrough(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                 const int t_max_level_deleted_, const bool t_is_intermediate_message_);
  void OnPriceLevelOverlay(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                           const int t_level_overlayed_, const double t_price_, const int t_new_size_,
                           const int t_new_ordercount_, const bool t_is_intermediate_message_) {}
  void OnTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_,
               const TradeType_t t_buysell_);

  void OnGlobalOrderChange(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_int_price_) {}

  void SetBestL1AskVariablesOnLift(const unsigned int t_security_id_, const double t_trade_price_,
                                   const int t_trade_size_);
  void SetBestL1BidVariablesOnHit(const unsigned int t_security_id_, const double t_trade_price_,
                                  const int t_trade_size_);

  void IndexRebuild(const unsigned int t_security_id_, const TradeType_t t_buysell_, int new_mid_price_,
                    int new_mid_index_, int last_base_price_, int last_base_index_);

  void DumpBook(const unsigned int t_security_id_);

  void DropIndexedBookForSource(const std::vector<SecurityMarketView*>& t_security_market_view_map_,
                                HFSAT::ExchSource_t _exch_source_, const int _security_id_);
};
}
