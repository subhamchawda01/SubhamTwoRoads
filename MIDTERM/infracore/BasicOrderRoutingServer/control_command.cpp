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
  } else if (strncmp(_cstr_, "SETMAXPOS", 9) == 0) {
    return kSetMaxPos;
  } else if (strncmp(_cstr_, "SETMAXSIZE", 10) == 0) {
    return kSetMaxSize;
  } else if (strncmp(_cstr_, "SETWORSTCASEPOS", 15) == 0) {
    return kSetWorstCasePos;
  } else if (strncmp(_cstr_, "UPDATESETTINGS", 14) == 0) {
    return kUpdateSettings;
  } else if (strncmp(_cstr_, "TMXDESYNC", 9) == 0) {
    return kTMXForceDeSync;
  } else if (strncmp(_cstr_, "TMXSYNC", 7) == 0) {
    return kTMXSync;
  } else if (strncmp(_cstr_, "SENDORDER", 9) == 0) {
    return kSendOrder;
  } else if (strncmp(_cstr_, "ADJUSTSYMBOL", 12) == 0) {
    return kAdjustSymbol;
  } else if (strncmp(_cstr_, "CANCELORDER", 11) == 0) {
    return kCancelOrder;
  } else if (strncmp(_cstr_, "MODIFYORDER", 11) == 0) {
    return kModifyOrder;
  } else if (strncmp(_cstr_, "TMXACCOUNTTYPE", 14) == 0) {
    return kTMXAccountType;
  } else if (strncmp(_cstr_, "TMXCLEARINGTRADETYPE", 20) == 0) {
    return kTMXClearingTradeType;
  } else if (strncmp(_cstr_, "TMXMODIFYORDERMODE", 18) == 0) {
    return kTMXModifyOrderMode;
  } else if (strncmp(_cstr_, "TMXSENDRFQ", 10) == 0) {
    return kTMXSendRFQ;
  } else if (strncmp(_cstr_, "TMXREGISTERACCT", 15) == 0) {
    return kTMXRegisterAccount;
  } else if (strncmp(_cstr_, "TMXDELETEACCT", 13) == 0) {
    return kTMXDeleteAccount;
  } else if (strncmp(_cstr_, "TMXPOSITIONENTRY", 16) == 0) {
    return kTMXPositionEntry;
  } else if (strncmp(_cstr_, "TMXPOSITIONDELETE", 17) == 0) {
    return kTMXPositionDelete;
  } else if (strncmp(_cstr_, "TMXPOSITIONDELIMITER", 20) == 0) {
    return kTMXPositionDelimiter;
  } else if (strncmp(_cstr_, "SETACKMAXPOS", 12) == 0) {
    return kSetAckMaxPos;
  } else if (strncmp(_cstr_, "SETACKWORSTCASEPOS", 12) == 0) {
    return kSetAckWorstCasePos;
  } else if (strncmp(_cstr_, "SETACKMAXSIZE", 13) == 0) {
    return kSetAckMaxSize;
  } else if (strncmp(_cstr_, "SETACKMAXNUMLIVE", 16) == 0) {
    return kSetAckMaxNumLive;
  } else if (!strncmp(_cstr_, "SETACKMAXNUMLIVE", 16)) {
    return kSetAckMaxNumLive;
  } else if (!strncmp(_cstr_, "IAMMARGINSERVER", 15)) {
    return kIAmMarginServer;
  } else if (!strncmp(_cstr_, "ADDLOGGINGLEVEL", 15)) {
    return kAddLoggingLevel;
  } else if (!strncmp(_cstr_, "SETSENDERID", 11)) {
    return kSetSenderId;
  } else if (!strncmp(_cstr_, "DISABLESELFTRADECHECK", 21)) {
    return kDisableSelfTradeCheck;
  } else if (!strncmp(_cstr_, "ENABLESELFTRADECHECK", 20)) {
    return kEnableSelfTradeCheck;
  } else if (!strncmp(_cstr_, "DisableNewOrders", 16)) {
    return kControlDisableNewOrders;
  } else if (!strncmp(_cstr_, "EnableNewOrders", 15)) {
    return kControlEnableNewOrders;
  } else if (!strncmp(_cstr_, "DUMPORSPOSITIONS", 16)) {
    return kDumpORSPositions;
  } else if (!strncmp(_cstr_, "ADDPRICELIMIT", 13)) {
    return kAddPriceLimit;
  } else if (!strncmp(_cstr_, "SETCLIENTINFO", 13)) {
    return kSetClientInfo;
  } else if (!strncmp(_cstr_, "DumpEngineLoggingInfo", 21)) {
    return kDumpEngineLoggingInfo;
  } else if (!strncmp(_cstr_, "TagETIAlgo", 10)) {
    return kTagETIAlgo;
  } else if (strncmp(_cstr_, "KILL", 4) == 0) {
    return kAckKill;
  } else if (strncmp(_cstr_, "REJECTDUETOFUNDS", 16) == 0) {
    return kSimulateFundsReject;
  } else if (strncmp(_cstr_, "ADDCOMBINEDTRADINGSYMBOL", 24) == 0) {
    return kAddCombinedTradingSymbol;
  } else if (0 == strncmp(_cstr_, "ADDEXTERNALPOSITIONS", 20)) {
    return kAddExternalPositions;
  } else if (0 == strncmp(_cstr_, "SENDCROSSTRADE", 14)) {
    return kSendCrossTrade;
  } else if (strncmp(_cstr_, "RELOADMARGINFILE", 16) == 0) {
    return kReloadMarginFile;
  } else if (strncmp(_cstr_, "UPDATENSECONFIGFILES", 20) == 0) {
    return kUpdateNSEConfigFiles;
  } else if (strncmp(_cstr_, "ADDNSETRADINGSYMBOL", 19) == 0) {
    return kAddNSETradingSymbol;
  } else if (strncmp(_cstr_, "ADDNSESPREADTRADINGSYMBOL", 25) == 0) {
    return kAddNSESpreadTradingSymbol;
  } else if (strncmp(_cstr_, "DUMPNSECONSTRAINTS", 18) == 0) {
    return kDumpNSEConstraints;
  } else if (strncmp(_cstr_, "RECOVERFUNDREJECTS", 16) == 0) {
    return kRecoverFundRejects;
  } else if (strncmp(_cstr_, "RELOADROLLOVERFILE", 16) == 0) {
    return kReloadRolloverFile;
  } else if (strncmp(_cstr_, "CHANGESPREADRATIO", 17) == 0) {
    return kChangeSpreadRatio;
  } else if (!strncmp(_cstr_, "SETPASSWORD", 11)) {
    return kSetPasswordOSEUser;
  } else if (!strncmp(_cstr_, "QUERYCLRINFO", 12)) {
    return kQueryClearingInfo;
  } else if (!strncmp(_cstr_, "QUERYORDERBOOK", 14)) {
    return kQueryOrderBook;
  } else if (!strncmp(_cstr_, "QUERYINACTIVEORDERBOOK", 22)) {
    return kQueryInactiveOrderBook;
  } else if (!strncmp(_cstr_, "QUERYMISSINGTRADE", 17)) {
    return kQueryMissingTrade;
  } else if (!strncmp(_cstr_, "QUERYMISSINGBROADCAST", 21)) {
    return kQueryMissingBroadcast;
  } else if (!strncmp(_cstr_, "CXLINACTIVE", 11)) {
    return kExplicitCancel;
  } else if (!strncmp(_cstr_, "EXPLICITCXLNORMAL", 17)) {
    return kExplicitCanceNormal;
  } else if (!strncmp(_cstr_, "EXPLICITMODIFY", 14)) {
    return kExplicitModify;
  } else if (!strncmp(_cstr_, "ACTIVATEORDER", 13)) {
    return kActivateOrder;
  } else if (!strncmp(_cstr_, "GEN", 3) || !strncmp(_cstr_, "gen", 3)) {
    return kGeneral;
  } else if (!strncmp(_cstr_, "SETEXCHORDERTYPE", 16)) {
    return kSetExchOrderType;
  } else if (!strncmp(_cstr_, "SETVALIDITY", 11)) {
    return kSetOrderValidity;
  } else if (!strncmp(_cstr_, "QUERYSERIESREALTIME", 19)) {
    return kQuerySeriesRealTime;
  } else if (!strncmp(_cstr_, "QUERYCOMBOSERIESREALTIME", 24)) {
    return kQueryComboSeriesRealTime;
  } else if (!strncmp(_cstr_, "QUERYMARKETSTATUS", 17)) {
    return kQueryMarketStatus;
  } else if (!strncmp(_cstr_, "SENDEXPLICITORDER", 17)) {
    return kSendExplicitOrder;
  } else if (!strncmp(_cstr_, "QUERYINSTRUMENTCLASS", 20)) {
    return kQueryInstrumentClass;
  } else if (!strncmp(_cstr_, "CXLEXPLICITORDER", 16)) {
    return kCxlExplicitOrder;
  } else if (!strncmp(_cstr_, "DUMPLASTTRADEDPRICES", 20)) {
    return kDumpLastTradedPrice;
  } else if (!strncmp(_cstr_, "SETPRICELIMITCHECK", 18)) {
    return kSetPriceLimitCheck;
  } else if (!strncmp(_cstr_, "SETALLOWEDPRICETICKLIMIT", 24)) {
    return kSetAllowedPriceTickLimit;
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
  }

  return kControlInvalid;
}
}
}
