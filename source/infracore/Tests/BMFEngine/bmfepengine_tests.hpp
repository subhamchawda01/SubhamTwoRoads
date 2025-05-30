#ifndef _BMFENGINE_TESTS_
#define _BMFENGINE_TESTS_

#include "infracore/Tests/run_test.hpp"
#include "infracore/BasicOrderRoutingServer/defines.hpp"

namespace HFTEST {

class BMFEngineTests : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(BMFEngineTests);
  CPPUNIT_TEST(CheckBuySetDynamicSendOrderFieldsUsingOrderStruct);
  CPPUNIT_TEST(CheckBuySetDynamicCancelOrderFieldsUsingOrderStruct);
  CPPUNIT_TEST(CheckBuySetDynamicCancelReplaceOrderFieldsUsingOrderStruct);
  CPPUNIT_TEST_SUITE_END();

 public:
  BMFEngineTests();
  ~BMFEngineTests();

 protected:
  void CheckBuySetDynamicSendOrderFieldsUsingOrderStruct();
  void CheckBuySetDynamicCancelOrderFieldsUsingOrderStruct();
  void CheckBuySetDynamicCancelReplaceOrderFieldsUsingOrderStruct();
};
}
#endif
