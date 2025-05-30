/**
    \file Tools/get_avg_l1sz_on_day.cpp

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
#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/Utils/signals.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"

#include "baseinfra/MarketAdapter/book_init_utils.hpp"
#include "basetrade/BTUtils/common_mds_info_util.hpp"
#include "basetrade/Tools/get_avg_l1sz_on_day.hpp"

// To be used in termination handler
std::string global_shortcode_ = "";
int global_input_date_ = 0;
int global_begin_secs_from_midnight_ = 0;
int global_end_secs_from_midnight_ = 0;

void termination_handler(int signum) {
  if (signum == SIGTERM || signum == SIGFPE) {  // On a segfault inform , so this is picked up and fixed.
    char hostname_[128];
    hostname_[127] = '\0';
    gethostname(hostname_, 127);

    std::string email_string_ = "";
    std::string email_address_ = "";
    {
      std::ostringstream t_oss_;

      t_oss_ << "get_avg_l1sz_on_day received " << SimpleSignalString(signum) << " on " << hostname_ << "\n";
      t_oss_ << "shortcode_= " << global_shortcode_ << "\n"
             << "input_date_= " << global_input_date_ << "\n"
             << "begin_secs_from_midnight_= " << global_begin_secs_from_midnight_ << "\n"
             << "end_secs_from_midnight_= " << global_end_secs_from_midnight_ << "\n\n";

      email_string_ = t_oss_.str();
    }

    HFSAT::Email email_;
    email_.setSubject(email_string_);

    {  // Not sure if others want to receive these emails.
      email_address_ = "nseall@tworoads.co.in";
    }

    email_.addRecepient(email_address_);
    email_.addSender(email_address_);
    email_.content_stream << email_string_ << "<br/>";

    email_.sendMail();

    abort();
  }

  exit(0);
}

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
  global_shortcode_ = shortcode_;
  global_input_date_ = input_date_;
  global_begin_secs_from_midnight_ = begin_secs_from_midnight_;
  global_end_secs_from_midnight_ = end_secs_from_midnight_;
}

/// input arguments : input_date
int main(int argc, char** argv) {
  signal(SIGFPE, termination_handler);
  signal(SIGTERM, termination_handler);
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

  HFSAT::ExchSource_t exch_src_ = common_mds_info_util->GetExchangeSrc();

  int avg_l1_sz_;
  // These exchanges don't have level information in their mds data. So using book for l1AvgSize.
  if (exch_src_ == HFSAT::kExchSourceLIFFE || exch_src_ == HFSAT::kExchSourceHKOMDCPF ||
      exch_src_ == HFSAT::kExchSourceEUREX || exch_src_ == HFSAT::kExchSourceNSE) {
    avg_l1_sz_ = getAvgL1Sz(shortcode_, input_date_, begin_secs_from_midnight_, end_secs_from_midnight_);
  } else {
    avg_l1_sz_ = common_mds_info_util->GetL1AvgSize();
  }

  char print_secname_[24] = {0};
  strcpy(print_secname_, t_exchange_symbol_);
  for (size_t i = 0; i < strlen(print_secname_); ++i) {
    if (print_secname_[i] == ' ') {  // Liffe symbols have space, which is bad for the post processing script
      print_secname_[i] = '~';
    }
  }
  std::cout << shortcode_ << ' ' << print_secname_ << ' ' << avg_l1_sz_ << std::endl;
  return 0;
}
