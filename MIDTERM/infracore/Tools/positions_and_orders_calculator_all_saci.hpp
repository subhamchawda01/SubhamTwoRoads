#ifndef _POSITIONS_AND_ORDERS_CALCULATOR_ALLSACI_HPP_
#define _POSITIONS_AND_ORDERS_CALCULATOR_ALLSACI_HPP_

#include <map>

#include "dvccode/ORSMessages/ors_message_listener.hpp"
#include "infracore/Tools/positions_and_orders_calculator.hpp"

namespace HFSAT {
class PositionAndOrderCalculatorAllSACI : public ORSMessagesListener {
 public:
  PositionAndOrderCalculatorAllSACI(int date, std::string short_code);
  ~PositionAndOrderCalculatorAllSACI();

  void ORSMessageBegin(const unsigned int _security_id_, const GenericORSReplyStruct& ors_reply) override;

  void ORSMessageEnd(const unsigned int _security_id_, const GenericORSReplyStruct& ors_reply) override;

  void OrderSequencedAtTime(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                            const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                            const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                            const int _size_executed_, const int _client_position_, const int _global_position_,
                            const int r_int_price_, const uint64_t exchange_order_id,
                            const ttime_t time_set_by_server) override;

  void OrderORSConfirmed(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                         const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                         const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                         const int _size_executed_, const int r_int_price_,
                         const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                         const ttime_t time_set_by_server) override;

  void OrderConfirmedAtTime(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                            const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                            const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                            const int _size_executed_, const int _client_position_, const int _global_position_,
                            const int r_int_price_, const uint64_t exchange_order_id,
                            const ttime_t time_set_by_server) override;

  void OrderCxlSequencedAtTime(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                               const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                               const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                               const int _client_position_, const int _global_position_, const int r_int_price_,
                               const uint64_t exchange_order_id, const ttime_t time_set_by_server) override;

  void OrderCanceledAtTime(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                           const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                           const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                           const int _client_position_, const int _global_position_, const int r_int_price_,
                           const uint64_t exchange_order_id, const ttime_t time_set_by_server) override;

  void OrderCancelRejected(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                           const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                           const double _price_, const TradeType_t t_buysell_, const int _size_remaining_,
                           const int _rejection_reason_, const int t_client_position_, const int t_global_position_,
                           const int r_int_price_, const uint64_t exchange_order_id,
                           const ttime_t time_set_by_server) override;

  void OrderExecuted(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
                     const TradeType_t r_buysell_, const int _size_remaining_, const int _size_executed_,
                     const int _client_position_, const int _global_position_, const int r_int_price_,
                     const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                     const ttime_t time_set_by_server) override;

  void OrderInternallyMatched(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                              const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                              const double _price_, const TradeType_t r_buysell_, const int _size_remaining_,
                              const int _size_executed_, const int _client_position_, const int _global_position_,
                              const int r_int_price_, const int32_t server_assigned_message_sequence,
                              const uint64_t exchange_order_id, const ttime_t time_set_by_server) override;

  void OrderRejected(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const unsigned int _security_id_, const double _price_, const TradeType_t r_buysell_,
                     const int _size_remaining_, const int _rejection_reason_, const int r_int_price_,
                     const uint64_t exchange_order_id, const ttime_t time_set_by_server) override;

 private:
  void CheckandCreateInstance(const int server_assigned_client_id_);
  std::map<int, PositionAndOrderCalculator*> saci_position_and_order_calculator_map;
  int date_;
  std::string shortcode_;
};
}

#endif
