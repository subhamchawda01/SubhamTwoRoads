/**
   \file MarketAdapter/indexed_fpga_market_view_manager.hpp

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

class IndexedFpgaMarketViewManager : public BaseMarketViewManager,
                                     public FPGAHalfBookGlobalListener,
                                     public PriceLevelGlobalListener,
                                     public GlobalOrderChangeListener {
 private:
  std::vector<bool> sec_id_to_prev_update_was_quote_;  // for setting of a variable in ontrade
  std::vector<std::vector<int> > sec_id_to_bid_indexes_;
  std::vector<std::vector<int> > sec_id_to_ask_indexes_;

  BookManagerErrorCode_t UpdateBids(const unsigned int t_security_id_, FPGAHalfBook* t_half_book_);
  BookManagerErrorCode_t UpdateAsks(const unsigned int t_security_id_, FPGAHalfBook* t_half_book_);

 public:
  IndexedFpgaMarketViewManager(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                               SecurityNameIndexer& t_sec_name_indexer_,
                               const std::vector<SecurityMarketView*>& t_security_market_view_map_);

  ~IndexedFpgaMarketViewManager() {}

  void OnHalfBookChange(const unsigned int t_security_id_, const TradeType_t t_buysell_, FPGAHalfBook* t_half_book_,
                        bool is_intermediate_);
  void OnTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_,
               const TradeType_t t_buysell_);

  inline void OnGlobalOrderChange(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                  const int t_int_price_) {}

  // PriceLevelGlobalListener APIs
  void OnPriceLevelNew(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_added_,
                       const double t_price_, const int t_new_size_, const int t_new_ordercount_,
                       const bool t_is_intermediate_message_) {}
  void OnPriceLevelDelete(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_removed_,
                          const double t_price_, const bool t_is_intermediate_message_) {}
  void OnPriceLevelChange(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_changed_,
                          const double t_price_, const int t_new_size_, const int t_new_ordercount_,
                          const bool t_is_intermediate_message_) {}
  void OnPriceLevelDeleteFrom(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                              const int t_min_level_deleted_, const bool t_is_intermediate_message_) {}
  void OnPriceLevelDeleteThrough(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                 const int t_max_level_deleted_, const bool t_is_intermediate_message_) {}
  void OnPriceLevelDeleteSynthetic(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                   const double t_price_, const bool t_is_intermediate_message_);
  void OnPriceLevelOverlay(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                           const int t_level_overlayed_, const double t_price_, const int t_new_size_,
                           const int t_new_ordercount_, const bool t_is_intermediate_message_) {}
};
}
