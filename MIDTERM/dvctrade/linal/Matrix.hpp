/**
   \file Matrix.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#pragma once

#include <vector>
#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sstream>

namespace LINAL {
inline double hypot(double a, double b) {
  double r;
  if (fabs(a) > fabs(b)) {
    r = b / a;
    r = fabs(a) * sqrt(1 + r * r);
  } else if (b != 0) {
    r = a / b;
    r = fabs(b) * sqrt(1 + r * r);
  } else {
    r = 0.0;
  }
  return r;
}
class Matrix {
 private:
  size_t m, n;  // rows, columns
  double* A;

  /** Construct an m-by-n matrix of zeros.
      @param m    Number of rows.
      @param n    Number of colums.
  */
 public:
  void create(size_t rows, size_t cols) {
    if (m == rows && n == cols) return;

    if (A) {
      free(A);
      A = NULL;
    }

    m = rows;
    n = cols;
    A = (double*)calloc((m) * (n), sizeof(double));
  }

  void freeSpace() {
    if (A) {
      free(A);
      A = NULL;
    }
    m = 0;
    n = 0;
  }

  Matrix() : m(0), n(0), A(NULL) {}  // default
  Matrix(size_t rows, size_t cols) : m(0), n(0), A(NULL) {
    create(rows, cols);
  }  // unintialized data, only space allocated

  /*~Matrix(){			//buggy destructor
    if (A) {
    free ( A );
    //delete[] A;
    A = NULL;
    }
    m = 0;
    n = 0;
  }*/

  Matrix& copy(const Matrix& src) {
    create(src.getRowDimension(), src.getColumnDimension());
    memcpy((void*)A, (void*)src.A, sizeof(double) * m * n);
    return *this;
  }

  Matrix& fill(double value) {
    for (size_t i = 0; i < m * n; i++) {
      *(A + i) = value;
    }
    return *this;
  }

  // constructor which sets all elements to the argument val
  Matrix(size_t& rows, size_t& cols, double val) : m(0), n(0), A(NULL) {
    create(rows, cols);
    fill(val);
  }
  Matrix(const Matrix& src) : m(0), n(0), A(0) { copy(src); }  // copy constructor

  /** Get row dimension.
      @return     m, the number of rows.
  */

  size_t getRowDimension() const { return m; }

  /** Get column dimension.
      @return     n, the number of columns.
  */

  size_t getColumnDimension() const { return n; }

  /** Get a single element.
      @param i    Row index.
      @param j    Column index.
      @return     A(i,j)
      @exception  ArrayIndexOutOfBoundsException
  */

  double get(size_t i, size_t j) const { return A[i * n + j]; }
  double& operator()(size_t i, size_t j) { return *(A + i * n + j); }
  void set(size_t i, size_t j, double s) { A[i * n + j] = s; }

  /** Matrix transpose.
      @return    A'
  */
  Matrix transpose() {
    Matrix result(n, m);
    for (size_t i = 0; i < m; i++) {
      for (size_t j = 0; j < n; j++) {
        result(j, i) = (*this)(i, j);
      }
    }
    return result;
  }

  /**  Unary minus
       @return    -A
  */

  Matrix uminus() {
    Matrix X = Matrix(m, n);
    for (size_t i = 0; i < m; i++) {
      for (size_t j = 0; j < n; j++) {
        X(i, j) = -(*this)(i, j);
      }
    }
    return X;
  }

  /** C = A + B
      @param B    another matrix
      @return     A + B
  */

  Matrix plus(Matrix B) {
    checkMatrixDimensions(B);
    Matrix X = Matrix(m, n);
    for (size_t i = 0; i < m; i++) {
      for (size_t j = 0; j < n; j++) {
        X(i, j) = (*this)(i, j) + B(i, j);
      }
    }
    return X;
  }

  /** A = A + B
      @param B    another matrix
      @return     A + B
  */

  Matrix& plusEquals(Matrix B) {
    checkMatrixDimensions(B);
    for (size_t i = 0; i < m; i++) {
      for (size_t j = 0; j < n; j++) {
        (*this)(i, j) += B(i, j);
      }
    }
    return *this;
  }

  /** C = A - B
      @param B    another matrix
      @return     A - B
  */

  Matrix minus(Matrix B) {
    checkMatrixDimensions(B);
    Matrix X = Matrix(m, n);
    for (size_t i = 0; i < m; i++) {
      for (size_t j = 0; j < n; j++) {
        X(i, j) = (*this)(i, j) - B(i, j);
      }
    }
    return X;
  }

  /** A = A - B
      @param B    another matrix
      @return     A - B
  */

  Matrix& minusEquals(Matrix B) {
    checkMatrixDimensions(B);
    for (size_t i = 0; i < m; i++) {
      for (size_t j = 0; j < n; j++) {
        (*this)(i, j) -= -B(i, j);
      }
    }
    return *this;
  }

  /** Element-by-element multiplication, C = A.*B
      @param B    another matrix
      @return     A.*B
  */

  Matrix arrayTimes(Matrix B) {
    checkMatrixDimensions(B);
    Matrix X = Matrix(m, n);
    for (size_t i = 0; i < m; i++) {
      for (size_t j = 0; j < n; j++) {
        X(i, j) = (*this)(i, j) * B(i, j);
      }
    }
    return X;
  }

  /** Element-by-element multiplication in place, A = A.*B
      @param B    another matrix
      @return     A.*B
  */

  Matrix& arrayTimesEquals(Matrix B) {
    checkMatrixDimensions(B);
    for (size_t i = 0; i < m; i++) {
      for (size_t j = 0; j < n; j++) {
        (*this)(i, j) = (*this)(i, j) * B(i, j);
      }
    }
    return *this;
  }

  /** Element-by-element right division, C = A./B
      @param B    another matrix
      @return     A./B
  */

  Matrix arrayRightDivide(Matrix B) {
    checkMatrixDimensions(B);
    Matrix X = Matrix(m, n);
    for (size_t i = 0; i < m; i++) {
      for (size_t j = 0; j < n; j++) {
        X(i, j) = (*this)(i, j) / B(i, j);
      }
    }
    return X;
  }

  /** Element-by-element right division in place, A = A./B
      @param B    another matrix
      @return     A./B
  */

  Matrix& arrayRightDivideEquals(Matrix B) {
    checkMatrixDimensions(B);
    for (size_t i = 0; i < m; i++) {
      for (size_t j = 0; j < n; j++) {
        (*this)(i, j) /= B(i, j);
      }
    }
    return *this;
  }

  /** Element-by-element left division, C = A.\B
      @param B    another matrix
      @return     A.\B
  */

  Matrix arrayLeftDivide(Matrix B) {
    checkMatrixDimensions(B);
    Matrix X = Matrix(m, n);
    for (size_t i = 0; i < m; i++) {
      for (size_t j = 0; j < n; j++) {
        X(i, j) = B(i, j) / (*this)(i, j);
      }
    }
    return X;
  }

  /** Element-by-element left division in place, A = A.\B
      @param B    another matrix
      @return     A.\B
  */

  Matrix& arrayLeftDivideEquals(Matrix B) {
    checkMatrixDimensions(B);
    for (size_t i = 0; i < m; i++) {
      for (size_t j = 0; j < n; j++) {
        (*this)(i, j) = B(i, j) / (*this)(i, j);
      }
    }
    return *this;
  }

  /** Multiply a matrix by a scalar, C = s*A
      @param s    scalar
      @return     s*A
  */

  Matrix times(double s) {
    Matrix X = Matrix(m, n);
    for (size_t i = 0; i < m; i++) {
      for (size_t j = 0; j < n; j++) {
        X(i, j) = s * (*this)(i, j);
      }
    }
    return X;
  }

  Matrix power(double p) {
    Matrix X = Matrix(m, n);
    for (size_t i = 0; i < m; i++) {
      for (size_t j = 0; j < n; j++) {
        X(i, j) = pow((*this)(i, j), p);
      }
    }
    return X;
  }

  double sum_elements() {
    double s = 0;
    double* tmp = A;
    size_t numEl = m * n;
    while (numEl--) {
      s += *(tmp++);  // cache friendly
    }
    return s;
  }

  /** Multiply a matrix by a scalar in place, A = s*A
      @param s    scalar
      @return     replace A by s*A
  */

  Matrix& timesEquals(double s) {
    for (size_t i = 0; i < m; i++) {
      for (size_t j = 0; j < n; j++) {
        (*this)(i, j) *= s;
      }
    }
    return *this;
  }

  /** Linear algebraic matrix multiplication, A * B
      @param B    another matrix
      @return     Matrix product, A * B
      @exception  IllegalArgumentException Matrix inner dimensions must agree.
  */

  Matrix times(const Matrix& B) {
    assert(B.m == n);  //
    Matrix result(m, B.n);
    double* Bcolj = new double[n];
    for (size_t j = 0; j < B.n; j++) {
      for (size_t k = 0; k < n; k++) {
        Bcolj[k] = B.get(k, j);
      }
      for (size_t i = 0; i < m; i++) {
        double* Arowi = A + i * n;
        double s = 0;
        for (size_t k = 0; k < n; k++) {
          s += Arowi[k] * Bcolj[k];
        }
        result(i, j) = s;
      }
    }
    return result;
  }

  double trace() {
    double t = 0;
    for (size_t i = 0; i < std::min(m, n); i++) {
      t += (*this)(i, i);
    }
    return t;
  }

  /** Generate identity matrix
      @param m    Number of rows.
      @param n    Number of colums.
      @return     An m-by-n matrix with ones on the diagonal and zeros elsewhere.
  */
  static Matrix identity(size_t m, size_t n) {
    Matrix I = Matrix(m, n);
    for (size_t i = 0; i < std::min(m, n); i++) {
      I(i, i) = 1.0;
    }
    return I;
  }

  void printToConsole() {
    std::stringstream st;
    ToString(st);
    std::cout << st.str();
  }

  void ToString(std::stringstream& matrix_stream) {
    for (size_t i = 0; i < m; ++i) {
      for (size_t j = 0; j < n; ++j) {
        matrix_stream << (*this)(i, j) << " ";
      }
      matrix_stream << "\n";
    }
  }

  /**
   * zero indexed numbering
   * all indices including
   */
  Matrix subMatrix(const size_t startRow, const size_t startCol, const size_t numRows, const size_t numCols) {
    assert(startRow + numRows <= m);
    assert(startCol + numCols <= n);
    Matrix toRet(numRows, numCols);
    double* target = (double*)(toRet.A);
    double* src = (double*)(A) + (startRow * n + startCol);
    for (size_t i = 0; i < numRows; ++i) {
      memcpy(target, src, numCols * sizeof(double));
      src += n;
      target += numCols;
    }
    return toRet;
  }

  std::vector<double> diagonal() {
    std::vector<double> diag;
    double* p = (double*)A;
    for (size_t i = 0; i < m && i < n; ++i, p += (n + 1)) {
      diag.push_back(*p);
    }
    return diag;
  }

 private:
  bool checkMatrixDimensions(Matrix B) {
    if (B.m != m || B.n != n) {
      return false;
    }
    return true;
  }
};
}
