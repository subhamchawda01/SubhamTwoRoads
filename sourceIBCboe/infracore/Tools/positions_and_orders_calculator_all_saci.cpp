#include <iostream>

#include "infracore/Tools/positions_and_orders_calculator_all_saci.hpp"

namespace HFSAT {

PositionAndOrderCalculatorAllSACI::PositionAndOrderCalculatorAllSACI(int date, std::string shortcode)
    : date_(date), shortcode_(shortcode) {}

PositionAndOrderCalculatorAllSACI::~PositionAndOrderCalculatorAllSACI() {
  for (auto iter = saci_position_and_order_calculator_map.begin(); iter != saci_position_and_order_calculator_map.end();
       iter++) {
    delete iter->second;
    iter->second = nullptr;
  }
}

void PositionAndOrderCalculatorAllSACI::ORSMessageBegin(const unsigned int _security_id_,
                                                        const GenericORSReplyStruct& ors_reply) {
  CheckandCreateInstance(ors_reply.server_assigned_client_id_);
  saci_position_and_order_calculator_map[ors_reply.server_assigned_client_id_]->ORSMessageBegin(_security_id_,
                                                                                                ors_reply);
}

void PositionAndOrderCalculatorAllSACI::ORSMessageEnd(const unsigned int _security_id_,
                                                      const GenericORSReplyStruct& ors_reply) {
  CheckandCreateInstance(ors_reply.server_assigned_client_id_);
  if (saci_position_and_order_calculator_map.find(ors_reply.server_assigned_client_id_) !=
      saci_position_and_order_calculator_map.end()) {
    saci_position_and_order_calculator_map[ors_reply.server_assigned_client_id_]->ORSMessageEnd(_security_id_,
                                                                                                ors_reply);
  }
}

void PositionAndOrderCalculatorAllSACI::OrderSequencedAtTime(
    const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t r_buysell_, const int _size_remaining_, const int _size_executed_, const int _client_position_,
    const int _global_position_, const int r_int_price_, const uint64_t exchange_order_id,
    const ttime_t time_set_by_server) {
  CheckandCreateInstance(t_server_assigned_client_id_);
  if (saci_position_and_order_calculator_map.find(t_server_assigned_client_id_) !=
      saci_position_and_order_calculator_map.end()) {
    saci_position_and_order_calculator_map[t_server_assigned_client_id_]->OrderSequencedAtTime(
        t_server_assigned_client_id_, _client_assigned_order_sequence_, _server_assigned_order_sequence_, _security_id_,
        _price_, r_buysell_, _size_remaining_, _size_executed_, _client_position_, _global_position_, r_int_price_,
        exchange_order_id, time_set_by_server);
  }
}

void PositionAndOrderCalculatorAllSACI::OrderORSConfirmed(
    const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t r_buysell_, const int _size_remaining_, const int _size_executed_, const int r_int_price_,
    const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
    const ttime_t time_set_by_server) {
  CheckandCreateInstance(t_server_assigned_client_id_);
  if (saci_position_and_order_calculator_map.find(t_server_assigned_client_id_) !=
      saci_position_and_order_calculator_map.end()) {
    saci_position_and_order_calculator_map[t_server_assigned_client_id_]->OrderORSConfirmed(
        t_server_assigned_client_id_, _client_assigned_order_sequence_, _server_assigned_order_sequence_, _security_id_,
        _price_, r_buysell_, _size_remaining_, _size_executed_, r_int_price_, server_assigned_message_sequence,
        exchange_order_id, time_set_by_server);
  }
}

void PositionAndOrderCalculatorAllSACI::OrderConfirmedAtTime(
    const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t r_buysell_, const int _size_remaining_, const int _size_executed_, const int _client_position_,
    const int _global_position_, const int r_int_price_, const uint64_t exchange_order_id,
    const ttime_t time_set_by_server) {
  CheckandCreateInstance(t_server_assigned_client_id_);
  if (saci_position_and_order_calculator_map.find(t_server_assigned_client_id_) !=
      saci_position_and_order_calculator_map.end()) {
    saci_position_and_order_calculator_map[t_server_assigned_client_id_]->OrderConfirmedAtTime(
        t_server_assigned_client_id_, _client_assigned_order_sequence_, _server_assigned_order_sequence_, _security_id_,
        _price_, r_buysell_, _size_remaining_, _size_executed_, _client_position_, _global_position_, r_int_price_,
        exchange_order_id, time_set_by_server);
  }
}

void PositionAndOrderCalculatorAllSACI::OrderCxlSequencedAtTime(
    const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t r_buysell_, const int _size_remaining_, const int _client_position_, const int _global_position_,
    const int r_int_price_, const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  CheckandCreateInstance(t_server_assigned_client_id_);
  if (saci_position_and_order_calculator_map.find(t_server_assigned_client_id_) !=
      saci_position_and_order_calculator_map.end()) {
    saci_position_and_order_calculator_map[t_server_assigned_client_id_]->OrderCxlSequencedAtTime(
        t_server_assigned_client_id_, _client_assigned_order_sequence_, _server_assigned_order_sequence_, _security_id_,
        _price_, r_buysell_, _size_remaining_, _client_position_, _global_position_, r_int_price_, exchange_order_id,
        time_set_by_server);
  }
}

void PositionAndOrderCalculatorAllSACI::OrderCanceledAtTime(
    const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t r_buysell_, const int _size_remaining_, const int _client_position_, const int _global_position_,
    const int r_int_price_, const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  CheckandCreateInstance(t_server_assigned_client_id_);
  if (saci_position_and_order_calculator_map.find(t_server_assigned_client_id_) !=
      saci_position_and_order_calculator_map.end()) {
    saci_position_and_order_calculator_map[t_server_assigned_client_id_]->OrderCanceledAtTime(
        t_server_assigned_client_id_, _client_assigned_order_sequence_, _server_assigned_order_sequence_, _security_id_,
        _price_, r_buysell_, _size_remaining_, _client_position_, _global_position_, r_int_price_, exchange_order_id,
        time_set_by_server);
  }
}

void PositionAndOrderCalculatorAllSACI::OrderCancelRejected(
    const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t t_buysell_, const int _size_remaining_, const int _rejection_reason_,
    const int t_client_position_, const int t_global_position_, const int r_int_price_,
    const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  CheckandCreateInstance(t_server_assigned_client_id_);
  if (saci_position_and_order_calculator_map.find(t_server_assigned_client_id_) !=
      saci_position_and_order_calculator_map.end()) {
    saci_position_and_order_calculator_map[t_server_assigned_client_id_]->OrderCancelRejected(
        t_server_assigned_client_id_, _client_assigned_order_sequence_, _server_assigned_order_sequence_, _security_id_,
        _price_, t_buysell_, _size_remaining_, _rejection_reason_, t_client_position_, t_global_position_, r_int_price_,
        exchange_order_id, time_set_by_server);
  }
}

void PositionAndOrderCalculatorAllSACI::OrderExecuted(
    const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t r_buysell_, const int _size_remaining_, const int _size_executed_, const int _client_position_,
    const int _global_position_, const int r_int_price_, const int32_t server_assigned_message_sequence,
    const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  CheckandCreateInstance(t_server_assigned_client_id_);
  if (saci_position_and_order_calculator_map.find(t_server_assigned_client_id_) !=
      saci_position_and_order_calculator_map.end()) {
    saci_position_and_order_calculator_map[t_server_assigned_client_id_]->OrderExecuted(
        t_server_assigned_client_id_, _client_assigned_order_sequence_, _server_assigned_order_sequence_, _security_id_,
        _price_, r_buysell_, _size_remaining_, _size_executed_, _client_position_, _global_position_, r_int_price_,
        server_assigned_message_sequence, exchange_order_id, time_set_by_server);
  }
}

void PositionAndOrderCalculatorAllSACI::OrderInternallyMatched(
    const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
    const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
    const TradeType_t r_buysell_, const int _size_remaining_, const int _size_executed_, const int _client_position_,
    const int _global_position_, const int r_int_price_, const int32_t server_assigned_message_sequence,
    const uint64_t exchange_order_id, const ttime_t time_set_by_server) {
  CheckandCreateInstance(t_server_assigned_client_id_);
  if (saci_position_and_order_calculator_map.find(t_server_assigned_client_id_) !=
      saci_position_and_order_calculator_map.end()) {
    saci_position_and_order_calculator_map[t_server_assigned_client_id_]->OrderInternallyMatched(
        t_server_assigned_client_id_, _client_assigned_order_sequence_, _server_assigned_order_sequence_, _security_id_,
        _price_, r_buysell_, _size_remaining_, _size_executed_, _client_position_, _global_position_, r_int_price_,
        server_assigned_message_sequence, exchange_order_id, time_set_by_server);
  }
}

void PositionAndOrderCalculatorAllSACI::OrderRejected(const int t_server_assigned_client_id_,
                                                      const int _client_assigned_order_sequence_,
                                                      const unsigned int _security_id_, const double _price_,
                                                      const TradeType_t r_buysell_, const int _size_remaining_,
                                                      const int _rejection_reason_, const int r_int_price_,
                                                      const uint64_t exchange_order_id,
                                                      const ttime_t time_set_by_server) {
  CheckandCreateInstance(t_server_assigned_client_id_);
  if (saci_position_and_order_calculator_map.find(t_server_assigned_client_id_) !=
      saci_position_and_order_calculator_map.end()) {
    saci_position_and_order_calculator_map[t_server_assigned_client_id_]->OrderRejected(
        t_server_assigned_client_id_, _client_assigned_order_sequence_, _security_id_, _price_, r_buysell_,
        _size_remaining_, _rejection_reason_, r_int_price_, exchange_order_id, time_set_by_server);
  }
}

void PositionAndOrderCalculatorAllSACI::CheckandCreateInstance(const int server_assigned_client_id_) {
  if (saci_position_and_order_calculator_map.find(server_assigned_client_id_) ==
      saci_position_and_order_calculator_map.end()) {
    saci_position_and_order_calculator_map[server_assigned_client_id_] =
        new PositionAndOrderCalculator(date_, shortcode_);
  }
}
}
