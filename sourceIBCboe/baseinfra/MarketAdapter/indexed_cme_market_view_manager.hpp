/**
   \file MarketAdapter/indexed_cme_market_view_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
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

class IndexedCmeMarketViewManager : public PriceLevelGlobalListener,
                                    public BaseMarketViewManager,
                                    public GlobalOrderChangeListener {
 private:
  std::vector<bool> sec_id_to_prev_update_was_quote_;  // for setting of a variable in ontrade
  TradeType_t ProcessTradeTypeNoInfo(TradeType_t t_buysell_);

 public:
  IndexedCmeMarketViewManager(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                              const SecurityNameIndexer& t_sec_name_indexer_,
                              const std::vector<SecurityMarketView*>& t_security_market_view_map_);

  void OnPriceLevelNew(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_added_,
                       const double t_price_, const int t_new_size_, const int t_new_ordercount_,
                       const bool t_is_intermediate_message_);
  void OnPriceLevelChange(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_changed_,
                          const double t_price_, const int t_new_size_, const int t_new_ordercount_,
                          const bool t_is_intermediate_message_);
  void OnPriceLevelDelete(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_removed_,
                          const double t_price_, const bool t_is_intermediate_message_);
  void OnTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_,
               const TradeType_t t_buysell_);

  void OnPriceLevelDeleteFrom(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                              const int t_min_level_deleted_, const bool t_is_intermediate_message_); /*{
    std::cerr << " PriceLevelDeleteFrom should never be called for CME .. exiting now \n";
    exit(1);
  }
*/
  void OnPriceLevelDeleteThrough(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                 const int t_max_level_deleted_, const bool t_is_intermediate_message_); /*{
    std::cerr << " PriceLevelDeleteThrough should never be called for CME .. exiting now \n";
    exit(1);
  }
*/
  void OnPriceLevelDeleteSynthetic(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                   const double t_price_, const bool t_is_intermediate_message_);
  void OnPriceLevelOverlay(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                           const int t_level_overlayed_, const double t_price_, const int t_new_size_,
                           const int t_new_ordercount_, const bool t_is_intermediate_message_); /* {
     std::cerr << " PriceLevelOverlay should never be called for CME .. exiting now \n";
     exit(1);
   }
 */

  double GetPriceForLevel(const unsigned int t_security_id, const TradeType_t buysell, const int t_level);

  inline void OnGlobalOrderChange(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                  const int t_int_price_) {}

  void DropIndexedBookForSource(HFSAT::ExchSource_t t_exch_source_, const int t_security_id_);
};
}
