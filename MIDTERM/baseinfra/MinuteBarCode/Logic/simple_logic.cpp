#include "baseinfra/MinuteBar/base_minute_bar_exec_logic.hpp"

namespace HFSAT {

void BaseMinuteBarExecLogic::SimpleLogicBarUpdate(const unsigned int security_id, const DataBar& minute_bar) {
  DataBar m = minute_bar;
  std::string sym = indexer_.GetSecurityNameFromId(security_id);
  DBGLOG_CLASS_FUNC_LINE << " Symbol: " << sym << " " << m.ToString() << DBGLOG_ENDL_FLUSH;
  SendTrade(kTradeTypeBuy, security_id, 1);
  SendTrade(kTradeTypeSell, security_id, 1);
}

void BaseMinuteBarExecLogic::SimpleLogicSignalUpdate(int signal_id, double signal_value) {
  DBGLOG_CLASS_FUNC_LINE << "SignalID: " << signal_id << " Value: " << signal_value << DBGLOG_ENDL_FLUSH;
}
}
