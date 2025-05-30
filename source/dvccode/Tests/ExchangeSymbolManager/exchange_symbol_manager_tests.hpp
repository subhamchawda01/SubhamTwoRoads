#pragma once

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/Utils/holiday_manager.hpp"

#include "dvccode/CDef/currency_convertor.hpp"
#include "dvccode/CDef/file_utils.hpp"

#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/Tests/TestUtils/cpptest_utils.hpp"
#include "dvccode/Tests/TestUtils/run_test.hpp"
namespace HFTEST {

/*
 * The Test class in which we write the test cases.
 */
class ExchangeSymbolManagerTests : public CppUnit::TestFixture {
  // Initializes the test suite with  the class name.
  CPPUNIT_TEST_SUITE(ExchangeSymbolManagerTests);

  CPPUNIT_TEST(TestGetExchangeSymbol);

  CPPUNIT_TEST_SUITE_END();

 public:
  // void tearDown(void) { CurrencyConvertor::RemoveInstance(); }
  void setUp(void) {}

 protected:
  // Test Cases

  void TestGetExchangeSymbol();

 private:
};
}  // namespace HFTEST
