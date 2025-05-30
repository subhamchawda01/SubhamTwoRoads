/**
    \file MarketAdapter/indexed_hkomd_price_level_market_view_manager.hpp

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

class IndexedHKOMDPriceLevelMarketViewManager : public PriceLevelGlobalListener, public BaseMarketViewManager {
 private:
  // Reason for having the changed variables at class level is to incorporate the effect of intermediate messages
  std::vector<bool> l1_price_changed_;
  std::vector<bool> l1_size_changed_;

 public:
  IndexedHKOMDPriceLevelMarketViewManager(DebugLogger& r_dbglogger_, const Watch& r_watch_,
                                          const SecurityNameIndexer& r_sec_name_indexer_,
                                          const std::vector<SecurityMarketView*>& r_security_market_view_map_);

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
                                 const int t_max_level_deleted_, const bool t_is_intermediate_message_) {}

  void OnPriceLevelOverlay(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                           const int t_level_overlayed_, const double t_price_, const int t_new_size_,
                           const int t_new_ordercount_, const bool t_is_intermediate_message_) {}

  void OnTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_,
               const TradeType_t t_buysell_);
};
}
