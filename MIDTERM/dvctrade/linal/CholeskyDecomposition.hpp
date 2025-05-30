/**
    \file SingularValueDecomposition.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#pragma once

#include "dvctrade/linal/Matrix.hpp"
/** Cholesky Decomposition.
<P>
For a symmetric, positive definite matrix A, the Cholesky decomposition
is an lower triangular matrix L so that A = L*L'.
<P>
If the matrix is not symmetric or positive definite, the constructor
returns a partial decomposition and sets an internal flag that may
be queried by the isSPD() method.
*/
namespace LINAL {

struct CholeskyDecomposition {
  Matrix L;
  bool isspd;  //@serial is symmetric and positive definite flag.
};

namespace Cholesky {
void decompose(const Matrix& Arg, CholeskyDecomposition& cd) {
  // Initialize.
  double** A = Arg.getArray();
  n = Arg.getRowDimension();
  cd.L.create(n, n);
  double** L = cd.L.getArray();
  cd.isspd = (Arg.getColumnDimension() == n);

  // Main loop.
  for (int j = 0; j < n; j++) {
    double* Lrowj = L[j];
    double d = 0.0;
    for (int k = 0; k < j; k++) {
      double* Lrowk = L[k];
      double s = 0.0;
      for (int i = 0; i < k; i++) {
        s += Lrowk[i] * Lrowj[i];
      }
      Lrowj[k] = s = (A[j][k] - s) / L[k][k];
      d = d + s * s;
      cd.isspd = cd.isspd & (A[k][j] == A[j][k]);
    }
    d = A[j][j] - d;
    cd.isspd = cd.isspd & (d > 0.0);
    L[j][j] = Math.sqrt(Math.max(d, 0.0));
    for (int k = j + 1; k < n; k++) {
      L[j][k] = 0.0;
    }
  }
}

/** Solve A*X = B
@param  B   A Matrix with as many rows as A and any number of columns.
@return     X so that L*L'*X = B
 */
Matrix solve(const Matrix& B, const CholeskyDecomposition& cd, Matrix& solution) {
  assert(B.getRowDimension() == cd.L.getRowDimension());
  assert(cd.isspd);

  // Copy right hand side.
  solution.copy(B);
  double** X = solution.getArray();
  int nx = B.getColumnDimension();

  // Solve L*Y = B;
  for (int k = 0; k < n; k++) {
    for (int j = 0; j < nx; j++) {
      for (int i = 0; i < k; i++) {
        X[k][j] -= X[i][j] * L[k][i];
      }
      X[k][j] /= L[k][k];
    }
  }
  // Solve L'*X = Y;
  for (int k = n - 1; k >= 0; k--) {
    for (int j = 0; j < nx; j++) {
      for (int i = k + 1; i < n; i++) {
        X[k][j] -= X[i][j] * L[i][k];
      }
      X[k][j] /= L[k][k];
    }
  }
  return Matrix(X, n, nx);
}
}
}
