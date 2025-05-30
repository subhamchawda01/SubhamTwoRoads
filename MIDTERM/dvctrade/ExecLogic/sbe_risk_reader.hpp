/**
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
  Old Madras Road, Near Garden City College,
  KR Puram, Bangalore 560049, India
  +91 80 4190 3551
*/

#pragma once
#include "dvccode/ExternalData/external_time_listener.hpp"
#include "dvctrade/ExecLogic/signal_based_trading.hpp"

/*
interested in running execution_logic from the orders obtained through dated files
 */
struct InstrumentStratID_t {
  std::string instrument_;
  int strat_id_;
  bool operator<(const InstrumentStratID_t& param) const {
    // lex order
    return std::tie(instrument_, strat_id_) < std::tie(param.instrument_, param.strat_id_);
  }
};

namespace HFSAT {
class SBERiskReader : public TimePeriodListener {
 public:

  SignalBasedTrading* exec_logic_;

  HFSAT::Watch& watch_;
  HFSAT::DebugLogger& dbglogger_;
  uint64_t exec_start_time_;

 public:
  SBERiskReader(HFSAT::Watch& watch_t, HFSAT::DebugLogger& dbglogger_t);
  void SubscribeRiskChanges(SignalBasedTrading* t_exec_logic_);
  void NotifyOrderListeners(std::string t_instrument_, std::string order_id_, int order_size_, double ref_px_);
  void NotifyPositionListeners(std::string t_instrument_, std::string strat_id_, int position_size_, double new_ref_px_);
  void OnTimePeriodUpdate(const int num_pages_to_add_) override {}

  virtual void LookupRiskLineInSimMode() {}
  virtual void LoadAllRiskLinesInSimMode() {}
};

}
