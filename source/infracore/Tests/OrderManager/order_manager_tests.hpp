#ifndef _ORDER_MANAGER_TESTS_
#define _ORDER_MANAGER_TESTS_

#include "dvccode/CDef/defines.hpp"

#include "infracore/BasicOrderRoutingServer/order_manager.hpp"
#include "infracore/Tests/run_test.hpp"

namespace HFTEST {

class OrderManagerTests : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(OrderManagerTests);
  CPPUNIT_TEST(CheckGetNewOrder);
  CPPUNIT_TEST(AddOrderToActiveMap);
  CPPUNIT_TEST(RemoveOrderFromMap);
  CPPUNIT_TEST(DeActivateOrder);
  CPPUNIT_TEST(GetOrderByExchOrderId);
  CPPUNIT_TEST(AddToPartialFillCrossedOrderMap);
  CPPUNIT_TEST(GetExchangeFillForOrder);
  CPPUNIT_TEST(GetBestActiveBidOrder);
  CPPUNIT_TEST(GetBestActiveAskOrder);
  CPPUNIT_TEST(AddCrossedCxlOrder);
  CPPUNIT_TEST(DisableNewOrderCreation);
  CPPUNIT_TEST_SUITE_END();

 public:
  OrderManagerTests();
  ~OrderManagerTests();

 protected:
  void CheckGetNewOrder();
  void AddOrderToActiveMap();
  void RemoveOrderFromMap();
  void DeActivateOrder();
  void GetOrderByExchOrderId();
  void GetAllPendingOrders();
  void AddToPartialFillCrossedOrderMap();
  void GetExchangeFillForOrder();
  void GetBestActiveBidOrder();
  void GetBestActiveAskOrder();
  void AddCrossedCxlOrder();
  void DisableNewOrderCreation();

 private:
  HFSAT::ORS::OrderManager& order_manager_;
};
}

#endif
