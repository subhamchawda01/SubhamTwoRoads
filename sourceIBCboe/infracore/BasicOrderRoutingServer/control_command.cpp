/**
   \file BasicOrderRoutingServer/control_command.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
 */
#include "infracore/BasicOrderRoutingServer/control_command.hpp"

namespace HFSAT {
namespace ORS {

ControlCommand_t StringToControlCommand(const char* _cstr_) {
  if (strncmp(_cstr_, "STARTWITHOUTPLAYBACK", 20) == 0 || strncmp(_cstr_, "startwithoutplayback", 20) == 0) {
    return kControlStartWithoutPlayback;
  } else if (strncmp(_cstr_, "START", 5) == 0 || strncmp(_cstr_, "start", 5) == 0) {
    return kControlStart;
  } else if (strncmp(_cstr_, "STOP", 4) == 0 || strncmp(_cstr_, "stop", 4) == 0) {
    return kControlStop;
  } else if (strncmp(_cstr_, "LOGIN", 5) == 0 || strncmp(_cstr_, "login", 5) == 0) {
    return kControlLogin;
  } else if (strncmp(_cstr_, "LOGOUT", 6) == 0 || strncmp(_cstr_, "logout", 6) == 0) {
    return kControlLogout;
  } else if (strncmp(_cstr_, "ADDTRADINGSYMBOL", 9) == 0) {
    return kAddTradingSymbol;
  } else if (strncmp(_cstr_, "ADDCOMBOSYMBOL", 14) == 0) {
    return kAddComboProduct;
  } else if (!strncmp(_cstr_, "DUMPORSPOSITIONS", 16)) {
    return kDumpORSPositions;
  } else if (!strncmp(_cstr_, "DUMPALLORSPOSITIONS", 19)) {
    return kDumpAllORSPositions;
  } else if (!strncmp(_cstr_, "CANCELLIVEORDERS", 16)) {
    return kCancelLiveOrders;
  } else if (0 == strncmp(_cstr_, "ADDEXTERNALPOSITIONS", 20)) {
    return kAddExternalPositions;
  } else if (strncmp(_cstr_, "RELOADMARGINFILE", 16) == 0) {
    return kReloadMarginFile;
  } else if (strncmp(_cstr_, "RELOADROLLOVERFILE", 16) == 0) {
    return kReloadRolloverFile;
  } else if (!strncmp(_cstr_, "GEN", 3) || !strncmp(_cstr_, "gen", 3)) {
    return kGeneral;
  } else if (!strncmp(_cstr_, "DUMPSACIPositions", 16)) {
    return kDumpSACIPositions;
  } else if (!strncmp(_cstr_, "DumpSecurityPositions", 16)) {
    return kDumpSecurityPositions;
  } else if (!strncmp(_cstr_, "SETORSPNLCHECK", 14)) {
    return kSetORSPnlCheck;
  } else if (!strncmp(_cstr_, "SETGROSSMARGINCHECK", 19)) {
    return kSetORSGrossMarginCheck;
  } else if (!strncmp(_cstr_, "SETNETMARGINCHECK", 17)) {
    return kSetORSNetMarginCheck;
  } else if (!strncmp(_cstr_, "UPDATESECURITYMARGIN", 20)) {
    return kUpdateSecurityMarginValue;
  } else if (!strncmp(_cstr_, "UPDATEMARGINFACTOR", 18)) {
    return kUpdateMarginFactor;
  } else if (!strncmp(_cstr_, "RELOADSECURITYMARGINFILE", 24)) {
    return kReloadSecurityMarginFile;
  } else if (!strncmp(_cstr_, "DUMPORSPNLMARGINSTATUS", 22)) {
    return kDumpORSPnlMarginStatus;
  } else if (!strncmp(_cstr_, "CLEARPOSITIONS", 14)) {
    return kClearPositionManager; 
  } else if (!strncmp(_cstr_, "LOADTRADINGSYMBOLFILE", 21)) {
    return kLoadTradingSymbolFile;
  } else if (!strncmp(_cstr_, "RESETEXPOSURE", 13)) {
    return kResetExposure;
  } else if (!strncmp(_cstr_, "SETSECURITYEXPOSURE", 19)) {
    return kSetSecurityExposure;
  } else if (!strncmp(_cstr_, "SETSECURITYFILEEXPOSURE", 23)) {
    return kSecurityExposureFile;
  } else if (!strncmp(_cstr_, "USESETEXPOSURE", 14)) {
    return kUseSetExposure;
  } else if (!strncmp(_cstr_, "ENABLEORSORDERS", 15)) {
    return kenableOrsOrders;
  } else if (!strncmp(_cstr_, "DISABLEORSORDERS", 16)) {
    return kdisableOrsOrders;
  } else if (!strncmp(_cstr_, "KILLSWITCH", 10)) {
    return kkillSwitch;
  }else if (!strncmp(_cstr_, "CONVEYSTRATMARGIN", 17)) {
    return kconveystratmargin;
  }else if (!strncmp(_cstr_, "LISTENCOMBOPRODUCTDATA", 22)) {
    return kListenComboProductData;
  }else if (!strncmp(_cstr_, "REQUESTPNL", 10)) {
    return kRequestPNL;
  } else if (!strncmp(_cstr_, "REQUESTOPENPOS", 14)) {
    return kRequestOpenPositions;
  } else if (!strncmp(_cstr_, "KILLSWITCH", 10)) {
    return kkillSwitch;
  } else if (!strncmp(_cstr_, "MARGINUSAGE", 11)) {
    return kMarginUsage;
  } else if (!strncmp(_cstr_, "REQEXECUTIONS", 12)) {
    return kReqExecutions;
  }
  
  return kControlInvalid;
}
}
}
