/**
    \file MarketAdapter/indexed_nse_market_view_manager.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#pragma once

#include <set>

#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvccode/CommonDataStructures/simple_mempool.hpp"
#include "baseinfra/MarketAdapter/trade_time_manager.hpp"

namespace HFSAT {

class IndexedNSEMarketViewManager : public OrderGlobalListenerNSE,
                                    public BaseMarketViewManager,
                                    public GlobalOrderChangeListener {
 private:
  // Reason for having the changed variables at class level is to incorporate the effect of intermediate messages
  std::vector<bool> l1_price_changed_;
  std::vector<bool> l1_size_changed_;

  std::vector<bool> sec_id_to_prev_update_was_quote_;  // for setting of a variable in ontrade

  std::vector<bool> sec_id_to_mask_levels_above_;  // Used for checking whether to mask levels after an order exec
  std::vector<bool> skip_notifications_;

  typedef struct {
    bool is_buy_order_;
    int32_t last_order_size_;  // remainint time being tracked
    uint64_t order_id_;
    double order_price_;
  } order_details_struct_;

  SimpleMempool<order_details_struct_> order_struct_mempool_;
  std::vector<std::map<uint64_t, order_details_struct_*> > sid_to_queued_orders_maps_vec_;
  std::vector<std::map<uint64_t, order_details_struct_*> > sid_to_live_orders_maps_vec_;

  /// variables to help sanitize the book if needed
  std::vector<TradeType_t> sid_to_side_to_be_sanitized_;
  std::vector<int> sid_to_sanitize_count_;
  std::vector<int> sid_to_int_px_to_sanitize_;
  std::vector<int> sid_to_msecs_at_first_cross_;
  std::vector<int> sid_to_size_at_sanitize_px_;

  // Changed to set as it's more suited to purpose,
  // Potenially there can be multiple instances of book running and replicating same day entry in-file before
  // they find it already there but nothing we care about
  std::set<int> crossed_data_days_;
  // we check whether it is live/sim the first time we have a sanitization case.
  // It is not possible to check in constructor since watch value will be uninitialized.
  bool is_live_;
  bool checked_for_live_;
  // To ensure that multiple email alarms are not sent.
  bool sent_alarm_in_hist_;
  TradeTimeManager& trade_time_manager_;
  std::vector<bool> currently_trading_;

  uint64_t last_trade_buy_order_num_;
  int32_t last_trade_buy_size_remaining_;
  uint64_t last_trade_sell_order_num_;
  int32_t last_trade_sell_size_remaining_;

 public:
  IndexedNSEMarketViewManager(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                              const SecurityNameIndexer& t_sec_name_indexer_,
                              const std::vector<SecurityMarketView*>& t_security_market_view_map_);

  void OnOrderAdd(const uint32_t t_security_id_, const TradeType_t t_buysell_, const uint64_t t_order_id_,
                  const double t_price_, const uint32_t t_size_, const bool t_is_intermediate_);
  void OnOrderModify(const uint32_t t_security_id_, const TradeType_t t_buysell_, const uint64_t t_order_id_,
                     const double t_price_, const uint32_t t_size_, const double t_prev_price_,
                     const uint32_t t_prev_size_);
  void OnOrderDelete(const uint32_t t_security_id_, const TradeType_t t_buysell_, const uint64_t t_order_id_,
                     const double t_price_, const int t_size_, const bool t_delete_order_,
                     const bool t_is_intermediate_);
  void OnTrade(const unsigned int t_security_id_, double const t_trade_price_, const int t_trade_size_,
               const uint64_t t_buy_order_num_, const uint64_t t_sell_order_num_, const int32_t t_bid_size_remaining_,
               const int32_t t_ask_size_remaining_);

  void AddOrderToLiveOrdersMap(const uint32_t t_security_id_, const TradeType_t t_buysell_, const double t_price_,
                               const uint32_t t_size_, const uint64_t t_order_id_);
  void AddOrderToQueuedOrdersMap(const uint32_t t_security_id_, const TradeType_t t_buysell_, const double t_price_,
                                 const uint32_t t_size_, const uint64_t t_order_id_);

  void UpdateOrderMapsOnTrade(const uint32_t t_security_id_, const uint64_t t_buy_order_id_,
                              const uint64_t t_sell_order_id_, const uint32_t t_size_, const double t_price_);

  void UpdateOrderMapsOnModify(const uint32_t t_security_id_, const uint64_t t_order_id_, const uint32_t t_size_);
  bool OrderIsQueued(const uint32_t t_security_id_, const uint64_t t_order_id_);
  bool OrderIsLive(const uint32_t t_security_id_, const uint64_t t_order_id_, double* t_price_);

  void OnHiddenTrade(const unsigned int t_security_id_, double const t_trade_price_, const int t_trade_size_,
                     const uint64_t t_buy_order_num_, const uint64_t t_sell_order_num_,
                     const int32_t t_bid_size_remaining_, const int32_t t_ask_size_remaining_);

  // return true if deleted order was not live ( i.e. in queuedordersmap )
  bool UpdateOrderMapsOnDelete(const uint32_t t_security_id_, const TradeType_t t_buysell_, const uint64_t t_order_id_,
                               const int t_size_, const bool t_delete_order_);

  void AddSimulatedOrders(const uint32_t t_security_id_);

  void OnGlobalOrderChange(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_int_price_);

  void SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_);

  /// functions to help sanitize
  bool IsRunningInLive();
  void SanitizeBook(const uint32_t t_security_id_, const int t_sanitize_price_);
  void LoadCrossedDataDays();
  bool CheckCrossedBookInstance(const uint32_t t_security_id_, const TradeType_t t_side_, const int t_crossed_int_px_,
                                const int t_sanitize_price_, const int t_size_at_crossed_px_);
  void AlertInHist(const uint32_t t_security_id_);
  void AlertInLive(const uint32_t t_security_id_);
  void UpdateSanitizeVarsOnL1Change(const uint32_t t_security_id_);
  bool CheckValidTime(int sec_id);
};
}
