/**
    \file Tools/get_vol_on_day.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717

*/

#include <iostream>
#include <stdlib.h>

#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/hybrid_sec.hpp"

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"

#include "baseinfra/LoggedSources/cme_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/eurex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/liffe_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/tmx_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ntp_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ose_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/hkex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/hkomd_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/rts_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/micex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/cfe_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ice_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/common_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/eobi_price_feed_logged_message_filenamer.hpp"
#include "baseinfra/MarketAdapter/book_init_utils.hpp"
#include "basetrade/BTUtils/common_mds_info_util.hpp"

void ParseCommandLineParams(const int argc, const char** argv, std::string& shortcode_, int& input_date_,
                            int& begin_secs_from_midnight_, int& end_secs_from_midnight_) {
  // expect :
  // 1. $0 shortcode date_YYYYMMDD [ start_tm HHMM ] [ end_tm HHMM ]
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " shortcode input_date_YYYYMMDD [ start_tm_utc_hhmm ] [ end_tm_utc_hhmm ]"
              << std::endl;
    exit(0);
  } else {
    shortcode_ = argv[1];
    input_date_ = atoi(argv[2]);

    if (argc > 3) {
      begin_secs_from_midnight_ = (atoi(argv[3]) / 100) * 60 * 60 + (atoi(argv[3]) % 100) * 60;
    }
    if (argc > 4) {
      end_secs_from_midnight_ = (atoi(argv[4]) / 100) * 60 * 60 + (atoi(argv[4]) % 100) * 60;
    }
  }
}

/// input arguments : input_date
int main(int argc, char** argv) {
  std::string shortcode_ = "";
  int input_date_ = 20110101;
  int begin_secs_from_midnight_ = 0;
  int end_secs_from_midnight_ = 24 * 60 * 60;
  ParseCommandLineParams(argc, (const char**)argv, shortcode_, input_date_, begin_secs_from_midnight_,
                         end_secs_from_midnight_);

  CommonMdsInfoUtil* common_mds_info_util =
      new CommonMdsInfoUtil(shortcode_, input_date_, begin_secs_from_midnight_, end_secs_from_midnight_);
  common_mds_info_util->Compute();

  const char* t_exchange_symbol_ = common_mds_info_util->GetExchangeSymbol();
  int num_trades_ = common_mds_info_util->GetNumTrades();

  char print_secname_[24] = {0};
  strcpy(print_secname_, t_exchange_symbol_);
  for (size_t i = 0; i < strlen(print_secname_); ++i) {
    if (print_secname_[i] == ' ') {  // Liffe symbols have space, which is bad for the post processing script
      print_secname_[i] = '~';
    }
  }

  std::cout << shortcode_ << ' ' << print_secname_ << ' ' << num_trades_ << std::endl;
}
