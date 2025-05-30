/**
    \file testbed/cpptest23.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/

// compile with g++ $file -I/apps/boost/include -L/apps/boost/lib -static

#include <iostream>
#include <boost/date_time/gregorian/gregorian.hpp>

int main(int argc, char** argv) {
  if (argc > 1) {
    int tradingdate_ = atoi(argv[1]);

    boost::gregorian::date d1(tradingdate_ / 10000, ((tradingdate_ / 100) % 100), (tradingdate_ % 100));
    d1 = d1 - boost::gregorian::date_duration(1);

    boost::gregorian::date::ymd_type ymd = d1.year_month_day();
    tradingdate_ = (((ymd.year * 100 + ymd.month) * 100) + ymd.day);

    std::cout << tradingdate_ << std::endl;
  }
}
