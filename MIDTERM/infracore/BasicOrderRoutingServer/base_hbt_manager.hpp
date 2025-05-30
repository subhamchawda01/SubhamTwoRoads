/**
    \file BasicOrderRoutingServer/base_hbt_manager.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
 */

#ifndef BASE_BASICORDERROUTINGSERVER_BASEHBTMANAGER_H
#define BASE_BASICORDERROUTINGSERVER_BASEHBTMANAGER_H

#include <string>

#include "dvccode/Utils/thread.hpp"
#include "dvccode/CDef/debug_logger.hpp"

#include "dvccode/Utils/settings.hpp"
//#include "infracore/BasicOrderRoutingServer/base_engine.hpp"

namespace HFSAT {
namespace ORS {

class BaseHBTManager : public Thread {
 public:
  BaseHBTManager(Settings& ses, DebugLogger& dbg) : m_settings(ses), dbglogger_(dbg) {}

  virtual ~BaseHBTManager() {}

 protected:
  virtual void thread_main() = 0;

 protected:
  Settings& m_settings;
  DebugLogger& dbglogger_;
  // BaseEngine* p_base_engine_;
};
}
}
#endif  // BASE_BASICORDERROUTINGSERVER_BASEHBTMANAGER_H
