/**
  \file Tests/dvctrade/MarketAdapter/mvm_test_utils.hpp

  \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
Address:
Suite No 353, Evoma, #14, Bhattarhalli,
Old Madras Road, Near Garden City College,
KR Puram, Bangalore 560049, India
+91 80 4190 3551
*/

#pragma once

#include <stdlib.h>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"

#include "baseinfra/MarketAdapter/security_market_view.hpp"

#include "dvccode/Tests/TestUtils/run_test.hpp"
#include "dvccode/Tests/TestUtils/cpptest_utils.hpp"

namespace HFTEST {

namespace MvmTestsUtils {

// using namespace HFSAT; // our coding practice is to use full paths.

/**
 *
 * @param exchange
 * @param shortcode_list
 * @param instruction_list
 */
void ReadInstructionFile(std::string exchange, std::vector<std::string>& shortcode_list,
                         std::vector<std::vector<std::string> >& instruction_list);

/**
 *
 * @param assert_line
 * @param smv
 */
void ParseAndRunAssert(std::string current_test_name, std::vector<std::string> assert_line,
                       HFSAT::SecurityMarketView* smv);

void ConstructDummyBook(HFSAT::SecurityMarketView* smv);
void ClearDummyBook(HFSAT::SecurityMarketView* smv);
}
}
