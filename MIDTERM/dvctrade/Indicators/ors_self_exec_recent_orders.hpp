/**
    \file Indicators/ors_self_exec_recent_orders.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once

#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"

namespace HFSAT {

/// Book Pressure Indicator
/// returning ewma_bid_size - ewma_ask-size
/// the decay-factor being pow ( sqrt ( decay_factor ), 2 * ( number of ticks from mid ) )
/// or pow ( decay_factor, number of tikcs from mid )
class ORSSelfExecRecentOrders : public CommonIndicator {
 protected:
  // variables
  SecurityMarketView& indep_market_view_;

  double factor_reduced_;
  double recent_order_threshold_;

  // computational variables
  std::map<unsigned int, unsigned int> recent_bid_orders_map_;  // saos timestamps for recent bid orders
  std::map<unsigned int, unsigned int> recent_ask_orders_map_;  // saos timestamps for recent ask orders
                                                                // functions
 protected:
  ORSSelfExecRecentOrders(DebugLogger& _dbglogger_, const Watch& _watch_,
                          const std::string& concise_indicator_description_, SecurityMarketView& _indep_market_view_,
                          double factor_reduced_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static ORSSelfExecRecentOrders* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                    const std::vector<const char*>& _tokens_,
                                                    PriceType_t _basepx_type_);

  static ORSSelfExecRecentOrders* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                    SecurityMarketView& _indep_market_view_, double _factor_reduced_);

 public:
  ~ORSSelfExecRecentOrders() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  void OrderExecuted(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
                     const TradeType_t r_buysell_, const int _size_remaining_, const int _size_executed_,
                     const int _client_position_, const int _global_position_, const int r_int_price_,
                     const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                     const ttime_t time_set_by_server) override;

  void OrderConfirmed(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                      const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                      const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                      const int _size_executed_, const int _client_position_, const int _global_position_,
                      const int r_int_price_, const int32_t server_assigned_message_sequence,
                      const uint64_t exchange_order_id, const ttime_t time_set_by_server) override;

  void OrderORSConfirmed(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                         const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                         const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                         const int _size_executed_, const int r_int_price_,
                         const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                         const ttime_t time_set_by_server) override;

  void OrderCanceled(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
                     const TradeType_t r_buysell_, const int _size_remaining_, const int _client_position_,
                     const int _global_position_, const int r_int_price_,
                     const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                     const ttime_t time_set_by_server) override;

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

  // functions
  static std::string VarName() { return "ORSSelfExecRecentOrders"; }
};
}
