/**
   \file ExecLogic/trading_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
*/
#ifndef BASE_EXECLOGIC_TRADING_MANAGER_H
#define BASE_EXECLOGIC_TRADING_MANAGER_H

#include "dvctrade/ExecLogic/base_trading.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"

typedef unsigned int uint32_t;

namespace HFSAT {

class TradingManager {
 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;

 public:
  TradingManager(DebugLogger& t_dbglogger_, const Watch& _watch_) : dbglogger_(t_dbglogger_), watch_(_watch_) {}

  virtual ~TradingManager(){};

  virtual void ReportResults(HFSAT::BulkFileWriter& trades_writer_) = 0;
};
}
#endif  // BASE_EXECLOGIC_TRADING_MANAGER_H
