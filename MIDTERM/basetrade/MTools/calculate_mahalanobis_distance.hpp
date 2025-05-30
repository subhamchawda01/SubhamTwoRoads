#include <iostream>
#include "../basetrade/Math/square_matrix.hpp"

namespace HFSAT {

void PrintMatrix(std::vector<std::vector<double> > mat) {
  std::cerr << "Prining matrix..\n";
  for (unsigned i = 0; i < mat.size(); i++) {
    for (unsigned j = 0; j < mat[i].size(); j++) std::cerr << mat[i][j] << " ";
    std::cerr << std::endl;
  }
  return;
}

void CalculateMean(const std::vector<std::vector<double> > &row_major_data_, std::vector<double> &mean_data_) {
  for (unsigned i = 0; i < row_major_data_.size(); i++) {
    for (unsigned j = 0; j < row_major_data_[i].size(); j++) {
      mean_data_[j] += row_major_data_[i][j];
    }
  }
  for (unsigned j = 0; j < mean_data_.size(); j++) mean_data_[j] /= row_major_data_.size();
  return;
}

void ComputeCovarianceMatrix(const std::vector<std::vector<double> > &row_major_data_,
                             std::vector<std::vector<double> > &covariance_matrix_) {
  unsigned col = 0;
  unsigned row = row_major_data_.size();
  if (row > 0) col = row_major_data_[0].size();

  for (unsigned i = 0; i < col; i++) {
    covariance_matrix_[i].assign(col, 0.0);
  }

  for (unsigned k = 0; k < row; k++) {
    for (unsigned i = 0; i < col; i++) {
      for (unsigned j = 0; j < col; j++) {
        covariance_matrix_[i][j] += row_major_data_[k][i] * row_major_data_[k][j];
      }
    }
  }
  return;
}

bool MultiplyVecMatrix(const std::vector<double> &vec_, const HFSAT::SquareMatrix<double> &matrix_,
                       const unsigned int m, const unsigned int n, std::vector<double> &mul_result_) {
  unsigned int vec_len_ = vec_.size();
  double sum;
  if (vec_len_ == m || vec_len_ == n) {
    if (vec_len_ == m) {
      for (auto i = 0u; i < n; i++) {
        sum = 0;
        for (unsigned int j = 0; j < m; j++) {
          sum += vec_[j] * matrix_(j, i);
        }
        mul_result_.push_back(sum);
      }
    } else {
      for (auto i = 0u; i < m; i++) {
        sum = 0;
        for (unsigned int j = 0; j < n; j++) {
          sum += matrix_(i, j) * vec_[j];
        }
        mul_result_.push_back(sum);
      }
    }
  } else {
    std::cerr << "The vector " << vec_len_ << " and matrix" << m << " " << n << " have not multipliable dimensions..\n";
    // cant multiply , the dimensions do not match for multiplication
    return false;
  }
  return true;
}

bool CalculateMahalanobisDistance(const std::vector<std::vector<double> > &row_major_data_,
                                  const std::vector<std::vector<double> > &covariance_matrix_,
                                  std::vector<double> &maha_distance_) {
  unsigned int m = covariance_matrix_.size();
  HFSAT::SquareMatrix<double> cov_matrix_(m);
  HFSAT::SquareMatrix<double> inverse_matrix_;
  inverse_matrix_.Init(m);
  for (unsigned i = 0; i < m; i++) {
    for (unsigned j = 0; j < m; j++) cov_matrix_.SetData(i, j, covariance_matrix_[i][j]);
  }

  if (!cov_matrix_.InvertMatrixSimple(inverse_matrix_)) {
    std::cerr << "Unable to invert matrix...\n";
    return false;
  }
  // std::cerr<< "Printing the covariance matrix\n" << cov_matrix_.ToString() << std::endl;
  // std::cerr<< "Printing the inverse covariance matrix \n" << inverse_matrix_.ToString() << std::endl;
  double sum;
  std::vector<double> diff_vector_;

  for (unsigned i = 0; i < row_major_data_.size(); i++) {
    diff_vector_.clear();
    for (unsigned j = 0; j < row_major_data_[i].size(); j++)
      diff_vector_.push_back(row_major_data_[i][j]);  // since the data is normalized, the mean will be zero

    std::vector<double> temp_vector_;
    if (MultiplyVecMatrix(diff_vector_, inverse_matrix_, m, m, temp_vector_)) {
      sum = 0.0;
      for (unsigned j = 0; j < m; j++) {
        sum += temp_vector_[j] * diff_vector_[j];
      }
      maha_distance_.push_back(sum);
    } else {
      std::cerr << "unable to multiply vec and matrix..\n";
      return false;
    }
  }
  return true;
}

bool ComputeMahaDistance(const std::vector<std::vector<double> > &train_data_independants_,
                         const std::vector<int> &in_included_, std::vector<double> &maha_distance_) {
  /* Here the matrix data is given in form of vector of vector and each data point is a colomn in the matrix
   * Calculate the covariance matrix from this matrix with only indices having _in_included_ == 0
   * To compute the inverse of the data, we need to change it in form of double** and row contsaining the data point
   */
  unsigned int num_dimensions_ = 0, num_data_points_ = 0;
  // std::vector<double> maha_distance_;
  maha_distance_.clear();
  // distance_vec_.clear();
  std::vector<std::vector<double> > covariance_matrix_;
  if (train_data_independants_.size() <= 0) {
    std::cerr << "the size of the independents is 0\n";
    return false;
  }

  num_data_points_ = train_data_independants_[0].size();
  std::vector<std::vector<double> > row_major_data_(num_data_points_);
  std::vector<double> temp_vec_;
  temp_vec_.clear();
  for (unsigned i = 0; i < in_included_.size(); i++) {
    if (in_included_[i] == 0) {
      temp_vec_.clear();
      for (unsigned j = 0; j < num_data_points_; j++) {
        row_major_data_[j].push_back(train_data_independants_[i][j]);
        temp_vec_.push_back(train_data_independants_[i][j]);
      }
      num_dimensions_++;
    }
  }
  if (num_dimensions_ < 1) {
    std::cerr << "there are no dimensions in data..\n";
    return false;
  }

  // std::cerr << "printing data\n";
  // PrintMatrix(row_major_data_);
  covariance_matrix_.resize(num_dimensions_, std::vector<double>(num_dimensions_));

  ComputeCovarianceMatrix(row_major_data_, covariance_matrix_);

  if (CalculateMahalanobisDistance(row_major_data_, covariance_matrix_, maha_distance_)) {
    return true;
  }
  std::cerr << "Distance not calculated..\n";
  return false;
}
}

/*
int main()
{
    std::vector<double> sing_vec_;
    std::vector <std::vector <double> > data_;
    std::vector <int> in_included_;
    in_included_.push_back(0);in_included_.push_back(0);
    in_included_.push_back(0);
    in_included_.push_back(0);

    std::vector <double> dist_;
    data_.push_back({1,2, 3});
    data_.push_back({3,4, 9});
    data_.push_back({11,23, 13});
    data_.push_back({23,45, 7});
    std::cerr << "calculating the maha distance..\n"<< std::endl;
    if (ComputeMahaDistance(data_, in_included_, dist_))
      std::cerr << "distance calculated..\n";
    else
    {
      std::cerr << "distance could not be calculated....\n";
    }
    for ( unsigned i = 0; i < dist_.size(); i++)
      std::cerr << dist_[i]<< " " ;
    std::cerr << std::endl;

    return 0;
}*/
