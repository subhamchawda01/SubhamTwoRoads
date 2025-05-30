/**
    \file testbed/cpptest8.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#include <iostream>
#include <string>
#include <sstream>

int main(int argc, char** argv) {
  std::ostringstream t_temp_oss_;
  int _YYYYMMDD_ = 20100922;
  t_temp_oss_ << "/spare/local/tradeinfo/"
              << "RolloverOverride/RO_" << _YYYYMMDD_ << ".txt";

  std::string s(t_temp_oss_.str());
  std::cout << s << std::endl;
  return 0;
}
