#include "infracore/Tests/MarginCheckerTests/margin_checker_tests_helper.hpp"

namespace HFTEST {
MarginCheckerTestsHelper::MarginCheckerTestsHelper(std::string exchange)
    : exchange_(exchange),
      dbglogger_(1024000, 1),
      position_manager_(HFSAT::ORS::PositionManager::GetUniqueInstance()),
      margin_checker_(HFSAT::ORS::MarginChecker::GetUniqueInstance(dbglogger_, HFSAT::StringToExchSource(exchange_))) {}

void MarginCheckerTestsHelper::UpdatePositionMgr(const HFSAT::TradeType_t order_side, const Positioninfo position_data,
                                                 const int security_id) {
  if (order_side == HFSAT::kTradeTypeBuy) {
    position_manager_.AddBidSize(security_id, position_data.add_bid_ask_size, 0, 0);
    position_manager_.AddBuyTrade(security_id, -1, position_data.trade_size, false);
    position_manager_.DecBidSize(security_id, position_data.dec_bid_ask_size, 0, 0);
  } else if (order_side == HFSAT::kTradeTypeSell) {
    position_manager_.AddAskSize(security_id, position_data.add_bid_ask_size, 0, 0);
    position_manager_.AddSellTrade(security_id, -1, position_data.trade_size, false);
    position_manager_.DecAskSize(security_id, position_data.dec_bid_ask_size, 0, 0);
  }
}

void MarginCheckerTestsHelper::SetSingleSecurityPositionAndOrderLimits(const HFSAT::TradeType_t order_side,
                                                                       const int security_id,
                                                                       const Positioninfo position_data,
                                                                       const OrderLimits order_limits) {
  margin_checker_.Set(security_id, order_limits.max_position_, order_limits.max_order_size_,
                      order_limits.max_worst_case_position_, order_limits.max_live_order_);
  UpdatePositionMgr(order_side, position_data, security_id);
}
void MarginCheckerTestsHelper::SetCombinedSecurityPositionAndOrderLimits(
    const HFSAT::TradeType_t order_side, const Positioninfo position_data, const int security_id,
    const OrderLimits order_limits, const double& security_weight, const int& combined_security_id) {
  position_manager_.AddSecurityToCombinedPool(security_id, combined_security_id, security_weight, false);
  margin_checker_.Set(combined_security_id, order_limits.max_position_, order_limits.max_order_size_,
                      order_limits.max_worst_case_position_, order_limits.max_live_order_);
  UpdatePositionMgr(order_side, position_data, security_id);
}

void MarginCheckerTestsHelper::PerformCheckSingleSecurity(const Positioninfo& position_data, const int& security_id,
                                                          const OrderLimits& order_limits, const Order& order,
                                                          const CheckType& check_type,
                                                          HFSAT::ORSRejectionReason_t& result) {
  SetSingleSecurityPositionAndOrderLimits(order.side, security_id, position_data, order_limits);

  switch (check_type) {
    case kOrderSizePosition: {
      result = margin_checker_.Allows(security_id, order.price, order.size, order.side, order.original_size);
    } break;
    case kIndividualSecurityMarginCheck: {
      result = margin_checker_.EnforceIndividualSecurityMarginChecks(security_id, order.size, order.side,
                                                                     order.original_size);
    } break;
    case kBMFStockExtendedCheck: {
      result = margin_checker_.AllowsBMFStocksExtendedChecks(security_id, order.price, order.size, order.side);
    } break;
  }
}

void MarginCheckerTestsHelper::PerformCheckCombinedSecurity(const Positioninfo& position_data, const int& security_id,
                                                            const double& security_weight,
                                                            const int& combined_security_id,
                                                            const OrderLimits& order_limits, const Order& order,
                                                            const CheckType& check_type,
                                                            HFSAT::ORSRejectionReason_t& result) {
  SetCombinedSecurityPositionAndOrderLimits(order.side, position_data, security_id, order_limits, security_weight,
                                            combined_security_id);
  switch (check_type) {
    case kOrderSizePosition: {
      result = margin_checker_.Allows(security_id, order.price, order.size, order.side, order.original_size);
    } break;
    case kIndividualSecurityMarginCheck: {
      result = margin_checker_.EnforceIndividualSecurityMarginChecks(security_id, order.size, order.side,
                                                                     order.original_size);
    } break;
    case kBMFStockExtendedCheck: {
      result = margin_checker_.AllowsBMFStocksExtendedChecks(security_id, order.price, order.size, order.side);
    } break;
  }
}
}
