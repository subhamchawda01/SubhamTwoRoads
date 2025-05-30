/*
  \file dvctrade/ExecLogic/curve_trading_manager_listener.hpp

  \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
  Address:
  Suite No 353, Evoma, #14, Bhattarhalli,
  Old Madras Road, Near Garden City College,
  KR Puram, Bangalore 560049, India
  +91 80 4190 3551
 */

#pragma once

#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "baseinfra/SmartOrderRouting/smart_order_manager.hpp"
#include "dvccode/CDef/control_messages.hpp"

#include "dvctrade/InitCommon/paramset.hpp"

namespace HFSAT {

/// Class used to interface beween execlogic and trading manager
class CurveTradingManagerListener {
 public:
  virtual ~CurveTradingManagerListener() {}
  virtual SmartOrderManager &order_manager() = 0;
  virtual const SecurityMarketView *smv() { return nullptr; }
  virtual const ParamSet *paramset() { return nullptr; }
  virtual void set_getflat_due_to_max_position(bool flag) {}
  virtual bool getflat_due_to_max_position() { return false; }

  virtual void set_getflat_due_to_non_standard_market_conditions(bool flag) {}
  virtual bool getflat_due_to_non_standard_market_conditions() { return false; }

  virtual void set_zero_bid_di_trade() {}
  virtual void set_zero_ask_di_trade() {}
  virtual void UpdateRiskPosition(int _new_inflrisk_position_, int _new_minrisk_position_,
                                  bool _reset_bid_trade_size_ = true, bool _reset_ask_trade_size_ = true) = 0;
  virtual void OnControlUpdate(const ControlMessage &_control_message_, const char *symbol, const int trader_id) = 0;
  virtual void Refresh() = 0;
  virtual void DisAllowOneSideTrade(TradeType_t buysell) {}
  virtual void AllowTradesForSide(TradeType_t buysell) {}
  virtual bool IsDisallowed() { return false; }
  virtual int runtime_id() = 0;
  virtual int PnlAtClose() { return 0; };
  virtual int PositionAtClose() { return 0; };
};
}
