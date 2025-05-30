/**
    \file MinuteBar/minute_bar_sim_trader.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#include "baseinfra/MinuteBar/minute_bar_sim_trader.hpp"

namespace HFSAT {

void MinuteBarSimTrader::BroadcastRejection(const int saci, const BaseOrder* sim_order,
                                            ORSRejectionReason_t reject_reason) {
  for (auto rejc_listener : order_rejected_listener_vec_) {
    rejc_listener->OrderRejected(saci, sim_order->client_assigned_order_sequence(), dep_security_id_,
                                 sim_order->price(), sim_order->buysell(), sim_order->size_remaining(), reject_reason,
                                 sim_order->int_price_, 0, ttime_t(0, 0));
  }
}

void MinuteBarSimTrader::BroadcastSequenced(const int saci, const BaseOrder* sim_order) {
  for (auto seqd_listener : order_sequenced_listener_vec_) {
    seqd_listener->OrderSequenced(saci, sim_order->client_assigned_order_sequence(),
                                  sim_order->server_assigned_order_sequence(), dep_security_id_, sim_order->price(),
                                  sim_order->buysell(), sim_order->size_remaining(), sim_order->size_executed(),
                                  client_position_, client_position_, sim_order->int_price_, 0, 0, ttime_t(0, 0));
  }
}

void MinuteBarSimTrader::BroadcastConfirm(const int saci, const BaseOrder* sim_order) {
  for (auto conf_listener : order_confirmed_listener_vec_) {
    conf_listener->OrderConfirmed(saci, sim_order->client_assigned_order_sequence(),
                                  sim_order->server_assigned_order_sequence(), dep_security_id_, sim_order->price(),
                                  sim_order->buysell(), sim_order->size_remaining(), sim_order->size_executed(),
                                  client_position_, client_position_, sim_order->int_price_, 0, 0, ttime_t(0, 0));
  }
}

void MinuteBarSimTrader::BroadcastCancelNotification(const int saci, const BaseOrder* sim_order) {
  for (auto cxl_listener : order_canceled_listener_vec_) {
    cxl_listener->OrderCanceled(saci, sim_order->client_assigned_order_sequence(),
                                sim_order->server_assigned_order_sequence(), dep_security_id_, sim_order->price(),
                                sim_order->buysell(), sim_order->size_remaining(), client_position_, client_position_,
                                sim_order->int_price_, 0, 0, ttime_t(0, 0));
  }
}

void MinuteBarSimTrader::BroadcastExecNotification(const int saci, const BaseOrder* sim_order) {
  for (auto exec_listener : order_executed_listener_vec_) {
    exec_listener->OrderExecuted(saci, sim_order->client_assigned_order_sequence(),
                                 sim_order->server_assigned_order_sequence(), dep_security_id_, sim_order->price(),
                                 sim_order->buysell(), sim_order->size_remaining(), sim_order->size_executed(),
                                 client_position_, client_position_, sim_order->int_price_, 0, 0, ttime_t(0, 0));
  }
}
}
