/**
    \file LinearModel.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#include <fstream>
#include <iostream>
#include <iomanip>

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
  LINAL::Matrix X(num_row, num_col - 1);
  LINAL::Matrix Y(num_row, 1);

  for (int i = 0; i < num_row; ++i)
    for (int j = 0; j < num_col; ++j) {
      double d;
      f >> d;
      if (j == 0)
        Y(i, 0) = d;
      else
        X(i, j - 1) = d;
    }
  f.close();

  LINAL::Matrix Xt = X.transpose();
  LINAL::Matrix Xt_X_pinv = LINAL::getPINV(Xt.times(X));
  LINAL::Matrix beta_hat = Xt_X_pinv.times(Xt.times(Y));

  LINAL::Matrix Y_hat = X.times(beta_hat);
  int degree_of_freedom = X.getRowDimension() - X.getColumnDimension();  //
  assert(degree_of_freedom > 0);
  double sigma2 = Y.minus(Y_hat).power(2).sum_elements() / degree_of_freedom;
  //  double residual_std_err = sqrt(sigma2);//

  //  LINAL::Matrix V = Xt_X_pinv.times(sigma2);
  std::cout << "     Value   Std_Err   t_value\n";
  for (size_t i = 0; i < Xt_X_pinv.getRowDimension(); i++) {
    double std_err = sqrt(Xt_X_pinv.get(i, i) * sigma2);
    double t_val = beta_hat.get(i, 0) / std_err;
    std::cout << std::setw(10) << std::setprecision(4) << std::setfill(' ') << beta_hat.get(i, 0) << std::setw(10)
              << std::setprecision(4) << std::setfill(' ') << std_err << std::setw(10) << std::setprecision(4)
              << std::setfill(' ') << t_val << "\n";
  }

  return 0;
}
