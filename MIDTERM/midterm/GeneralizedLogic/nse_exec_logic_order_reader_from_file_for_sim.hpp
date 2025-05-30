#pragma once
#include "nse_exec_logic_order_reader_from_file.hpp"
#include "midterm/Execs/nse_exec_logic_helper_for_sim.hpp"

namespace NSE_SIMPLEEXEC {
class SimpleNseExecLogicOrderReaderFromFileForSim
    : public SimpleNseExecLogicOrderReaderFromFile {
public:
  // backup file is only needed for unexecuted orders in live mode
  SimpleNseExecLogicOrderReaderFromFileForSim(
      HFSAT::Watch &watch_t, HFSAT::DebugLogger &dbglogger_t, bool is_live_t,
      std::string orders_file_t,
      std::map<std::string, SyntheticLegInfo> leg_info_t,
      NseExecLogicHelper *exec_logic_helper_t)
      : SimpleNseExecLogicOrderReaderFromFile(watch_t, dbglogger_t, is_live_t,
                                              orders_file_t, "", leg_info_t,
                                              exec_logic_helper_t) {}
};
}
