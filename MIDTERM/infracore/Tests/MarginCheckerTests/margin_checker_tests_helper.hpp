#ifndef _Margin_Checker_TESTS_HELPER
#define _Margin_Checker_TESTS_HELPER

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/defines.hpp"

#include "infracore/BasicOrderRoutingServer/position_manager.hpp"
#include "infracore/BasicOrderRoutingServer/margin_checker.hpp"

namespace HFTEST {

struct Positioninfo {
  int trade_size;
  int add_bid_ask_size;
  int dec_bid_ask_size;
  int internal_trade_size;

  Positioninfo() : trade_size(0), add_bid_ask_size(0), dec_bid_ask_size(0), internal_trade_size(0) {}
};

struct OrderLimits {
  int32_t max_position_;
  int32_t max_order_size_;
  int32_t max_worst_case_position_;
  int32_t max_live_order_;

  OrderLimits() : max_position_(0), max_order_size_(0), max_worst_case_position_(0), max_live_order_(0) {}
};
struct Order {
  int size;
  int original_size;
  double price;
  HFSAT::TradeType_t side;
  Order() : size(0), original_size(-1), price(3), side(HFSAT::kTradeTypeNoInfo) {}
};
typedef enum {
  kOrderSizePosition,
  kIndividualSecurityMarginCheck,
  kBMFStockExtendedCheck,
} CheckType;

class MarginCheckerTestsHelper {
 public:
  MarginCheckerTestsHelper(std::string exchange);
  void PerformCheckSingleSecurity(const Positioninfo& position_data, const int& security_id,
                                  const OrderLimits& order_limits, const Order& order, const CheckType& check_type,
                                  HFSAT::ORSRejectionReason_t& result);
  void PerformCheckCombinedSecurity(const Positioninfo& position_data, const int& security_id,
                                    const double& security_weight, const int& combined_security_id,
                                    const OrderLimits& order_limits, const Order& order, const CheckType& check_type,
                                    HFSAT::ORSRejectionReason_t& result);
  void UpdatePositionMgr(const HFSAT::TradeType_t order_side, const Positioninfo position_data, const int security_id);

 protected:
  void SetSingleSecurityPositionAndOrderLimits(const HFSAT::TradeType_t order_side, const int security_id,
                                               const Positioninfo position_data, const OrderLimits order_limits);
  void SetCombinedSecurityPositionAndOrderLimits(const HFSAT::TradeType_t order_side, const Positioninfo position_data,
                                                 const int security_id, const OrderLimits order_limits,
                                                 const double& security_weight, const int& combined_security_id);

 private:
  std::string exchange_;
  HFSAT::DebugLogger dbglogger_;
  HFSAT::ORS::PositionManager& position_manager_;
  HFSAT::ORS::MarginChecker& margin_checker_;
};
}

#endif
