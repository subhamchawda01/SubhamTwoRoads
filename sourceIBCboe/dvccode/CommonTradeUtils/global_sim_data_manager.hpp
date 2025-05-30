#ifndef GLOBAL_SIM_DATA_MANAGER_H
#define GLOBAL_SIM_DATA_MANAGER_H

#include "dvccode/CDef/debug_logger.hpp"

namespace HFSAT {

class GlobalSimDataManager {
 public:
  static GlobalSimDataManager& GetUniqueInstance(HFSAT::DebugLogger& dbglogger);
  int GetTradingDate();
  void SetTradingDate(int date_) { trading_date_ =  date_; }

 private:
  GlobalSimDataManager(HFSAT::DebugLogger& dbglogger);
  GlobalSimDataManager(const GlobalSimDataManager& global_sim_data_manager);

 private:
  int trading_date_;
};
}

#endif
