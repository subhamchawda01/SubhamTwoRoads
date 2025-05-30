/**
 \file Tests/dvctrade/MarketAdapter/ntp_mvm_tests.hpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite No 353, Evoma, #14, Bhattarhalli,
 Old Madras Road, Near Garden City College,
 KR Puram, Bangalore 560049, India
 +91 80 4190 3551
 */

#pragma once

#include <fstream>
#include <set>
#include <map>

#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/Tests/TestUtils/run_test.hpp"

namespace HFTEST {

using namespace HFSAT;

/*
 * The Test class in which we write the test cases.
 */
class TranslationTests : public CppUnit::TestFixture {
  // Initializes the test suite with  the class name.
  CPPUNIT_TEST_SUITE(TranslationTests);

  // List of the test case functions written.
  // To add a test case, we just add a function name here and write its definition below.
  CPPUNIT_TEST(TestTranslation);
  CPPUNIT_TEST(TestTranslationUnique);

  CPPUNIT_TEST_SUITE_END();

 public:
  void tearDown(void) {
    HFSAT::ExchangeSymbolManager::RemoveUniqueInstance();
    HFSAT::CurrencyConvertor::RemoveInstance();
  }
  void setUp(void) {}

 protected:
  // Test Cases

  //-----------------------------------------------------------------------------

  /*
   * Test Case 1: To check if the shortcodes as mapped properly
   * Test Case 2: To check that no two shortcodes are mapped to same exchange symbol
   *
   */
  void TestTranslation(void) {
    std::ifstream input;
    input.open("input");

    std::string shortcode;
    int input_date;
    std::string expected_out;
    std::string actual_out;

    while (input >> shortcode >> input_date >> expected_out) {
      // date can be checked with previous loop for creation of new exchangesymbolmanager

      // ExchangeSymbolManager is deleted before assertion to not effect other test cases
      // in case of failure of this test case
      HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date);
      actual_out = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode);
      HFSAT::ExchangeSymbolManager::RemoveUniqueInstance();
      HFSAT::CurrencyConvertor::RemoveInstance();

      if (actual_out != expected_out) {
        std::cout << "ERROR: Shortcode " << shortcode << "does not translate to " << expected_out << " on "
                  << input_date << std::endl;
        std::cout << "       It translates to " << actual_out << std::endl;
      }

      CPPUNIT_ASSERT(actual_out == expected_out);
    }
  }

  void TestTranslationUnique(void) {
    return;
    std::map<std::string, std::string> exchange_codes;

    std::string shortcode;
    int input_date = 20150623;
    std::string expected_out;

    HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date);
    HFSAT::ShortcodeContractSpecificationMap& contract_specification_map =
        HFSAT::SecurityDefinitions::GetUniqueInstance(input_date).contract_specification_map_;

    for (auto it = contract_specification_map.begin(); it != contract_specification_map.end(); it++) {
      expected_out = HFSAT::ExchangeSymbolManager::GetExchSymbol(it->first);

      // Instance of ExchangeSymbolManager is deleted to not effect other test cases
      // in case of failure of this test case
      if (expected_out != "ULp" && expected_out != "ULa" && expected_out != "FTEp" && expected_out != "ORAp" &&
          expected_out != "NSE64960" && expected_out != "NSE75635" && expected_out != "NSE39960" &&
          expected_out != "NGU5" && exchange_codes.find(expected_out) != exchange_codes.end()) {
        std::cout << "\n ERROR: " << it->first << " and " << exchange_codes[expected_out] << " translate to "
                  << expected_out << std::endl;
        HFSAT::ExchangeSymbolManager::RemoveUniqueInstance();
        HFSAT::CurrencyConvertor::RemoveInstance();
      }

      // hardcoding for certain symbols. have to be resolved or removed.
      if (expected_out != "ORAp" && expected_out != "ULa" && expected_out != "ULp" && expected_out != "FTEp" &&
          expected_out != "NSE64960" && expected_out != "NSE75635" && expected_out != "NSE39960" &&
          expected_out != "SIZ5" && expected_out != "GCZ5" && expected_out != "NGU5")
        CPPUNIT_ASSERT(exchange_codes.find(expected_out) == exchange_codes.end());
      exchange_codes[expected_out] = it->first;
    }
  }
};
}
