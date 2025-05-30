/**
   \file MarketAdapter/l1_price_market_view_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_MARKETADAPTER_OSE_L1_PRICE_MARKET_VIEW_MANAGER_H
#define BASE_MARKETADAPTER_OSE_L1_PRICE_MARKET_VIEW_MANAGER_H

#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "baseinfra/OrderRouting/prom_order_manager.hpp"

namespace HFSAT {

/// Assumption PLChange never changes price .. used to detect missing pkts
class OSEL1PriceMarketViewManager : public BaseMarketViewManager,
                                    public FullBookGlobalListener,
                                    public GlobalOrderChangeListener {
 public:
  OSEL1PriceMarketViewManager(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                              const SecurityNameIndexer& t_sec_name_indexer_,
                              const std::vector<SecurityMarketView*>& t_security_market_view_map_);

  void OnL1Change(const unsigned int t_security_id_, double bid_price, int bid_size, int num_bid_orders,
                  double ask_price, int ask_size, int num_ask_orders) {}
  void OnL1Change(const unsigned int t_security_id_, const TradeType_t t_buysell_, const double t_price_,
                  const int t_new_size_, const int t_new_ordercount_, const bool t_is_intermediate_message_);
  void OnTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_,
               const TradeType_t t_buysell_);

  inline void OnGlobalOrderChange(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                  const int t_int_price_) {}
  // Dummy

  void OnFullBookChange(const unsigned int t_security_id_, const FullBook* t_full_book_) {}

 private:
  std::vector<PromOrderManager*> sid_to_prom_order_manager_;
};
}
#endif  // BASE_MARKETADAPTER_L1_PRICE_MARKET_VIEW_MANAGER_H
