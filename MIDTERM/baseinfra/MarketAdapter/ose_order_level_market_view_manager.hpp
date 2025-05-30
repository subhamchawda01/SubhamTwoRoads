/**
   \file MarketAdapter/ose_order_level_market_view_manager.hpp
   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_MARKETADAPTER_OSE_ORDER_LEVEL_MARKET_VIEW_MANAGER_H
#define BASE_MARKETADAPTER_OSE_ORDER_LEVEL_MARKET_VIEW_MANAGER_H

#include <vector>
#include <map>

#include "dvccode/CommonDataStructures/simple_mempool.hpp"

#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/trade_time_manager.hpp"
namespace HFSAT {

class OSEOrderLevelMarketViewManager : public BaseMarketViewManager,
                                       public OrderLevelGlobalListenerOSE,
                                       public GlobalOrderChangeListener {
  /// The main reason to keep this struct is that when we get
  /// an OnOrderLevelDelete message we do not get the size of the order.
  /// We get the level + orderid(?)
  struct OSEMapOrder {
    uint64_t order_id_;  // Order ids for GTS are too large to be accomodated in an int.
    int int_price_;
    int d_price_;
    int size_;
    TradeType_t buysell_;

    OSEMapOrder() {}

    OSEMapOrder(uint64_t t_order_id_, int t_int_price_, int t_price_, int t_size_, TradeType_t t_buysell_)
        : order_id_(t_order_id_), int_price_(t_int_price_), d_price_(t_price_), size_(t_size_), buysell_(t_buysell_) {}
    /*
    // Destructor may need to be private, since there is a possibility that std :: vector :: erase () in
    OrderOrderLevelDelete () will
    // call the destructor and damage this object which is reused by the SimpleMempool.
    private:
    ~OSEMapOrder () {
    std :: cout << "Destructor called. Bad news.\n";
    }
    */

    void set(uint64_t t_order_id_, int t_int_price_, int t_price_, int t_size_, TradeType_t t_buysell_) {
      int_price_ = t_int_price_;
      d_price_ = t_price_;
      order_id_ = t_order_id_;

      size_ = t_size_;
      buysell_ = t_buysell_;
    }

    std::string ToString() {
      std::ostringstream t_temp_oss_;

      t_temp_oss_ << order_id_;
      t_temp_oss_.width(6);
      t_temp_oss_ << int_price_;
      t_temp_oss_.width(10);
      t_temp_oss_ << d_price_;
      t_temp_oss_.width(5);
      t_temp_oss_ << size_;
      t_temp_oss_.width(3);
      t_temp_oss_ << ((buysell_ == kTradeTypeBuy) ? 'B' : 'S');

      return t_temp_oss_.str();
    }
  };

  /// This class holds all the info for 1 security
  struct OSEOrderMarketView {
    OSEOrderMarketView() {
      bid_order_depth_book_.clear();
      ask_order_depth_book_.clear();
    }

   public:
    // These structures maintain the order depth book for the bid side and the ask side.
    std::vector<OSEMapOrder *> bid_order_depth_book_;
    std::vector<OSEMapOrder *> ask_order_depth_book_;
  };

 protected:
  SimpleMempool<OSEMapOrder> osemaporder_mempool_;
  std::vector<OSEOrderMarketView> security_id_to_ose_omv_map_;  ///< map from security_id_

 public:
  HFSAT::TradeTimeManager &trade_time_manager_;
  OSEOrderLevelMarketViewManager(DebugLogger &t_dbglogger_, const Watch &t_watch_,
                                 const SecurityNameIndexer &t_sec_name_indexer_,
                                 const std::vector<SecurityMarketView *> &t_security_market_view_map_);

  void SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_);

  void OnOrderLevelNew(const unsigned int t_security_id_, uint64_t order_id_, const TradeType_t t_buysell_,
                       const int t_level_added_, const int t_price_, const int t_new_size_,
                       const bool t_is_intermediate_message_);
  void OnOrderLevelDelete(const unsigned int t_security_id_, uint64_t order_id_, const TradeType_t t_buysell_,
                          const int t_level_removed_, const int t_price_, const bool t_is_intermediate_message_);
  void OnOrderLevelChange(const unsigned int t_security_id_, uint64_t order_id_, const TradeType_t t_buysell_,
                          const int t_level_changed_, const int t_new_price_, const int t_new_size_,
                          const bool t_is_intermediate_message_);

  void OnOrderLevelSnapNew(const unsigned int t_security_id_, uint64_t order_id_, const TradeType_t t_buysell_,
                           const int t_level_added_, const int t_price_, const int t_new_size_,
                           const bool t_is_intermediate_message_);

  /// This has same signature as pricelevel messages
  void OnTrade(const unsigned int t_security_id_, const int t_trade_price_, const int t_trade_size_,
               const TradeType_t t_buysell_);
  inline bool IsNormalTradeTime(int securityId, ttime_t tv) {
    return trade_time_manager_.isValidTimeToTrade(securityId, tv.tv_sec % 86400);
  }
  inline void OnGlobalOrderChange(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                  const int t_int_price_) {}

  void resetBook(int sec_id_);

 protected:
  std::string ShowOrderDepthBook(const OSEOrderMarketView &this_omv_) const;
};
}
#endif  // BASE_MARKETADAPTER_OSE_ORDER_LEVEL_MARKET_VIEW_MANAGER_H
