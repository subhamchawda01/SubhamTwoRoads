/**
    \file compute_pca_coefficients.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#include <fstream>
#include <iostream>
#include <iomanip>

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvctrade/linal/PCA.hpp"

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cout << "usage " << argv[0] << "<symbol> <data_file>"
              << "\n";
    std::cout << "expecting tabular data of numbers separated by space and newline (without any special characters or "
                 "comments) (first 2 column entries are ignored)\n";
    exit(0);
  }

  HFSAT::BulkFileReader f;
  std::string symbol = argv[1];
  std::string data_filename_ = argv[2];
  f.open(data_filename_);
  static int buf_size = 10240;
  char* buf = new char[buf_size];

  std::vector<double*> num_list;
  unsigned int num_col = 0;
  while (f.GetLine(buf, buf_size) > 0) {
    std::vector<const char*> tokens = HFSAT::PerishableStringTokenizer(buf, buf_size).GetTokens();
    double* d = new double[tokens.size()];
    if (num_col == 0) {
      num_col = tokens.size();
    }
    if (num_col != tokens.size()) {
      std::cerr << "incorrect data file format. incorrect number of columns in data...";
      exit(0);
    }
    for (auto i = 0u; i < tokens.size(); ++i) {
      d[i] = atof(tokens[i]);
    }
    num_list.push_back(d);
  }
  unsigned int num_row = num_list.size();
  f.close();

  LINAL::Matrix X(num_row, num_col);
  for (auto i = 0u; i < num_row; ++i)
    for (unsigned int j = 0; j < num_col; ++j) X(i, j) = num_list[i][j];
  PCA pca = PCA(X, true);
  std::vector<double> orig_std = pca.get_original_std();
  std::cout << "PORTFOLIO_STDEV " << symbol;
  for (auto i = 0u; i < orig_std.size(); ++i) {
    std::cout << " " << orig_std[i];
  }
  std::cout << "\n";

  std::vector<PrincipleComponent> pc_vec = pca.getKPrincipalComponents(2);
  for (auto i = 0u; i < pc_vec.size(); ++i) {
    std::cout << "PORTFOLIO_EIGEN " << symbol << " " << (i + 1) << " " << pc_vec[i].eigenValue / X.getColumnDimension();
    for (unsigned int j = 0; j < pc_vec[i].eigenVector.size(); ++j) {
      std::cout << " " << pc_vec[i].eigenVector[j];
    }
    std::cout << "\n";
  }

  return 0;
}
