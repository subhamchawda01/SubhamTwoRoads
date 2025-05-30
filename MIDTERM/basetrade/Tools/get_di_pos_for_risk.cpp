/**
    \file Tools/get_di_pos_for_risk.cpp

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
#include "baseinfra/BaseUtils/curve_utils.hpp"

void ParseCommandLineParams(const int argc, const char **argv, std::string &shortcode_, int &r_tradingdate_,
                            double &risk_) {
  // expect :
  // 1. $0 shortcode tradingdate
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " shortcode tradingdate risk " << std::endl;
    exit(0);
  } else {
    shortcode_ = argv[1];
    r_tradingdate_ = atoi(argv[2]);
    risk_ = atoi(argv[3]);
  }
}

/// input arguments : shortcode
int main(int argc, char **argv) {
  std::string shortcode_ = "";
  int tradingdate_ = 20130101;
  double risk_ = 1.00;
  ParseCommandLineParams(argc, (const char **)argv, shortcode_, tradingdate_, risk_);

  double price_ = HFSAT::CurveUtils::GetLastDayClosingPrice(tradingdate_, shortcode_);

  double term_ = HFSAT::CurveUtils::_get_term_(tradingdate_, shortcode_);

  double p_value_ = HFSAT::CurveUtils::_get_pvalue_(term_, price_);

  double dv01_ = 0.01 * 0.01 * term_ * p_value_ / (252 * (1 + 0.01 * price_));

  int max_pos_ = 5 * ((int)(risk_ / (dv01_ * 5)));

  printf("%d\n", max_pos_);
}
