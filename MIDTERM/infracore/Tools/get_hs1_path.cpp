// =====================================================================================
//
//       Filename:  get_hs1_path.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  Monday 07 November 2016 08:06:07  UTC
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#include <iostream>
#include <string>
#include <sstream>
#define NUM_DISKS 16
using namespace std;

inline static unsigned long get_disk_num(const char *str) {
  unsigned long hash = 5381;
  int c;

  while ((c = *str++) != 0) hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

  return (hash % NUM_DISKS);
}

inline static std::string get_physical_path(const char *str) {
  cout << "!!" << str << "!!" << endl;
  unsigned long disk_num = get_disk_num(str);  // get disk number where this file would be present
  std::stringstream ss;
  ss << "/media/shared/ephemeral" << disk_num << "/s3_cache" << str;
  return ss.str();
}

int main(int argc, char **argv) {
  string path(argv[1]);
  cout << path << " " << get_physical_path(path.c_str()) << endl;
  return 0;
}
