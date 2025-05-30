/**
    \file pinv.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#include <fstream>
#include "dvctrade/linal/linal_util.hpp"

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cout << "usage " << argv[0] << " matrix_text_file"
              << "\n";
    std::cout << "expecting format: num_rows num_cols data.... "
              << "(num_rows*num_cols times space and new line separated)\n";
    exit(0);
  }

  std::ifstream f;
  f.open(argv[1]);
  int num_row, num_col;
  f >> num_row >> num_col;
  LINAL::Matrix M(num_row, num_col);

  for (int i = 0; i < num_row; ++i)
    for (int j = 0; j < num_col; ++j) {
      double d;
      f >> d;
      M(i, j) = d;
    }
  f.close();

  LINAL::Matrix inv = LINAL::getPINV(M);
  inv.printToConsole();

  return 0;
}
