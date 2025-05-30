#ifndef _BMF_PLAYBACK_MANAGER_TESTS_
#define _BMF_PLAYBACK_MANAGER_TESTS_

#include "infracore/Tests/run_test.hpp"

#include "infracore/BMFEP/bmf_playback_defines.hpp"

namespace HFTEST {

/*
 * The Test class in which we write the test cases.
 */
class BMFPlaybackManagerTests : public CppUnit::TestFixture {
  // Initializes the test suite with  the class name.
  CPPUNIT_TEST_SUITE(BMFPlaybackManagerTests);

  // List of the test case functions written.
  // To add a test case, we just add a function name here and write its definition below.
  CPPUNIT_TEST(Test1);
  CPPUNIT_TEST(Test2);
  CPPUNIT_TEST(Test3);
  CPPUNIT_TEST(Test4);
  CPPUNIT_TEST(Test5);
  CPPUNIT_TEST(Test6);
  CPPUNIT_TEST(Test7);
  CPPUNIT_TEST(Test8);
  CPPUNIT_TEST(Test9);
  CPPUNIT_TEST(Test10);
  CPPUNIT_TEST(Test11);
  CPPUNIT_TEST(Test12);
  CPPUNIT_TEST(Test13);
  CPPUNIT_TEST(Test14);
  CPPUNIT_TEST(Test15);
  CPPUNIT_TEST(Test16);
  CPPUNIT_TEST(Test17);
  CPPUNIT_TEST(Test18);
  CPPUNIT_TEST(Test19);
  CPPUNIT_TEST(Test20);
  CPPUNIT_TEST(Test21);
  CPPUNIT_TEST(Test22);
  CPPUNIT_TEST(Test23);
  CPPUNIT_TEST(Test24);
  CPPUNIT_TEST(Test25);
  CPPUNIT_TEST(Test26);
  CPPUNIT_TEST(Test27);
  CPPUNIT_TEST(Test28);
  CPPUNIT_TEST(Test29);
  CPPUNIT_TEST(Test30);
  CPPUNIT_TEST(Test31);
  CPPUNIT_TEST(Test32);

  CPPUNIT_TEST_SUITE_END();

 protected:
  // Test Cases

  void Test1();
  void Test2();
  void Test3();
  void Test4();
  void Test5();
  void Test6();
  void Test7();
  void Test8();
  void Test9();
  void Test10();
  void Test11();
  void Test12();
  void Test13();
  void Test14();
  void Test15();
  void Test16();
  void Test17();
  void Test18();
  void Test19();
  void Test20();
  void Test21();
  void Test22();
  void Test23();
  void Test24();
  void Test25();
  void Test26();
  void Test27();
  void Test28();
  void Test29();
  void Test30();
  void Test31();
  void Test32();
  bool IsOrderValid(HFSAT::ORS::BMFPlaybackStruct order, int side, int size, unsigned long long id);
};
}

#endif
