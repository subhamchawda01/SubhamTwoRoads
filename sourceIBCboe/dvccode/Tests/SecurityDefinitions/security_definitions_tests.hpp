#pragma once

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/Utils/holiday_manager.hpp"

#include "dvccode/CDef/currency_convertor.hpp"
#include "dvccode/CDef/file_utils.hpp"

#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/Tests/TestUtils/cpptest_utils.hpp"
#include "dvccode/Tests/TestUtils/run_test.hpp"

namespace HFTEST {

/*
 * The Test class in which we write the test cases.
 */
class SecurityDefinitionsTests : public CppUnit::TestFixture {
  // Initializes the test suite with  the class name.
  CPPUNIT_TEST_SUITE(SecurityDefinitionsTests);

  CPPUNIT_TEST(TestGetContractSpecifications);

  CPPUNIT_TEST(TestGetDIReserves);

  CPPUNIT_TEST(TestGetASXBondPrice);

  CPPUNIT_TEST(TestGetExpiryPriceFactor);

  CPPUNIT_TEST(TestNonSelfEnabledEntities);

  CPPUNIT_TEST_SUITE_END();

 public:
  // void tearDown(void) { CurrencyConvertor::RemoveInstance(); }
  void setUp(void) {}

 protected:
  // Test Cases

  // Test file for GetContractSpecifications function
  void TestGetContractSpecifications();

  // Test file for GetDIReserves function
  void TestGetDIReserves();

  // Test file for GetASXBondPrice function
  void TestGetASXBondPrice();

  // Test file for _GetExpiryPriceFactor function
  void TestGetExpiryPriceFactor();

  // Test file for GetNonSelfEnabledSecurities function
  void TestNonSelfEnabledEntities();

 private:
};
}  // namespace HFTEST
