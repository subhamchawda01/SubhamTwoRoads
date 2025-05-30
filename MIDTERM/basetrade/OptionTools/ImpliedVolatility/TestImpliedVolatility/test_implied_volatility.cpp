#include <cmath>
#include "dvccode/Tests/TestUtils/run_test.hpp"
#include "basetrade/OptionTools/ImpliedVolatility/lets_be_rational.hpp"

using namespace CppUnit;

/*
 * The Test class in which we write the test cases.
 */
class TestImpliedVolatility : public CppUnit::TestFixture {
  // Initializes the test suite with  the class name.
  CPPUNIT_TEST_SUITE(TestImpliedVolatility);

  // List of the test case functions written.
  // To add a test case, we just add a function name here and write its definition below.
  CPPUNIT_TEST(testVolatilityCorrectness);

  CPPUNIT_TEST_SUITE_END();

 public:
  // These are like constructors and destructors of the Test suite
  // setUp -> Constructor | Initialize all your variables here which are going to be used in the test case you write.
  // tearDown -> Destructor | Just free memory etc.
  void setUp(void);
  void tearDown(void);

 protected:
  // Test Cases
  void testVolatilityCorrectness(void);
};

//-----------------------------------------------------------------------------

/*
 * Test Case 1: testVolatilityCorrectness
 * We call the NormalisedBlack function with our input fixed volatility
 * to get a normalized strike price. We then use this same price to get volatility,
 * which should be same as input volatility.
 */
void TestImpliedVolatility::testVolatilityCorrectness(void) {
  double input_volatility = 0.2;
  double normalized_strike_px = 0.0002;
  double q = 1;

  double corresbonding_beta = HFSAT::NormalisedBlack(normalized_strike_px, input_volatility, q);

  double output_volatility = HFSAT::NormalisedImpliedVolatilityFromTransformedRationalGuessWithLimitedIterations(
      corresbonding_beta, normalized_strike_px, q, 2);
  bool equal = false;

  if (fabs(input_volatility - output_volatility) < 0.000001) {
    equal = true;
  }
  CPPUNIT_ASSERT(equal == true);
}

// Like contructor
void TestImpliedVolatility::setUp(void) {}

// Like Destructor
void TestImpliedVolatility::tearDown(void) {}

//-----------------------------------------------------------------------------

CPPUNIT_TEST_SUITE_REGISTRATION(TestImpliedVolatility);

int main(int argc, char* argv[]) {
  const std::string output_folder = "TestImpliedVolatility/";
  std::string output_file = "test_implied_volatility";

  // Call RunTests, we give the file name where the TestResult file will be saved
  RunTest(output_folder + output_file);
}
