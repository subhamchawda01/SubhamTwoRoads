
// =====================================================================================
//
//       Filename:  ose_pricefeed_market_view_manager.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  04/29/2013 11:49:42 AM
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

#ifndef BASE_MARKETADAPTER_OSE_PRICEFEED_MARKET_VIEW_MANAGER_H
#define BASE_MARKETADAPTER_OSE_PRICEFEED_MARKET_VIEW_MANAGER_H

#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "baseinfra/OrderRouting/prom_order_manager.hpp"

#define SECONDS_TO_WAIT 1
#define MSECS_TO_WAIT 000

#define MAX_TRADE_TICK_DIFFERENCE 1

namespace HFSAT {

/// Assumption PLChange never changes price .. used to detect missing pkts
class OSEPriceFeedMarketViewManager : public BaseMarketViewManager,
                                      public PriceLevelGlobalListener,
                                      public GlobalOrderChangeListener {
 public:
  OSEPriceFeedMarketViewManager(DebugLogger& t_dbglogger_, const Watch& t_watch_,
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
                              const int t_min_level_deleted_, const bool t_is_intermediate_message_);
  void OnPriceLevelDeleteThrough(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                 const int t_max_level_deleted_, const bool t_is_intermediate_message_);
  void OnPriceLevelOverlay(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                           const int t_level_overlayed_, const double t_price_, const int t_new_size_,
                           const int t_new_ordercount_, const bool t_is_intermediate_message_) {}
  void OnTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_,
               const TradeType_t t_buysell_);

  // not sure why this was added
  inline void OnGlobalOrderChange(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                  const int t_int_price_) {}

 private:
 protected:
  LockFreeSimpleMempool<MarketOrder> market_orders_mempool_;
  std::vector<int> sid_to_bidside_trade_size_;
  std::vector<int> sid_to_bidside_trade_price_;
  std::vector<int> sid_to_highest_accumulated_bidside_trade_size_;
  std::vector<ttime_t> sid_to_last_bidside_trade_time_;
  std::vector<int> sid_to_askside_trade_size_;
  std::vector<int> sid_to_askside_trade_price_;
  std::vector<int> sid_to_highest_accumulated_askside_trade_size_;
  std::vector<ttime_t> sid_to_last_askside_trade_time_;
};
}
#endif  // BASE_MARKETADAPTER_EUREX_PRICE_LEVEL_MARKET_VIEW_MANAGER_H
