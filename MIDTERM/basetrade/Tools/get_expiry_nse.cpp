// =====================================================================================
//
//       Filename:  get_expiry_date_nse.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  Wednesday 18 November 2015 05:48:57  IST
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

#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iomanip>

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CDef/security_definitions.hpp"

#include "dvccode/CDef/exchange_symbol_manager.hpp"

#include "dvccode/CDef/nse_security_definition.hpp"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " YYYYMMDD" << std::endl;
    exit(0);
  }

  int date_ = atoi(argv[1]);

  HFSAT::SecurityDefinitions::GetUniqueInstance(date_).LoadNSESecurityDefinitions();

  int expiry_date_ = HFSAT::NSESecurityDefinitions::ComputeNextExpiry(date_);

  std::cout << expiry_date_ << std::endl;
}
