/**
   \file baseinfra/Tests/VolatileTradingInfo/commish_tests.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/Tests/VolatileTradingInfo/commish_tests.hpp"

namespace HFTEST {

using namespace HFSAT;

void CommishTests::setUp(void) { CurrencyConvertor::SetUniqueInstance(20160310); }
void CommishTests::TestFixedCommish(void) {
  auto commish = HFSAT::BaseCommish::GetCommishPerContract("FGBL_0", 20160310) *
                 HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyUSD, HFSAT::kCurrencyEUR);

  // Assert
  CPPUNIT_ASSERT_DOUBLES_EQUAL(0.22, commish, DOUBLE_ASSERT_PRECISION);
}

/**
 * Tests the commissions for a shortcode for which commiss is dependant on price
 */
void CommishTests::TestMICEXCommish(void) {
  // Tier 1 commision at a price of 10 and quantity 10
  auto commish = HFSAT::BaseCommish::GetCommishPerContract("USD000000TOD", 100, 10, 1) *
                 HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyUSD, HFSAT::kCurrencyRUB);

  // Assert
  CPPUNIT_ASSERT_DOUBLES_EQUAL(11, commish, DOUBLE_ASSERT_PRECISION);
}
void CommishTests::TestNSECommish(void) {
  auto commish = HFSAT::BaseCommish::GetNSECommishPerContract("NSE_NIFTY_FUT0", 10000, 1) *
                 HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyUSD, HFSAT::kCurrencyINR);

  // Assert
  CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0651, commish,
                               DOUBLE_ASSERT_PRECISION);  // Clearing + SEBI + STT + TXN + FUND + STAMP = 0.00010651
}

void CommishTests::tearDown() { CurrencyConvertor::RemoveInstance(); }
}
