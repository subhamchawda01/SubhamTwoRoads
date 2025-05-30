/**
    \file Tools/get_dv01_for_shortcode.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/

#include <iostream>
#include <stdlib.h>
#include "baseinfra/BaseUtils/curve_utils.hpp"
#include "dvccode/CDef/security_definitions.hpp"

void ParseCommandLineParams(const int argc, const char **argv, std::string &shortcode_, int &yyyymmdd_,
                            double &_price_) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " shc_code_ date [price_]" << std::endl;
    exit(0);
  } else {
    shortcode_ = argv[1];
    yyyymmdd_ = atoi(argv[2]);
  }

  if (argc > 3) {
    _price_ = atof(argv[3]);
  }
}

int main(int argc, char **argv) {
  std::string shortcode_ = "";
  int yyyymmdd_ = -1;
  double price_ = -1.00;

  ParseCommandLineParams(argc, (const char **)argv, shortcode_, yyyymmdd_, price_);

  double dv01 = 0.0;
  if (yyyymmdd_ > 0) {
    dv01 = HFSAT::CurveUtils::dv01(shortcode_, yyyymmdd_, price_);
  }
  printf("%f\n", dv01);
}
