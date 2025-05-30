/**
    \file MarketAdapter/indexed_eobi_market_view_manager.hpp

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

class IndexedEobiMarketViewManager : public OrderGlobalListenerEOBI,
                                     public BaseMarketViewManager,
                                     public GlobalOrderChangeListener {
 private:
  // Reason for having the changed variables at class level is to incorporate the effect of intermediate messages
  std::vector<bool> l1_price_changed_;
  std::vector<bool> l1_size_changed_;

  std::vector<bool> sec_id_to_prev_update_was_quote_;  // for setting of a variable in ontrade

  std::vector<bool> sec_id_to_mask_levels_above_;  // Used for checking whether to mask levels after full order
                                                   // exec/partial order exec
  std::vector<bool> skip_notifications_;

  std::vector<int> bid_side_exec_summary_seen_;
  std::vector<int> ask_side_exec_summary_seen_;

 public:
  IndexedEobiMarketViewManager(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                               const SecurityNameIndexer& t_sec_name_indexer_,
                               const std::vector<SecurityMarketView*>& t_security_market_view_map_);

  void OnOrderAdd(const uint32_t t_security_id_, const TradeType_t t_buysell_, const double t_price_,
                  const uint32_t t_size_, const bool t_is_intermediate_);
  void OnOrderModify(const uint32_t t_security_id_, const TradeType_t t_buysell_, const double t_price_,
                     const uint32_t t_size_, const double t_prev_price_, const uint32_t t_prev_size_);
  void OnOrderDelete(const uint32_t t_security_id_, const TradeType_t t_buysell_, const double t_price_,
                     const int t_size_, const bool t_delete_order_, const bool t_is_intermediate_);
  void OnOrderMassDelete(const uint32_t t_security_id_);
  void OnPartialOrderExecution(const uint32_t t_security_id_, const TradeType_t t_buysell_,
                               const double t_traded_price_, const uint32_t t_traded_size_);
  void OnFullOrderExecution(const uint32_t t_security_id_, const TradeType_t t_buysell_, const double t_traded_price_,
                            const uint32_t t_traded_size_);

  void OnExecutionSummary(const uint32_t t_security_id_, TradeType_t t_aggressor_side_, const double t_price_,
                          const uint32_t t_size_);

  void OnGlobalOrderChange(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_int_price_);

  void SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_);
};
}
