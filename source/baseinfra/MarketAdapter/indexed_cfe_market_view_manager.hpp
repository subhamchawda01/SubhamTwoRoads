// =====================================================================================
//
//       Filename:  indexed_cfe_market_view_manager.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  05/12/2014 09:22:40 AM
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

#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"

namespace HFSAT {

class IndexedCfeMarketViewManager : public CFEPriceLevelGlobalListener,
                                    public BaseMarketViewManager,
                                    public GlobalOrderChangeListener {
 private:
  std::vector<bool> sec_id_to_prev_update_was_quote_;  // for setting of a variable in ontrade
  std::vector<bool> is_spread_;  // This is used to do some handling specifically for spread contracts

  std::vector<bool> last_l1_delete_was_bid_;
  std::vector<int> last_l1_delete_int_price_;
  std::vector<int> last_l1_delete_size_;
  std::vector<int> last_l1_delete_msecs_;

 public:
  IndexedCfeMarketViewManager(DebugLogger& t_dbglogger_, const Watch& t_watch_,
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
  void OnPriceLevelOverlay(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                           const int t_level_overlayed_, const double t_price_, const int t_new_size_,
                           const int t_new_ordercount_, const bool t_is_intermediate_message_);
  void OnTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_);
  void OnSpreadTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_);

  inline void OnGlobalOrderChange(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                  const int t_int_price_) {}

  void DropIndexedBookForSource(HFSAT::ExchSource_t t_exch_source_, const int t_security_id_);
};
}
