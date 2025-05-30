// =====================================================================================
//
//       Filename:  dvctrade/Tests/Indicators/pca_weights_manager_tests.cpp
//
//    Description:  Tests for Utility Functions for fetching the Sample Data values from the last 30 days
//
//        Version:  1.0
//        Created:  01/14/2016 04:31:28 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#pragma once

#include "dvccode/Tests/TestUtils/run_test.hpp"
#include "dvctrade/Indicators/pca_weights_manager.hpp"

namespace HFTEST {
using namespace HFSAT;

class PcaWeightsManagerTests : public CppUnit::TestFixture {
  // Initializes the test suite with  the class name.
  CPPUNIT_TEST_SUITE(PcaWeightsManagerTests);

  // List of the test case functions written.
  // To add a test case, we just add a function name here and write its definition below.

  CPPUNIT_TEST(TestPCAInfoFileName);
  CPPUNIT_TEST(TestStdevInfoFile);
  CPPUNIT_TEST(TestREGInfoFileName);
  CPPUNIT_TEST(TestStdevInfoLongevityFileName);
  CPPUNIT_TEST(TestConstituentsList);
  CPPUNIT_TEST(TestConstitutentWeigths);

  CPPUNIT_TEST_SUITE_END();

 public:
  // These are like constructors and destructors of the Test suite
  // setUp -> Constructor | Initialize all your variables here which are going to be used in the test case you write.
  // tearDown -> Destructor | Just free memory etc.
  void setUp(void);
  void tearDown(void);

 protected:
  void TestPCAInfoFileName(void);
  void TestStdevInfoFile(void);
  void TestREGInfoFileName(void);
  void TestStdevInfoLongevityFileName(void);
  void TestConstituentsList(void);
  void TestConstitutentWeigths(void);
};
}
