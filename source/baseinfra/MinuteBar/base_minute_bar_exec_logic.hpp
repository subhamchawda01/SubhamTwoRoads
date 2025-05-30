/**
    \file MinuteBar/base_minute_bar_exec_logic.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#pragma once

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"

#include "baseinfra/MinuteBar/minute_bar_security_market_view.hpp"
#include "baseinfra/MinuteBar/minute_bar_order_manager.hpp"

#include "baseinfra/MinuteBar/Signal/signal_list.hpp"

namespace HFSAT {

class BaseMinuteBarExecLogic : public MinuteBarSMVListener, public SignalListener {
  typedef void (BaseMinuteBarExecLogic::*BarUpdateFuncPtr)(const unsigned int security_id, const DataBar& minute_bar);
  typedef void (BaseMinuteBarExecLogic::*SignalUpdateFuncPtr)(int signal_id, double signal_value);

  HFSAT::DebugLogger& dbglogger_;
  const HFSAT::Watch& watch_;
  std::string logic_name_;
  std::string config_file_;
  int client_id_;
  HFSAT::StrategySecurityPairToMinuteBarOrderManager& strat_secid_to_om_map_;
  std::map<std::string, std::pair<BarUpdateFuncPtr, SignalUpdateFuncPtr>> logic_name_to_func_map_;
  BarUpdateFuncPtr bar_update_function_;
  SignalUpdateFuncPtr signal_update_function_;
  HFSAT::SecurityNameIndexer& indexer_;

 public:
  BaseMinuteBarExecLogic(DebugLogger& dbglogger, const Watch& watch, const std::string& logic_name,
                         const std::string& config_file, int client_id,
                         StrategySecurityPairToMinuteBarOrderManager& om_map);
  virtual ~BaseMinuteBarExecLogic() {}
  void OnBarUpdate(const unsigned int security_id, const DataBar& minute_bar) override;
  void OnSignalUpdate(int signal_id, double signal_value) override;
  void SendTrade(HFSAT::TradeType_t buy_sell, int security_id, int size);
  void InitFunctionPtrMap();
  void CheckValidLogicType();

  // BarUpdateFunctions
  void SimpleLogicBarUpdate(const unsigned int security_id, const DataBar& minute_bar);
  void SimpleLogicSignalUpdate(int signal_id, double signal_value);
};
}
