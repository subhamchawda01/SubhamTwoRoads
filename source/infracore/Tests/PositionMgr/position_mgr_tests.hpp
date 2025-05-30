#ifndef _POSITION_MGR_TESTS_
#define _POSITION_MGR_TESTS_

#include "dvccode/CDef/defines.hpp"

#include "infracore/BasicOrderRoutingServer/position_manager.hpp"
#include "infracore/Tests/run_test.hpp"
#include "infracore/Tests/PositionMgr/position_mgr_tests_helper.hpp"

namespace HFTEST {

class PositionMgrTests : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(PositionMgrTests);
  CPPUNIT_TEST(SingleSecurityGlobalBidSize);
  CPPUNIT_TEST(SingleSecurityGlobalAskSize);
  CPPUNIT_TEST(SingleSecurityLiveBuyOrders);
  CPPUNIT_TEST(SingleSecurityLiveSellOrders);
  CPPUNIT_TEST(SingleSecurityGlobalPositivePosition);
  CPPUNIT_TEST(SingleSecurityGlobalNegativePosition);
  CPPUNIT_TEST(SingleSecurityLongPositionClients);
  CPPUNIT_TEST(SingleSecurityShortPositionClients);
  CPPUNIT_TEST(SingleSecurityWorstCaseBidPosition);
  CPPUNIT_TEST(SingleSecurityWorstCaseAskPosition);
  CPPUNIT_TEST(SingleSecurityClientPositionExchangeTrade);
  CPPUNIT_TEST(SingleSecurityClientPositionInternalTrade);
  CPPUNIT_TEST(SingleSecurityClientPositionInternalAndExchangeTrade);
  CPPUNIT_TEST(CombinedSecurityGlobalBidSize);
  CPPUNIT_TEST(CombinedSecurityGlobalAskSize);
  CPPUNIT_TEST(CombinedSecurityLiveBuyOrders);
  CPPUNIT_TEST(CombinedSecurityLiveSellOrders);
  CPPUNIT_TEST(CombinedSecurityGlobalPositivePosition);
  CPPUNIT_TEST(CombinedSecurityGlobalNegativePosition);
  CPPUNIT_TEST(CombinedSecurityShortPositionClients);
  CPPUNIT_TEST(CombinedSecurityLongPositionClients);
  CPPUNIT_TEST(CombinedSecurityWorstCaseBidPosition);
  CPPUNIT_TEST(CombinedSecurityWorstCaseAskPosition);
  CPPUNIT_TEST_SUITE_END();

 public:
  PositionMgrTests();
  ~PositionMgrTests();

 protected:
  void SingleSecurityGlobalBidSize();
  void SingleSecurityGlobalAskSize();
  void SingleSecurityLiveBuyOrders();
  void SingleSecurityLiveSellOrders();
  void SingleSecurityGlobalPositivePosition();
  void SingleSecurityGlobalNegativePosition();
  void SingleSecurityLongPositionClients();
  void SingleSecurityShortPositionClients();
  void SingleSecurityWorstCaseBidPosition();
  void SingleSecurityWorstCaseAskPosition();
  void SingleSecurityClientPositionExchangeTrade();
  void SingleSecurityClientPositionInternalTrade();
  void SingleSecurityClientPositionInternalAndExchangeTrade();
  void CombinedSecurityGlobalBidSize();
  void CombinedSecurityGlobalAskSize();
  void CombinedSecurityLiveBuyOrders();
  void CombinedSecurityLiveSellOrders();
  void CombinedSecurityGlobalPositivePosition();
  void CombinedSecurityGlobalNegativePosition();
  void CombinedSecurityShortPositionClients();
  void CombinedSecurityLongPositionClients();
  void CombinedSecurityWorstCaseBidPosition();
  void CombinedSecurityWorstCaseAskPosition();

 private:
  HFTEST::PositionMgrTestsHelper* position_mgr_test_helper;
  HFSAT::ORS::PositionManager& position_mgr_;
};
}
#endif
