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
#include "dvccode/CommonDataStructures/simple_mempool.hpp"
#include "dvccode/CommonTradeUtils/trade_time_manager.hpp"
#include "baseinfra/MarketAdapter/sparse_index_book_pxlevel_manager.hpp"
#include "baseinfra/MarketAdapter/order_history_cache.hpp"

namespace HFSAT {
class IndexedNSEMarketViewManager : public OrderGlobalListenerNSE,
                                    public BaseMarketViewManager,
                                    public GlobalOrderChangeListener {
 public:
  IndexedNSEMarketViewManager(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                              const SecurityNameIndexer& t_sec_name_indexer_,
                              const std::vector<SecurityMarketView*>& t_security_market_view_map_);

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
  void OnTradeExecRange(const unsigned int t_security_id_, double const t_low_exec_band_, double const t_high_exec_band_) {}

 private:
  void ModifyOrderSize(const uint32_t t_security_id_, const uint64_t order_id_, const int size_diff,
                       const bool t_is_intermediate_);
  void PredictTrade(const uint32_t t_security_id_, const TradeType_t t_buysell_, const int t_int_price_,
                    const uint32_t t_size_, const bool t_is_intermediate_);

  void SanitizeBook(const uint32_t t_security_id_, const int t_sanitize_price_);
  void UpdateSanitizeVarsOnBaseL1Change(const uint32_t t_security_id_, const TradeType_t t_buysell_);

  bool IsBookCrossed();
  bool CheckCrossedBookInstance(const uint32_t t_security_id_, const TradeType_t t_side_, const int t_crossed_int_px_,
                                const int t_sanitize_price_, const int t_size_at_crossed_px_);

  void NotifyListenersOnLevelChange(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                                    bool t_is_intermediate_);
  void NotifyListenersOnTrade(const uint32_t t_security_id_, const int t_trade_int_price_, const int t_trade_size_,
                              const TradeType_t t_buysell_);
  void OnGlobalOrderChange(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_int_price_) {}

  bool CheckValidTime(int sec_id);
  void SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_);
  bool IsCrossingIndex(double price, TradeType_t t_buysell_);
  bool GetUncrossedSynVars(const int t_security_id_, const bool t_is_intermediate_);
  uint32_t GetNextIndex(uint32_t base_index_, TradeType_t t_buysell_);
  void CheckIfL1Changed(const int t_security_id_);

  // debug functions
  // Checks consistency of L1 book of indexed_book_manager and px_level_book_manager
  void CheckL1Consistency(int security_id);

 private:
  // Reason for having the changed variables at class level is to incorporate the effect of intermediate messages
  std::vector<bool> l1_price_changed_;
  std::vector<bool> l1_size_changed_;

  /// variables to help sanitize the book if needed
  std::vector<TradeType_t> sid_to_side_to_be_sanitized_;
  std::vector<int> sid_to_sanitize_count_;
  std::vector<int> sid_to_int_px_to_sanitize_;
  std::vector<int> sid_to_msecs_at_first_cross_;
  std::vector<int> sid_to_size_at_sanitize_px_;

  TradeTimeManager& trade_time_manager_;
  std::vector<bool> currently_trading_;

  OrderHistory order_history_instance;
  SecurityMarketView* market_view_ptr_;
};
}
