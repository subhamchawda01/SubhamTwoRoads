/**
    \file OrderRouting/base_sim_trader.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_ORDERROUTING_BASE_SIM_TRADER_H
#define BASE_ORDERROUTING_BASE_SIM_TRADER_H

#include <string>

#include "baseinfra/OrderRouting/base_order.hpp"
#include "baseinfra/OrderRouting/base_trader.hpp"
#include "baseinfra/SimMarketMaker/base_sim_market_maker.hpp"

namespace HFSAT {
/** class extending interface BaseTrader that is associated to
 * sending sim orders of a unique account, security
 */
class BaseSimTrader : public BaseTrader {
 protected:
  std::string account_string_;
  BaseSimMarketMaker* base_sim_market_maker_;
  int client_id_;

 public:
  BaseSimTrader(const std::string& account, BaseSimMarketMaker* base_sim_market_maker)
      : account_string_(account),
        base_sim_market_maker_(base_sim_market_maker),
        client_id_(base_sim_market_maker_->Connect()) {}

  virtual ~BaseSimTrader() {}

  void SendTrade(const BaseOrder& order) {
    base_sim_market_maker_->SendOrderExch(client_id_, order.security_name(), order.buysell(), order.price(),
                                          order.size_requested(), order.int_price(),
                                          order.client_assigned_order_sequence(), order.is_fok_, order.is_ioc_);
  }

  void Cancel(const BaseOrder& order) {
    base_sim_market_maker_->CancelOrderExch(client_id_, order.server_assigned_order_sequence(), order.buysell(),
                                            order.int_price());
  }

  void Modify(const BaseOrder& order, double _new_price_, int _new_int_price_, int _new_size_requested_) {
    base_sim_market_maker_->CancelReplaceOrderExch(client_id_, order.server_assigned_order_sequence(), order.buysell(),
                                                   order.price_, order.int_price_, order.size_remaining(), _new_price_,
                                                   _new_int_price_, _new_size_requested_);
  }

  void Replay(const BaseOrder& order) const {
    base_sim_market_maker_->ReplayOrderExch(client_id_, order.client_assigned_order_sequence(), order.buysell(),
                                            order.int_price(), order.server_assigned_order_sequence());
  }

  int GetClientId() const { return client_id_; }

  void IgnoreFromGlobalPos() {}
};
}
#endif  // BASE_ORDERROUTING_BASE_SIM_TRADER_H
