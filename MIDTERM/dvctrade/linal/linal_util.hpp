/**
    \file linal_util.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551

*/
#pragma once
#include "dvctrade/linal/Matrix.hpp"
#include "dvctrade/linal/SingularValueDecomposition.hpp"

namespace LINAL {

inline Matrix getPINV(const Matrix& M) {
  double eps = pow(2.0, -52.0);

  /**
   * A computationally simple and accurate way to compute the pseudoinverse is by using the singular value
   * decomposition.If A = U S V' is the singular value decomposition of A, then
   * A' = V S' U*. For a diagonal matrix such as S, we get the pseudoinverse by taking the reciprocal of
   * each non-zero element on the diagonal, leaving the zeros in place, and transposing the resulting matrix.
   */
  LINAL::Matrix U;
  LINAL::Matrix V;
  LINAL::Matrix S;
  LINAL::SingularValueDecomposition(M, U, S, V);
  for (size_t i = 0; i < S.getRowDimension(); ++i)
    if (fabs(S.get(i, i)) > eps) S(i, i) = 1.0 / S(i, i);
  LINAL::Matrix PINV = V.times(S.times(U.transpose()));
  return PINV;
}
}
