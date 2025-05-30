/**
    \file MTools/covariance_mat_inverse.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#pragma once

/**
 * incremental way to compute inverse of a covariance matrix
 * In the constructor we pass the data as a vector of vectors
 * We can also undo the effect of adding the last incremental index
 *
 * The idea is to store the covariance matrix and incrementally update it when required.
 * The inverse is computed by the blockwise inverse matrix inverse algorithm described here
 * http://en.wikipedia.org/wiki/Invertible_matrix#Blockwise_inversion
 */

#include "dvctrade/linal/Matrix.hpp"
#include <vector>

namespace HFSAT {
#define MAX_COV_MAT_SIZE 30
class CovMatInv {
  std::vector<std::vector<double> > data_;
  std::vector<size_t> all_indices;
  bool canUndoLast;

  double Cov(const std::vector<double>& d1, const std::vector<double>& d2) {
    double sum = 0;
    for (size_t i = 0; i < d1.size(); ++i) {
      sum += d1[i] * d2[i];
    }
    //      sum /= ( d1.size() -1 );
    return sum;
  }

 public:
  LINAL::Matrix cov_mat;
  LINAL::Matrix inv_mat;
  LINAL::Matrix prev_inv;

 private:
  /**
   * join matrix A, B,C,D to form larger matrix. C is Column matrix, B is Row matrix . Result is updated in A
   *
   * | A B |
   * | C D |
   */
  void ClubABCD(LINAL::Matrix& A, const std::vector<double>& B, const std::vector<double>& C, double D) {
    LINAL::Matrix A_copy;
    A_copy.copy(A);
    size_t A_sz = A.getRowDimension() + 1;
    A.create(A_sz, A_sz);

    for (size_t i = 0; i < A_sz - 1; ++i) {
      for (size_t j = 0; j < A_sz - 1; ++j) {
        A(i, j) = A_copy(i, j);
      }
      A(i, A_sz - 1) = B[i];
      A(A_sz - 1, i) = C[i];
    }
    A(A_sz - 1, A_sz - 1) = D;
  }

  // Update A to include covar values in vector B
  // Update A_inv using blockwise inverse Algo
  bool UpdateInverse(LINAL::Matrix& A, LINAL::Matrix& A_inv, const std::vector<double>& B, double D) {
    if (D == 0) return false;
    if (A.getRowDimension() == 0) {
      A.create(1, 1);
      A(0, 0) = D;
      A_inv.create(1, 1);
      A_inv(0, 0) = 1 / D;
      return false;
    }
    // Assume C = B
    // A is square matrix
    double CAiB = 0;
    std::vector<double> CAinv;
    CAinv.resize(A.getRowDimension(), 0.0);
    std::vector<double> AinvB;
    AinvB.resize(A.getRowDimension(), 0.0);
    for (size_t i = 0; i < A.getRowDimension(); ++i) {
      for (size_t j = 0; j < A.getColumnDimension(); ++j) {
        double a_i_j = A_inv(i, j);
        AinvB[i] += a_i_j * B[j];
        CAinv[j] += B[i] * a_i_j;
      }
    }
    for (size_t j = 0; j < A.getRowDimension(); ++j) CAiB += CAinv[j] * B[j];
    if (fabs(D - CAiB) < 1.0e-16) return false;

    double D_inv = 1 / (D - CAiB);
    std::vector<double> C_inv = CAinv;
    std::vector<double> B_inv = AinvB;
    for (size_t i = 0; i < A.getRowDimension(); ++i) {
      C_inv[i] *= -D_inv;
      B_inv[i] *= -D_inv;
    }

    // Update A_inv . A_inv = A_inv + AinvB ( D_inv ) CAinv
    for (size_t i = 0; i < A.getRowDimension(); ++i) {
      for (size_t j = 0; j < A.getColumnDimension(); ++j) {
        A_inv(i, j) -= B_inv[i] * CAinv[j];
      }
    }

    ClubABCD(A, B, B, D);
    ClubABCD(A_inv, B_inv, C_inv, D_inv);
    return true;
  }

 public:
  CovMatInv(const std::vector<std::vector<double> >& data)
      : data_(data), all_indices(), canUndoLast(false), cov_mat(), inv_mat(), prev_inv() {}

  // Adding a GetUniqInstance to be used by Regress Execs.
  // this is primarily to avoid passing this to the CallFS* regression functions
  // In future we could clean this kludgy design
  static CovMatInv& GetUniqueInstance(const std::vector<std::vector<double> >& data) {
    static CovMatInv* p_unique_instance_ = NULL;
    if (p_unique_instance_ == NULL) {
      p_unique_instance_ = new CovMatInv(data);
    }
    return *p_unique_instance_;
  }

  bool AddIndex(size_t indx) {
    for (size_t i = 0; i < all_indices.size(); ++i) {
      if (all_indices[i] == indx) return false;
    }
    std::vector<double> covar;
    covar.resize(all_indices.size(), 0);
    all_indices.push_back(indx);
    for (size_t i = 0; i < covar.size(); ++i) covar[i] = Cov(data_[indx], data_[all_indices[i]]);
    double D = Cov(data_[indx], data_[indx]);
    prev_inv = inv_mat;
    UpdateInverse(cov_mat, inv_mat, covar, D);
    return true;
  }

  std::vector<double> NoAddIndex(size_t indx) {
    std::vector<double> ans;
    for (size_t i = 0; i < all_indices.size(); ++i) {
      if (all_indices[i] == indx) {
        return ans;
      }
    }
    std::vector<double> covar;
    covar.resize(all_indices.size(), 0);

    for (size_t i = 0; i < covar.size(); ++i) covar[i] = Cov(data_[indx], data_[all_indices[i]]);
    double D = Cov(data_[indx], data_[indx]);

    LINAL::Matrix A = cov_mat;
    LINAL::Matrix B = inv_mat;
    UpdateInverse(A, B, covar, D);

    return B.diagonal();
  }

  // true if successful
  bool UndoLastAdd() {
    if (canUndoLast == false) return false;
    inv_mat = prev_inv;
    cov_mat = cov_mat.subMatrix(0, 0, cov_mat.getRowDimension() - 1, cov_mat.getColumnDimension() - 1);
    all_indices.pop_back();
    canUndoLast = false;
    return true;
  }
};
}
