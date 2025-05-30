/**
    \file pca_test.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#include <fstream>
#include "dvctrade/linal/linal_util.hpp"
#include "dvctrade/linal/PCA.hpp"

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
    for (int j = 0; j < num_col; ++j) f >> M(i, j);
  f.close();

  PCA pca_matrix(M);

  std::vector<PrincipleComponent> pc = pca_matrix.getPrincipalComponents();
  for (auto i = 0u; i < pc.size(); ++i) {
    std::cout << pc[i].eigenValue << "\t::\t";
    for (unsigned int j = 0; j < pc[i].eigenVector.size(); ++j) {
      std::cout << pc[i].eigenVector[j] << "\t";
    }
    std::cout << "\n";
  }

  std::vector<std::vector<double> > ev;
  pca_matrix.getSignificantEigenVectors(3, 99, ev);

  std::cout << "significant ev size " << ev.size() << "\n";
  for (size_t i = 0; i < ev.size(); ++i) {
    std::cout << (i + 1) << "\t::\t";
    for (size_t j = 0; j < ev[i].size(); ++j) {
      std::cout << ev[i][j] << "\t";
    }
    std::cout << "\n";
  }

  std::cout << "Feature Matrix \n";
  LINAL::Matrix fm = pca_matrix.getFeatureMatrix(ev.size());
  fm.printToConsole();
  std::cout << "=======================\n";

  std::vector<double> principal_std = pca_matrix.get_principal_std();
  std::cout << "stdev_principal::\t";
  for (auto i = 0u; i < principal_std.size(); ++i) {
    std::cout << principal_std[i] << "\t";
  }
  std::cout << "\n";

  std::vector<double> std_original = pca_matrix.get_original_std();
  std::cout << "stdev_original::\t";
  for (auto i = 0u; i < std_original.size(); ++i) {
    std::cout << std_original[i] << "\t";
  }
  std::cout << "\n";

  std::vector<double> pct_var = pca_matrix.get_pct_cum_var();
  std::cout << "pct_var::\t";
  for (auto i = 0u; i < pct_var.size(); ++i) {
    std::cout << pct_var[i] << "\t";
  }
  std::cout << "\n";

  return 0;
}
