#ifndef _CMEENGINE_TESTS_
#define _CMEENGINE_TESTS_

#include "infracore/BasicOrderRoutingServer/defines.hpp"

#include "infracore/Tests/run_test.hpp"

namespace HFTEST {

class CMEEngineTests : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(CMEEngineTests);
  CPPUNIT_TEST(CheckBuySetDynamicSendOrderFieldsUsingOrderStruct);
  CPPUNIT_TEST(CheckBuySetDynamicCancelOrderFieldsUsingOrderStruct);
  CPPUNIT_TEST(CheckSelSetDynamicSendOrderFieldsUsingOrderStruct);
  CPPUNIT_TEST(CheckSelSetDynamicCancelOrderFieldsUsingOrderStruct);
  CPPUNIT_TEST_SUITE_END();

 public:
  CMEEngineTests();
  ~CMEEngineTests();

 protected:
  void CheckBuySetDynamicSendOrderFieldsUsingOrderStruct();
  void CheckBuySetDynamicCancelOrderFieldsUsingOrderStruct();
  void CheckSelSetDynamicSendOrderFieldsUsingOrderStruct();
  void CheckSelSetDynamicCancelOrderFieldsUsingOrderStruct();
};
}
#endif
