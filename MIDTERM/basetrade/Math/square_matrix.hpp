/**
    \file basetrade/Math/square_matrix.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_MATH_SQUARE_MATRIX_H
#define BASE_MATH_SQUARE_MATRIX_H

#include <stdio.h>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>

#include "dvccode/CDef/math_utils.hpp"

namespace HFSAT {

/// SquareMatrix class, primarily used to store positive semidefinite covariance matrices
template <typename T>
class SquareMatrix {
 private:
  unsigned int row_count_;  ///< the number of rows = number of columns
  T** square_matrix_data_;  ///< the storage on heap of the data

 protected:
 public:
  SquareMatrix(unsigned int t_row_count_ = 0) : row_count_(t_row_count_), square_matrix_data_(NULL) {
    if (t_row_count_ > 0) {
      Init(t_row_count_);
    }
  }

  void Init(unsigned int t_row_count_) {
    CleanDelete();
    row_count_ = std::min(t_row_count_, 16384u);  // 1 <= t_row_count_ <= 2^14

    // allocate memory to hold the pointers for the rows
    square_matrix_data_ = (T**)calloc(row_count_, sizeof(T*));

    // for each row
    for (auto i = 0u; i < row_count_; i++) {
      // allocate memory for the row
      square_matrix_data_[i] = (T*)calloc(row_count_, sizeof(T));

      for (auto j = 0u; j < row_count_; j++) {
        // initialize to 0.
        square_matrix_data_[i][j] = 0;
      }
    }
  }

  SquareMatrix(const SquareMatrix& t_source_square_matrix_)
      : row_count_(t_source_square_matrix_.row_count()), square_matrix_data_(NULL) {
    // Initializes a square matrix of this count.
    Init(t_source_square_matrix_.row_count());

    // copy from the source matrix
    for (auto i = 0u; i < row_count_; i++) {
      for (auto j = 0u; j < row_count_; j++) {
        square_matrix_data_[i][j] = t_source_square_matrix_(i, j);
      }
    }
  }

  ~SquareMatrix() { CleanDelete(); }

  void CleanDelete() {
    if (square_matrix_data_ == NULL) return;

    for (auto i = 0u; i < row_count_; i++) {
      if (square_matrix_data_[i] != NULL) {
        free(square_matrix_data_[i]);
      }
      square_matrix_data_[i] = NULL;
    }
    free(square_matrix_data_);
    square_matrix_data_ = NULL;
  }

  inline unsigned int row_count() const { return row_count_; }

  inline const T& operator()(const unsigned int i, const unsigned int j) const { return square_matrix_data_[i][j]; }

  inline T& operator()(const unsigned int i, const unsigned int j) { return square_matrix_data_[i][j]; }

  inline void SetData(const unsigned int i, const unsigned int j, T val) { square_matrix_data_[i][j] = val; }

  inline T** GetDataPtr() { return square_matrix_data_; }

  inline void AddToSelf(const SquareMatrix& t_square_matrix_) {
    if (row_count_ != t_square_matrix_.row_count()) return;  // fail

    for (auto i = 0u; i < row_count(); i++) {
      for (unsigned int j = 0; j < row_count(); j++) {
        (*this)(i, j) += t_square_matrix_(i, j);
      }
    }
  }

  inline void SetSqrtDiagonal(std::vector<T>& stdev_vec_) {
    for (auto i = 0u; i < row_count_; i++) {
      stdev_vec_[i] = sqrt(std::max(0.0, square_matrix_data_[i][i]));
    }
  }

  std::string ToString() {
    std::ostringstream t_oss_;
    for (auto i = 0u; i < row_count(); i++) {
      for (unsigned int j = 0; j < row_count(); j++) {
        t_oss_ << square_matrix_data_[i][j];
        if ((j + 1) < row_count()) {
          t_oss_ << ' ';
        }
      }
      t_oss_ << std::endl;
    }
    return t_oss_.str();
  }

  std::string ToString4() {
    std::ostringstream t_oss_;
    for (auto i = 0u; i < row_count(); i++) {
      for (unsigned int j = 0; j < row_count(); j++) {
        t_oss_ << std::setiosflags(std::ios::fixed) << std::setprecision(4) << square_matrix_data_[i][j];
        if ((j + 1) < row_count()) {
          t_oss_ << ' ';
        }
      }
      t_oss_ << std::endl;
    }
    return t_oss_.str();
  }

  /// without changing the values of this matrix,
  /// compute the inverse of this matrix in out_matrix_
  bool InvertMatrixSimple(SquareMatrix& out_matrix_) const {
    if (out_matrix_.row_count() == row_count()) {
      for (auto i = 0u; i < row_count_; i++) {
        for (unsigned int j = 0; j < row_count_; j++) {
          out_matrix_(i, j) = (i == j) ? 1 : 0;
        }
      }

      // Setup a copy of the initial matrix, a working copy since we
      // donot want to change the original copy
      T** working_square_matrix_data_ = (T**)calloc(row_count_, sizeof(T*));
      for (auto i = 0u; i < row_count_; i++) {
        working_square_matrix_data_[i] = (T*)calloc(row_count_, sizeof(T));

        // copy from the source matrix data
        for (unsigned int j = 0; j < row_count_; j++) {
          working_square_matrix_data_[i][j] = square_matrix_data_[i][j];
        }
      }

      // Adjust the Pivots ( )
      // Here we go through the principal diagonal of the matrix
      // and make sure that none of the pivots are 0.
      // If any pivot is 0, we perform row operations to make it non zero
      for (unsigned int rowindex = 0; rowindex < row_count_; rowindex++) {
        if (working_square_matrix_data_[rowindex][rowindex] != 0.0) continue;

        unsigned int first_nonzero_index = 0u;
        for (first_nonzero_index = 0u; first_nonzero_index < row_count_; first_nonzero_index++) {
          if (working_square_matrix_data_[first_nonzero_index][row_count_] != 0.0) break;
        }
        if (first_nonzero_index == row_count_) return false;

        for (unsigned int col_index = 0; col_index < row_count_; col_index++) {
          // Bad loop for caching ..TODO {}
          working_square_matrix_data_[rowindex][col_index] +=
              working_square_matrix_data_[first_nonzero_index][col_index];
          out_matrix_(rowindex, col_index) += out_matrix_(first_nonzero_index, col_index);
        }
      }
      //@End of Pivotization
      double factor_double = 0.0;
      for (unsigned int row_index_int = 0u; row_index_int < row_count_; row_index_int++) {
        factor_double = (double)working_square_matrix_data_[row_index_int][row_index_int];

        for (int col_index_int = row_count_ - 1; col_index_int >= 0; col_index_int--) {
          out_matrix_(row_index_int, col_index_int) /= factor_double;
        }
        // Doing spearate loop for better cache loading
        for (int col_index_int = row_count_ - 1; col_index_int >= 0; col_index_int--) {
          working_square_matrix_data_[row_index_int][col_index_int] /= factor_double;
        }
        // For every other row make the elements in the column zero
        for (unsigned int row_iter_int = 0; row_iter_int < row_count_; row_iter_int++) {
          if (row_iter_int == row_index_int || working_square_matrix_data_[row_iter_int][row_index_int] == 0.0) {
            // All execpt these rows
            continue;
          }
          factor_double = (double)working_square_matrix_data_[row_iter_int][row_index_int];
          for (unsigned int col_index_int = 0; col_index_int < row_count_; col_index_int++) {
            working_square_matrix_data_[row_iter_int][col_index_int] -=
                ((factor_double) * (working_square_matrix_data_[row_index_int][col_index_int]));
          }
          for (unsigned int col_index_int = 0; col_index_int < row_count_; col_index_int++) {
            out_matrix_(row_iter_int, col_index_int) -= ((factor_double) * (out_matrix_(row_index_int, col_index_int)));
          }
          // Done for this row

        }  // Done for all rows in Mat

      }  // Expecting IXI in the working copy
      // Cleanup
      for (auto i = 0u; i < row_count_; i++) {
        free(working_square_matrix_data_[i]);
        working_square_matrix_data_[i] = NULL;
      }
      free(working_square_matrix_data_);
      working_square_matrix_data_ = NULL;
      return true;
    }
    return false;
  }
};
}

#endif  // BASE_MATH_SQUARE_MATRIX_H
