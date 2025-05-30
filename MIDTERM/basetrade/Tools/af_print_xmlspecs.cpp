// =====================================================================================
//
//       Filename:  parse_alphaflash_msg_specs.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  Wednesday 10 December 2014 02:22:46  GMT
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

#include "dvccode/CDef/af_xmlspecs.hpp"

using namespace AF_MSGSPECS;

int main(int argc, char **argv) {
  std::string arg_xml_(argv[1]);
  std::vector<Category *> catg_list;

  if (arg_xml_.compare("ALL") == 0) {
    GetCatgXMLSpecs(catg_list);
  } else {
    parseXML(arg_xml_, catg_list);
  }

  for (std::vector<Category *>::iterator it = catg_list.begin(); it != catg_list.end(); it++) {
    std::cout << printCategory(*it) << std::endl;
  }
}
