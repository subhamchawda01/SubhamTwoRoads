/**
    \file Tools/get_shortcodes_for_exchange.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717

*/

#include "dvccode/CDef/security_definitions.hpp"

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage: get_shortcodes_for_exchange exchange_name date" << std::endl;
    exit(-1);
  }

  HFSAT::ExchSource_t exch_source = HFSAT::StringToExchSource(argv[1]);
  int date = atoi(argv[2]);

  HFSAT::SecurityDefinitions& sec_defs = HFSAT::SecurityDefinitions::GetUniqueInstance(date);
  for (auto& shortcode_pair : sec_defs.contract_specification_map_) {
    if (shortcode_pair.second.exch_source_ == exch_source) {
      std::cout << shortcode_pair.first << std::endl;
    }
  }
  return 0;
}
