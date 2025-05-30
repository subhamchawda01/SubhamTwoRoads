#ifndef BASE_BASICORDERROUTINGSERVER_MARGINCHECKER_H
#define BASE_BASICORDERROUTINGSERVER_MARGINCHECKER_H

#include <iostream>
#include <limits>

#include "dvccode/CDef/assumptions.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/debug_logger.hpp"

#include "dvccode/CDef/ors_messages.hpp"

#include "infracore/BasicOrderRoutingServer/common_modules.hpp"
#include "infracore/BasicOrderRoutingServer/defines.hpp"
#include "infracore/BasicOrderRoutingServer/position_manager.hpp"
#include "dvccode/Utils/live_trade_price_manager.hpp"

#include "infracore/ORSUtils/ors_pnl_manager.hpp"
#include "infracore/ORSUtils/ors_margin_manager.hpp"

#define DEFAULT_UPPER_PRICE_LIMIT 999999
#define DEFAULT_LOWER_PRICE_LIMIT 0

#define MAX_MSG_MIFID 160000

namespace HFSAT {
namespace ORS {

/// Class that keeps the limits ( ordersize, position and worst_case_position )
/// and is an interface to allowing orders that check pass those checks
/// Shares a common security_id semantics to all ClientThread instances ( though the common SimpleSecuritySymbolIndexer
/// )
/// security_id being a low integer the map from security_id to symbol can be made
/// a vector
/// Possible improvements ... make as many vectors into arrays as possible
class MarginChecker {
 private:
  /// Added private copy constructor to disable it
  MarginChecker(const MarginChecker&);

  MarginChecker(DebugLogger& _dbglogger_, HFSAT::ExchSource_t exchange)
      : dbglogger_(_dbglogger_),
        position_manager_(PositionManager::GetUniqueInstance()),
        is_price_checks_enabled_(false),
        price_manager_(nullptr),
        sec_id_to_last_traded_price_(DEF_MAX_SEC_ID),
        exchange_(exchange),
        ors_pnl_manager_(HFSAT::ORSUtils::ORSPnlManager::GetUniqueInstance(dbglogger_)),
        ors_margin_manager_(HFSAT::ORSUtils::ORSMarginManager::GetUniqueInstance(dbglogger_)),
        is_query_cumulative_risk_check_enabled_(false),
        query_risk_checks_feature_flag_(false),
        is_mifid_check_enabled_(false) {
    for (int sec_id_ = 0; sec_id_ < DEF_MAX_SEC_ID; sec_id_++) {
      Set(sec_id_, -1, -1, -1, -1);
      SetMifidLimits(sec_id_, 10000, 10000, -100, -100, -1);
    }

    for (int i = 0; i < ORS_MAX_NUM_OF_CLIENTS; i++) {
      saci_to_query_limits_[i].worstcase_pos = -1;
      saci_to_query_limits_[i].sec_id = -1;
    }

    for (int i = 0; i < DEF_MAX_SEC_ID; i++) {
      sec_id_to_last_traded_price_[i].last_trade_price = std::numeric_limits<double>::max();
    }

    if (exchange_ == HFSAT::kExchSourceSGX || exchange_ == HFSAT::kExchSourceHONGKONG) {
      is_price_checks_enabled_ = true;
    }

    if (is_price_checks_enabled_ && price_manager_ == nullptr) {
      price_manager_ = new HFSAT::Utils::LiveTradePriceManager(exchange, sec_id_to_last_traded_price_, dbglogger_);
      price_manager_->run();
    }

    if (HFSAT::IsMiFidCheckApplicable(exchange_)) {
      is_mifid_check_enabled_ = true;
    }
  }

 public:
  static inline MarginChecker& GetUniqueInstance(DebugLogger& _dbglogger_, HFSAT::ExchSource_t exchange) {
    static MarginChecker uniqueinstance_(_dbglogger_, exchange);
    return uniqueinstance_;
  }

  ~MarginChecker() {
    if (price_manager_ != nullptr) {
      delete price_manager_;
    }
  }

  /// accessor functions for setting margin params

  inline void Set(int t_security_id_, int pos_, int max_ord_size_, int worst_pos, int numords) {
    if (t_security_id_ < DEF_MAX_SEC_ID) {
      limits_[t_security_id_].max_position_ = pos_;
      limits_[t_security_id_].max_order_size_ = max_ord_size_;
      limits_[t_security_id_].max_worst_case_position_ = worst_pos;
      limits_[t_security_id_].max_live_order_ = numords;
      limits_[t_security_id_].sum_clients_worstcase_pos_limit_ = 0;
    }
  }

  inline void SetMifidLimits(int t_security_id_, int min_vol, int min_count, int limit_vol, int limit_count,
                             int msg_count) {
    if (t_security_id_ < DEF_MAX_SEC_ID) {
      limits_[t_security_id_].otr_min_vol_ = min_vol;
      limits_[t_security_id_].otr_min_cnt_ = min_count;
      limits_[t_security_id_].otr_limit_vol_ = limit_vol;
      limits_[t_security_id_].otr_limit_cnt_ = limit_count;
      limits_[t_security_id_].msg_count_ = msg_count;
    }
  }

  inline void setMaxPos(int sec_id, int pos) { limits_[sec_id].max_position_ = pos; }

  inline void setMaxOrdSize(int sec_id, int size) { limits_[sec_id].max_order_size_ = size; }

  inline void setWorstPos(int sec_id, int worst_pos) { limits_[sec_id].max_worst_case_position_ = worst_pos; }

  inline void setMaxLiveOrd(int sec_id, int numords) { limits_[sec_id].max_live_order_ = numords; }

  /// accessor functions
  inline int getMaxPos(int sec_id) const {
    return limits_[position_manager_.GetProcessedSecurityToEnforceMarginChecks(sec_id)].max_position_;
  }

  inline int getMaxOrdSize(int sec_id) const {
    return limits_[position_manager_.GetProcessedSecurityToEnforceMarginChecks(sec_id)].max_order_size_;
  }

  inline int getWorstPos(int sec_id) const {
    return limits_[position_manager_.GetProcessedSecurityToEnforceMarginChecks(sec_id)].max_worst_case_position_;
  }

  inline int getMaxLiveOrd(int sec_id) const {
    return limits_[position_manager_.GetProcessedSecurityToEnforceMarginChecks(sec_id)].max_live_order_;
  }

  inline ORSRejectionReason_t EnforceIndividualSecurityMarginChecks(const int _security_id_, int _size_requested_,
                                                                    const TradeType_t _buysell_,
                                                                    const int originial_size_ = -1) {
    const OrderLimits& limit = limits_[_security_id_];

    // check if order size is fine
    if ((uint32_t)(_size_requested_) > (uint32_t)(limit.max_order_size_)) {
      return kORSRejectMarginCheckFailedOrderSizes;
    }

    // For Modify case, order size check needs to be done with full new size ( as being done above )
    // But for MaxPos, WrostPos, LiveOrders check we need to take the diff from the original size and then check
    if (originial_size_ != -1) {
      _size_requested_ -= originial_size_;
    }

    if (_buysell_ == kTradeTypeBuy) {
      // check if position could exceed max_position by allowing trade, assuming
      // none of the currently resting orders will be filled and this trade will
      // be filled
      if (((_size_requested_) + position_manager_.GetGlobalPosition(_security_id_)) > limit.max_position_) {
        return kORSRejectMarginCheckFailedMaxPosition;
      }

      // if not check if position could exceed max_worst_case_positions_ by
      // allowing trade, assuming all the currently resting orders will be
      // filled
      if (((_size_requested_) + position_manager_.GetGlobalWorstCaseBidPosition(_security_id_)) >
          limit.max_worst_case_position_) {
        return kORSRejectMarginCheckFailedWorstCasePosition;
      }
    } else {
      // check if position could exceed max_position by allowing trade, assuming
      // none of the currently resting orders will be filled and this trade will
      // be filled
      if (((_size_requested_)-position_manager_.GetGlobalPosition(_security_id_)) > limit.max_position_) {
        return kORSRejectMarginCheckFailedMaxPosition;
      }

      // if not check if position could exceed max_worst_case_positions_ by
      // allowing trade, assuming all the currently resting orders will be
      // filled
      if (((_size_requested_) + position_manager_.GetGlobalWorstCaseAskPosition(_security_id_)) >
          limit.max_worst_case_position_) {
        return kORSRejectMarginCheckFailedWorstCasePosition;
      }
    }

    return kORSOrderAllowed;
  }

  /// returns 0 if passed,
  /// non zero value refers to failure, and the error code
  inline ORSRejectionReason_t Allows(const int _security_id_, const double _price_, int _size_requested_,
                                     const TradeType_t _buysell_, const int originial_size_ = -1) const {
    const HFSAT::ORS::PositionInfo* info = position_manager_.GetPositionInfoStruct(_security_id_);

    int32_t processed_security_id = _security_id_;

    double this_security_weight_ = 1.00;

    if (info->combined_security_id != -1) {
      processed_security_id = info->combined_security_id;
      this_security_weight_ = info->combined_bucket_weight_factor;
      info = position_manager_.GetPositionInfoStruct(processed_security_id);
    }

    const OrderLimits& limit = limits_[processed_security_id];

    // check if order size is fine
    if ((uint32_t)(_size_requested_ * fabs(this_security_weight_)) > (uint32_t)(limit.max_order_size_)) {
      return kORSRejectMarginCheckFailedOrderSizes;
    }

    int weight_sign_ = 1;

    if (this_security_weight_ < 0) {
      weight_sign_ = -1;
    }

    // For Modify case, order size check needs to be done with full new size ( as being done above )
    // But for MaxPos, WrostPos, LiveOrders check we need to take the diff from the original size and then check
    if (originial_size_ != -1) {
      _size_requested_ -= originial_size_;
    }

    int pos = info->position;

    if (_buysell_ == kTradeTypeBuy) {
      // if not check if position could exceed max_worst_case_positions_ by allowing trade, assuming all the currently
      // resting orders will be filled

      int global_worst_bid_pos = std::max(0, pos + info->bid_size);
      int resultant_global_worst_bid_pos =
          weight_sign_ * ((_size_requested_ * this_security_weight_) + global_worst_bid_pos);

      if (is_query_cumulative_risk_check_enabled_ &&
          (resultant_global_worst_bid_pos > limits_[_security_id_].sum_clients_worstcase_pos_limit_)) {
        return kORSRejectMarginCheckFailedSumQueryWorstCasePosition;
      }

      if (resultant_global_worst_bid_pos > limit.max_worst_case_position_) {
        return kORSRejectMarginCheckFailedWorstCasePosition;
      }

    } else {
      // if not check if position could exceed max_worst_case_positions_ by allowing trade, assuming all the currently
      // resting orders will be filled

      int global_worst_ask_pos = std::max(0, info->ask_size - pos);
      int resultant_global_worst_ask_pos =
          weight_sign_ * ((_size_requested_ * this_security_weight_) + global_worst_ask_pos);

      if (is_query_cumulative_risk_check_enabled_ &&
          (resultant_global_worst_ask_pos > limits_[_security_id_].sum_clients_worstcase_pos_limit_)) {
        return kORSRejectMarginCheckFailedSumQueryWorstCasePosition;
      }

      if (resultant_global_worst_ask_pos > limit.max_worst_case_position_) {
        return kORSRejectMarginCheckFailedWorstCasePosition;
      }
    }

    if (is_price_checks_enabled_ &&
        (sec_id_to_last_traded_price_[_security_id_].last_trade_price == std::numeric_limits<double>::max() ||
         _price_ > sec_id_to_last_traded_price_[_security_id_].max_price ||
         _price_ < sec_id_to_last_traded_price_[_security_id_].min_price)) {
      // Race Condition. Should be harmless.
      return kORSRejectFailedPriceCheck;
    }

    if (false == ors_pnl_manager_.AllowsPnlCheck()) return kORSRejectFailedPnlCheck;

    if (true == ors_margin_manager_.IsMarginChecksInPlace()) {
      if (false == ors_margin_manager_.AllowsGrossMarginCheck()) return kORSRejectFailedGrossMarginCheck;
      if (false == ors_margin_manager_.AllowsNetMarginCheck()) return kORSRejectFailedNetMarginCheck;
    }

    return kORSOrderAllowed;
  }

  inline ORSRejectionReason_t AllowsBMFStocksExtendedChecks(const int32_t& _security_id_, const double& _price_,
                                                            const int32_t& _size_requested_,
                                                            const TradeType_t& _buysell_) {
    const OrderLimits& limit = limits_[_security_id_];

    if ((uint32_t)(_size_requested_ * _price_) > (uint32_t)limit.max_order_size_) {
      return kORSRejectMarginCheckFailedOrderSizes;
    }

    switch (_buysell_) {
      case HFSAT::kTradeTypeBuy: {
        int buy_size = position_manager_.GetGlobalBidSize(_security_id_) + _size_requested_ +
                       position_manager_.GetGlobalPosition(_security_id_);
        int buy_notional = (int)(buy_size * _price_);

        if (buy_notional > limit.max_position_) {
          return kORSRejectMarginCheckFailedBidNotionalValue;
        }
        break;
      }

      case HFSAT::kTradeTypeSell: {
        int sell_size = position_manager_.GetGlobalAskSize(_security_id_) + _size_requested_ -
                        position_manager_.GetGlobalPosition(_security_id_);
        int sell_notional = (int)(sell_size * _price_);

        if (sell_notional > limit.max_position_) {
          return kORSRejectMarginCheckFailedAskNotionalValue;
        }

        break;
      }

      default: {
        // Reject the order
        return kORSRejectFailedPriceCheck;
        break;
      }
    }

    return kORSOrderAllowed;
  }

  void SetPriceCheckMode(bool is_enabled) { is_price_checks_enabled_ = is_enabled; }
  void SetAllowedPriceTickLimit(int tick_size) {
    if (price_manager_ != nullptr) {
      price_manager_->SetAllowedPriceTicks(tick_size);
    }
  }

  void DumpLastTradedPrices() {
    if (is_price_checks_enabled_ && price_manager_ != nullptr) {
      price_manager_->ShowLastTradedPrices();
    }
  }

  void SetQueryCumulativeRiskChecks(bool is_enabled) {
    if (query_risk_checks_feature_flag_) {
      is_query_cumulative_risk_check_enabled_ = is_enabled;
    }
  }

  void SetQueryRiskFeatureFlag(bool is_enabled) {
    query_risk_checks_feature_flag_ = is_enabled;
    if (!query_risk_checks_feature_flag_) {
      is_query_cumulative_risk_check_enabled_ = false;
    }
  }

  inline void SetClientWorstCasePositionLimit(int _server_assigned_client_id_, int worstcase_pos_limit_, int sec_id) {
    // We have taken std::max() below because -1 denotes no limits specified. If limits are increased to 4, then we want
    // to add (4-0) to sum_clients_worstcase_pos_limit_ instead of (4-(-1))
    int current_worstcase_pos_limit =
        std::max(0, saci_to_query_limits_[SACItoKey(_server_assigned_client_id_)].worstcase_pos);
    int new_worstcase_pos_limit = std::max(0, worstcase_pos_limit_);

    limits_[sec_id].sum_clients_worstcase_pos_limit_ += (new_worstcase_pos_limit - current_worstcase_pos_limit);

    saci_to_query_limits_[SACItoKey(_server_assigned_client_id_)].worstcase_pos = worstcase_pos_limit_;
    saci_to_query_limits_[SACItoKey(_server_assigned_client_id_)].sec_id = sec_id;
  }

  inline int GetClientWorstCasePositionLimit(int _server_assigned_client_id_, int& output_sec_id) const {
    output_sec_id = saci_to_query_limits_[SACItoKey(_server_assigned_client_id_)].sec_id;
    return saci_to_query_limits_[SACItoKey(_server_assigned_client_id_)].worstcase_pos;
  }

 protected:
 private:
  // cache line aligned
  struct OrderLimits {
    int32_t max_position_;
    int32_t max_order_size_;
    int32_t max_worst_case_position_;  // ORS ADDTS Limit
    int32_t max_live_order_;
    int32_t sum_clients_worstcase_pos_limit_;  // Query Requested Limits, can be less than ORS ADDTS Limit

    // MiFid limits
    int32_t otr_min_vol_;
    int32_t otr_min_cnt_;
    int32_t otr_limit_vol_;
    int32_t otr_limit_cnt_;
    int32_t msg_count_;
  };

  DebugLogger& dbglogger_;
  const PositionManager& position_manager_;

  OrderLimits limits_[DEF_MAX_SEC_ID];
  SACILimitsInfo saci_to_query_limits_[ORS_MAX_NUM_OF_CLIENTS];

  bool is_price_checks_enabled_;
  HFSAT::Utils::LiveTradePriceManager* price_manager_;
  std::vector<HFSAT::Utils::LastTradePriceInfo> sec_id_to_last_traded_price_;

  HFSAT::ExchSource_t exchange_;
  HFSAT::ORSUtils::ORSPnlManager& ors_pnl_manager_;
  HFSAT::ORSUtils::ORSMarginManager& ors_margin_manager_;

  bool is_query_cumulative_risk_check_enabled_;
  bool query_risk_checks_feature_flag_;

  bool is_mifid_check_enabled_;
};
}
}

#endif  // BASE_BASICORDERROUTINGSERVER_MARGINCHECKER_H
