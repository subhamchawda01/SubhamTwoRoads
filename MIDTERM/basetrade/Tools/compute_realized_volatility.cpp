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
#include "baseinfra/Tools/common_smv_source.hpp"

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << "Usage: " << argv[0]
              << " shortcode input_date_YYYYMMDD look_back [ start_tm_utc_hhmm ] [ end_tm_utc_hhmm ]" << std::endl;
    exit(0);
  } else {
    std::string fut_code_ = argv[1];
    int input_date_ = atoi(argv[2]);
    unsigned int look_back_ = atoi(argv[3]);

    int yyyymmdd_ = input_date_;

    // realized vol
    // with fut prices
    unsigned int i = 0;
    std::vector<double> prices_;
    std::map<std::string, std::string> t_shortcode_2_token_map_;
    while (i < look_back_) {
      std::string price_ = HFSAT::NSESecurityDefinitions::GetBhavCopyToken(yyyymmdd_, fut_code_.c_str(), 10, std::string("STKFUT"), t_shortcode_2_token_map_);
      prices_.push_back(atof(price_.c_str()));
      yyyymmdd_ = HFSAT::HolidayManagerUtils::GetPrevBusinessDayForExchange("NSE", yyyymmdd_);
      i++;
    }

    std::vector<double> returns_;
    i = 1;
    while (i < prices_.size()) {
      returns_.push_back(log(prices_[i] / prices_[i - 1]));
      i++;
    }

    double realized_vol_ = 0;
    ;
    for (unsigned int j = 0; j < returns_.size(); j++) {
      realized_vol_ += (returns_[j] * returns_[j]);
    }

    std::cout << 100 * sqrt(252 / returns_.size() * realized_vol_) << "\n";

    // repeat with stock price

    i = 0;
    yyyymmdd_ = input_date_;
    prices_.clear();
    realized_vol_ = 0;
    while (i < look_back_) {
      double price_ = HFSAT::NSESecurityDefinitions::GetLastTradePrice(yyyymmdd_, fut_code_.c_str());
      prices_.push_back(price_);
      yyyymmdd_ = HFSAT::HolidayManagerUtils::GetPrevBusinessDayForExchange("NSE", yyyymmdd_);
      i++;
    }

    returns_.clear();
    i = 1;
    while (i < prices_.size()) {
      returns_.push_back(log(prices_[i] / prices_[i - 1]));
      i++;
    }

    for (unsigned int j = 0; j < returns_.size(); j++) {
      realized_vol_ += (returns_[j] * returns_[j]);
    }

    std::cout << 100 * sqrt(252 / returns_.size() * realized_vol_) << "\n";
  }
}
