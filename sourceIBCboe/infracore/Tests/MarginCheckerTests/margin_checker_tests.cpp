#include "dvccode/CDef/defines.hpp"
#include "infracore/Tests/MarginCheckerTests/margin_checker_tests.hpp"

namespace HFTEST {

MarginCheckerTests::MarginCheckerTests()
    : exchange_("NSE"), margin_checker_tests_helper_(new MarginCheckerTestsHelper(exchange_)) {}

void MarginCheckerTests::BuyOrderSizeExceedLimitSingleSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 200;

  order_limits.max_order_size_ = 100;
  order.price = 3;
  order.size = 200;
  order.side = HFSAT::kTradeTypeBuy;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result == HFSAT::kORSRejectMarginCheckFailedOrderSizes);
}

void MarginCheckerTests::BuyOrderSizeBelowLimitSingleSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 201;

  order_limits.max_order_size_ = 100;
  order.price = 3;
  order.size = 98;
  order.side = HFSAT::kTradeTypeBuy;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result != HFSAT::kORSRejectMarginCheckFailedOrderSizes);
}

void MarginCheckerTests::BuyOrderSizeEqualsLimitSingleSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 202;

  order_limits.max_order_size_ = 100;
  order.price = 3;
  order.size = 100;
  order.side = HFSAT::kTradeTypeBuy;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result != HFSAT::kORSRejectMarginCheckFailedOrderSizes);
}
void MarginCheckerTests::SellOrderSizeExceedLimitSingleSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 203;

  order_limits.max_order_size_ = 100;
  order.price = 3;
  order.size = 200;
  order.side = HFSAT::kTradeTypeSell;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result == HFSAT::kORSRejectMarginCheckFailedOrderSizes);
}

void MarginCheckerTests::SellOrderSizeBelowLimitSingleSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 204;

  order_limits.max_order_size_ = 100;
  order.price = 3;
  order.size = 98;
  order.side = HFSAT::kTradeTypeSell;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result != HFSAT::kORSRejectMarginCheckFailedOrderSizes);
}

void MarginCheckerTests::SellOrderSizeEqualsLimitSingleSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 205;

  order_limits.max_order_size_ = 100;
  order.price = 3;
  order.size = 100;
  order.side = HFSAT::kTradeTypeSell;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result != HFSAT::kORSRejectMarginCheckFailedOrderSizes);
}

void MarginCheckerTests::BuyOrderSizeExceedLimitCombinedSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 206;
  const int combined_security_id = 307;
  const double security_weight = 0.8;

  order_limits.max_order_size_ = 400;
  order.price = 7;
  order.size = 600;
  order.side = HFSAT::kTradeTypeBuy;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckCombinedSecurity(position_info, security_id, security_weight,
                                                             combined_security_id, order_limits, order,
                                                             kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result == HFSAT::kORSRejectMarginCheckFailedOrderSizes);
}

void MarginCheckerTests::BuyOrderSizeBelowLimitCombinedSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 207;
  const int combined_security_id = 307;
  const double security_weight = 0.8;

  order_limits.max_order_size_ = 400;
  order.price = 9;
  order.size = 300;
  order.side = HFSAT::kTradeTypeBuy;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckCombinedSecurity(position_info, security_id, security_weight,
                                                             combined_security_id, order_limits, order,
                                                             kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result != HFSAT::kORSRejectMarginCheckFailedOrderSizes);
}

void MarginCheckerTests::BuyOrderSizeEqualsLimitCombinedSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 208;
  const int combined_security_id = 308;
  const double security_weight = 0.8;

  order_limits.max_order_size_ = 400;
  order.price = 9;
  order.size = 500;
  order.side = HFSAT::kTradeTypeBuy;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckCombinedSecurity(position_info, security_id, security_weight,
                                                             combined_security_id, order_limits, order,
                                                             kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result != HFSAT::kORSRejectMarginCheckFailedOrderSizes);
}

void MarginCheckerTests::SellOrderSizeExceedLimitCombinedSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 209;
  const int combined_security_id = 309;
  const double security_weight = 0.8;

  order_limits.max_order_size_ = 400;
  order.price = 7;
  order.size = 600;
  order.side = HFSAT::kTradeTypeSell;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckCombinedSecurity(position_info, security_id, security_weight,
                                                             combined_security_id, order_limits, order,
                                                             kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result == HFSAT::kORSRejectMarginCheckFailedOrderSizes);
}

void MarginCheckerTests::SellOrderSizeBelowLimitCombinedSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 210;
  const int combined_security_id = 310;
  const double security_weight = 0.8;

  order_limits.max_order_size_ = 400;
  order.price = 9;
  order.size = 300;
  order.side = HFSAT::kTradeTypeSell;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckCombinedSecurity(position_info, security_id, security_weight,
                                                             combined_security_id, order_limits, order,
                                                             kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result != HFSAT::kORSRejectMarginCheckFailedOrderSizes);
}

void MarginCheckerTests::SellOrderSizeEqualsLimitCombinedSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 211;
  const int combined_security_id = 311;
  const double security_weight = 0.8;

  order_limits.max_order_size_ = 400;
  order.price = 9;
  order.size = 500;
  order.side = HFSAT::kTradeTypeSell;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckCombinedSecurity(position_info, security_id, security_weight,
                                                             combined_security_id, order_limits, order,
                                                             kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result != HFSAT::kORSRejectMarginCheckFailedOrderSizes);
}

void MarginCheckerTests::BuyWorstCasePositionExceedSingleSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 212;

  order_limits.max_order_size_ = 400;
  order_limits.max_worst_case_position_ = 600;
  position_info.add_bid_ask_size = 300;
  position_info.trade_size = 200;
  order.price = 3;
  order.size = 200;
  order.side = HFSAT::kTradeTypeBuy;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result == HFSAT::kORSRejectMarginCheckFailedWorstCasePosition);
}

void MarginCheckerTests::BuyWorstCasePositionEqualsSingleSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 213;

  order_limits.max_order_size_ = 700;
  order_limits.max_worst_case_position_ = 800;
  position_info.add_bid_ask_size = 100;
  position_info.trade_size = 100;
  order.price = 3;
  order.size = 600;
  order.side = HFSAT::kTradeTypeBuy;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kOrderSizePosition, margin_check_result);
  CPPUNIT_ASSERT(margin_check_result != HFSAT::kORSRejectMarginCheckFailedWorstCasePosition);
}

void MarginCheckerTests::BuyWorstCasePositionBelowSingleSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 214;

  order_limits.max_order_size_ = 700;
  order_limits.max_worst_case_position_ = 800;
  position_info.add_bid_ask_size = 300;
  position_info.trade_size = 200;
  order.price = 3;
  order.size = 200;
  order.side = HFSAT::kTradeTypeBuy;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result != HFSAT::kORSRejectMarginCheckFailedWorstCasePosition);
}

void MarginCheckerTests::SellWorstCasePositionExceedSingleSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 215;

  order_limits.max_order_size_ = 400;
  order_limits.max_worst_case_position_ = 600;
  position_info.add_bid_ask_size = 300;
  position_info.trade_size = 200;
  order.price = 3;
  order.size = 200;
  order.side = HFSAT::kTradeTypeSell;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result == HFSAT::kORSRejectMarginCheckFailedWorstCasePosition);
}

void MarginCheckerTests::SellWorstCasePositionEqualsSingleSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 216;

  order_limits.max_order_size_ = 700;
  order_limits.max_worst_case_position_ = 800;
  position_info.add_bid_ask_size = 100;
  position_info.trade_size = 100;
  order.price = 3;
  order.size = 600;
  order.side = HFSAT::kTradeTypeSell;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kOrderSizePosition, margin_check_result);
  CPPUNIT_ASSERT(margin_check_result != HFSAT::kORSRejectMarginCheckFailedWorstCasePosition);
}

void MarginCheckerTests::SellWorstCasePositionBelowSingleSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 217;

  order_limits.max_order_size_ = 700;
  order_limits.max_worst_case_position_ = 800;
  position_info.add_bid_ask_size = 300;
  position_info.trade_size = 200;
  order.price = 3;
  order.size = 200;
  order.side = HFSAT::kTradeTypeSell;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result != HFSAT::kORSRejectMarginCheckFailedWorstCasePosition);
}

void MarginCheckerTests::WorstCaseBuyPositionExceedCombinedSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 218;
  const int combined_security_id = 318;
  const double security_weight = 0.9;

  order_limits.max_order_size_ = 800;
  order_limits.max_worst_case_position_ = 700;
  position_info.add_bid_ask_size = 400;
  position_info.trade_size = 200;
  order.price = 7;
  order.size = 200;
  order.side = HFSAT::kTradeTypeBuy;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckCombinedSecurity(position_info, security_id, security_weight,
                                                             combined_security_id, order_limits, order,
                                                             kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result == HFSAT::kORSRejectMarginCheckFailedWorstCasePosition);
}

void MarginCheckerTests::WorstCaseBuyPositionEqualsCombinedSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 219;
  const int combined_security_id = 319;
  const double security_weight = 0.9;

  order_limits.max_order_size_ = 800;
  order_limits.max_worst_case_position_ = 720;
  position_info.add_bid_ask_size = 400;
  position_info.trade_size = 200;
  order.price = 7;
  order.size = 200;
  order.side = HFSAT::kTradeTypeBuy;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckCombinedSecurity(position_info, security_id, security_weight,
                                                             combined_security_id, order_limits, order,
                                                             kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result != HFSAT::kORSRejectMarginCheckFailedWorstCasePosition);
}

void MarginCheckerTests::WorstCaseBuyPositionBelowCombinedSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 220;
  const int combined_security_id = 320;
  const double security_weight = 0.9;

  order_limits.max_order_size_ = 800;
  order_limits.max_worst_case_position_ = 740;
  position_info.add_bid_ask_size = 400;
  position_info.trade_size = 200;
  order.price = 7;
  order.size = 200;
  order.side = HFSAT::kTradeTypeBuy;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckCombinedSecurity(position_info, security_id, security_weight,
                                                             combined_security_id, order_limits, order,
                                                             kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result != HFSAT::kORSRejectMarginCheckFailedWorstCasePosition);
}
void MarginCheckerTests::WorstCaseSellPositionExceedCombinedSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 221;
  const int combined_security_id = 321;
  const double security_weight = 0.9;

  order_limits.max_order_size_ = 800;
  order_limits.max_worst_case_position_ = 700;
  position_info.add_bid_ask_size = 400;
  position_info.trade_size = 200;
  order.price = 7;
  order.size = 200;
  order.side = HFSAT::kTradeTypeSell;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckCombinedSecurity(position_info, security_id, security_weight,
                                                             combined_security_id, order_limits, order,
                                                             kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result == HFSAT::kORSRejectMarginCheckFailedWorstCasePosition);
}

void MarginCheckerTests::WorstCaseSellPositionEqualsCombinedSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 222;
  const int combined_security_id = 322;
  const double security_weight = 0.9;

  order_limits.max_order_size_ = 800;
  order_limits.max_worst_case_position_ = 720;
  position_info.add_bid_ask_size = 400;
  position_info.trade_size = 200;
  order.price = 7;
  order.size = 200;
  order.side = HFSAT::kTradeTypeSell;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckCombinedSecurity(position_info, security_id, security_weight,
                                                             combined_security_id, order_limits, order,
                                                             kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result != HFSAT::kORSRejectMarginCheckFailedWorstCasePosition);
}

void MarginCheckerTests::WorstCaseSellPositionBelowCombinedSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 223;
  const int combined_security_id = 323;
  const double security_weight = 0.9;

  order_limits.max_order_size_ = 800;
  order_limits.max_worst_case_position_ = 740;
  position_info.add_bid_ask_size = 400;
  position_info.trade_size = 200;
  order.price = 7;
  order.size = 200;
  order.side = HFSAT::kTradeTypeSell;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckCombinedSecurity(position_info, security_id, security_weight,
                                                             combined_security_id, order_limits, order,
                                                             kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result != HFSAT::kORSRejectMarginCheckFailedWorstCasePosition);
}

void MarginCheckerTests::BuyOrderSizeExceedsMaxPositionSingleSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 224;

  order_limits.max_order_size_ = 50;
  order_limits.max_worst_case_position_ = 800;
  order_limits.max_position_ = 15;
  position_info.add_bid_ask_size = 15;
  position_info.trade_size = 10;
  order.price = 3;
  order.size = 6;
  order.side = HFSAT::kTradeTypeBuy;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kIndividualSecurityMarginCheck, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result == HFSAT::kORSRejectMarginCheckFailedMaxPosition);
}

void MarginCheckerTests::BuyOrderSizeExceedsWorstCasePosSingleSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 225;

  order_limits.max_order_size_ = 40;
  order_limits.max_worst_case_position_ = 25;
  order_limits.max_position_ = 25;
  position_info.add_bid_ask_size = 10;
  position_info.trade_size = 10;
  order.price = 3;
  order.size = 10;
  order.side = HFSAT::kTradeTypeBuy;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kIndividualSecurityMarginCheck, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result == HFSAT::kORSRejectMarginCheckFailedWorstCasePosition);
}

void MarginCheckerTests::BuyOrderSizeExceedsLimitBMFStocks() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 226;

  order_limits.max_order_size_ = 100;
  order.price = 2;
  order.size = 51;
  order.side = HFSAT::kTradeTypeBuy;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kBMFStockExtendedCheck, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result == HFSAT::kORSRejectMarginCheckFailedOrderSizes);
}

void MarginCheckerTests::SellOrderSizeExceedsMaxPositionSingleSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 227;

  order_limits.max_order_size_ = 50;
  order_limits.max_worst_case_position_ = 800;
  order_limits.max_position_ = 15;
  position_info.add_bid_ask_size = 15;
  position_info.trade_size = 10;
  order.price = 3;
  order.size = 6;
  order.side = HFSAT::kTradeTypeSell;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kIndividualSecurityMarginCheck, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result == HFSAT::kORSRejectMarginCheckFailedMaxPosition);
}

void MarginCheckerTests::SellOrderSizeExceedsWorstCasePosSingleSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 228;

  order_limits.max_order_size_ = 40;
  order_limits.max_worst_case_position_ = 25;
  order_limits.max_position_ = 25;
  position_info.add_bid_ask_size = 10;
  position_info.trade_size = 10;
  order.price = 3;
  order.size = 10;
  order.side = HFSAT::kTradeTypeSell;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kIndividualSecurityMarginCheck, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result == HFSAT::kORSRejectMarginCheckFailedWorstCasePosition);
}

void MarginCheckerTests::SellOrderSizeExceedsLimitBMFStocks() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 229;

  order_limits.max_order_size_ = 100;
  order.price = 2;
  order.size = 51;
  order.side = HFSAT::kTradeTypeSell;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kBMFStockExtendedCheck, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result == HFSAT::kORSRejectMarginCheckFailedOrderSizes);
}

void MarginCheckerTests::BuyNotionalValueExceedsLimitBMFStocks() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 230;

  order_limits.max_order_size_ = 200;
  order_limits.max_position_ = 100;
  position_info.add_bid_ask_size = 20;
  position_info.trade_size = 16;
  order.price = 2;
  order.size = 15;
  order.side = HFSAT::kTradeTypeBuy;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kBMFStockExtendedCheck, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result == HFSAT::kORSRejectMarginCheckFailedBidNotionalValue);
}

void MarginCheckerTests::SellNotionalValueExceedsLimitBMFStocks() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 231;

  order_limits.max_order_size_ = 200;
  order_limits.max_position_ = 100;
  position_info.add_bid_ask_size = 20;
  position_info.trade_size = 16;
  order.price = 2;
  order.size = 15;
  order.side = HFSAT::kTradeTypeSell;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kBMFStockExtendedCheck, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result == HFSAT::kORSRejectMarginCheckFailedAskNotionalValue);
}

void MarginCheckerTests::ReplaceBuyOrderSizeExceedLimitSingleSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 232;

  order_limits.max_order_size_ = 100;
  order.price = 3;
  order.size = 200;
  order.side = HFSAT::kTradeTypeBuy;
  order.original_size = 90;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result == HFSAT::kORSRejectMarginCheckFailedOrderSizes);
}

void MarginCheckerTests::ReplaceBuyOrderSizeBelowLimitSingleSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 233;

  order_limits.max_order_size_ = 100;
  order.price = 3;
  order.size = 98;
  order.side = HFSAT::kTradeTypeBuy;
  order.original_size = 80;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result != HFSAT::kORSRejectMarginCheckFailedOrderSizes);
}

void MarginCheckerTests::ReplaceBuyOrderSizeEqualsLimitSingleSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 234;

  order_limits.max_order_size_ = 100;
  order.price = 3;
  order.size = 100;
  order.side = HFSAT::kTradeTypeBuy;
  order.original_size = 90;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result != HFSAT::kORSRejectMarginCheckFailedOrderSizes);
}
void MarginCheckerTests::ReplaceSellOrderSizeExceedLimitSingleSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 235;

  order_limits.max_order_size_ = 100;
  order.price = 3;
  order.size = 200;
  order.side = HFSAT::kTradeTypeSell;
  order.original_size = 90;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result == HFSAT::kORSRejectMarginCheckFailedOrderSizes);
}

void MarginCheckerTests::ReplaceSellOrderSizeBelowLimitSingleSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 236;

  order_limits.max_order_size_ = 100;
  order.price = 3;
  order.size = 98;
  order.side = HFSAT::kTradeTypeSell;
  order.original_size = 80;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result != HFSAT::kORSRejectMarginCheckFailedOrderSizes);
}

void MarginCheckerTests::ReplaceSellOrderSizeEqualsLimitSingleSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 237;

  order_limits.max_order_size_ = 100;
  order.price = 3;
  order.size = 100;
  order.side = HFSAT::kTradeTypeSell;
  order.original_size = 90;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result != HFSAT::kORSRejectMarginCheckFailedOrderSizes);
}

void MarginCheckerTests::WorstCaseBuyPositionReplaceExceedSingleSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 238;

  order_limits.max_order_size_ = 400;
  order_limits.max_worst_case_position_ = 600;
  position_info.add_bid_ask_size = 300;
  position_info.trade_size = 200;
  order.price = 3;
  order.size = 200;
  order.side = HFSAT::kTradeTypeBuy;
  order.original_size = 99;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result == HFSAT::kORSRejectMarginCheckFailedWorstCasePosition);
}

void MarginCheckerTests::WorstCaseBuyPositionReplaceEqualsSingleSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 239;

  order_limits.max_order_size_ = 700;
  order_limits.max_worst_case_position_ = 700;
  position_info.add_bid_ask_size = 100;
  position_info.trade_size = 100;
  order.price = 3;
  order.size = 600;
  order.side = HFSAT::kTradeTypeBuy;
  order.original_size = 100;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kOrderSizePosition, margin_check_result);
  CPPUNIT_ASSERT(margin_check_result != HFSAT::kORSRejectMarginCheckFailedWorstCasePosition);
}

void MarginCheckerTests::WorstCaseBuyPositionReplaceBelowSingleSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 240;

  order_limits.max_order_size_ = 700;
  order_limits.max_worst_case_position_ = 800;
  position_info.add_bid_ask_size = 300;
  position_info.trade_size = 200;
  order.price = 3;
  order.size = 300;
  order.side = HFSAT::kTradeTypeBuy;
  order.original_size = 100;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result != HFSAT::kORSRejectMarginCheckFailedWorstCasePosition);
}

void MarginCheckerTests::WorstCaseSellPositionReplaceExceedSingleSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 241;

  order_limits.max_order_size_ = 400;
  order_limits.max_worst_case_position_ = 600;
  position_info.add_bid_ask_size = 300;
  position_info.trade_size = 200;
  order.price = 3;
  order.size = 200;
  order.side = HFSAT::kTradeTypeSell;
  order.original_size = 99;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result == HFSAT::kORSRejectMarginCheckFailedWorstCasePosition);
}

void MarginCheckerTests::WorstCaseSellPositionReplaceEqualsSingleSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 242;

  order_limits.max_order_size_ = 700;
  order_limits.max_worst_case_position_ = 700;
  position_info.add_bid_ask_size = 100;
  position_info.trade_size = 100;
  order.price = 3;
  order.size = 600;
  order.side = HFSAT::kTradeTypeSell;
  order.original_size = 100;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kOrderSizePosition, margin_check_result);
  CPPUNIT_ASSERT(margin_check_result != HFSAT::kORSRejectMarginCheckFailedWorstCasePosition);
}

void MarginCheckerTests::WorstCaseSellPositionReplaceBelowSingleSecurity() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 243;

  order_limits.max_order_size_ = 700;
  order_limits.max_worst_case_position_ = 800;
  position_info.add_bid_ask_size = 300;
  position_info.trade_size = 200;
  order.price = 3;
  order.size = 300;
  order.side = HFSAT::kTradeTypeSell;
  order.original_size = 100;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result != HFSAT::kORSRejectMarginCheckFailedWorstCasePosition);
}

void MarginCheckerTests::BidNotionalValueReject() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 244;

  position_info.add_bid_ask_size = 10;
  position_info.trade_size = 40;
  order_limits.max_order_size_ = 120;
  order_limits.max_position_ = 190;
  order.price = 2;
  order.size = 50;
  order.side = HFSAT::kTradeTypeBuy;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kBMFStockExtendedCheck, margin_check_result);
  CPPUNIT_ASSERT(margin_check_result == HFSAT::kORSRejectMarginCheckFailedBidNotionalValue);
}

void MarginCheckerTests::AskNotionalValueReject() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 245;

  position_info.add_bid_ask_size = 10;
  position_info.trade_size = 40;
  order_limits.max_order_size_ = 120;
  order_limits.max_position_ = 190;
  order.price = 2;
  order.size = 50;
  order.side = HFSAT::kTradeTypeSell;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kBMFStockExtendedCheck, margin_check_result);
  CPPUNIT_ASSERT(margin_check_result == HFSAT::kORSRejectMarginCheckFailedAskNotionalValue);
}

void MarginCheckerTests::FailedPriceCheck() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 246;

  position_info.add_bid_ask_size = 10;
  position_info.trade_size = 40;
  order_limits.max_order_size_ = 120;
  order_limits.max_position_ = 190;
  order.price = 2;
  order.size = 50;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kBMFStockExtendedCheck, margin_check_result);
  CPPUNIT_ASSERT(margin_check_result == HFSAT::kORSRejectFailedPriceCheck);
}
void MarginCheckerTests::NegativeWeightBuyCombinedSecurityExceedLimit() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 247;
  const int combined_security_id = 347;
  const double security_weight = 0.8;

  order_limits.max_order_size_ = 400;
  order.price = 7;
  order.size = 600;
  order.side = HFSAT::kTradeTypeBuy;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckCombinedSecurity(position_info, security_id, security_weight,
                                                             combined_security_id, order_limits, order,
                                                             kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result == HFSAT::kORSRejectMarginCheckFailedOrderSizes);
}
void MarginCheckerTests::NegativeWeightBuyCombinedSecurityBelowLimit() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 248;
  const int combined_security_id = 348;
  const double security_weight = -0.8;

  order_limits.max_order_size_ = 400;
  order.price = 7;
  order.size = 500;
  order.side = HFSAT::kTradeTypeBuy;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckCombinedSecurity(position_info, security_id, security_weight,
                                                             combined_security_id, order_limits, order,
                                                             kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result != HFSAT::kORSRejectMarginCheckFailedOrderSizes);
}

void MarginCheckerTests::NegativeWeightSellCombinedSecurityExceedLimit() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 249;
  const int combined_security_id = 349;
  const double security_weight = 0.8;

  order_limits.max_order_size_ = 400;
  order.price = 7;
  order.size = 600;
  order.side = HFSAT::kTradeTypeSell;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckCombinedSecurity(position_info, security_id, security_weight,
                                                             combined_security_id, order_limits, order,
                                                             kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result == HFSAT::kORSRejectMarginCheckFailedOrderSizes);
}
void MarginCheckerTests::NegativeWeightSellCombinedSecurityBelowLimit() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 250;
  const int combined_security_id = 350;
  const double security_weight = -0.8;

  order_limits.max_order_size_ = 400;
  order.price = 7;
  order.size = 500;
  order.side = HFSAT::kTradeTypeSell;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckCombinedSecurity(position_info, security_id, security_weight,
                                                             combined_security_id, order_limits, order,
                                                             kOrderSizePosition, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result != HFSAT::kORSRejectMarginCheckFailedOrderSizes);
}

void MarginCheckerTests::BuyAndSellMaxPositionLimitExceed() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 251;

  position_info.trade_size = 50;
  margin_checker_tests_helper_->UpdatePositionMgr(HFSAT::kTradeTypeBuy, position_info, security_id);

  position_info.trade_size = 20;
  margin_checker_tests_helper_->UpdatePositionMgr(HFSAT::kTradeTypeSell, position_info, security_id);

  order_limits.max_position_ = 35;
  order_limits.max_order_size_ = 10;

  order.price = 7;
  order.size = 6;
  order.side = HFSAT::kTradeTypeBuy;

  position_info.trade_size = 0;
  position_info.add_bid_ask_size = 0;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kIndividualSecurityMarginCheck, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result == HFSAT::kORSRejectMarginCheckFailedMaxPosition);
}

void MarginCheckerTests::BuyAndSellWorstCasePositionLimitExceed() {
  Positioninfo position_info;
  OrderLimits order_limits;
  Order order;
  const int security_id = 252;

  position_info.trade_size = 100;
  position_info.add_bid_ask_size = 40;
  margin_checker_tests_helper_->UpdatePositionMgr(HFSAT::kTradeTypeBuy, position_info, security_id);

  position_info.trade_size = 50;
  position_info.add_bid_ask_size = 60;
  margin_checker_tests_helper_->UpdatePositionMgr(HFSAT::kTradeTypeSell, position_info, security_id);

  order_limits.max_order_size_ = 35;
  order_limits.max_worst_case_position_ = 100;
  order_limits.max_position_ = 85;

  order.price = 7;
  order.size = 15;
  order.side = HFSAT::kTradeTypeBuy;

  position_info.trade_size = 0;
  position_info.add_bid_ask_size = 0;

  HFSAT::ORSRejectionReason_t margin_check_result;
  margin_checker_tests_helper_->PerformCheckSingleSecurity(position_info, security_id, order_limits, order,
                                                           kIndividualSecurityMarginCheck, margin_check_result);

  CPPUNIT_ASSERT(margin_check_result == HFSAT::kORSRejectMarginCheckFailedWorstCasePosition);
}
}
