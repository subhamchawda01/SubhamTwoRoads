/**
    \file QRDecomoposition.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

/** QR Decomposition.
<P>
   For an m-by-n matrix A with m >= n, the QR decomposition is an m-by-n
   orthogonal matrix Q and an n-by-n upper triangular matrix R so that
   A = Q*R.
<P>
   The QR decompostion always exists, even if the matrix does not have
   full rank, so the constructor will never fail.  The primary use of the
   QR decomposition is in the least squares solution of nonsquare systems
   of simultaneous linear equations.  This will fail if isFullRank()
   returns false.
*/
#pragma once
#include <vector>
#include "dvctrade/linal/Matrix.hpp"

namespace LINAL {

// Column oriented matrix representation in this file
// Column oriented means columns are contiguous and many columns are packed in the vector to give a matrix

// Store result in C of A%*%B
void Mult(const std::vector<std::vector<double> >& A, const std::vector<std::vector<double> >& B,
          std::vector<std::vector<double> >& C) {
  int a_numrow = (int)A[0].size();
  int a_num_col = (int)A.size();
  int b_numrow = (int)B[0].size();
  int b_num_col = (int)B.size();
  int r_numrow = (int)C[0].size();
  int r_num_col = (int)C.size();

  assert(a_numrow == r_numrow);
  assert(b_num_col == r_num_col);
  assert(a_num_col == b_numrow);

  for (int i = 0; i < a_numrow; ++i) {
    for (int j = 0; j < b_num_col; ++j) {
      double s = 0;
      for (int t = 0; t < a_num_col; ++t) {
        s += A[t][i] * B[j][t];
      }
      C[j][i] = s;
    }
  }
}

// column based representation in conformance with other structures we use
void QRDecomposition(std::vector<std::vector<double> > QR, std::vector<std::vector<double> >& Q,
                     std::vector<std::vector<double> >& R) {
  int m = QR[0].size();  // num rows
  int n = QR.size();     // num cols

  std::vector<double> Rdiag;
  Rdiag.resize(n, 0);

  // Main loop.
  for (int k = 0; k < n; k++) {
    // Compute 2-norm of k-th column without under/overflow.
    double nrm = 0;
    for (int i = k; i < m; i++) {
      nrm = LINAL::hypot(nrm, QR[k][i]);
    }

    if (nrm != 0.0) {
      // Form k-th Householder vector.
      if (QR[k][k] < 0) {
        nrm = -nrm;
      }
      for (int i = k; i < m; i++) {
        QR[k][i] /= nrm;
      }
      QR[k][k] += 1.0;

      // Apply transformation to remaining columns.
      for (int j = k + 1; j < n; j++) {
        double s = 0.0;
        for (int i = k; i < m; i++) {
          s += QR[k][i] * QR[j][i];
        }
        s = -s / QR[k][k];
        for (int i = k; i < m; i++) {
          QR[j][i] += s * QR[k][i];
        }
      }
    }
    Rdiag[k] = -nrm;
  }

  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      if (i > j) {
        R[i][j] = QR[i][j];
      } else if (i == j) {
        R[i][j] = Rdiag[i];
      } else {
        R[i][j] = 0.0;
      }
    }
  }

  for (int k = n - 1; k >= 0; k--) {
    for (int i = 0; i < m; i++) {
      Q[k][i] = 0.0;
    }
    Q[k][k] = 1.0;
    for (int j = k; j < n; j++) {
      if (QR[k][k] != 0) {
        double s = 0.0;
        for (int i = k; i < m; i++) {
          s += QR[k][i] * Q[j][i];
        }
        s = -s / QR[k][k];
        for (int i = k; i < m; i++) {
          Q[j][i] += s * QR[k][i];
        }
      }
    }
  }
}
}
