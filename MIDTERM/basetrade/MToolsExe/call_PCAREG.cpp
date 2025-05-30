/**
    \file MToolsExe/call_PCAREG.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
 */

//#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvctrade/linal/linal_util.hpp"
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

/// Exec used to find PCA Regression coefficients
/// Computes PCA of the indicators
/// Runs FSLR on the eigen vectors
/// Projects coefficients back to indicator space
int main(int argc, char** argv) {
  // local variables
  std::vector<double> train_data_dependant_;
  std::vector<std::vector<double> > train_data_independants_;
  std::vector<std::vector<double> > pca_eigen_vectors_;

  std::vector<IndexCoeffPairSimple_t> finmodel_;
  std::vector<IndexCoeffPairSimple_t> finmodel_final_;
  std::string infilename_ = "";
  std::string eigen_infilename_ = "";
  std::string regression_output_filename_ = "";

  double min_correlation_ = 0.01;
  const double kMaxSharpeDependant = 0.20;
  const double kMaxSharpeIndependant = 0.22;
  unsigned int max_model_size_ = 16u;
  unsigned int num_components_ = 6u;
  double target_stdev_model_ = 0;

  // command line processing
  if (argc < 7) {
    std::cerr << "USAGE: " << argv[0] << " transformed_reg_data_file_name pca_eigen_vectors_filename min_correlation  "
                                         "regression_output_filename  max_model_size  num_components" << std::endl;
    exit(0);
  }

  infilename_ = argv[1];
  eigen_infilename_ = argv[2];
  min_correlation_ = atof(argv[3]);
  regression_output_filename_ = argv[4];
  max_model_size_ = (unsigned int)std::min(30, std::max(1, atoi(argv[5])));
  num_components_ = (unsigned int)std::min(5, std::max(1, atoi(argv[6])));

  // read data
  ReadData(infilename_, train_data_dependant_, train_data_independants_);
  ReadInData(eigen_infilename_, pca_eigen_vectors_);
  for (auto i = 0u; i < pca_eigen_vectors_.size(); i++) {
    for (unsigned int j = 0; j < pca_eigen_vectors_.size(); j++) {
      if (std::isnan(pca_eigen_vectors_[i][j])) {
        std::cerr << "Nan found in eigen vectors..exiting\n";
        std::ofstream regression_output_file_(regression_output_filename_.c_str(), std::ofstream::out);
        regression_output_file_
            << "OutConst " << 0 << " " << 0
            << std::endl;  // assuming that the best we can do is assume everything is mean 0 in unseen data
        regression_output_file_.close();
        exit(-1);
      }
    }
  }

  bool ignore_zeros_ = false;
  if (argc >= 8 && *argv[7] == 'Y') {
    std::cerr << "selecting indicators via correlation excluding zeros\n";
    ignore_zeros_ = true;
  }

  std::vector<int> in_included_(train_data_independants_.size(), 0);  ///< included [ i ] = 0 if eligible to be selected
                                                                      ///, 1 if the indep(i) is already in the model, -1
  /// if it is ineligible to be selected
  // mark all indices eligible to be selected

  // computing mean and stdev ... to filter on high sharpe
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

  VectorUtils::NormalizeData(train_data_dependant_, mean_orig_dependant_, stdev_orig_dependant_);

  std::vector<double> mean_orig_indep_(train_data_independants_.size(), 0);
  std::vector<double> stdev_orig_indep_(train_data_independants_.size(), 1);

  VectorUtils::CalcNonZeroMeanStdevVec(train_data_independants_, mean_orig_indep_, stdev_orig_indep_);

  for (unsigned int indep_index_ = 0; indep_index_ < train_data_independants_.size(); indep_index_++) {
    if ((in_included_[indep_index_] == 0) &&
        ((stdev_orig_indep_[indep_index_] <= 0) ||
         (fabs(mean_orig_indep_[indep_index_]) > (kMaxSharpeIndependant * stdev_orig_indep_[indep_index_])))) {
      double sharpe_ = 0;
      if (stdev_orig_indep_[indep_index_] > 0) {
        sharpe_ = (fabs(mean_orig_indep_[indep_index_]) / stdev_orig_indep_[indep_index_]);
        if (std::isnan(sharpe_)) {
          sharpe_ = 0;
        } else {
          std::cerr << " Sharpe of indep ( " << indep_index_ << " ) is " << sharpe_ << " more than "
                    << kMaxSharpeIndependant << " since fabs(mean) " << fabs(mean_orig_indep_[indep_index_])
                    << " > kMaxSharpeIndependant * stdev = " << stdev_orig_indep_[indep_index_] << std::endl;
        }
      } else {
        in_included_[indep_index_] = -1;  // marking zero stdev indicators as ineligible for selection
      }
    }
  }

  if (NextNotIncluded(in_included_) == kInvalidArrayIndex) {
    std::cerr << "No valid independant left after eliminating high sharpe ones" << std::endl;
    exit(kCallIterativeRegressHighSharpeIndep);
  }

  NormalizeNonZeroDataVec(train_data_independants_, mean_orig_indep_, stdev_orig_indep_, in_included_);

  // computing initial correlation to filter on min correlation
  std::vector<double> initial_correlations_(train_data_independants_.size(),
                                            0);  // sum_j ( x_i_j * y_j ) = corr ( x_i, y ) // since all data normalized

  ComputeCorrelationsOfNormalizedDataVec(train_data_dependant_, train_data_independants_, initial_correlations_,
                                         in_included_);

  std::vector<double> train_data_orig_dependant_ = train_data_dependant_;

  PCA_REG(train_data_dependant_, train_data_independants_, in_included_, finmodel_, min_correlation_,
          initial_correlations_, num_components_, ignore_zeros_);

  std::ofstream regression_output_file_(regression_output_filename_.c_str(), std::ofstream::out);

  std::vector<int> indicators_selected_(pca_eigen_vectors_[0].size(), 0);
  ChooseIndicatorsPerPCAComponent(finmodel_, pca_eigen_vectors_, max_model_size_, indicators_selected_);

  for (auto i = 0u; i < finmodel_.size(); i++) {
    finmodel_[i].coeff_ = (finmodel_[i].coeff_ * target_stdev_model_) / (stdev_orig_indep_[finmodel_[i].origindex_]);
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
