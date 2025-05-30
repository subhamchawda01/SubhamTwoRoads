/**
    \file MToolsExe/call_FSLR.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/

#include "dvccode/Profiler/cpucycle_profiler.hpp"

#include "basetrade/MTools/iterative_regress.hpp"
#include "basetrade/MToolsExe/call_read_reg_data.hpp"

using namespace HFSAT;

/// Exec used to call Forward Stagewise Linear Regression
/// Reads command line arguments
/// Compute mean & stdev of dependant
/// Compute mean & stdev of all independants
/// Normalize dependant
/// Normalize independants
/// Compute initial correlations and mark the columns ineligible that do not make the min_correlation_ cut
/// make a copy of dependant column ( to compute stats later on )
/// call FSLR ( on data that is mean zero to save needless mean calcing operations )
int main(int argc, char** argv) {
  // local variables
  std::vector<double> train_data_dependant_;
  std::vector<std::vector<double> > train_data_independants_;

  int must_add_next_k_indeps_ = 0;

  std::vector<IndexCoeffPair_t> finmodel_;

  std::string infilename_ = "";
  std::string regression_output_filename_ = "";

  double min_correlation_ = 0.01;
  const double kMaxSharpeDependant = 0.20;    // 0.10
  const double kMaxSharpeIndependant = 0.22;  // 0.10
  double max_indep_correlation_ = 0.7;
  unsigned int max_model_size_ = 20;

  bool first_indep_is_weight_ = false;

  // command line processing
  if (argc < 7) {
    std::cerr << "USAGE: " << argv[0]
              << " input_file_name  min_correlation  first_indep_is_weight  mult_include_first_k_independants  "
                 "max_indep_correlation  regression_output_filename  max_model_size" << std::endl;
    exit(0);
  }

  infilename_ = argv[1];
  min_correlation_ = atof(argv[2]);
  first_indep_is_weight_ = (atoi(argv[3]) != 0);
  must_add_next_k_indeps_ = atoi(argv[4]);
  max_indep_correlation_ = atof(argv[5]);
  regression_output_filename_ = argv[6];
  max_model_size_ = (unsigned int)std::min(30, std::max(1, atoi(argv[7])));

  // read data
  ReadData(infilename_, train_data_dependant_, train_data_independants_);

  std::vector<int> in_included_(train_data_independants_.size(), 0);  ///< included [ i ] = 0 if eligible to be selected
                                                                      ///, 1 if the indep(i) is already in the model, -1
  /// if it is ineligible to be selected
  // mark all indices eligible to be selected

  if (first_indep_is_weight_) {
    in_included_[0] = -1;  // mark this index as ineligible to be selected as a chosen independant in the model
    for (auto i = 0u; i < train_data_dependant_.size(); i++) {
      train_data_dependant_[i] *= train_data_independants_[0][i];
    }
    for (unsigned int indep_index_ = 1; indep_index_ < train_data_independants_.size(); indep_index_++) {
      for (auto i = 0u; i < train_data_dependant_.size(); i++) {
        train_data_independants_[indep_index_][i] *= train_data_independants_[0][i];
      }
    }
  }

  // computing mean and stdev ... to filter on high sharpe
  // normalize data and filter ( mark ineligible for selection )
  double mean_orig_dependant_ = 0;
  double stdev_orig_dependant_ = 0;
  VectorUtils::CalcMeanStdev(train_data_dependant_, mean_orig_dependant_, stdev_orig_dependant_);
  if ((stdev_orig_dependant_ <= 0) || (fabs(mean_orig_dependant_) > (kMaxSharpeDependant * stdev_orig_dependant_))) {
    std::cerr << " Sharpe of the dependant is " << (fabs(mean_orig_dependant_) / stdev_orig_dependant_) << " more than "
              << kMaxSharpeDependant << " since fabs(mean) " << fabs(mean_orig_dependant_)
              << " > kMaxSharpeDependant * stdev = " << stdev_orig_dependant_ << std::endl;

    // not stopping if dependant has a high sharpe
    // just reporting it, and removing mean like normal
    // exit ( kCallIterativeRegressHighSharpeDependant );
  }

  VectorUtils::NormalizeData(train_data_dependant_, mean_orig_dependant_, stdev_orig_dependant_);
  // std::cout << "Dependant: Mean: " << mean_orig_dependant_ << " Stdev: " << stdev_orig_dependant_ << std::endl;

  std::vector<double> mean_orig_indep_(train_data_independants_.size(), 0);
  std::vector<double> stdev_orig_indep_(train_data_independants_.size(), 1);

  VectorUtils::CalcNonZeroMeanStdevVec(train_data_independants_, mean_orig_indep_, stdev_orig_indep_);

  for (unsigned int indep_index_ = 0; indep_index_ < train_data_independants_.size(); indep_index_++) {
    if ((in_included_[indep_index_] == 0) &&
        ((stdev_orig_indep_[indep_index_] <= 0) ||
         (mean_orig_indep_[indep_index_] > (kMaxSharpeIndependant * stdev_orig_indep_[indep_index_])))) {
      // in_included_ [ indep_index_ ] = -1; // marking high sharpe indicators as ineligible for selection

      std::cerr << " Sharpe of indep ( " << indep_index_ << " ) is "
                << (fabs(mean_orig_indep_[indep_index_]) / stdev_orig_indep_[indep_index_]) << " more than "
                << kMaxSharpeDependant << " since fabs(mean) " << fabs(mean_orig_indep_[indep_index_])
                << " > kMaxSharpeDependant * stdev = " << stdev_orig_indep_[indep_index_] << std::endl;
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

  for (unsigned int indep_index_ = 0; indep_index_ < train_data_independants_.size(); indep_index_++) {
    if (in_included_[indep_index_] == 0) {
      if (fabs(initial_correlations_[indep_index_]) < min_correlation_) {
        in_included_[indep_index_] =
            -1;  // making indicator ineligible to be selected since it's outright correlation is very low
      }
    }
  }

  std::vector<double> train_data_orig_dependant_ =
      train_data_dependant_;  // making a copy of this to compute model statistics
  std::vector<std::vector<double> > train_data_independants_original_ = train_data_independants_;

  FSLR_SO(train_data_dependant_, train_data_independants_, must_add_next_k_indeps_, in_included_, finmodel_,
          min_correlation_, max_indep_correlation_, initial_correlations_, max_model_size_,
          train_data_independants_original_);

  std::ofstream regression_output_file_(regression_output_filename_.c_str(), std::ofstream::out);

  // ignoring constant for now, since probability of the constant in the dependant to be predictable is low.
  // double would_be_constant_term_ = 0;
  // would_be_constant_term_ += mean_orig_dependant_ ;
  regression_output_file_
      << "OutConst " << 0 << " " << 0
      << std::endl;  // assuming that the best we can do is assume everything is mean 0 in unseen data
  for (auto i = 0u; i < finmodel_.size(); i++) {
    double stdev_readjusted_coeff_ =
        (finmodel_[i].coeff_ * stdev_orig_dependant_) / (stdev_orig_indep_[finmodel_[i].origindex_]);
    // would_be_constant_term_ -= ( stdev_readjusted_coeff_ * mean_orig_indep_ [ finmodel_[i].origindex_ ] ) ;
    regression_output_file_ << "OutCoeff " << finmodel_[i].origindex_ << ' ' << stdev_readjusted_coeff_
                            << " InitCorrelation " << initial_correlations_[finmodel_[i].origindex_] << " Tstat "
                            << finmodel_[i].tstat_ << " TstatSelect " << finmodel_[i].tstat_residual_dependant_
                            << std::endl;
  }
  // regression_output_file_ << "Wouldbeconstantterm " << would_be_constant_term_ << std::endl ;

  double model_rsquared_ = 0;
  double model_correlation_ = 0;
  double stdev_final_dependant_ = 0;
  double stdev_model_ = 0;
  ComputeModelStatisticsOrigNormalized(train_data_orig_dependant_, train_data_dependant_, model_rsquared_,
                                       model_correlation_, stdev_final_dependant_, stdev_model_);

  regression_output_file_ << "RSquared " << model_rsquared_ << std::endl;
  regression_output_file_ << "Adjustedrsquared "
                          << (model_rsquared_ * (train_data_dependant_.size() - finmodel_.size()) /
                              (train_data_dependant_.size())) << std::endl;
  regression_output_file_ << "Correlation " << model_correlation_ << std::endl;
  regression_output_file_ << "StdevDependant " << stdev_orig_dependant_ << std::endl;
  // regression_output_file_ << "StdevFINALDependant " << stdev_final_dependant_ << std::endl ;
  regression_output_file_ << "StdevResidual " << (stdev_orig_dependant_ * stdev_final_dependant_) << std::endl;
  regression_output_file_ << "StdevModel " << (stdev_orig_dependant_ * stdev_model_) << std::endl;

  regression_output_file_.close();

  return 0;
}
