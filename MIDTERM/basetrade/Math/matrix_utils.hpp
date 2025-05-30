/**
    \file Math/matrix_utils.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_MATH_MATRIX_UTILS_H
#define BASE_MATH_MATRIX_UTILS_H

#include "basetrade/Math/square_matrix.hpp"

namespace HFSAT {
namespace MatrixUtils {

/// Returns a covariance matrix from given matrix.
inline void GetCovarianceMatrix(const std::vector<std::vector<double> >& t_in_matrix_,
                                SquareMatrix<double>& covar_matrix_) {
  if (t_in_matrix_.size() == covar_matrix_.row_count()) {  // check dimensions
    // compute diagonal ( variance )
    // and upper diagonal elements
    for (auto i = 0u; i < t_in_matrix_.size(); i++) {
      for (unsigned int j = i; j < t_in_matrix_.size(); j++) {  // starting from j = i .. so variance as well.
        for (unsigned int row_count_ = 0; row_count_ < t_in_matrix_[0].size(); row_count_++) {
          covar_matrix_(i, j) += t_in_matrix_[i][row_count_] * t_in_matrix_[j][row_count_];
        }
      }
    }

    // copy to lower diagonal
    for (auto i = 0u; i < t_in_matrix_.size(); i++) {
      for (unsigned int j = 0; j < i; j++) {
        covar_matrix_(i, j) = covar_matrix_(j, i);
      }
    }
  }
}

/// Called in get_correlation_matrix.cpp.
/// first gets sdev from variance on diagonals.
/// for non diagonal elements dividign by stdev (i  * stdev (j ) gets correlation.
inline void CovarToCorrMatrix(SquareMatrix<double>& covar_matrix_) {
  // first sqrt diagonals
  for (auto i = 0u; i < covar_matrix_.row_count(); i++) {
    if (covar_matrix_(i, i) <= 0) {
      covar_matrix_(i, i) = 0;
    } else {
      covar_matrix_(i, i) = sqrt(covar_matrix_(i, i));
    }
  }

  // now non diagonal elements
  for (auto i = 0u; i < covar_matrix_.row_count(); i++) {
    for (unsigned int j = 0; j < covar_matrix_.row_count(); j++) {
      if (i != j) {
        covar_matrix_(i, j) = covar_matrix_(i, j) / (covar_matrix_(i, i) * covar_matrix_(j, j));
      }
    }
  }

  // set all diagonal elements to 1
  for (auto i = 0u; i < covar_matrix_.row_count(); i++) {
    covar_matrix_(i, i) = 1;
  }
}
}
}

#endif  // BASE_MATH_MATRIX_UTILS_H
