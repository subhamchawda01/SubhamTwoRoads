/**
    \file BasicOrderRoutingServer/control_thread.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066
         India
         +91 80 4060 0717
 */

#ifndef BASE_BASICORDERROUTINGSERVER_CONTROL_COMMAND_H
#define BASE_BASICORDERROUTINGSERVER_CONTROL_COMMAND_H

#include <iostream>
#include <string.h>

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "infracore/BasicOrderRoutingServer/defines.hpp"

namespace HFSAT {
namespace ORS {

typedef enum {
  kControlStart = 0,
  kControlStartWithoutPlayback,
  kControlStop,
  kControlLogin,
  kControlLogout,
  // kAddSymbol,
  /// Margin Related Stuff
  kAddTradingSymbol,
  kControlInvalid,
  kDumpORSPositions,
  kDumpAllORSPositions,
  kCancelLiveOrders,
  kAddExternalPositions,
  kReloadMarginFile,
  kReloadRolloverFile,
  kGeneral,
  kDumpSACIPositions,
  kDumpSecurityPositions,
  kSetORSPnlCheck,
  kSetORSGrossMarginCheck,
  kSetORSNetMarginCheck,
  kUpdateSecurityMarginValue,
  kUpdateMarginFactor,
  kReloadSecurityMarginFile,
  kDumpORSPnlMarginStatus,
  kClearPositionManager,
  kLoadTradingSymbolFile, 
  kResetExposure,
  kSetSecurityExposure,
  kSecurityExposureFile,
  kUseSetExposure,
  kenableOrsOrders,
  kdisableOrsOrders,
  kkillSwitch,
  kconveystratmargin,
  kAddComboProduct,
  kListenComboProductData,
  kRequestPNL,
  kRequestOpenPositions,
  kMarginUsage,
  kReqExecutions
} ControlCommand_t;

ControlCommand_t StringToControlCommand(const char* _cstr_);
}
}
#endif  // BASE_BASICORDERROUTINGSERVER_CONTROL_COMMAND_H
