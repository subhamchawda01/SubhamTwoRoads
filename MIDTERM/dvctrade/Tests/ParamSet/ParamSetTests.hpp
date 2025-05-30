/*
 * ParamSetTests.hpp
 *
 *  Created on: 05-Jul-2017
 *      Author: mehul
 */

#ifndef TESTS_PARAMSET_PARAMSETTESTS_HPP_
#define TESTS_PARAMSET_PARAMSETTESTS_HPP_

#pragma once

#include "dvccode/Tests/TestUtils/run_test.hpp"
#include "dvccode/Tests/TestUtils/cpptest_utils.hpp"
#include "baseinfra/Tools/common_smv_source.hpp"
#include "dvctrade/InitCommon/paramset.hpp"

namespace HFTEST {

using namespace HFSAT;

class ParamSetTests : public CppUnit::TestFixture {
  // Initializes the test suite with  the class name.
  CPPUNIT_TEST_SUITE(ParamSetTests);

  // List of the test case functions written.
  // To add a test case, we just add a function name here and write its definition below.
  CPPUNIT_TEST(TestLoadParamSetImportantParams);
  CPPUNIT_TEST(TestLoadParamSetAllParams);
  CPPUNIT_TEST(TestLoadParamSetAssignesDefaults);
  CPPUNIT_TEST(TestReconcileParams);

//  CPPUNIT_TEST(TestHighPosTradeVarsValue);

  CPPUNIT_TEST_SUITE_END();

 public:
  // These are like constructors and destructors of the Test suite
  // setUp -> Constructor | Initialize all your variables here which are going to be used in the test case you write.
  // tearDown -> Destructor | Just free memory etc.
  void setUp(void);
  void tearDown(void);

 protected:
  void TestLoadParamSetImportantParams(void);
  void TestLoadParamSetAllParams(void);
  void TestLoadParamSetAssignesDefaults(void);
  void TestReconcileParams(void);

 private:
  std::vector<std::string> shortcode_vec_;
  int tradingdate_;

  CommonSMVSource* common_smv_source_;
  SecurityMarketView* smv;
  Watch* watch_;
  DebugLogger* dbglogger_;
  ParamSet* param_1;
  ParamSet* param_2;
  ParamSet* param_3;
};
};

#endif /* TESTS_PARAMSET_PARAMSETTESTS_HPP_ */
