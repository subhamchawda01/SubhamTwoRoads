/**
    \file MarketAdapter/indexed_eobi_price_level_market_view_manager.hpp

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
#include "baseinfra/MarketAdapter/tse_trade_time_manager.hpp"

namespace HFSAT {

class IndexedTsePriceLevelMarketViewManager : public PriceLevelGlobalListener,
                                              public BaseMarketViewManager,
                                              public GlobalOrderChangeListener {
 private:
  // Reason for having the changed variables at class level is to incorporate the effect of intermediate messages
  std::vector<bool> l1_price_changed_;
  std::vector<bool> l1_size_changed_;
  // deal with weird trades etc at beginning - guides index rebuild
  std::vector<bool> valid_trade_seen_;
  std::vector<int> last_trade_price_;

  std::vector<bool> last_l1_delete_was_bid_;
  std::vector<int> last_l1_delete_int_price_;
  std::vector<bool> last_update_was_delete_l1_;
  std::vector<bool> sec_id_to_prev_update_was_quote_;  // for setting of a variable in ontrade
  HFSAT::TseTradeTimeManager& tse_trade_time_manager_;

 public:
  IndexedTsePriceLevelMarketViewManager(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                                        const SecurityNameIndexer& t_sec_name_indexer_,
                                        const std::vector<SecurityMarketView*>& t_security_market_view_map_);

  void OnPriceLevelNew(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_added_,
                       const double t_price_, const int t_new_size_, const int t_new_ordercount_,
                       const bool t_is_intermediate_message_);
  void OnPriceLevelDelete(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_removed_,
                          const double t_price_, const bool t_is_intermediate_message_);
  void OnTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_,
               const TradeType_t t_buysell_);

  inline void OnGlobalOrderChange(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                  const int t_int_price_) {}

  void OnPriceLevelChange(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_changed_,
                          const double t_price_, const int t_new_size_, const int t_new_ordercount_,
                          const bool t_is_intermediate_message_) {
    std::cerr << " PriceLevelChange should never be called for Tse .. exiting now \n";
    exit(1);
  }

  void OnPriceLevelDeleteFrom(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                              const int t_min_level_deleted_, const bool t_is_intermediate_message_) {
    std::cerr << " PriceLevelDeleteFrom should never be called for Tse .. exiting now \n";
    exit(1);
  }

  void OnPriceLevelDeleteThrough(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                 const int t_max_level_deleted_, const bool t_is_intermediate_message_) {
    std::cerr << " PriceLevelDeleteThrough should never be called for Tse .. exiting now \n";
    exit(1);
  }

  void OnPriceLevelOverlay(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                           const int t_level_overlayed_, const double t_price_, const int t_new_size_,
                           const int t_new_ordercount_, const bool t_is_intermediate_message_) {
    std::cerr << " PriceLevelOverlay should never be called for Tse .. exiting now \n";
    exit(1);
  }
};
}
