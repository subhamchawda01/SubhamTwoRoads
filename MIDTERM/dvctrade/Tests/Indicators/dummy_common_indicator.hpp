/*
 * dummy_common_indicator.hpp
 *
 *  Created on: 07-Jul-2017
 *      Author: anubhavpandey
 */

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/Tools/common_smv_source.hpp"

namespace HFTEST {

using namespace HFSAT;
class DummyCommonIndicator : public HFSAT::CommonIndicator {
 public:
  DummyCommonIndicator(DebugLogger& _dbglogger_, const Watch& _watch_,
                       const std::string& r_concise_indicator_description_)
      : CommonIndicator(_dbglogger_, _watch_, r_concise_indicator_description_) {}

  ~DummyCommonIndicator() {}
   void set_start_mfm(int32_t t_start_mfm_) {}
   void set_end_mfm(int32_t t_end_mfm_) {}
   void OnGlobalPositionChange(const unsigned int _security_id_, int _new_global_position_) {}
   void OrderExecuted(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                            const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                            const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                            const int _size_executed_, const int _client_position_, const int _global_position_,
                            const int r_int_price_, const int32_t server_assigned_message_sequence,
                            const uint64_t exchange_order_id, const ttime_t time_set_by_server) {}
   void OrderConfirmed(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                             const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                             const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                             const int _size_executed_, const int _client_position_, const int _global_position_,
                             const int r_int_price_, const int32_t server_assigned_message_sequence,
                             const uint64_t exchange_order_id, const ttime_t time_set_by_server) {}

   void OrderORSConfirmed(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                                const int _size_executed_, const int r_int_price_,
                                const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                                const ttime_t time_set_by_server) {}

   void OrderCanceled(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                            const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                            const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                            const int _client_position_, const int _global_position_, const int r_int_price_,
                            const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                            const ttime_t time_set_by_server) {}

   void OrderCancelRejected(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                  const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                  const double _price_, const TradeType_t t_buysell_, const int _size_remaining_,
                                  const int _rejection_reason_, const int t_client_position_,
                                  const int t_global_position_, const int r_int_price_,
                                  const uint64_t exchange_order_id, const ttime_t time_set_by_server) {}
   void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
   void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {}
   void OnMarketStatusChange(const unsigned int _security_id_, const MktStatus_t _new_market_status_) {}
   void OnPortfolioPriceChange(double _new_price_) {}
   void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {}
   void OnMarketDataResumed(const unsigned int _security_id_) {}

   void OrderConfirmedAtTime(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                   const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                   const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                                   const int _size_executed_, const int _client_position_, const int _global_position_,
                                   const int r_int_price_, const uint64_t exchange_order_id,
                                   const ttime_t _time_set_by_server_) {}

   void OrderCanceledAtTime(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                  const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                  const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                                  const int _client_position_, const int _global_position_, const int r_int_price_,
                                  const uint64_t exchange_order_id, const ttime_t time_set_by_server) {}
   void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}
};
}
