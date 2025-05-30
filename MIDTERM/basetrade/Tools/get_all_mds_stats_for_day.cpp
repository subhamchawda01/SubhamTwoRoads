/**
   \file Tools/get_all_mds_stats_for_day.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717

 */

#include <iostream>
#include <stdlib.h>
#include "basetrade/BTUtils/common_mds_info_util.hpp"

void ParseCommandLineParams(const int argc, const char** argv, std::string& shortcode_, int& input_date_,
                            int& begin_secs_from_midnight_, int& end_secs_from_midnight_) {
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

  double l1_event_count_ = common_mds_info_util->GetL1EventCount();
  double avg_l1_szs_ = common_mds_info_util->GetL1AvgSize();
  double num_trades_ = common_mds_info_util->GetNumTrades();
  double traded_volume_ = common_mds_info_util->GetVolumeTraded();
  double avg_tr_sz_ = common_mds_info_util->GetAvgTradeSize();
  double high_trade_price_ = common_mds_info_util->GetHighTradePrice();
  double low_trade_price_ = common_mds_info_util->GetLowTradePrice();
  double open_trade_price_ = common_mds_info_util->GetOpenTradePrice();
  double close_trade_price_ = common_mds_info_util->GetCloseTradePrice();
  double tr_sw_px_ = common_mds_info_util->GetAvgTradePrice();

  char print_secname_[24] = {0};
  strcpy(print_secname_, t_exchange_symbol_);
  for (size_t i = 0; i < strlen(print_secname_); ++i) {
    if (print_secname_[i] == ' ') {  // Liffe symbols have space, which is bad for the post processing script
      print_secname_[i] = '~';
    }
  }

  std::cout << shortcode_ << ' ' << print_secname_ << "\n"
            << "L1Events " << l1_event_count_ << "\n"
            << "AvgL1Size " << avg_l1_szs_ << "\n"
            << "NumTrds " << num_trades_ << "\n"
            << "Volume " << traded_volume_ << "\n"
            << "AvgTrdSz " << avg_tr_sz_ << "\n"
            << "HTrdPx " << high_trade_price_ << "\n"
            << "LTrdPx " << low_trade_price_ << "\n"
            << "OpenTrdPx " << open_trade_price_ << "\n"
            << "CloseTrdPx " << close_trade_price_ << "\n"
            << "TrdSizeWPrice " << tr_sw_px_ << std::endl;
}
