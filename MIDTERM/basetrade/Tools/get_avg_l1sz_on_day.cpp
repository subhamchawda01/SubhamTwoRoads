/**
   \file CommonDataStructures/simple_security_symbol_indexer.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
*/
#include "basetrade/Tools/get_avg_l1sz_on_day.hpp"

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " shortcode input_date_YYYYMMDD [ start_tm_utc_hhmm ] [ end_tm_utc_hhmm ]"
              << std::endl;
    exit(0);
  }

  std::string _this_shortcode_ = argv[1];
  int tradingdate_ = atoi(argv[2]);
  int start_unix_time_ = 0;
  int end_unix_time_ = 24 * 60 * 60;

  if (argc > 3) {
    start_unix_time_ = (atoi(argv[3]) / 100) * 60 * 60 + (atoi(argv[3]) % 100) * 60;
  }
  if (argc > 4) {
    end_unix_time_ = (atoi(argv[4]) / 100) * 60 * 60 + (atoi(argv[4]) % 100) * 60;
  }

  int avg_l1_sz_ = getAvgL1Sz(_this_shortcode_, tradingdate_, start_unix_time_, end_unix_time_);

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);
  const char* t_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(_this_shortcode_);

  char print_secname_[24] = {0};
  strcpy(print_secname_, t_exchange_symbol_);
  for (size_t i = 0; i < strlen(print_secname_); ++i) {
    if (print_secname_[i] == ' ') {  // Liffe symbols have space, which is bad for the post processing script
      print_secname_[i] = '~';
    }
  }
  std::cout << _this_shortcode_ << ' ' << print_secname_ << ' ' << avg_l1_sz_ << std::endl;
  return 0;
}
