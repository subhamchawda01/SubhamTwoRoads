#include "infracore/Tests/OrderManager/order_manager_tests.hpp"
#include "dvccode/CDef/order.hpp"

namespace HFTEST {

OrderManagerTests::OrderManagerTests() : order_manager_(HFSAT::ORS::OrderManager::GetUniqueInstance()) {}

void OrderManagerTests::CheckGetNewOrder() {
  HFSAT::ORS::Order* order = order_manager_.GetNewOrder();
  CPPUNIT_ASSERT(order != nullptr);
}

void OrderManagerTests::AddOrderToActiveMap() {
  HFSAT::ORS::Order order;
  order.server_assigned_order_sequence_ = 10;
  order_manager_.AddToActiveMap(&order);
  HFSAT::ORS::Order* order_ptr = order_manager_.GetOrderByOrderSequence(order.server_assigned_order_sequence_);
  order_manager_.RemoveOrderFromMaps(&order);
  CPPUNIT_ASSERT(order_ptr == &order);
}

void OrderManagerTests::RemoveOrderFromMap() {
  HFSAT::ORS::Order order;
  order.server_assigned_order_sequence_ = 11;
  order_manager_.AddToActiveMap(&order);
  bool order_deleted = order_manager_.RemoveOrderFromMaps(&order);
  CPPUNIT_ASSERT(order_deleted == true);
}

void OrderManagerTests::DeActivateOrder() {
  HFSAT::ORS::Order order;
  int server_assigned_order_sequence_ = 12;
  order.server_assigned_order_sequence_ = server_assigned_order_sequence_;
  order_manager_.AddToActiveMap(&order);
  order_manager_.DeactivateOrder(&order);
  HFSAT::ORS::Order* order_ptr = order_manager_.GetInactiveOrderByOrderSequence(server_assigned_order_sequence_);
  CPPUNIT_ASSERT(order_ptr->server_assigned_order_sequence_ == server_assigned_order_sequence_);
}

void OrderManagerTests::GetOrderByExchOrderId() {
  HFSAT::ORS::Order order;
  order.server_assigned_order_sequence_ = 13;
  strcpy(order.exch_assigned_order_sequence_, "13");
  order_manager_.AddToActiveMap(&order);
  HFSAT::ORS::Order* order_ptr = order_manager_.GetOrderByExchOrderId(order.exch_assigned_order_sequence_);
  CPPUNIT_ASSERT(order_ptr == &order);
}

void OrderManagerTests::GetAllPendingOrders() {
  HFSAT::ORS::Order order;
  order.server_assigned_order_sequence_ = 14;
  strcpy(order.exch_assigned_order_sequence_, "14");
  order_manager_.AddToActiveMap(&order);
  HFSAT::ORS::Order* order_ptr = order_manager_.GetOrderByExchOrderId(order.exch_assigned_order_sequence_);
  CPPUNIT_ASSERT(order_ptr == &order);
}

void OrderManagerTests::AddToPartialFillCrossedOrderMap() {
  int saos = 15;
  int order_size_executed = 10;
  order_manager_.AddToPartialFillCrossedOrderMap(saos, order_size_executed);
  CPPUNIT_ASSERT(order_manager_.IsPartialFillCrossedOrdersSaos(saos) == true);
}

void OrderManagerTests::GetExchangeFillForOrder() {
  int saos = 16;
  int order_size_executed = 20;
  order_manager_.AddToPartialFillCrossedOrderMap(saos, order_size_executed);
  CPPUNIT_ASSERT(order_manager_.GetExchangeFillForThisSaos(saos) == order_size_executed);
}

void OrderManagerTests::GetBestActiveBidOrder() {
  HFSAT::ORS::Order* order_1 = new HFSAT::ORS::Order;
  order_1->server_assigned_order_sequence_ = 17;
  order_1->buysell_ = HFSAT::kTradeTypeBuy;
  order_1->int_price_ = 10;
  order_1->size_remaining_ = 20;
  order_1->security_id_ = 141;
  order_manager_.AddToActiveMap(order_1);
  HFSAT::ORS::Order* order_2 = new HFSAT::ORS::Order;
  order_2->server_assigned_order_sequence_ = 18;
  order_2->buysell_ = HFSAT::kTradeTypeBuy;
  order_2->int_price_ = 15;
  order_2->size_remaining_ = 50;
  order_2->security_id_ = 141;
  order_manager_.AddToActiveMap(order_2);

  int best_bid_price, best_bid_size;
  order_manager_.GetBestActiveBidOrders(order_1->security_id_, best_bid_price, best_bid_size);
  delete order_1;
  delete order_2;
  CPPUNIT_ASSERT(best_bid_price == 15 && best_bid_size == 50);
}

void OrderManagerTests::GetBestActiveAskOrder() {
  HFSAT::ORS::Order* order_1 = new HFSAT::ORS::Order;
  order_1->server_assigned_order_sequence_ = 19;
  order_1->buysell_ = HFSAT::kTradeTypeSell;
  order_1->int_price_ = 70;
  order_1->size_remaining_ = 200;
  order_1->security_id_ = 142;
  order_manager_.AddToActiveMap(order_1);
  HFSAT::ORS::Order* order_2 = new HFSAT::ORS::Order;
  order_2->server_assigned_order_sequence_ = 20;
  order_2->buysell_ = HFSAT::kTradeTypeSell;
  order_2->int_price_ = 68;
  order_2->size_remaining_ = 500;
  order_2->security_id_ = 142;
  order_manager_.AddToActiveMap(order_2);

  int best_ask_price, best_ask_size;
  order_manager_.GetBestActiveAskOrders(order_1->security_id_, best_ask_price, best_ask_size);
  delete order_1;
  delete order_2;
  CPPUNIT_ASSERT(best_ask_price == 68 && best_ask_size == 500);
}

void OrderManagerTests::AddCrossedCxlOrder() {
  int saos = 21;
  order_manager_.AddToCrossedCxlOrderMap(saos);
  CPPUNIT_ASSERT(order_manager_.IsCrossedCxlOrder(saos) == true);
}

void OrderManagerTests::DisableNewOrderCreation() {
  order_manager_.DisableNewOrders();
  HFSAT::ORS::Order* order = order_manager_.GetNewOrder();
  CPPUNIT_ASSERT(order == nullptr);
}

OrderManagerTests::~OrderManagerTests() {}
}
