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
#include <list>

#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/trade_time_manager.hpp"
#include "baseinfra/MarketAdapter/sparse_index_book_pxlevel_manager.hpp"
#include "baseinfra/MarketAdapter/order_history_cache.hpp"
#include "baseinfra/TradeUtils/big_trades_listener.hpp"
// listens to ORS messages for supoprting non-self book
#include "dvccode/ORSMessages/ors_message_listener.hpp"

namespace HFSAT {

class IndexedNSEMarketViewManager2 : public OrderGlobalListenerNSE,
                                     public BaseMarketViewManager,
                                     public OrderCanceledListener {
 public:
  IndexedNSEMarketViewManager2(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                               const SecurityNameIndexer& t_sec_name_indexer_,
                               const std::vector<SecurityMarketView*>& t_security_market_view_map_,
                               bool _use_self_book_);

  void OnOrderAdd(const uint32_t t_security_id_, const TradeType_t t_buysell_, const uint64_t t_order_id_,
                  const double t_price_, const uint32_t t_size_, const bool t_is_intermediate_);
  void OnOrderModify(const uint32_t t_security_id_, const TradeType_t t_buysell_, const uint64_t t_order_id_,
                     const double t_price_, const uint32_t t_size_);
  void OnOrderDelete(const uint32_t t_security_id_, const TradeType_t t_buysell_, const uint64_t t_order_id_,
                     const double t_price_, const bool t_delete_order_, const bool t_is_intermediate_);
  void OnTrade(const unsigned int t_security_id_, double const t_trade_price_, const int t_trade_size_,
               const uint64_t t_buy_order_num_, const uint64_t t_sell_order_num_);
  void OnHiddenTrade(const unsigned int t_security_id_, double const t_trade_price_, const int t_trade_size_,
                     const uint64_t t_buy_order_num_, const uint64_t t_sell_order_num_);
  void OnTradeExecRange(const unsigned int t_security_id_, double const t_low_exec_band_, double const t_high_exec_band_);

  // utility functions for direct use by strategies etc
  MarketUpdateInfoLevelStruct* GetLowerBidIntPL(const unsigned int t_security_id_, int price_) {
    return (px_level_manager_.underlying_bid_ds_)->GetLowerPriceLevel(t_security_id_, price_);
  }

  MarketUpdateInfoLevelStruct* GetHigherAskIntPL(const unsigned int t_security_id_, int price_) {
    return (px_level_manager_.underlying_ask_ds_)->GetHigherPriceLevel(t_security_id_, price_);
  }

  MarketUpdateInfoLevelStruct* GetSynBidPL(const unsigned int t_security_id_, int level_) {
    return px_level_manager_.GetSynBidPL(t_security_id_, level_);
  }

  MarketUpdateInfoLevelStruct* GetSynAskPL(const unsigned int t_security_id_, int level_) {
    return px_level_manager_.GetSynAskPL(t_security_id_, level_);
  }

  void DumpBook(int sec_id_, int t_depth_ = 5) { px_level_manager_.DumpBook(sec_id_, t_depth_); }

  int GetIntPxForOrderId(int sec_id_, uint64_t order_id_) {
    bool is_order_seen = order_history_instance.IsOrderSeen(sec_id_, order_id_);
    if (is_order_seen) {
      OrderDetailsStruct* t_order_ = order_history_instance.GetOrderDetails(sec_id_, order_id_);
      return security_market_view_map_[sec_id_]->GetIntPx(t_order_->order_price);
    }
    return kInvalidIntPrice;
  }

  // if we want non self book to only filter orderids of same strategy we need to
  // populate the self_trader_ids _ vector with appropriate ids
  void AddSelfTraderId(int trader_id_) { self_trader_ids_.insert(trader_id_); }

  void AddBigTradesListener(BigTradesListener* _p_this_listener_, unsigned int security_id_) {
    if (_p_this_listener_ == NULL) return;
    VectorUtils::UniqueVectorAdd(big_trades_listener_vec_[security_id_], _p_this_listener_);
  }

 private:
  void TriggerTrade(const uint32_t t_security_id_, const TradeType_t t_buysell_, const int t_int_price_,
                    const uint32_t t_size_, int t_int_price_level_);

  // Checks to see if book has been crossed for a sufficient time and sanitizes if needed
  void SanitizeBookOnCrossedBook(const uint32_t t_security_id_);
  // Checks to see if trade occurs at sub-best level and sanitizes if needed
  void SanitizeBookOnCrossedTrade(const uint32_t t_security_id_, double t_tradepx_);
  // Underlying function called by SanitizeBookOnCrossedBook/Trade
  void SanitizeBook(const uint32_t t_security_id_, HFSAT::TradeType_t t_sanitize_side_, double t_sanitize_price_);

  void NotifyListenersOnLevelChange(const uint32_t t_security_id_, LevelChangeType_t change_type);
  void NotifyListenersOnTrade(const uint32_t t_security_id_, const int t_trade_int_price_, const int t_trade_size_,
                              const TradeType_t t_buysell_, bool is_intermediate, int _num_levels_cleared_);

  bool CheckValidTime(int sec_id);
  void SetSMVBestVars(int sec_id);
  void SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_);

  // debug functions
  // Checks consistency of L1 book of indexed_book_manager and px_level_book_manager
  void CheckL1Consistency(int security_id);

  // returns the price ( inclusive ) till which level book will be sanitized
  double GetSanitizePx(int security_id_, TradeType_t sanitize_side_,
                       MarketUpdateInfoLevelStruct* best_crossed_bid_level_,
                       MarketUpdateInfoLevelStruct* best_crossed_ask_level_);

  // ORS functions to support for self book maintenance
  void OrderConfirmed(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                      const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                      const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                      const int _size_executed_, const int _client_position_, const int _global_position_,
                      const int r_int_price_, const int32_t server_assigned_message_sequence,
                      const uint64_t exchange_order_id, const ttime_t time_set_by_server);

  inline void OrderORSConfirmed(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                const double _price_, const TradeType_t _buysell_, const int _size_remaining_,
                                const int _size_executed_, const int _int_price_,
                                const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                                const ttime_t time_set_by_server) {}

  void OrderCanceled(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
                     const TradeType_t r_buysell_, const int _size_remaining_, const int _client_position_,
                     const int _global_position_, const int r_int_price_,
                     const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                     const ttime_t time_set_by_server);

  void OrderCancelRejected(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                           const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                           const double _price_, const TradeType_t t_buysell_, const int _size_remaining_,
                           const int _rejection_reason_, const int t_client_position_, const int t_global_position_,
                           const int r_int_price_, const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  }

  void OrderExecuted(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
                     const TradeType_t r_buysell_, const int _size_remaining_, const int _size_executed_,
                     const int _client_position_, const int _global_position_, const int r_int_price_,
                     const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                     const ttime_t time_set_by_server);

 private:
  TradeTimeManager& trade_time_manager_;
  OrderHistory order_history_instance;
  OrderHistory self_order_history_instance;  // maintaining self orders for supporting non self book
  SecurityMarketView* market_view_ptr_;

  SparseIndexBookPxLevelManager px_level_manager_;

  // variables to support predict trade
  int pre_predicted_best_bid_int_price_;
  int pre_predicted_best_bid_size_;
  int pre_predicted_best_bid_lvl_;
  int pre_predicted_best_ask_int_price_;
  int pre_predicted_best_ask_size_;
  int pre_predicted_best_ask_lvl_;

  bool use_self_book_;
  int big_aggressive_order_size_limit_;
  std::vector<std::vector<BigTradesListener*> > big_trades_listener_vec_;

  // support for removing self orders of just same strategy instead of same multicast channel
  std::set<int> self_trader_ids_;
};
}
