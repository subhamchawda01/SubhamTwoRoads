/**
    \file MToolsExe/closed_form_lr.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/

#include "dvccode/Profiler/cpucycle_profiler.hpp"

#include "basetrade/Math/square_matrix.hpp"
#include "basetrade/MTools/iterative_regress.hpp"
#include "basetrade/MToolsExe/call_read_reg_data.hpp"

using namespace HFSAT;

/// Exec used to call closed form solution of Linear Regression
/// Reads command line arguments
int main(int argc, char** argv) {
  // local variables
  std::vector<double> train_data_dependant_;
  std::vector<std::vector<double> > train_data_independants_;

  std::vector<unsigned int> chosen_indep_index_vec_;

  std::vector<double> initial_correlations_;

  std::string input_reg_data_file_name_ = "";
  std::string input_variable_selection_filename_ = "";
  std::string regression_output_filename_ = "";

  double regularization_coeff_ = 0.50;
  bool is_first_indep_weight_ = false;

  // command line processing
  if (argc < 5) {
    std::cerr << argv[0] << " input_reg_data_file_name  input_variable_selection_filename  regularization_coeff  "
                            "is_first_indep_weight  regression_output_filename  " << std::endl;
    exit(0);
  }

  input_reg_data_file_name_ = argv[1];
  input_variable_selection_filename_ = argv[2];
  regularization_coeff_ = std::max(0.0, atof(argv[3]));
  is_first_indep_weight_ = (atoi(argv[4]) != 0);
  regression_output_filename_ = argv[5];

  ReadInIndicatorIndices(input_variable_selection_filename_, chosen_indep_index_vec_, initial_correlations_);
  train_data_independants_.resize(chosen_indep_index_vec_.size());

  // read data
  ReadSelectedData(input_reg_data_file_name_, chosen_indep_index_vec_, train_data_dependant_, train_data_independants_);

  unsigned int start_indep_index_ = 0;
  unsigned int indep_covar_matrix_dimension_ = train_data_independants_.size();

  std::vector<int> in_included_(train_data_independants_.size(), 0);  ///< included [ i ] = 0 if eligible to be selected
                                                                      ///, 1 if the indep(i) is already in the model, -1
  /// if it is ineligible to be selected

  if (is_first_indep_weight_) {
    in_included_[0] = -1;  // mark this index as ineligible to be selected as a chosen independant in the model
    for (auto i = 0u; i < train_data_dependant_.size(); i++) {
      train_data_dependant_[i] *= train_data_independants_[0][i];
    }
    for (unsigned int indep_index_ = 1; indep_index_ < train_data_independants_.size(); indep_index_++) {
      for (auto i = 0u; i < train_data_dependant_.size(); i++) {
        train_data_independants_[indep_index_][i] *= train_data_independants_[0][i];
      }
    }

    // so that the rest of the math does not include the first column
    start_indep_index_ = 1;
    indep_covar_matrix_dimension_ = (unsigned int)std::max((int)0, (((int)train_data_independants_.size()) - 1));
  }

  // computing mean and stdev ... to filter on high sharpe
  // normalize data and filter ( mark ineligible for selection )
  double mean_orig_dependant_ = 0;
  double stdev_orig_dependant_ = 0;
  VectorUtils::CalcMeanStdev(train_data_dependant_, mean_orig_dependant_, stdev_orig_dependant_);
  VectorUtils::NormalizeData(train_data_dependant_, mean_orig_dependant_, stdev_orig_dependant_);

  std::vector<double> mean_orig_indep_(train_data_independants_.size(), 0);
  std::vector<double> stdev_orig_indep_(train_data_independants_.size(), 1);
  VectorUtils::CalcNonZeroMeanStdevVec(train_data_independants_, mean_orig_indep_, stdev_orig_indep_);
  NormalizeNonZeroDataVec(train_data_independants_, mean_orig_indep_, stdev_orig_indep_, in_included_);

  std::vector<double> train_data_orig_dependant_ =
      train_data_dependant_;  // making a copy of this to compute model statistics

  std::vector<double> norm_reg_coeffs_(indep_covar_matrix_dimension_, 0);
  {  // computing norm_reg_coeffs_ = ( ( X' * X )  +  ( lambda * Identity ) )^-1 * ( X' * y )

    std::vector<double> dep_covar_array_(
        indep_covar_matrix_dimension_,
        0);  ///< ith value is meant to be Sum_(line_num_ = 0 to N) ( indep(i)(line_num_) * dep(line_num_) )
    HFSAT::SquareMatrix<double> covar_matrix_(indep_covar_matrix_dimension_);  ///< i,jth value is meant to be
    /// Sum_(line_num_ = 0 to N) (
    /// indep(i)(line_num_) *
    /// indep(j)(line_num_) )

    for (unsigned int line_num_ = 0; line_num_ < train_data_dependant_.size(); line_num_++) {
      for (unsigned int i = start_indep_index_; i < train_data_independants_.size(); i++) {
        dep_covar_array_[i - start_indep_index_] +=
            (train_data_dependant_[line_num_] * train_data_independants_[i][line_num_]);

        for (unsigned int j = start_indep_index_; j < train_data_independants_.size(); j++) {
          covar_matrix_((i - start_indep_index_), (j - start_indep_index_)) +=
              (train_data_independants_[i][line_num_] * train_data_independants_[j][line_num_]);
        }
      }
    }

    // scale regularization_coeff_ to the order of train_data_dependant_.size() since
    // covar_matrix_(i,i) = train_data_dependant_.size ().
    // This is because data has been normalized already.
    regularization_coeff_ *= (double)train_data_dependant_.size();
    // compute lambda * Identity
    HFSAT::SquareMatrix<double> regularized_identity_(covar_matrix_.row_count());
    for (auto i = 0u; i < regularized_identity_.row_count(); i++) {
      regularized_identity_(i, i) = regularization_coeff_;  // sort of like lambda * 1
    }
    covar_matrix_.AddToSelf(regularized_identity_);  // covar_matrix_ += regularized_identity_

    // now need to compute covar_matrix_ inverse
    HFSAT::SquareMatrix<double> inv_covar_matrix_(covar_matrix_.row_count());
    covar_matrix_.InvertMatrixSimple(inv_covar_matrix_);  // not implemented yet in basetrade/Math/square_matrix.hpp

    // multiply inv_covar_matrix with dep_covar_array_
    for (auto i = 0u; i < indep_covar_matrix_dimension_; i++) {
      for (unsigned int j = 0; j < indep_covar_matrix_dimension_; j++) {
        norm_reg_coeffs_[i] += inv_covar_matrix_(i, j) * dep_covar_array_[j];
      }
    }
  }

  std::ofstream regression_output_file_(regression_output_filename_.c_str(), std::ofstream::out);
  regression_output_file_
      << "OutConst " << 0 << " " << 0
      << std::endl;  // assuming that the best we can do is assume everything is mean 0 in unseen data
  for (auto i = 0u; i < indep_covar_matrix_dimension_; i++) {
    double stdev_readjusted_coeff_ = (norm_reg_coeffs_[i] * stdev_orig_dependant_) / (stdev_orig_indep_[i]);
    regression_output_file_ << "OutCoeff " << chosen_indep_index_vec_[i] << " " << stdev_readjusted_coeff_
                            << " InitCorrelation " << initial_correlations_[i] << std::endl;
  }

  double model_rsquared_ = 0;
  double model_correlation_ = 0;
  double stdev_final_dependant_ = 0;
  double stdev_model_ = 0;
  ComputeModelStatisticsOrigNormalized(train_data_orig_dependant_, train_data_dependant_, model_rsquared_,
                                       model_correlation_, stdev_final_dependant_, stdev_model_);

  regression_output_file_ << "RSquared " << model_rsquared_ << std::endl;
  double adjusted_model_rsquared_ =
      1 - ((1 - model_rsquared_) *
           ((train_data_dependant_.size() - 1) / (train_data_dependant_.size() - indep_covar_matrix_dimension_ - 1)));
  regression_output_file_ << "Adjustedrsquared " << adjusted_model_rsquared_ << std::endl;
  regression_output_file_ << "Correlation " << model_correlation_ << std::endl;
  regression_output_file_ << "StdevDependant " << stdev_orig_dependant_ << std::endl;
  // regression_output_file_ << "StdevFINALDependant " << stdev_final_dependant_ << std::endl ;
  regression_output_file_ << "StdevResidual " << (stdev_orig_dependant_ * stdev_final_dependant_) << std::endl;
  regression_output_file_ << "StdevModel " << (stdev_orig_dependant_ * stdev_model_) << std::endl;

  regression_output_file_.close();

  return 0;
}
