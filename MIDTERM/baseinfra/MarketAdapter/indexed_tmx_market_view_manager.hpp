/**
   \file MarketAdapter/indexed_tmx_market_view_manager.hpp

   \author: (c) Copyright K2 Research LLC 2010
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"

namespace HFSAT {

class IndexedTmxMarketViewManager : public BaseMarketViewManager,
                                    public FullBookGlobalListener,
                                    public GlobalOrderChangeListener {
 private:
  std::vector<bool> sec_id_to_prev_update_was_quote_;  // for setting of a variable in ontrade
  std::vector<std::vector<int> > sec_id_to_bid_indexes_;
  std::vector<std::vector<int> > sec_id_to_ask_indexes_;

 public:
  IndexedTmxMarketViewManager(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                              SecurityNameIndexer& t_sec_name_indexer_,
                              const std::vector<SecurityMarketView*>& t_security_market_view_map_);

  ~IndexedTmxMarketViewManager() {}

  void OnFullBookChange(const unsigned int t_security_id_, const FullBook* t_full_book_);
  void OnTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_,
               const TradeType_t t_buysell_);
  void OnL1Change(const unsigned int t_security_id, double bid_px, int bid_sz, int bid_count, double ask_px, int ask_sz,
                  int ask_count);

  inline void OnGlobalOrderChange(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                  const int t_int_price_) {}
};
}
