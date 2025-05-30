/**
   \file Tests/run_tests.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include <cppunit/TestCase.h>
#include <cppunit/TestFixture.h>
#include <cppunit/ui/text/TextTestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/XmlOutputter.h>
#include <netinet/in.h>
#include <iostream>

#include "infracore/Tests/PlaybackMgr/bmf_playback_manager_tests.hpp"
#include "infracore/Tests/MarginCheckerTests/margin_checker_tests.hpp"
#include "infracore/Tests/PositionMgr/position_mgr_tests.hpp"
#include "infracore/Tests/OrderManager/order_manager_tests.hpp"
#include "infracore/Tests/ASXEngine/asxengine_tests.hpp"
#include "infracore/Tests/CMEEngine/cmeengine_tests.hpp"
#include "infracore/Tests/BMFEngine/bmfepengine_tests.hpp"

using namespace HFTEST;

int main(int argc, char* argv[]) {
  std::string output_file = "ALL";

  CPPUNIT_TEST_SUITE_REGISTRATION(BMFPlaybackManagerTests);

  CPPUNIT_TEST_SUITE_REGISTRATION(MarginCheckerTests);

  CPPUNIT_TEST_SUITE_REGISTRATION(PositionMgrTests);

  CPPUNIT_TEST_SUITE_REGISTRATION(OrderManagerTests);

  CPPUNIT_TEST_SUITE_REGISTRATION(ASXEngineTests);

  CPPUNIT_TEST_SUITE_REGISTRATION(CMEEngineTests);

  CPPUNIT_TEST_SUITE_REGISTRATION(BMFEngineTests);
  // Call RunTests, we give the file name where the TestResult file will be saved
  RunTest(output_file);
}
