/**
   \file MarketAdapter/l1_price_market_view_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef IBKR_L1_PRICE_MARKET_VIEW_MANAGER_H_
#define IBKR_L1_PRICE_MARKET_VIEW_MANAGER_H_

#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/OrderRouting/order_manager_listeners.hpp"

namespace HFSAT {

/// Assumption PLChange never changes price .. used to detect missing pkts
class IBKRL1PriceMarketViewManager : public OrderGlobalListenerIBKR,
                                 public BaseMarketViewManager,
                                 public GlobalOrderChangeListener {
 public:
  IBKRL1PriceMarketViewManager(DebugLogger& dbglogger, const Watch& watch, const SecurityNameIndexer& sec_name_indexer,
                           const std::vector<SecurityMarketView*>& security_market_view_map);
  void Ib_Tick_Update(const int32_t t_security_id_,  const TradeType_t t_buysell_,double t_price_, const int32_t t_size_, const bool is_intermediate_message);
  void Ib_Tick_Size_Only_Update(const int32_t t_security_id_,  const TradeType_t t_buysell_, const int32_t t_size_,const bool is_intermediate_message);
  void Ib_Trade(const int32_t t_security_id_, const double trade_price_, const int32_t trade_size_,const TradeType_t t_buysell);
  private:

  inline void OnGlobalOrderChange(const unsigned int security_id, const TradeType_t t_buysell, const int t_inprice) {}
  // Dummy

  inline void OnPriceLevelNew(const unsigned int security_id, const TradeType_t t_buysell, const int t_level_added_,
                              const double price, const int new_size, const int new_ordercount,
                              const bool is_intermediate_message) {}

  inline void OnPriceLevelDelete(const unsigned int security_id, const TradeType_t t_buysell, const int level_removed,
                                 const double price, const bool is_intermediate_message) {}

  inline void OnPriceLevelChange(const unsigned int security_id, const TradeType_t t_buysell,
                                 const int t_level_changed_, const double price, const int new_size,
                                 const int new_ordercount, const bool is_intermediate_message) {}

  inline void OnPriceLevelDeleteFrom(const unsigned int security_id, const TradeType_t t_buysell,
                                     const int min_level_deleted, const bool is_intermediate_message) {}

  inline void OnPriceLevelDeleteThrough(const unsigned int security_id, const TradeType_t t_buysell,
                                        const int max_level_deleted, const bool is_intermediate_message) {}

  inline void OnPriceLevelOverlay(const unsigned int security_id, const TradeType_t t_buysell,
                                  const int level_overlayed, const double price, const int new_size,
                                  const int new_ordercount, const bool is_intermediate_message) {}
   void SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_) { std::cout << " CALLED CHILD EMPTY FUNC " << std::endl;}
   void OnIndexPrice(const unsigned int t_security_id_, double const t_spot_price_);
};
}
#endif  // IBKR_L1_PRICE_MARKET_VIEW_MANAGER_H_
