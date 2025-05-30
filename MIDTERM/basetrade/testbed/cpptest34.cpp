#include <stdio.h>
#include <iostream>

#include <boost/date_time/gregorian/gregorian.hpp>

/// to compile g++ -o exec_add_20091201_to_timeval add_20091201_to_timeval.cpp -I$HOME/infrabase_install
/// -I/apps/boost/include -L/apps/boost/lib -L$HOME/infrabase_install/libdebug

int main(int argc, char** argv) {
  // arguments : filename_to_read filename_to_write

  // get timeval corresponding to 20091201 midnight ( EST )
  boost::gregorian::date d1(2009, 12, 1);
  std::tm _this_tm_ = to_tm(d1);

  time_t abc = mktime(&_this_tm_);
  timeval tv_;
  tv_.tv_sec = abc;
  tv_.tv_usec = 0;

  std::cout << tv_.tv_sec << " - " << tv_.tv_usec << std::endl;
  // read every event in the file and add timeval to it.

  // write to filename_to_write

  return 0;
}
