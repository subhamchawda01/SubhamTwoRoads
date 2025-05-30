#ifndef _ASXENGINE_TESTS_
#define _ASXENGINE_TESTS_

#include "infracore/BasicOrderRoutingServer/defines.hpp"

#include "infracore/Tests/run_test.hpp"

namespace HFTEST {

class ASXEngineTests : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ASXEngineTests);
  CPPUNIT_TEST(CheckBuySetDynamicSendOrderFieldsUsingOrderStruct);
  CPPUNIT_TEST(CheckBuySetDynamicCancelOrderFieldsUsingOrderStruct);
  CPPUNIT_TEST(CheckBuySetDynamicModifyOrderFieldsUsingOrderStruct);
  CPPUNIT_TEST(CheckSelSetDynamicSendOrderFieldsUsingOrderStruct);
  CPPUNIT_TEST(CheckSelSetDynamicCancelOrderFieldsUsingOrderStruct);
  CPPUNIT_TEST(CheckSelSetDynamicModifyOrderFieldsUsingOrderStruct);
  CPPUNIT_TEST_SUITE_END();

 public:
  ASXEngineTests();
  ~ASXEngineTests();

 protected:
  void CheckBuySetDynamicSendOrderFieldsUsingOrderStruct();
  void CheckBuySetDynamicCancelOrderFieldsUsingOrderStruct();
  void CheckBuySetDynamicModifyOrderFieldsUsingOrderStruct();
  void CheckSelSetDynamicSendOrderFieldsUsingOrderStruct();
  void CheckSelSetDynamicCancelOrderFieldsUsingOrderStruct();
  void CheckSelSetDynamicModifyOrderFieldsUsingOrderStruct();
};
}
#endif
