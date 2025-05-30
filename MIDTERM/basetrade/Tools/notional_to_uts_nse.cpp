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
    std::cerr << "Usage: " << argv[0] << " SHC YYYYMMDD NOTIONAL_SIZE [return_ceiled_notional_size=0]" << std::endl;
    exit(0);
  }

  std::string dep_short_code = argv[1];
  int date_ = atoi(argv[2]);
  int notional_size_ = atoi(argv[3]);

  int return_ceiled_notional_size_ = 0;
  if (argc > 4) {
    return_ceiled_notional_size_ = atoi(argv[4]);
  }

  HFSAT::SecurityDefinitions::GetUniqueInstance(date_).LoadNSESecurityDefinitions();

  int t_min_lotsize_ = HFSAT::SecurityDefinitions::GetContractMinOrderSize(dep_short_code, date_);

  double t_last_close_ = HFSAT::NSESecurityDefinitions::GetUniqueInstance(date_).GetLastClose(dep_short_code);

  int unit_trade_size_ = (int)std::ceil(notional_size_ / (t_last_close_ * t_min_lotsize_ *
                                                          HFSAT::NSESecurityDefinitions::GetUniqueInstance(date_)
                                                              .GetContractMultiplier(dep_short_code))) *
                         t_min_lotsize_;

  if (return_ceiled_notional_size_ == 1) {
    int ceiled_notional_size_ =
        unit_trade_size_ * t_last_close_ *
        HFSAT::NSESecurityDefinitions::GetUniqueInstance(date_).GetContractMultiplier(dep_short_code);
    std::cout << ceiled_notional_size_ << std::endl;
  } else {
    std::cout << unit_trade_size_ << std::endl;
  }
}
