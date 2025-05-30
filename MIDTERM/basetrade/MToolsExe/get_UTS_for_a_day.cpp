/**
   \file MToolsExe/summarize_strategy_results.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
 */

#include <strings.h>
#include <iostream>
#include <fstream>
#include <string>
#include <dirent.h>
#include <signal.h>

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CDef/file_utils.hpp"

#include "dvctrade/InitCommon/paramset.hpp"
#include "dvccode/CDef/email_utils.hpp"

#include "basetrade/MToolsExe/get_uts_from_strat.hpp"

#define INVALID_MAX_LOSS 9999999

using namespace HFSAT;
std::string g_sigv_string_;
void termination_handler(int signum) {
  if (signum == SIGSEGV) {  // On a segfault inform , so this is picked up and fixed.
    char hostname_[128];
    hostname_[127] = '\0';
    gethostname(hostname_, 127);

    std::string email_string_ = "get_UTS_for_a_day received sigsegv on " + std::string(hostname_) + "\n";
    email_string_ += g_sigv_string_ + "\n";
    std::string email_address_ = "";

    HFSAT::Email email_;
    email_.setSubject(email_string_);

    if (!strncmp(getenv("USER"), "dvctrader", strlen("dvctrader"))) {
      email_address_ = "nseall@tworoads.co.in";
    } else if (!strncmp(getenv("USER"), "ankit", strlen("ankit"))) {
      email_address_ = "ankit@circulumvite.com";
    } else if (!strncmp(getenv("USER"), "rkumar", strlen("rkumar"))) {
      email_address_ = "rakesh.kumar@tworoads.co.in";
    } else if (!strncmp(getenv("USER"), "ravi", strlen("ravi"))) {
      email_address_ = "ravi.parikh@tworoads.co.in";
    } else if (!strncmp(getenv("USER"), "kputta", strlen("kputta"))) {
      email_address_ = "kp@circulumvite.com";
    } else if (!strncmp(getenv("USER"), "anshul", strlen("anshul"))) {
      email_address_ = "anshul@tworoads.co.in";
    } else if (!strncmp(getenv("USER"), "sushant", strlen("sushant"))) {
      email_address_ = "sushant.garg@tworoads.co.in";
    } else if (!strncmp(getenv("USER"), "diwakar", strlen("diwakar"))) {
      email_address_ = "diwakar@circulumvite.com";
    } else if (!strncmp(getenv("USER"), "hagarwal", strlen("hagarwal"))) {
      email_address_ = "hrishav.agarwal@tworoads.co.in";
    } else {  // Not sure if others want to receive these emails.
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

/// This script is meant to be called by run_simulations.pl
/// After having generated data for all the trading days on all the files
/// At the end it calls this exec with
/// the shortcode,
/// the list of strategy files,
/// the starting trading date
/// the ending trading date
///
int getMaxLoss(std::string strategy_filename_base_, std::string shortcode_, int tradingdate_);

// Checks if any of the instruments are NSE symbols and if so adds NSE contract specs
void CheckAndAddNSEDefinitions(std::string t_shortcode_, int trading_date_) {
  bool is_nse_present_ = false;
  if (strncmp(t_shortcode_.c_str(), "NSE_", 4) == 0) {
    is_nse_present_ = true;
  }
  if (is_nse_present_) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(trading_date_).LoadNSESecurityDefinitions();
  }
}

int main(int argc, char** argv) {
  // local variables
  signal(SIGINT, termination_handler);
  signal(SIGSEGV, termination_handler);
  for (int i = 0; i < argc; i++) {
    g_sigv_string_ += std::string(argv[i]) + " ";
  }
  int trading_date_ = 0;

  // command line processing
  if (argc <= 3) {
    std::cerr << argv[0] << " shortcode strategy_list_filename_ trading_date type[0/1=0, 0=normal, 1=stir, 2=config] "
                            "notional_UTS[1(default for NSE/BSE)/0]" << std::endl;
    exit(0);
  }
  std::string strat_file_name_ = argv[2];
  trading_date_ = atoi(argv[3]);
  std::string shortcode_ = argv[1];
  int is_stir_ = 0;  // 0 for normal, 1 for stir, 2 for conifg
  if (argc > 4) {
    is_stir_ = (atoi(argv[4]));
  }

  // use_notional_uts_: default is TRUE for NSE and BSE products
  bool use_notional_uts_ = false;
  if (strncmp(shortcode_.c_str(), "NSE_", 4) == 0 || strncmp(shortcode_.c_str(), "BSE_", 4) == 0) {
    use_notional_uts_ = true;
  }

  if (argc > 5) {
    use_notional_uts_ = (atoi(argv[5]) != 0);
  }

  if (use_notional_uts_ &&
      !(strncmp(shortcode_.c_str(), "NSE_", 4) == 0 || strncmp(shortcode_.c_str(), "BSE_", 4) == 0)) {
    std::cerr << "Notional UTS only used for NSE and BSE.. Exiting\n";
  }

  CheckAndAddNSEDefinitions(shortcode_, trading_date_);

  int unit_trade_size_ = -1;
  if (is_stir_ == 0) {
    unit_trade_size_ = getUTS(strat_file_name_, shortcode_, trading_date_, use_notional_uts_);
  } else if (is_stir_ == 1) {
    unit_trade_size_ = getStirUTS(strat_file_name_, shortcode_, trading_date_, use_notional_uts_);
  } else if (is_stir_ == 2) {
    unit_trade_size_ = GetUTSFromParamPath(strat_file_name_, shortcode_, trading_date_, use_notional_uts_);
  }

  std::cout << unit_trade_size_ << std::endl;
  return unit_trade_size_;
}
