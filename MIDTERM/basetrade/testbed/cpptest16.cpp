/**
    \file testbed/cpptest16.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/

/// to compile g++ -o exec_test16 cpptest16.cpp -I$HOME/basetrade_install -I/apps/boost/include -L/apps/boost/lib
/// -L$HOME/basetrade_install/libdebug

#include <map>
#include <iostream>
#include "dvccode/CDef/defines.hpp"
int main() {
  std::map<DataInfo, int> data_info_int_map_;

  DataInfo d1 = DataInfo("hello", 1234);
  d1.bcast_ip_ = "127.0.0.1";

  data_info_int_map_[d1] = 1;

  DataInfo d2 = DataInfo("127.0.0.1", 1234);
  data_info_int_map_[d2] = 3;

  std::cout << data_info_int_map_[DataInfo("127.0.0.1", 1234)] << std::endl;
  return 0;
}
