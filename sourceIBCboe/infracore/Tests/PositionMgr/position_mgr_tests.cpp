#include "infracore/Tests/PositionMgr/position_mgr_tests.hpp"

namespace HFTEST {
PositionMgrTests::PositionMgrTests()
    : position_mgr_test_helper(new HFTEST::PositionMgrTestsHelper),
      position_mgr_(HFSAT::ORS::PositionManager::GetUniqueInstance()) {}

void PositionMgrTests::SingleSecurityGlobalBidSize() {
  OrderStruct buy_order;
  buy_order.side_ = HFSAT::kTradeTypeBuy;
  buy_order.security_id_ = 1;
  buy_order.msg_type_ = HFTEST::DELTAINC;
  buy_order.size_ = 10;
  buy_order.saci_ = 103;
  buy_order.price_ = 70;

  position_mgr_test_helper->AddOrderInfo(buy_order);
  CPPUNIT_ASSERT(position_mgr_.GetGlobalBidSize(buy_order.security_id_) == 10);
}

void PositionMgrTests::SingleSecurityGlobalAskSize() {
  OrderStruct sell_order;
  sell_order.side_ = HFSAT::kTradeTypeSell;
  sell_order.security_id_ = 2;
  sell_order.msg_type_ = HFTEST::DELTAINC;
  sell_order.size_ = 50;
  sell_order.saci_ = 101;
  sell_order.price_ = 11;

  position_mgr_test_helper->AddOrderInfo(sell_order);
  CPPUNIT_ASSERT(position_mgr_.GetGlobalAskSize(sell_order.security_id_) == sell_order.size_);
}

void PositionMgrTests::SingleSecurityLiveSellOrders() {
  OrderStruct sell_order;
  sell_order.side_ = HFSAT::kTradeTypeSell;
  sell_order.security_id_ = 3;
  sell_order.msg_type_ = HFTEST::DELTAINC;
  sell_order.size_ = 50;
  sell_order.saci_ = 102;
  sell_order.price_ = 12;
  position_mgr_test_helper->AddOrderInfo(sell_order);
  CPPUNIT_ASSERT(position_mgr_.CheckifAnyLiveSellOrders(sell_order.security_id_) == true);
}

void PositionMgrTests::SingleSecurityLiveBuyOrders() {
  OrderStruct buy_order;
  buy_order.side_ = HFSAT::kTradeTypeBuy;
  buy_order.security_id_ = 4;
  buy_order.msg_type_ = HFTEST::DELTAINC;
  buy_order.size_ = 25;
  buy_order.saci_ = 103;
  buy_order.price_ = 11;
  position_mgr_test_helper->AddOrderInfo(buy_order);
  CPPUNIT_ASSERT(position_mgr_.CheckifAnyLiveBuyOrders(buy_order.security_id_) == true);
}

void PositionMgrTests::SingleSecurityGlobalPositivePosition() {
  OrderStruct buy_order;
  buy_order.side_ = HFSAT::kTradeTypeBuy;
  buy_order.security_id_ = 5;
  buy_order.msg_type_ = HFTEST::TRADE;
  buy_order.size_ = 100;
  buy_order.saci_ = 103;
  buy_order.price_ = 11;

  OrderStruct sell_order;
  sell_order.side_ = HFSAT::kTradeTypeSell;
  sell_order.security_id_ = 5;
  sell_order.msg_type_ = HFTEST::TRADE;
  sell_order.size_ = 75;
  sell_order.saci_ = 103;
  sell_order.price_ = 11;
  position_mgr_test_helper->AddOrderInfo(buy_order);
  position_mgr_test_helper->AddOrderInfo(sell_order);
  CPPUNIT_ASSERT(position_mgr_.GetGlobalPosition(buy_order.security_id_) == (buy_order.size_ - sell_order.size_));
}

void PositionMgrTests::SingleSecurityGlobalNegativePosition() {
  OrderStruct buy_order;
  buy_order.side_ = HFSAT::kTradeTypeBuy;
  buy_order.security_id_ = 6;
  buy_order.msg_type_ = HFTEST::TRADE;
  buy_order.size_ = 25;
  buy_order.saci_ = 103;
  buy_order.price_ = 11;

  OrderStruct sell_order;
  sell_order.side_ = HFSAT::kTradeTypeSell;
  sell_order.security_id_ = 6;
  sell_order.msg_type_ = HFTEST::TRADE;
  sell_order.size_ = 100;
  sell_order.saci_ = 103;
  sell_order.price_ = 11;
  position_mgr_test_helper->AddOrderInfo(buy_order);
  position_mgr_test_helper->AddOrderInfo(sell_order);
  CPPUNIT_ASSERT(position_mgr_.GetGlobalPosition(buy_order.security_id_) == (buy_order.size_ - sell_order.size_));
}

void PositionMgrTests::SingleSecurityShortPositionClients() {
  OrderStruct buy_order;
  buy_order.side_ = HFSAT::kTradeTypeBuy;
  buy_order.security_id_ = 7;
  buy_order.msg_type_ = HFTEST::TRADE;
  buy_order.size_ = 400;
  buy_order.saci_ = 103;
  buy_order.price_ = 11;

  OrderStruct sell_order;
  sell_order.side_ = HFSAT::kTradeTypeSell;
  sell_order.security_id_ = 7;
  sell_order.msg_type_ = HFTEST::TRADE;
  sell_order.size_ = 350;
  sell_order.saci_ = 103;
  sell_order.price_ = 11;
  position_mgr_test_helper->AddOrderInfo(buy_order);
  position_mgr_test_helper->AddOrderInfo(sell_order);
  CPPUNIT_ASSERT(position_mgr_.CheckifAnyLongClient(buy_order.security_id_) == true);
}

void PositionMgrTests::SingleSecurityLongPositionClients() {
  OrderStruct buy_order;
  buy_order.side_ = HFSAT::kTradeTypeBuy;
  buy_order.security_id_ = 8;
  buy_order.msg_type_ = HFTEST::TRADE;
  buy_order.size_ = 250;
  buy_order.saci_ = 103;
  buy_order.price_ = 11;

  OrderStruct sell_order;
  sell_order.side_ = HFSAT::kTradeTypeSell;
  sell_order.security_id_ = 8;
  sell_order.msg_type_ = HFTEST::TRADE;
  sell_order.size_ = 1000;
  sell_order.saci_ = 103;
  sell_order.price_ = 11;
  position_mgr_test_helper->AddOrderInfo(buy_order);
  position_mgr_test_helper->AddOrderInfo(sell_order);
  CPPUNIT_ASSERT(position_mgr_.CheckifAnyShortClient(buy_order.security_id_) == true);
}

void PositionMgrTests::SingleSecurityWorstCaseBidPosition() {
  OrderStruct buy_order;
  buy_order.side_ = HFSAT::kTradeTypeBuy;
  buy_order.security_id_ = 9;
  buy_order.msg_type_ = HFTEST::TRADE;
  buy_order.size_ = 250;
  buy_order.saci_ = 103;
  buy_order.price_ = 11;

  OrderStruct sell_order;
  sell_order.side_ = HFSAT::kTradeTypeSell;
  sell_order.security_id_ = 9;
  sell_order.msg_type_ = HFTEST::TRADE;
  sell_order.size_ = 200;
  sell_order.saci_ = 103;
  sell_order.price_ = 11;

  // Add trades - buy of qty - 250 & sell of qty - 200
  position_mgr_test_helper->AddOrderInfo(buy_order);
  position_mgr_test_helper->AddOrderInfo(sell_order);

  // Add bid orders of Qty -100, 400
  buy_order.msg_type_ = HFTEST::DELTAINC;
  buy_order.size_ = 100;
  position_mgr_test_helper->AddOrderInfo(buy_order);
  buy_order.size_ = 400;
  position_mgr_test_helper->AddOrderInfo(buy_order);

  // Worst Case Bid Position = 250 - 200 + 100 + 400
  int worst_case_bid_position = 550;

  CPPUNIT_ASSERT(position_mgr_.GetGlobalWorstCaseBidPosition(buy_order.security_id_) == worst_case_bid_position);
}

void PositionMgrTests::SingleSecurityWorstCaseAskPosition() {
  OrderStruct buy_order;
  buy_order.side_ = HFSAT::kTradeTypeBuy;
  buy_order.security_id_ = 10;
  buy_order.msg_type_ = HFTEST::TRADE;
  buy_order.size_ = 250;
  buy_order.saci_ = 103;
  buy_order.price_ = 11;

  OrderStruct sell_order;
  sell_order.side_ = HFSAT::kTradeTypeSell;
  sell_order.security_id_ = 10;
  sell_order.msg_type_ = HFTEST::TRADE;
  sell_order.size_ = 400;
  sell_order.saci_ = 103;
  sell_order.price_ = 11;

  // Add trades - buy of qty - 250 & sell of qty - 400
  position_mgr_test_helper->AddOrderInfo(buy_order);
  position_mgr_test_helper->AddOrderInfo(sell_order);

  // Add ask orders of Qty -100, 400
  sell_order.msg_type_ = HFTEST::DELTAINC;
  sell_order.size_ = 500;
  position_mgr_test_helper->AddOrderInfo(sell_order);
  sell_order.size_ = 600;
  position_mgr_test_helper->AddOrderInfo(sell_order);

  // Worst Case Ask Position = -1*(250 - 400) + 500 + 600
  int worst_case_ask_position = 1250;

  CPPUNIT_ASSERT(position_mgr_.GetGlobalWorstCaseAskPosition(buy_order.security_id_) == worst_case_ask_position);
}

void PositionMgrTests::SingleSecurityClientPositionExchangeTrade() {
  OrderStruct buy_order;
  buy_order.side_ = HFSAT::kTradeTypeBuy;
  buy_order.security_id_ = 11;
  buy_order.msg_type_ = HFTEST::TRADE;
  buy_order.size_ = 300;
  buy_order.saci_ = 111;
  buy_order.price_ = 11;

  OrderStruct sell_order;
  sell_order.side_ = HFSAT::kTradeTypeSell;
  sell_order.security_id_ = 11;
  sell_order.msg_type_ = HFTEST::TRADE;
  sell_order.size_ = 200;
  sell_order.saci_ = 111;
  sell_order.price_ = 11;

  position_mgr_test_helper->AddOrderInfo(buy_order);
  position_mgr_test_helper->AddOrderInfo(sell_order);

  // total_client_position = 300 -200
  int total_client_position = 100;
  CPPUNIT_ASSERT(position_mgr_.GetClientPosition(buy_order.saci_) == total_client_position);
}

void PositionMgrTests::SingleSecurityClientPositionInternalTrade() {
  OrderStruct buy_order;
  buy_order.side_ = HFSAT::kTradeTypeBuy;
  buy_order.security_id_ = 12;
  buy_order.msg_type_ = HFTEST::INTERNAl;
  buy_order.size_ = 400;
  buy_order.saci_ = 112;
  buy_order.price_ = 11;

  OrderStruct sell_order;
  sell_order.side_ = HFSAT::kTradeTypeSell;
  sell_order.security_id_ = 12;
  sell_order.msg_type_ = HFTEST::INTERNAl;
  sell_order.size_ = 600;
  sell_order.saci_ = 112;
  sell_order.price_ = 11;

  position_mgr_test_helper->AddOrderInfo(buy_order);
  position_mgr_test_helper->AddOrderInfo(sell_order);

  // total_client_position = 400 -600
  int total_client_position = -200;
  CPPUNIT_ASSERT(position_mgr_.GetClientPosition(buy_order.saci_) == total_client_position);
}

void PositionMgrTests::SingleSecurityClientPositionInternalAndExchangeTrade() {
  OrderStruct buy_order;
  buy_order.side_ = HFSAT::kTradeTypeBuy;
  buy_order.security_id_ = 13;
  buy_order.msg_type_ = HFTEST::INTERNAl;
  buy_order.size_ = 600;
  buy_order.saci_ = 113;
  buy_order.price_ = 11;

  OrderStruct sell_order;
  sell_order.side_ = HFSAT::kTradeTypeSell;
  sell_order.security_id_ = 13;
  sell_order.msg_type_ = HFTEST::TRADE;
  sell_order.size_ = 500;
  sell_order.saci_ = 113;
  sell_order.price_ = 11;

  position_mgr_test_helper->AddOrderInfo(buy_order);
  position_mgr_test_helper->AddOrderInfo(sell_order);

  // total_client_position = 600 - 500
  int total_client_position = 100;
  CPPUNIT_ASSERT(position_mgr_.GetClientPosition(buy_order.saci_) == total_client_position);
}

void PositionMgrTests::CombinedSecurityGlobalBidSize() {
  OrderStruct buy_order;
  buy_order.side_ = HFSAT::kTradeTypeBuy;
  buy_order.security_id_ = 14;
  buy_order.msg_type_ = HFTEST::DELTAINC;
  buy_order.size_ = 50;
  buy_order.saci_ = 101;
  buy_order.price_ = 11;

  double security_weight = 0.9;
  int combined_security_id = 114;

  position_mgr_test_helper->AddOrderInfo(buy_order, combined_security_id, security_weight);

  // 0.9 * 50 (weight * order size)
  int combined_bid_size = 45;
  CPPUNIT_ASSERT(position_mgr_.GetGlobalBidSize(combined_security_id) == combined_bid_size);
}

void PositionMgrTests::CombinedSecurityGlobalAskSize() {
  OrderStruct sell_order;
  sell_order.side_ = HFSAT::kTradeTypeSell;
  sell_order.security_id_ = 15;
  sell_order.msg_type_ = HFTEST::DELTAINC;
  sell_order.size_ = 80;
  sell_order.saci_ = 101;
  sell_order.price_ = 11;

  double security_weight = 0.8;
  int combined_security_id = 115;

  position_mgr_test_helper->AddOrderInfo(sell_order, combined_security_id, security_weight);

  // 0.8 * 80 (weight * order size)
  int combined_bid_size = 64;
  CPPUNIT_ASSERT(position_mgr_.GetGlobalAskSize(combined_security_id) == combined_bid_size);
}

void PositionMgrTests::CombinedSecurityLiveSellOrders() {
  OrderStruct sell_order;
  sell_order.side_ = HFSAT::kTradeTypeSell;
  sell_order.security_id_ = 16;
  sell_order.msg_type_ = HFTEST::DELTAINC;
  sell_order.size_ = 50;
  sell_order.price_ = 15;

  double security_weight = 0.5;
  int combined_security_id = 116;

  position_mgr_test_helper->AddOrderInfo(sell_order, combined_security_id, security_weight);
  CPPUNIT_ASSERT(position_mgr_.CheckifAnyLiveSellOrders(combined_security_id) == true);
}

void PositionMgrTests::CombinedSecurityLiveBuyOrders() {
  OrderStruct buy_order;
  buy_order.side_ = HFSAT::kTradeTypeBuy;
  buy_order.security_id_ = 17;
  buy_order.msg_type_ = HFTEST::DELTAINC;
  buy_order.size_ = 30;
  buy_order.price_ = 16;

  double security_weight = 0.7;
  int combined_security_id = 117;

  position_mgr_test_helper->AddOrderInfo(buy_order, combined_security_id, security_weight);
  CPPUNIT_ASSERT(position_mgr_.CheckifAnyLiveBuyOrders(combined_security_id) == true);
}

void PositionMgrTests::CombinedSecurityGlobalPositivePosition() {
  OrderStruct buy_order;
  buy_order.side_ = HFSAT::kTradeTypeBuy;
  buy_order.security_id_ = 18;
  buy_order.msg_type_ = HFTEST::TRADE;
  buy_order.size_ = 100;
  buy_order.price_ = 11;

  OrderStruct sell_order;
  sell_order.side_ = HFSAT::kTradeTypeSell;
  sell_order.security_id_ = 18;
  sell_order.msg_type_ = HFTEST::TRADE;
  sell_order.size_ = 70;
  sell_order.price_ = 11;

  double security_weight = 0.9;
  int combined_security_id = 118;

  position_mgr_test_helper->AddOrderInfo(buy_order, combined_security_id, security_weight);
  position_mgr_test_helper->AddOrderInfo(sell_order, combined_security_id, security_weight);
  CPPUNIT_ASSERT(position_mgr_.GetGlobalPosition(combined_security_id) ==
                 (buy_order.size_ - sell_order.size_) * security_weight);
}

void PositionMgrTests::CombinedSecurityGlobalNegativePosition() {
  OrderStruct buy_order;
  buy_order.side_ = HFSAT::kTradeTypeBuy;
  buy_order.security_id_ = 19;
  buy_order.msg_type_ = HFTEST::TRADE;
  buy_order.size_ = 20;
  buy_order.saci_ = 103;
  buy_order.price_ = 11;

  OrderStruct sell_order;
  sell_order.side_ = HFSAT::kTradeTypeSell;
  sell_order.security_id_ = 19;
  sell_order.msg_type_ = HFTEST::TRADE;
  sell_order.size_ = 100;
  sell_order.saci_ = 103;
  sell_order.price_ = 11;

  double security_weight = 0.6;
  int combined_security_id = 119;

  position_mgr_test_helper->AddOrderInfo(buy_order, combined_security_id, security_weight);
  position_mgr_test_helper->AddOrderInfo(sell_order, combined_security_id, security_weight);
  CPPUNIT_ASSERT(position_mgr_.GetGlobalPosition(combined_security_id) ==
                 (buy_order.size_ - sell_order.size_) * security_weight);
}

void PositionMgrTests::CombinedSecurityShortPositionClients() {
  OrderStruct buy_order;
  buy_order.side_ = HFSAT::kTradeTypeBuy;
  buy_order.security_id_ = 20;
  buy_order.msg_type_ = HFTEST::TRADE;
  buy_order.size_ = 900;
  buy_order.price_ = 11;

  OrderStruct sell_order;
  sell_order.side_ = HFSAT::kTradeTypeSell;
  sell_order.security_id_ = 20;
  sell_order.msg_type_ = HFTEST::TRADE;
  sell_order.size_ = 800;
  sell_order.price_ = 11;

  double security_weight = 0.7;
  int combined_security_id = 120;

  position_mgr_test_helper->AddOrderInfo(buy_order, combined_security_id, security_weight);
  position_mgr_test_helper->AddOrderInfo(sell_order, combined_security_id, security_weight);
  CPPUNIT_ASSERT(position_mgr_.CheckifAnyLongClient(combined_security_id) == true);
}

void PositionMgrTests::CombinedSecurityLongPositionClients() {
  OrderStruct buy_order;
  buy_order.side_ = HFSAT::kTradeTypeBuy;
  buy_order.security_id_ = 21;
  buy_order.msg_type_ = HFTEST::TRADE;
  buy_order.size_ = 600;
  buy_order.price_ = 11;

  OrderStruct sell_order;
  sell_order.side_ = HFSAT::kTradeTypeSell;
  sell_order.security_id_ = 21;
  sell_order.msg_type_ = HFTEST::TRADE;
  sell_order.size_ = 1000;
  sell_order.price_ = 11;

  double security_weight = 0.6;
  int combined_security_id = 121;

  position_mgr_test_helper->AddOrderInfo(buy_order, combined_security_id, security_weight);
  position_mgr_test_helper->AddOrderInfo(sell_order, combined_security_id, security_weight);
  CPPUNIT_ASSERT(position_mgr_.CheckifAnyShortClient(combined_security_id) == true);
}

void PositionMgrTests::CombinedSecurityWorstCaseBidPosition() {
  OrderStruct buy_order;
  buy_order.side_ = HFSAT::kTradeTypeBuy;
  buy_order.security_id_ = 22;
  buy_order.msg_type_ = HFTEST::TRADE;
  buy_order.size_ = 400;
  buy_order.price_ = 11;

  OrderStruct sell_order;
  sell_order.side_ = HFSAT::kTradeTypeSell;
  sell_order.security_id_ = 22;
  sell_order.msg_type_ = HFTEST::TRADE;
  sell_order.size_ = 300;
  sell_order.price_ = 11;

  double security_weight = 0.9;
  int combined_security_id = 122;
  // Add trades - buy of qty - 400 & sell of qty - 300
  position_mgr_test_helper->AddOrderInfo(buy_order, combined_security_id, security_weight);
  position_mgr_test_helper->AddOrderInfo(sell_order, combined_security_id, security_weight);

  // Add bid orders of Qty - 800, 1200
  buy_order.msg_type_ = HFTEST::DELTAINC;
  buy_order.size_ = 800;
  position_mgr_test_helper->AddOrderInfo(buy_order, combined_security_id, security_weight);
  buy_order.size_ = 1200;
  position_mgr_test_helper->AddOrderInfo(buy_order, combined_security_id, security_weight);

  // Worst Case Bid Position = (400-300+800+1200)*0.9
  int worst_case_bid_position = 1890;

  CPPUNIT_ASSERT(position_mgr_.GetGlobalWorstCaseBidPosition(combined_security_id) == worst_case_bid_position);
}

void PositionMgrTests::CombinedSecurityWorstCaseAskPosition() {
  OrderStruct buy_order;
  buy_order.side_ = HFSAT::kTradeTypeBuy;
  buy_order.security_id_ = 23;
  buy_order.msg_type_ = HFTEST::TRADE;
  buy_order.size_ = 250;
  buy_order.price_ = 11;

  OrderStruct sell_order;
  sell_order.side_ = HFSAT::kTradeTypeSell;
  sell_order.security_id_ = 23;
  sell_order.msg_type_ = HFTEST::TRADE;
  sell_order.size_ = 350;
  sell_order.price_ = 11;

  double security_weight = 0.8;
  int combined_security_id = 123;

  // Add trades - buy of qty - 250 & sell of qty - 350
  position_mgr_test_helper->AddOrderInfo(buy_order, combined_security_id, security_weight);
  position_mgr_test_helper->AddOrderInfo(sell_order, combined_security_id, security_weight);

  // Add ask orders of Qty -500, 600
  sell_order.msg_type_ = HFTEST::DELTAINC;
  sell_order.size_ = 500;
  position_mgr_test_helper->AddOrderInfo(sell_order);
  sell_order.size_ = 600;
  position_mgr_test_helper->AddOrderInfo(sell_order);

  // Worst Case Ask Position = ( (250-350)*-1 +500+600)*0.8
  int worst_case_ask_position = 960;

  CPPUNIT_ASSERT(position_mgr_.GetGlobalWorstCaseAskPosition(combined_security_id) == worst_case_ask_position);
}

PositionMgrTests::~PositionMgrTests() { delete position_mgr_test_helper; }
}
