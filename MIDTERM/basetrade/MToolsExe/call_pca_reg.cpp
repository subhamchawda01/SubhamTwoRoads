/**
    \file MToolsExe/call_pca_reg.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
 */

//#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvctrade/linal/linal_util.hpp"
#include "dvctrade/linal/PCA.hpp"
#include "basetrade/MTools/data_processing.hpp"
#include "basetrade/MTools/iterative_regress.hpp"
#include "basetrade/MToolsExe/call_read_reg_data.hpp"
#include "basetrade/MToolsExe/mtools_utils.hpp"

using namespace HFSAT;

void ChooseIndicatorsPerPCAComponent(const std::vector<IndexCoeffPairSimple_t>& finmodel_,
                                     const std::vector<std::vector<double> >& pca_eigen_vectors_,
                                     const int max_model_size_, std::vector<int>& indicators_selected_) {
  double sum_coeffs_ = 0.0;
  for (auto i = 0u; i < finmodel_.size(); i++) sum_coeffs_ += fabs(finmodel_[i].coeff_);

  unsigned int carry_over_slots_ = 0;
  for (auto i = 0u; i < finmodel_.size(); i++) {
    std::vector<size_t> top_indicators_this_component_;
    HFSAT::VectorUtils::index_sort(pca_eigen_vectors_[finmodel_[i].origindex_], top_indicators_this_component_);
    unsigned int num_indicators_added_from_this_component_ = 0;
    unsigned int t_num_indicators_allowed_for_this_component_ =
        std::max(0, (int)((fabs(finmodel_[i].coeff_) * max_model_size_) / sum_coeffs_)) +
        carry_over_slots_;  // passing on the remaining slots in max_model_size to other components
    for (unsigned k = 0;
         k < top_indicators_this_component_.size() &&
             num_indicators_added_from_this_component_ < t_num_indicators_allowed_for_this_component_ &&
             fabs(pca_eigen_vectors_[finmodel_[i].origindex_][top_indicators_this_component_[k]]) > 0.01;
         k++) {
      if (indicators_selected_[top_indicators_this_component_[k]] == 0) {
        indicators_selected_[top_indicators_this_component_[k]] = 1;
        num_indicators_added_from_this_component_++;
      }
    }
    carry_over_slots_ = t_num_indicators_allowed_for_this_component_ - num_indicators_added_from_this_component_;
  }
}

void MakeMatrix(std::vector<std::vector<double> >& data, LINAL::Matrix& matrix,
                const std::vector<unsigned int>& removed_columns_indices) {
  matrix.create(data[0].size(), data.size() - removed_columns_indices.size());
  unsigned int removed_count = 0;

  for (auto i = 0u; i < data.size(); ++i) {
    if (removed_columns_indices.size() > removed_count && i == removed_columns_indices[removed_count]) {
      ++removed_count;
      continue;
    }
    for (unsigned int j = 0; j < data[0].size(); ++j) {
      matrix(j, i - removed_count) = data[i][j];
    }
  }
}

/// Computes PCA of the indicators
/// Runs FSLR on the eigen vectors
/// Projects coefficients back to indicator space
/*
load data into memory and normalize ( and set included -1 of high sharpe indicators, and maybe zero out values if needed
)
use linal pca to compute eigen vectors and values ( perhaps handling of zero-value or constant-value columns )
make new data ( which is original data projected onto eigen vectors ).
We aren't limiting to eigen vectors that explain at least a good fraction of the variance since we could have a very
good indicator
that does not explain too much of the variance.
Since the new data is orthogonal, compute correlations of new data columns with dependant, and sort them by correlation
for only the columns with correlation > min_corr_threshold compute linear regression coefficients.
to transfer coefficients to original indicator space use
coeff_indicator [ i ] is sum over all non zero eigen vector coeffs { lr_coeff_eigen [ j ] * eigen_vectpr [ i ] [ j ] }
and then we select the top max_model_size indicators based on absolute value of coefficient.
finally we divide on-zero coeff_indicator values by original stdev
*/
int main(int argc, char** argv) {
  // local variables
  std::vector<double> train_data_dependant_;
  std::vector<std::vector<double> > train_data_independants_;
  std::vector<std::vector<double> > pca_eigen_vectors_;

  std::vector<IndexCoeffPairSimple_t> finmodel_;
  std::vector<IndexCoeffPairSimple_t> finmodel_final_;
  std::string infilename_ = "";
  std::string regression_output_filename_ = "";

  double min_correlation_ = 0.01;
  const double kMaxSharpeDependant = 0.20;
  const double kMaxSharpeIndependant = 0.22;
  unsigned int max_model_size_ = 16u;
  unsigned int num_components_ = 6u;
  double target_stdev_model_ = 0;

  // command line processing
  if (argc < 6) {
    std::cerr << "USAGE: " << argv[0]
              << " reg_data_file_name min_correlation  regression_output_filename  max_model_size  num_components"
              << std::endl;
    exit(0);
  }

  int arg_index = 0;
  infilename_ = argv[++arg_index];
  min_correlation_ = atof(argv[++arg_index]);
  regression_output_filename_ = argv[++arg_index];
  max_model_size_ = (unsigned int)std::min(30, std::max(1, atoi(argv[++arg_index])));
  num_components_ = (unsigned int)std::min(5, std::max(1, atoi(argv[++arg_index])));

  // read data
  ReadData(infilename_, train_data_dependant_, train_data_independants_);

  // normalize data and filter ( mark ineligible for selection )
  double mean_orig_dependant_ = 0;
  double stdev_orig_dependant_ = 0;
  VectorUtils::CalcMeanStdev(train_data_dependant_, mean_orig_dependant_, stdev_orig_dependant_);
  if ((stdev_orig_dependant_ <= 0) || (fabs(mean_orig_dependant_) > (kMaxSharpeDependant * stdev_orig_dependant_))) {
    double sharpe_ = (fabs(mean_orig_dependant_) / stdev_orig_dependant_);
    if (std::isnan(sharpe_)) {
      sharpe_ = 0;
    } else {
      std::cerr << " Sharpe of the dependant is " << sharpe_ << " more than " << kMaxSharpeDependant
                << " since fabs(mean) " << fabs(mean_orig_dependant_)
                << " > kMaxSharpeDependant * stdev = " << stdev_orig_dependant_ << std::endl;
    }
    // not stopping if dependant has a high sharpe
    // just reporting it, and removing mean like normal
    // exit ( kCallIterativeRegressHighSharpeDependant );
  }

  if (target_stdev_model_ <= 0.00000001) {
    target_stdev_model_ = stdev_orig_dependant_;
  }

  std::vector<double> mean_indep_(train_data_independants_.size(), 0);
  std::vector<double> std_indep_(train_data_independants_.size(), 0);
  std::vector<int> dummy_incl_list_(train_data_independants_.size(), 0);
  HFSAT::CalcMeanStdevNormalizeDataVec(train_data_independants_, mean_indep_, std_indep_, dummy_incl_list_);
  // remove high sharpe indep
  std::vector<unsigned int> removed_columns;
  std::vector<unsigned int>
      selected_colums;  // A complement of removed_columns.. Will be later used to rename indices back to original
  for (auto i = 0u; i < mean_indep_.size(); ++i) {
    if (std_indep_[i] == 0) {
      removed_columns.push_back(i);
      continue;
    }
    double sharpe = fabs(mean_indep_[i] / std_indep_[i]);
    if (sharpe > kMaxSharpeIndependant)
      removed_columns.push_back(i);
    else
      selected_colums.push_back(i);
  }

  LINAL::Matrix data_matrix_;  // Make a compact matrix removing high sharpe indep
  MakeMatrix(train_data_independants_, data_matrix_, removed_columns);
  PCA data_pca(data_matrix_, true);  // mean_removed = true
  data_pca.GetSignificanceSortedEigenVectors(pca_eigen_vectors_);
  LINAL::Matrix feature_matrix = data_pca.getFeatureMatrix(pca_eigen_vectors_.size());
  {  // limiting scope of transformed_data_matrix
    LINAL::Matrix transformed_data_matrix = data_matrix_.times(feature_matrix);

    if (transformed_data_matrix.getColumnDimension() < train_data_independants_.size())
      train_data_independants_.resize(transformed_data_matrix.getColumnDimension());

    for (size_t i = 0; i < transformed_data_matrix.getRowDimension(); ++i) {
      for (size_t j = 0; j < transformed_data_matrix.getColumnDimension(); ++j) {
        train_data_independants_[j][i] = transformed_data_matrix.get(i, j);
      }
    }
  }

  // free space
  data_matrix_.freeSpace();

  // Normalize transformed data
  // This is required as transforming the original transformed data based on its principal components destroys the
  // std-dev=1 of original data.
  // Its still zero mean however.
  std::vector<double> mean_indep_tranformed_(train_data_independants_.size(), 0);
  std::vector<double> std_indep_tranformed_(train_data_independants_.size(), 0);
  std::vector<int> in_included_(train_data_independants_.size(), 0);
  HFSAT::CalcMeanStdevNormalizeDataVec(train_data_independants_, mean_indep_tranformed_, std_indep_tranformed_,
                                       in_included_);

  // Call PCA_REG ( similar to FSLR except that it assumes orthogonality of independents

  std::vector<double> initial_correlations_(train_data_independants_.size(), 0);
  HFSAT::ComputeCorrelationsOfNormalizedDataVec(train_data_dependant_, train_data_independants_, initial_correlations_,
                                                in_included_);

  std::vector<double> train_data_orig_dependant_ = train_data_dependant_;
  HFSAT::PCA_REG(train_data_dependant_, train_data_independants_, in_included_, finmodel_, min_correlation_,
                 initial_correlations_, num_components_);

  std::ofstream regression_output_file_(regression_output_filename_.c_str(), std::ofstream::out);

  std::vector<int> indicators_selected_(pca_eigen_vectors_[0].size(), 0);
  ChooseIndicatorsPerPCAComponent(finmodel_, pca_eigen_vectors_, max_model_size_, indicators_selected_);

  for (auto i = 0u; i < finmodel_.size(); i++) {
    finmodel_[i].coeff_ =
        (finmodel_[i].coeff_ * target_stdev_model_) / (std_indep_tranformed_[finmodel_[i].origindex_]);
  }

  for (auto i = 0u; i < pca_eigen_vectors_.size(); i++) {
    int in_model_index_ = -1;
    for (unsigned int j = 0; j < finmodel_.size(); j++) {
      if (i == finmodel_[j].origindex_) {
        in_model_index_ = j;
      }
    }

    for (unsigned int j = 0; j < pca_eigen_vectors_.size(); j++) {
      if (in_model_index_ < 0) {
        pca_eigen_vectors_[i][j] *= 0;
      } else {
        pca_eigen_vectors_[i][j] *= finmodel_[in_model_index_].coeff_;
      }
    }
  }

  for (unsigned int j = 0; j < indicators_selected_.size(); j++) {
    if (indicators_selected_[j] == 1) {
      double sum_weights_ = 0.0;
      for (auto i = 0u; i < pca_eigen_vectors_.size(); i++) {
        sum_weights_ += pca_eigen_vectors_[i][j];
      }
      IndexCoeffPairSimple_t icps_t;
      icps_t.origindex_ = j;
      icps_t.coeff_ = sum_weights_;
      finmodel_final_.push_back(icps_t);
    }
  }

  // Because we have worked with reduced data set after removing high sharpe columns, we want to account for that those
  // in the final answer
  // implicitly assume removed columns is ascending
  // Correct final model coefficients keeping in mind that certain indices were removed initially
  for (auto i = 0u; i < finmodel_final_.size(); i++) {
    finmodel_final_[i].origindex_ = selected_colums[finmodel_final_[i].origindex_];
    // now adjust for stdev
    finmodel_final_[i].coeff_ = std_indep_[finmodel_final_[i].origindex_];
  }

  // ignoring constant for now, since probability of the constant in the dependant to be predictable is low.
  // double would_be_constant_term_ = 0;
  regression_output_file_
      << "OutConst " << 0 << " " << 0
      << std::endl;  // assuming that the best we can do is assume everything is mean 0 in unseen data
  for (auto i = 0u; i < finmodel_final_.size(); i++) {
    regression_output_file_ << "OutCoeff " << finmodel_final_[i].origindex_ << ' ' << finmodel_final_[i].coeff_
                            << std::endl;
  }
  double model_rsquared_ = 0;
  double model_correlation_ = 0;
  double stdev_final_dependant_ = 0;
  double stdev_model_ = 0;
  double mse_ = 0.0;

  for (auto i = 0u; i < train_data_dependant_.size(); i++) {
    mse_ += train_data_dependant_[i] * train_data_dependant_[i];
  }
  mse_ = mse_ / train_data_dependant_.size();

  ComputeModelStatisticsOrigNormalized(train_data_orig_dependant_, train_data_dependant_, model_rsquared_,
                                       model_correlation_, stdev_final_dependant_, stdev_model_);

  regression_output_file_ << "RSquared " << model_rsquared_ << std::endl;
  double adjusted_model_rsquared_ = 1 - ((1 - model_rsquared_) * ((train_data_dependant_.size() - 1) /
                                                                  (train_data_dependant_.size() - finmodel_.size())));
  regression_output_file_ << "Adjustedrsquared " << adjusted_model_rsquared_ << std::endl;
  regression_output_file_ << "Correlation " << model_correlation_ << std::endl;
  regression_output_file_ << "StdevDependent " << (target_stdev_model_) << std::endl;
  regression_output_file_ << "StdevResidual " << (target_stdev_model_ * stdev_final_dependant_) << std::endl;
  regression_output_file_ << "StdevModel " << (target_stdev_model_ * stdev_model_) << std::endl;
  regression_output_file_ << "MSE " << mse_ << "\n";
  regression_output_file_.close();

  return 0;
}
