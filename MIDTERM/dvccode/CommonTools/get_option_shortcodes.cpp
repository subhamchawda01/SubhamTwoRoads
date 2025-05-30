/**
    \file Tools/get_option_shortcodes.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717

*/

#include <iostream>
#include <stdlib.h>
#include "dvccode/CDef/security_definitions.hpp"

void ParseCommandLineParams(const int argc, const char **argv, std::string &underlying, int &input_date_, int &topn_) {
  // expect :
  // 1. $0 shortcode date_YYYYMMDD
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " Underlying YYYYMMDD" << std::endl;
    exit(0);
  } else {
    underlying = argv[1];
    input_date_ = atoi(argv[2]);
    topn_ = atoi(argv[3]);
  }
}

bool cmp(const std::pair<std::string, std::string> &p1, const std::pair<std::string, std::string> &p2) {
  return (stoi(p1.second) > stoi(p2.second));
}

void split(const std::string &s, char delim, std::vector<std::string> &elems) {
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
}

/// input arguments : input_date
int main(int argc, char **argv) {
  std::string underlying = "";
  int input_date_ = 20110101;
  int topn_ = 5;

  //
  std::map<double, std::string> strike_2_call_shc_;
  std::map<double, int> strike_2_call_contracts_;
  std::map<double, int> strike_2_call_oi_;
  std::map<double, double> strike_2_call_distance_from_last_close_;

  std::map<double, std::string> strike_2_put_shc_;
  std::map<double, int> strike_2_put_contracts_;
  std::map<double, int> strike_2_put_oi_;
  std::map<double, double> strike_2_put_distance_from_last_close_;

  ParseCommandLineParams(argc, (const char **)argv, underlying, input_date_, topn_);
  HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadNSESecurityDefinitions();
  std::map<std::string, double> opt_2_iv_ = HFSAT::NSESecurityDefinitions::GetOptionsIntrinsicValue(underlying);

  for (std::map<std::string, double>::iterator kv = opt_2_iv_.begin(); kv != opt_2_iv_.end(); kv++) {
    double t_strike_ = HFSAT::NSESecurityDefinitions::GetStrikePriceFromShortCode(kv->first);
    if (HFSAT::NSESecurityDefinitions::GetOptionType(kv->first) == 1) {
      strike_2_call_shc_[t_strike_] = kv->first;
      strike_2_call_distance_from_last_close_[t_strike_] = kv->second;
    } else {
      strike_2_put_shc_[t_strike_] = kv->first;
      strike_2_put_distance_from_last_close_[t_strike_] = kv->second;
    }
  }
  std::string t_temp_shc_ = "NSE_" + underlying + "_FUT0";
  int lot_size_ = HFSAT::SecurityDefinitions::GetContractMinOrderSize(t_temp_shc_, input_date_);
  int this_date_ = HFSAT::HolidayManagerUtils::GetNextBusinessDayForExchange("NSE", input_date_);
  double fut_last_close_ = HFSAT::NSESecurityDefinitions::GetLastClose(t_temp_shc_);

  std::map<std::string, std::string> t_shortcode_2_token_map1_;
  std::string return_val1_ = HFSAT::NSESecurityDefinitions::GetBhavCopyToken(input_date_, t_temp_shc_.c_str(), 13,
                                                                             "OPTSTK", t_shortcode_2_token_map1_);
  std::vector<std::pair<std::string, std::string>> v1(t_shortcode_2_token_map1_.begin(),
                                                      t_shortcode_2_token_map1_.end());
  sort(v1.begin(), v1.end(), cmp);

  std::map<std::string, std::string> t_shortcode_2_token_map2_;
  std::string return_val2_ = HFSAT::NSESecurityDefinitions::GetBhavCopyToken(this_date_, t_temp_shc_.c_str(), 11,
                                                                             "OPTSTK", t_shortcode_2_token_map2_);
  std::vector<std::pair<std::string, std::string>> v2(t_shortcode_2_token_map2_.begin(),
                                                      t_shortcode_2_token_map2_.end());
  sort(v2.begin(), v2.end(), cmp);

  int c_counter_ = 0;
  int p_counter_ = 0;

  for (auto kv : v1) {
    if (stoi(kv.second) > 0) {
      std::vector<std::string> tokens1_;
      split(kv.first, '_', tokens1_);
      if (tokens1_[2][0] == 'C') {
        strike_2_call_oi_[stof(tokens1_[1])] = c_counter_;
        c_counter_++;
      } else if (tokens1_[2][0] == 'P') {
        strike_2_put_oi_[stof(tokens1_[1])] = p_counter_;
        p_counter_++;
      }
    }
  }
  c_counter_ = 0;
  p_counter_ = 0;
  for (auto kv : v2) {
    if (stoi(kv.second) > 0) {
      std::vector<std::string> tokens1_;
      split(kv.first, '_', tokens1_);
      if (tokens1_[2][0] == 'C' && c_counter_ < topn_) {
        strike_2_call_contracts_[stof(tokens1_[1])] = (stoi(kv.second) / lot_size_);
        c_counter_++;
      } else if (tokens1_[2][0] == 'P' && p_counter_ < topn_) {
        strike_2_put_contracts_[stof(tokens1_[1])] = (stoi(kv.second) / lot_size_);
        p_counter_++;
      }
    }
  }

  std::vector<std::string> t_call_options_ =
      HFSAT::NSESecurityDefinitions::GetOptionShortcodesForUnderlyingInCurrScheme(underlying, 0, 1, topn_);
  std::vector<std::string> t_put_options_ =
      HFSAT::NSESecurityDefinitions::GetOptionShortcodesForUnderlyingInCurrScheme(underlying, 0, -1, topn_);

  for (auto a : t_call_options_) {
    std::cout << a << " ";
  }
  std::cout << "\n";

  for (auto a : t_put_options_) {
    std::cout << a << " ";
  }
  std::cout << "\n";

  for (std::map<double, int>::iterator it = strike_2_call_contracts_.begin(); it != strike_2_call_contracts_.end();
       it++) {
    std::cout << fut_last_close_ << " " << it->first << " " << it->second << " " << strike_2_call_shc_[it->first] << " "
              << fabs(strike_2_call_distance_from_last_close_[it->first]) << " " << strike_2_call_oi_[it->first]
              << "\n";
  }
  for (std::map<double, int>::iterator it = strike_2_put_contracts_.begin(); it != strike_2_put_contracts_.end();
       it++) {
    std::cout << fut_last_close_ << " " << it->first << " " << it->second << " " << strike_2_put_shc_[it->first] << " "
              << fabs(strike_2_put_distance_from_last_close_[it->first]) << " " << strike_2_put_oi_[it->first] << "\n";
  }
}
