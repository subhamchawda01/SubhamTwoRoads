/**
    \file dvccode/ORSMessages/control_messages.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_ORSMESSAGES_CONTROL_MESSAGES_H
#define BASE_ORSMESSAGES_CONTROL_MESSAGES_H

#include <sys/time.h>
#include <string>
#include <sstream>
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"

namespace HFSAT {

#define CONTROLMESSAGECODESTART 0  ///< change this if we want a different startcode

/**
 * \brief control messages from the trader
 *
 * typically this will be a message from the trader :
 * getflat
 * start trading ( remove all external stoppers like freeze cancelall getflat )
 * freeze all order routing activity
 * remove external freeze of order routing activity
 * cancel all orders and freeze all order routing activity
 * mult unit_trade_size_ & max_position_ & worst_case_position_ by X
 * add X to unit_trade_size_
 * add X to max_position_
 * add X to worst_case_position_
 * disable improve
 * enable improve
 * disable aggressive
 * enable aggressive
 * log params, broadcast them with qid
 */
typedef enum {
  kControlMessageCodeGetflat = CONTROLMESSAGECODESTART,
  kControlMessageCodeStartTrading,
  kControlMessageCodeFreezeTrading,
  kControlMessageCodeUnFreezeTrading,
  kControlMessageCodeCancelAllFreezeTrading,
  kControlMessageCodeSetTradeSizes,
  kControlMessageCodeSetUnitTradeSize,
  kControlMessageCodeSetMaxPosition,
  kControlMessageCodeSetMaxGlobalRisk,
  kControlMessageCodeSetWorstCasePosition,
  kControlMessageCodeDisableImprove,
  kControlMessageCodeEnableImprove,
  kControlMessageCodeDisableAggressive,
  kControlMessageCodeEnableAggressive,
  kControlMessageCodeShowParams,
  kControlMessageCodeShowIndicators,
  kControlMessageCodeAddPosition,
  kControlMessageCodeDumpPositions,
  kControlMessageCodeCleanSumSizeMaps,
  kControlMessageCodeSetEcoSeverity,
  kControlMessageEnableSmartEco,
  kControlMessageDisableSmartEco,
  kControlMessageEnablePriceManager,
  kControlMessageDisablePriceManager,
  kControlMessageEnableMarketManager,
  kControlMessageDisableMarketManager,
  kControlMessageDisableSelfOrderCheck,
  kControlMessageEnableSelfOrderCheck,
  kControlMessageDumpNonSelfSMV,
  kControlMessageCodeSetMaxUnitRatio,
  kControlMessageCodeSetWorstCaseUnitRatio,
  kControlMessageCodeEnableAggCooloff,
  kControlMessageCodeDisableAggCooloff,
  kControlMessageCodeEnableNonStandardCheck,
  kControlMessageCodeDisableNonStandardCheck,
  kControlMessageCodeForceIndicatorReady,
  kControlMessageCodeForceAllIndicatorReady,
  kControlMessageCodeSetMaxSecurityPosition,
  kControlMessageCodeEnableLogging,
  kControlMessageCodeDisableLogging,
  kControlMessageCodeSetMaxLoss,
  kControlMessageCodeSetGlobalMaxLoss,
  kControlMessageCodeSetMaxDrawDown,
  kControlMessageCodeSetShortTermGlobalMaxLoss,
  kControlMessageCodeShowOrders,
  kControlMessageCodeSetMaxGlobalPosition,
  kControlMessageCodeEnableZeroLoggingMode,
  kControlMessageCodeDisableZeroLoggingMode,
  kControlMessageCodeSetStartTime,
  kControlMessageCodeSetEndTime,
  kControlMessageCodeSetOpenTradeLoss,
  kControlMessageCodeAggGetflat,
  kControlMessageCodeSetMaxIntSpreadToPlace,
  kControlMessageCodeSetMaxIntLevelDiffToPlace,
  kControlMessageCodeSetMaxPnl,
  kControlMessageCodeSetExplicitMaxLongPosition,
  kControlMessageCodeSetExplicitWorstLongPosition,
  kControlMessageCodeSetBreakMsecsOpenTradeLoss,
  kControlMessageReloadEconomicEvents,   // Used for debugging in case of intraday FXStreet web-page updates
  kControlMessageShowEconomicEvents,     // Used for debugging in case of intraday FXStreet web-page updates
  kControlMessageReloadAfEstimates,      // For Alphaflash (eventbiasaggressivetrading) strategies
  kControlMessageCodeGetFlatOnThisHost,  // A Single Message Which Indicates Getflat On All Queries On This Host
  kControlMessageDisableFreezeOnRejects,
  kControlMessageEnableFreezeOnRejects,
  kControlMessageSetDeltaThreshold,
  kControlMessageCodeMax  // should remain at the end , error condition
} ControlMessageCode_t;

inline const char *GetControlMessageCodeString(ControlMessageCode_t _control_message_code_) {
  switch (_control_message_code_) {
    case kControlMessageCodeGetflat:
      return "kControlMessageCodeGetflat";
      break;
    case kControlMessageCodeStartTrading:
      return "kControlMessageCodeStartTrading";
      break;
    case kControlMessageCodeFreezeTrading:
      return "kControlMessageCodeFreezeTrading";
      break;
    case kControlMessageCodeUnFreezeTrading:
      return "kControlMessageCodeUnFreezeTrading";
      break;
    case kControlMessageCodeCancelAllFreezeTrading:
      return "kControlMessageCodeCancelAllFreezeTrading";
      break;
    case kControlMessageCodeSetTradeSizes:
      return "kControlMessageCodeSetTradeSizes";
      break;
    case kControlMessageCodeSetUnitTradeSize:
      return "kControlMessageCodeSetUnitTradeSize";
      break;
    case kControlMessageCodeSetMaxPosition:
      return "kControlMessageCodeSetMaxPosition";
      break;
    case kControlMessageCodeSetWorstCasePosition:
      return "kControlMessageCodeSetWorstCasePosition";
      break;
    case kControlMessageCodeDisableImprove:
      return "kControlMessageCodeDisableImprove";
      break;
    case kControlMessageCodeEnableImprove:
      return "kControlMessageCodeEnableImprove";
      break;
    case kControlMessageCodeDisableAggressive:
      return "kControlMessageCodeDisableAggressive";
      break;
    case kControlMessageCodeEnableAggressive:
      return "kControlMessageCodeEnableAggressive";
      break;
    case kControlMessageCodeShowParams:
      return "kControlMessageCodeShowParams";
      break;
    case kControlMessageCodeShowIndicators:
      return "kControlMessageCodeShowIndicators";
      break;
    case kControlMessageCodeCleanSumSizeMaps:
      return "kControlMessageCodeCleanSumSizeMaps";
      break;
    case kControlMessageCodeSetEcoSeverity:
      return "kControlMessageCodeSetEcoSeverity";
      break;
    case kControlMessageEnableSmartEco:
      return "kControlMessageEnableSmartEco";
      break;
    case kControlMessageDisableSmartEco:
      return "kControlMessageDisableSmartEco";
      break;
    case kControlMessageEnablePriceManager:
      return "kControlMessageEnablePriceManager";
      break;
    case kControlMessageDisablePriceManager:
      return "kControlMessageDisablePriceManager";
      break;
    case kControlMessageEnableMarketManager:
      return "kControlMessageEnableMarketManager";
      break;
    case kControlMessageDisableMarketManager:
      return "kControlMessageDisableMarketManager";
      break;
    case kControlMessageCodeMax:
      return "kControlMessageCodeMax";
      break;
    case kControlMessageDisableSelfOrderCheck:
      return "kControlMessageDisableSelfOrderCheck";
      break;
    case kControlMessageEnableSelfOrderCheck:
      return "kControlMessageEnableSelfOrderCheck";
      break;
    case kControlMessageDumpNonSelfSMV:
      return "kControlMessageDumpNonSelfSMV";
      break;
    case kControlMessageCodeSetMaxUnitRatio:
      return "kControlMessageCodeSetMaxUnitRatio";
      break;
    case kControlMessageCodeSetWorstCaseUnitRatio:
      return "kControlMessageCodeSetWorstCaseUnitRatio";
      break;
    case kControlMessageCodeEnableAggCooloff:
      return "kControlMessageCodeEnableAggCooloff";
      break;
    case kControlMessageCodeDisableAggCooloff:
      return "kControlMessageCodeDisableAggCooloff";
      break;
    case kControlMessageCodeEnableNonStandardCheck:
      return "kControlMessageCodeEnableNonStandardCheck";
      break;
    case kControlMessageCodeDisableNonStandardCheck:
      return "kControlMessageCodeDisableNonStandardCheck";
      break;
    case kControlMessageCodeForceIndicatorReady:
      return "kControlMessageCodeForceIndicatorReady";
      break;
    case kControlMessageCodeForceAllIndicatorReady:
      return "kControlMessageCodeForceAllIndicatorReady";
      break;
    case kControlMessageCodeSetMaxSecurityPosition:
      return "kControlMessageCodeSetMaxSecurityPosition";
    case kControlMessageCodeEnableLogging:
      return "kControlMessageCodeEnableLogging";
      break;
    case kControlMessageCodeDisableLogging:
      return "kControlMessageCodeDisableLogging";
      break;
    case kControlMessageCodeSetMaxLoss:
      return "kControlMessageCodeSetMaxLoss";
      break;
    case kControlMessageCodeSetMaxDrawDown:
      return "kControlMessageCodeSetMaxDrawDown";
      break;
    case kControlMessageCodeSetGlobalMaxLoss:
      return "kControlMessageCodeSetGlobalMaxLoss";
      break;
    case kControlMessageCodeSetShortTermGlobalMaxLoss:
      return "kControlMessageCodeSetShortTermGlobalMaxLoss";
      break;
    case kControlMessageCodeShowOrders:
      return "kControlMessageCodeShowOrders";
      break;
    case kControlMessageCodeSetMaxGlobalPosition:
      return "kControlMessageCodeSetMaxGlobalPosition";
      break;
    case kControlMessageCodeEnableZeroLoggingMode:
      return "kControlMessageCodeEnableZeroLoggingMode";
      break;
    case kControlMessageCodeDisableZeroLoggingMode:
      return "kControlMessageCodeDisableLoggingMode";
    case kControlMessageCodeSetStartTime:
      return "kControlMessageCodeSetStartTime";
      break;
    case kControlMessageCodeSetEndTime:
      return "kControlMessageCodeSetEndTime";
      break;
    case kControlMessageCodeSetOpenTradeLoss:
      return "kControlMessageCodeSetOpenTradeLoss";
      break;
    case kControlMessageCodeAggGetflat:
      return "kControlMessageCodeAggGetflat";
      break;
    case kControlMessageCodeSetMaxIntSpreadToPlace:
      return "kControlMessageCodeSetMaxIntSpreadToPlace";
      break;
    case kControlMessageCodeSetMaxIntLevelDiffToPlace:
      return "kControlMessageCodeSetMaxIntLevelDiffToPlace";
      break;
    case kControlMessageCodeSetMaxPnl:
      return "kControlMessageCodeSetMaxPnl";
    case kControlMessageCodeSetExplicitMaxLongPosition:
      return "kControlMessagesCodeSetExplicitMaxLongPosition";
    case kControlMessageCodeSetExplicitWorstLongPosition:
      return "kControlMessagesCodeSetExplicitWorstLongPosition";
    case kControlMessageCodeSetBreakMsecsOpenTradeLoss:
      return "kControlMessagesCodeSetBreakMsecsOpenTradeLoss";
      break;
    case kControlMessageReloadEconomicEvents:
      return "kControlMessageReloadEconomicEvents";
    case kControlMessageShowEconomicEvents:
      return "kControlMessageShowEconomicEvents";
    case kControlMessageReloadAfEstimates:
      return "kControlMessageReloadAfEstimates";
    case kControlMessageCodeGetFlatOnThisHost:
      return "kControlMessageCodeGetFlatOnThisHost";
    case kControlMessageDisableFreezeOnRejects:
      return "kControlMessageDisableFreezeOnRejects";
    case kControlMessageEnableFreezeOnRejects:
      return "kControlMessageEnableFreezeOnRejects";
    case kControlMessageSetDeltaThreshold:
      return "kControlMessageSetDeltaThreshold";
    default:
      break;
  }

  return "kControlMessageCodeUnknown";
}

struct ControlMessage {
 public:
  inline std::string ToString() const {
    std::ostringstream t_oss_;
    t_oss_ << GetControlMessageCodeString(message_code_) << " " << intval_1_ << " " << intval_2_ << " " << intval_3_
           << " " << intval_4_ << " " << doubleval_1_ << " " << doubleval_2_ << " " << strval_1_;
    return t_oss_.str();
  }

  friend std::ostream &operator<<(std::ostream &out, const ControlMessage &control_message_);

 public:
  // explicitly setting the variables
  ControlMessage()
      : message_code_(kControlMessageCodeMax),
        intval_1_(0),
        intval_2_(0),
        intval_3_(0),
        intval_4_(0),
        doubleval_1_(0.0),
        doubleval_2_(0.0),
        strval_1_{0} {}

  ControlMessageCode_t message_code_;

  int intval_1_;
  int intval_2_;
  int intval_3_;
  int intval_4_;

  double doubleval_1_;
  double doubleval_2_;

  char strval_1_[24];
};

inline std::ostream &operator<<(std::ostream &out, const ControlMessage &control_message_) {
  out << control_message_.ToString();
  return out;
}

/** GenericControlRequestStruct received from Clients on UDP
 * size b() on ttime_t ) or b() on timeval on x86_64
 */
struct GenericControlRequestStruct {
 public:
  inline std::string ToString() const {
    std::ostringstream t_oss_;
    t_oss_ << time_set_by_frontend_ << " " << symbol_ << " " << trader_id_ << " " << control_message_ << "\n";
    return t_oss_.str();
  }
  const char *getContract() const { return symbol_; }

  friend std::ostream &operator<<(std::ostream &out, const GenericControlRequestStruct &control_message_);

 public:
  char symbol_[kSecNameLen];        ///< b(16) exchange symbol this order was about
  ttime_t time_set_by_frontend_;    ///< b(8) or b(16) [ timeval ]
  int trader_id_;                   ///< b(4) used to uniquely identify this trader of this security
  ControlMessage control_message_;  ///< b() used to convey the ControlMessageCode_t and parameters
};

// The following are used to OR ( bitwise to get query_control_bits_ in ParamSetSendStruct
#define QCB_SBGF 0x00000001
#define QCB_GFDTC 0x00000002
#define QCB_GFDTMP 0x00000004
#define QCB_GFDTML 0x00000008
#define QCB_GFDTMOTL 0x00000010
#define QCB_GFDTET 0x00000020
#define QCB_EGF 0x00000040
#define QCB_EFT 0x00000080
#define QCB_ECAOO 0x00000100
#define QCB_GFDTMPTL 0x0000200

struct ParamSetSendStruct {
  int query_control_bits_;  ///< b(4)
  int worst_case_position_;
  int max_position_;
  int unit_trade_size_;
  int max_global_position_;
  int max_security_position_;

  double highpos_limits_;
  double highpos_thresh_factor_;
  double highpos_thresh_decrease_;
  double highpos_size_factor_;
  double increase_place_;
  double increase_keep_;
  double zeropos_limits_;
  double zeropos_place_;
  double zeropos_keep_;
  double decrease_place_;
  double decrease_keep_;

  double place_keep_diff_;
  double increase_zeropos_diff_;
  double zeropos_decrease_diff_;

  int safe_distance_;

  bool allowed_to_improve_;
  bool allowed_to_aggress_;
  double improve_;
  double aggressive_;

  double max_self_ratio_at_level_;
  double longevity_support_;

  int max_position_to_lift_;
  int max_position_to_bidimprove_;
  int max_position_to_cancel_on_lift_;
  int max_size_to_aggress_;
  int min_position_to_hit_;
  int min_position_to_askimprove_;
  int min_position_to_cancel_on_hit_;

  int max_int_spread_to_place_;
  int max_int_level_diff_to_place_;
  int max_int_spread_to_cross_;
  int min_int_spread_to_improve_;
  unsigned int num_non_best_bid_levels_monitored_;
  unsigned int num_non_best_ask_levels_monitored_;

  int max_loss_;
  int max_pnl_;
  int global_max_loss_;
  int max_opentrade_loss_;
  int max_pertrade_loss_;
  int max_short_term_loss_;
  int max_drawdown_;

  int cooloff_interval_;
  int agg_cooloff_interval_;
  int highpos_aversion_msecs_;

  double stdev_fact_;
  double stdev_cap_;
  int px_band_;
  double low_stdev_lvl_;
  int min_size_to_join_;
  double spread_add_;

  bool use_throttle_manager_;
  int throttle_message_limit_;
};

/** @brief GenericControlReplyStruct sent to Trader Frontend via UDP
 *
 * b ( ) on x86_64
 */
struct GenericControlReplyStruct {
  char symbol_[kSecNameLen];   ///< b(16) used in SendOrder
  ttime_t time_set_by_query_;  ///< b(8) or b(16) [ timeval ]
  int trader_id_;              ///< b(4) used to uniquely identify this trader of this security
  int my_position_;
  double total_pnl_;
  ParamSetSendStruct param_set_send_struct_;
};
}

#endif  // BASE_ORSMESSAGES_CONTROL_MESSAGES_H
