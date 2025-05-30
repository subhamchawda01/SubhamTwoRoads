// =====================================================================================
//
//       Filename:  indexed_hkomd_market_viewManager.hpp
//
//    Description:
//
//        Created:  Monday 05 May 2014 09:22:29  GMT
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
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"

namespace HFSAT {

struct PriceSizePair {
  double price_;
  uint32_t size_;
};

class HKOMDIndexedMarketViewManager : public BaseMarketViewManager, public OrderLevelGlobalListenerHKOMD {
 private:
  std::vector<std::map<uint64_t, PriceSizePair> > bidside_oid_to_qty_map_vec_;
  std::vector<std::map<uint64_t, PriceSizePair> > askside_oid_to_qty_map_vec_;
  std::vector<bool> l1_price_changed_;
  std::vector<bool> l1_size_changed_;
  std::vector<bool> sec_id_to_mask_levels_above_;

 public:
  HKOMDIndexedMarketViewManager(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                                const SecurityNameIndexer& t_sec_name_indexer_,
                                const std::vector<SecurityMarketView*>& t_security_market_view_map_);

  void OnOrderAdd(const uint32_t t_security_id_, const uint64_t t_order_id_, const double t_price_,
                  const uint32_t t_quantity_, const TradeType_t t_buy_sell_, bool t_intermediate_);

  void OnOrderModify(const uint32_t t_security_id_, const uint64_t t_order_id_, const double t_price_,
                     const uint32_t t_quantity_, const TradeType_t t_buy_sell_, bool t_intermediate_);

  void OnOrderDelete(const uint32_t t_security_id_, const uint64_t t_order_id_, const TradeType_t t_buy_sell_,
                     bool is_delete_order_, bool t_intermediate_);

  void OnTrade(const uint32_t t_security_id_, const uint64_t t_order_id_, const double t_price_,
               const uint32_t t_quantity_, const TradeType_t t_buy_sell_);
};
}
