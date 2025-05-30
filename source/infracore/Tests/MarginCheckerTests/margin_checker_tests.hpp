#ifndef _Margin_Checker_TESTS_
#define _Margin_Checker_TESTS_

#include "dvccode/CDef/debug_logger.hpp"

#include "infracore/BasicOrderRoutingServer/position_manager.hpp"
#include "infracore/BasicOrderRoutingServer/margin_checker.hpp"
#include "infracore/Tests/MarginCheckerTests/margin_checker_tests.hpp"
#include "infracore/Tests/run_test.hpp"
#include "infracore/Tests/MarginCheckerTests/margin_checker_tests_helper.hpp"

namespace HFTEST {

class MarginCheckerTests : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(MarginCheckerTests);

  // List of Test Cases for the Margin Checker
  CPPUNIT_TEST(BuyOrderSizeExceedLimitSingleSecurity);
  CPPUNIT_TEST(BuyOrderSizeBelowLimitSingleSecurity);
  CPPUNIT_TEST(BuyOrderSizeEqualsLimitSingleSecurity);
  CPPUNIT_TEST(SellOrderSizeExceedLimitSingleSecurity);
  CPPUNIT_TEST(SellOrderSizeBelowLimitSingleSecurity);
  CPPUNIT_TEST(SellOrderSizeEqualsLimitSingleSecurity);
  CPPUNIT_TEST(BuyOrderSizeExceedLimitCombinedSecurity);
  CPPUNIT_TEST(BuyOrderSizeBelowLimitCombinedSecurity);
  CPPUNIT_TEST(BuyOrderSizeEqualsLimitCombinedSecurity);
  CPPUNIT_TEST(SellOrderSizeExceedLimitCombinedSecurity);
  CPPUNIT_TEST(SellOrderSizeBelowLimitCombinedSecurity);
  CPPUNIT_TEST(SellOrderSizeEqualsLimitCombinedSecurity);
  CPPUNIT_TEST(BuyWorstCasePositionExceedSingleSecurity);
  CPPUNIT_TEST(BuyWorstCasePositionEqualsSingleSecurity);
  CPPUNIT_TEST(BuyWorstCasePositionBelowSingleSecurity);
  CPPUNIT_TEST(SellWorstCasePositionExceedSingleSecurity);
  CPPUNIT_TEST(SellWorstCasePositionEqualsSingleSecurity);
  CPPUNIT_TEST(SellWorstCasePositionBelowSingleSecurity);
  CPPUNIT_TEST(WorstCaseBuyPositionExceedCombinedSecurity);
  CPPUNIT_TEST(WorstCaseBuyPositionEqualsCombinedSecurity);
  CPPUNIT_TEST(WorstCaseBuyPositionBelowCombinedSecurity);
  CPPUNIT_TEST(WorstCaseSellPositionExceedCombinedSecurity);
  CPPUNIT_TEST(WorstCaseSellPositionEqualsCombinedSecurity);
  CPPUNIT_TEST(WorstCaseSellPositionBelowCombinedSecurity);
  CPPUNIT_TEST(BuyOrderSizeExceedsMaxPositionSingleSecurity);
  CPPUNIT_TEST(BuyOrderSizeExceedsWorstCasePosSingleSecurity);
  CPPUNIT_TEST(BuyOrderSizeExceedsLimitBMFStocks);
  CPPUNIT_TEST(SellOrderSizeExceedsMaxPositionSingleSecurity);
  CPPUNIT_TEST(SellOrderSizeExceedsWorstCasePosSingleSecurity);
  CPPUNIT_TEST(SellOrderSizeExceedsLimitBMFStocks);
  CPPUNIT_TEST(BuyNotionalValueExceedsLimitBMFStocks);
  CPPUNIT_TEST(SellNotionalValueExceedsLimitBMFStocks);
  CPPUNIT_TEST(ReplaceBuyOrderSizeExceedLimitSingleSecurity);
  CPPUNIT_TEST(ReplaceBuyOrderSizeBelowLimitSingleSecurity);
  CPPUNIT_TEST(ReplaceBuyOrderSizeEqualsLimitSingleSecurity);
  CPPUNIT_TEST(ReplaceSellOrderSizeExceedLimitSingleSecurity);
  CPPUNIT_TEST(ReplaceSellOrderSizeBelowLimitSingleSecurity);
  CPPUNIT_TEST(ReplaceSellOrderSizeEqualsLimitSingleSecurity);
  CPPUNIT_TEST(WorstCaseBuyPositionReplaceExceedSingleSecurity);
  CPPUNIT_TEST(WorstCaseBuyPositionReplaceEqualsSingleSecurity);
  CPPUNIT_TEST(WorstCaseBuyPositionReplaceBelowSingleSecurity);
  CPPUNIT_TEST(WorstCaseSellPositionReplaceExceedSingleSecurity);
  CPPUNIT_TEST(WorstCaseSellPositionReplaceEqualsSingleSecurity);
  CPPUNIT_TEST(WorstCaseSellPositionReplaceBelowSingleSecurity);
  CPPUNIT_TEST(BidNotionalValueReject);
  CPPUNIT_TEST(AskNotionalValueReject);
  CPPUNIT_TEST(FailedPriceCheck);
  CPPUNIT_TEST(NegativeWeightBuyCombinedSecurityExceedLimit);
  CPPUNIT_TEST(NegativeWeightBuyCombinedSecurityBelowLimit);
  CPPUNIT_TEST(NegativeWeightSellCombinedSecurityExceedLimit);
  CPPUNIT_TEST(NegativeWeightSellCombinedSecurityBelowLimit);
  CPPUNIT_TEST(BuyAndSellMaxPositionLimitExceed);
  CPPUNIT_TEST(BuyAndSellWorstCasePositionLimitExceed);

  CPPUNIT_TEST_SUITE_END();

 public:
  MarginCheckerTests();

 protected:
  void BuyOrderSizeExceedLimitSingleSecurity();
  void BuyOrderSizeBelowLimitSingleSecurity();
  void BuyOrderSizeEqualsLimitSingleSecurity();
  void SellOrderSizeExceedLimitSingleSecurity();
  void SellOrderSizeBelowLimitSingleSecurity();
  void SellOrderSizeEqualsLimitSingleSecurity();
  void BuyOrderSizeBelowLimitCombinedSecurity();
  void BuyOrderSizeEqualsLimitCombinedSecurity();
  void BuyOrderSizeExceedLimitCombinedSecurity();
  void SellOrderSizeBelowLimitCombinedSecurity();
  void SellOrderSizeEqualsLimitCombinedSecurity();
  void SellOrderSizeExceedLimitCombinedSecurity();
  void BuyWorstCasePositionExceedSingleSecurity();
  void BuyWorstCasePositionEqualsSingleSecurity();
  void BuyWorstCasePositionBelowSingleSecurity();
  void SellWorstCasePositionExceedSingleSecurity();
  void SellWorstCasePositionEqualsSingleSecurity();
  void SellWorstCasePositionBelowSingleSecurity();
  void WorstCaseBuyPositionExceedCombinedSecurity();
  void WorstCaseBuyPositionEqualsCombinedSecurity();
  void WorstCaseBuyPositionBelowCombinedSecurity();
  void WorstCaseSellPositionExceedCombinedSecurity();
  void WorstCaseSellPositionEqualsCombinedSecurity();
  void WorstCaseSellPositionBelowCombinedSecurity();
  void BuyOrderSizeExceedsMaxPositionSingleSecurity();
  void BuyOrderSizeExceedsWorstCasePosSingleSecurity();
  void BuyOrderSizeExceedsLimitBMFStocks();
  void SellOrderSizeExceedsMaxPositionSingleSecurity();
  void SellOrderSizeExceedsWorstCasePosSingleSecurity();
  void SellOrderSizeExceedsLimitBMFStocks();
  void BuyNotionalValueExceedsLimitBMFStocks();
  void SellNotionalValueExceedsLimitBMFStocks();
  void ReplaceBuyOrderSizeExceedLimitSingleSecurity();
  void ReplaceBuyOrderSizeBelowLimitSingleSecurity();
  void ReplaceBuyOrderSizeEqualsLimitSingleSecurity();
  void ReplaceSellOrderSizeExceedLimitSingleSecurity();
  void ReplaceSellOrderSizeBelowLimitSingleSecurity();
  void ReplaceSellOrderSizeEqualsLimitSingleSecurity();
  void WorstCaseBuyPositionReplaceExceedSingleSecurity();
  void WorstCaseBuyPositionReplaceEqualsSingleSecurity();
  void WorstCaseBuyPositionReplaceBelowSingleSecurity();
  void WorstCaseSellPositionReplaceExceedSingleSecurity();
  void WorstCaseSellPositionReplaceEqualsSingleSecurity();
  void WorstCaseSellPositionReplaceBelowSingleSecurity();
  void BidNotionalValueReject();
  void AskNotionalValueReject();
  void FailedPriceCheck();
  void NegativeWeightBuyCombinedSecurityExceedLimit();
  void NegativeWeightBuyCombinedSecurityBelowLimit();
  void NegativeWeightSellCombinedSecurityExceedLimit();
  void NegativeWeightSellCombinedSecurityBelowLimit();
  void BuyAndSellMaxPositionLimitExceed();
  void BuyAndSellWorstCasePositionLimitExceed();

 private:
  std::string exchange_;
  MarginCheckerTestsHelper* margin_checker_tests_helper_;
};
}

#endif
