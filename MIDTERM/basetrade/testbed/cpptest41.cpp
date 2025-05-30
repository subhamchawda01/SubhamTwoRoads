#include <iostream>

#define BASESYSINFODIR "infracore_install/SysInfo/"
#define BOOST_ZONESPEC_FILE BASESYSINFODIR "libs/date_time/data/date_time_zonespec.csv"

int main(int argc, char** argv) {
  std::string new_str_ = BOOST_ZONESPEC_FILE;
  std::string home_string_ = "/home/gchak/";
  std::cout << (home_string_ + new_str_) << std::endl;
}
