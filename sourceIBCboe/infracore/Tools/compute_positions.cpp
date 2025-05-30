// =====================================================================================
//
//       Filename:  compute_positions.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  01/16/2019 08:39:22 AM
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

#include <cstdlib>
#include <iostream>
#include <fstream>
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

int main(int argc, char* argv[]) {
  std::map<std::string, int32_t> prod_to_pos;

  for (int32_t count = 1; count < argc; count++) {
    std::ifstream infile;
    infile.open(argv[count]);

    char buffer[1024];
    memset((void*)buffer, 0, 1024);

    while (infile.good()) {
      infile.getline(buffer, 1024);

      HFSAT::PerishableStringTokenizer st(buffer, 1024);
      const std::vector<const char*>& tokens = st.GetTokens();

      if (tokens.size() != 2) continue;

      if (prod_to_pos.end() == prod_to_pos.find(tokens[0])) {
        prod_to_pos[tokens[0]] = atoi(tokens[1]);
      } else {
        prod_to_pos[tokens[0]] += atoi(tokens[1]);
      }
    }
    infile.close();
  }

  for (auto& itr : prod_to_pos) {
    std::cout << itr.first << " " << itr.second << std::endl;
  }

  return EXIT_SUCCESS;
}  // ----------  end of function main  ----------
