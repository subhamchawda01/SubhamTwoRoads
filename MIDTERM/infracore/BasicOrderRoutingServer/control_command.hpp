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
  kUpdateSettings,
  // kAddSymbol,
  /// Margin Related Stuff
  kAddTradingSymbol,
  kSetMaxPos,
  kSetMaxSize,
  kSetWorstCasePos,
  /// TMX Conformance related stuff
  kTMXForceDeSync,
  kTMXSync,
  kSendOrder,
  kAdjustSymbol,
  // #ifdef ONLY_FOR_AUTOCERTPLUS
  //       kSendTIFOrder,
  // #endif
  kCancelOrder,
  kModifyOrder,
  kTMXAccountType,
  kTMXClearingTradeType,
  kTMXModifyOrderMode,
  kTMXSendRFQ,
  /// TMX LOPR related stuff
  kTMXRegisterAccount,
  kTMXDeleteAccount,
  kTMXPositionEntry,
  kTMXPositionDelete,
  kTMXPositionDelimiter,
  kControlInvalid,
  /// Ack based margin stuff for use by GUIs
  kSetAckMaxPos,
  kSetAckMaxSize,
  kSetAckWorstCasePos,
  kSetAckMaxNumLive,
  kAckKill,
  kIAmMarginServer,
  kAddLoggingLevel,
  kSetSenderId,
  kDisableSelfTradeCheck,
  kEnableSelfTradeCheck,
  kControlDisableNewOrders,
  kControlEnableNewOrders,
  kDumpORSPositions,
  kAddPriceLimit,
  kSetClientInfo,
  kDumpEngineLoggingInfo,
  kSimulateFundsReject,
  kTagETIAlgo,
  kAddCombinedTradingSymbol,
  kAddExternalPositions,
  kReloadMarginFile,
  kSendCrossTrade,
  kUpdateNSEConfigFiles,
  kAddNSETradingSymbol,
  kAddNSESpreadTradingSymbol,
  kDumpNSEConstraints,
  kRecoverFundRejects,
  kReloadRolloverFile,
  kChangeSpreadRatio,
  kSetPasswordOSEUser,
  kQueryClearingInfo,
  kQueryOrderBook,
  kQueryMissingTrade,
  kQueryInactiveOrderBook,
  kExplicitCancel,
  kQueryMissingBroadcast,
  kExplicitCanceNormal,
  kExplicitModify,
  kActivateOrder,
  kGeneral,
  kSetExchOrderType,
  kSetOrderValidity,
  kQuerySeriesRealTime,
  kQueryComboSeriesRealTime,
  kSendExplicitOrder,
  kQueryInstrumentClass,
  kQueryMarketStatus,
  kCxlExplicitOrder,
  kDumpLastTradedPrice,
  kSetPriceLimitCheck,
  kSetAllowedPriceTickLimit,
  kDumpSACIPositions,
  kDumpSecurityPositions,
  kSetORSPnlCheck,
  kSetORSGrossMarginCheck,
  kSetORSNetMarginCheck,
  kUpdateSecurityMarginValue,
  kUpdateMarginFactor,
  kReloadSecurityMarginFile,
  kDumpORSPnlMarginStatus
} ControlCommand_t;

ControlCommand_t StringToControlCommand(const char* _cstr_);
}
}
#endif  // BASE_BASICORDERROUTINGSERVER_CONTROL_COMMAND_H
