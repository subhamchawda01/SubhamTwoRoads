/**
 *
 *
 *        \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 *             Address:
 *                   Suite No 353, Evoma, #14, Bhattarhalli,
 *                   Old Madras Road, Near Garden City College,
 *                   KR Puram, Bangalore 560049, India
 *                   +91 80 4190 355
 **/

#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"
#include "dvctrade/ModelMath/mult_model_creator.hpp"
#include <stdio.h>

void split(const std::string& s, char delim, std::vector<std::string>& elems) {
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " input_date_YYYYMMDD algo_p1 algo_p2 ....." << std::endl;
    exit(0);
  }
  int tradingdate_ = atoi(argv[1]);
  const std::string underlying_ = argv[2];
  int moneyness_ = atoi(argv[3]);
  int call_only_ = atoi(argv[4]);
  int num_of_contracts_ = atoi(argv[5]);

  HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadNSESecurityDefinitions();

  // std::vector<std::string> options_ = HFSAT::MultModelCreator::GetOptionsToTrade(algo_tokens_);
  std::vector<std::string> options_ = HFSAT::NSESecurityDefinitions::GetOptionShortcodesForUnderlyingInCurrScheme(
      underlying_, moneyness_, call_only_, num_of_contracts_);

  for (auto i = 0u; i < options_.size(); i++) {
    std::cout << options_[i] << " ";
  }

  std::cout << "\n";

  return 0;
}
