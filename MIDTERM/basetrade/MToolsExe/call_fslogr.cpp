/**
    \file MToolsExe/call_fslogr.cpp

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
#include "basetrade/MToolsExe/mtools_utils.hpp"

using namespace HFSAT;

/// Exec used to call Forward Stagewise Linear Regression
/// Reads command line arguments
/// Compute mean & stdev of dependent
/// Compute mean & stdev of all independents
/// Normalize dependent
/// Normalize independents
/// Compute initial correlations and mark the columns ineligible that do not make the min_correlation_ cut
/// make a copy of dependent column ( to compute stats later on )
/// call FSLR ( on data that is mean zero to save needless mean calcing operations )
int main(int argc, char** argv) {
  // local variables
  std::vector<double> train_data_dependent_;
  std::vector<std::vector<double> > train_data_independents_;

  std::vector<std::vector<double> > model_;

  std::string infilename_ = "";
  std::string regression_output_filename_ = "";

  const double kMaxSharpeIndependent = 0.22;
  double max_indep_correlation_ = 0.7;
  double zero_threshold_ = 0.2;
  unsigned int max_model_size_ = 10u;
  std::string avoid_high_sharpe_check_vector_filename_ = "";

  // command line processing
  if (argc < 4) {
    std::cerr << "USAGE: " << argv[0] << " input_reg_data_file_name zero_threshold max_model_size" << std::endl;
    exit(0);
  }

  infilename_ = argv[1];
  max_indep_correlation_ = atof(argv[2]);
  zero_threshold_ = atof(argv[3]);
  max_model_size_ = (unsigned int)std::min(30, std::max(1, atoi(argv[4])));

  // read data
  ReadData(infilename_, train_data_dependent_, train_data_independents_);

  std::vector<int> in_included_(train_data_independents_.size(), 0);  ///< included [ i ] = 0 if eligible to be selected
                                                                      ///, 1 if the indep(i) is already in the model, -1
  /// if it is ineligible to be selected
  // mark all indices eligible to be selected

  std::vector<int> avoid_high_sharpe_check_vector_(train_data_independents_.size(), 0);

  double mean_orig_dependent_ = 0;
  double stdev_orig_dependent_ = 0;
  VectorUtils::CalcMeanStdev(train_data_dependent_, mean_orig_dependent_, stdev_orig_dependent_);

  VectorUtils::NormalizeData(train_data_dependent_, mean_orig_dependent_, stdev_orig_dependent_);

  std::vector<int> discrete_train_data_dependent_;
  // discretize the dependent based upon the threshold
  int zero_count_ = 0;
  int pos_count_ = 0;
  int neg_count_ = 0;
  for (auto i = 0u; i < train_data_dependent_.size(); i++) {
    if (abs(train_data_dependent_[i]) < zero_threshold_) {
      // 0  for small/no change in price
      discrete_train_data_dependent_.push_back(0);
      zero_count_++;
    } else if (train_data_dependent_[i] > zero_threshold_) {
      // 1 for increase in price
      discrete_train_data_dependent_.push_back(1);
      pos_count_++;
    } else {
      // 2 for decrease in price
      discrete_train_data_dependent_.push_back(2);
      neg_count_++;
    }
  }
  std::cout << neg_count_ << " " << zero_count_ << " " << pos_count_ << "\n";
  std::vector<double> mean_orig_indep_(train_data_independents_.size(), 0);
  std::vector<double> stdev_orig_indep_(train_data_independents_.size(), 1);

  VectorUtils::CalcNonZeroMeanStdevVec(train_data_independents_, mean_orig_indep_, stdev_orig_indep_);

  for (unsigned int indep_index_ = 0; indep_index_ < train_data_independents_.size(); indep_index_++) {
    if ((in_included_[indep_index_] == 0) &&
        ((stdev_orig_indep_[indep_index_] <= 0) ||
         (fabs(mean_orig_indep_[indep_index_]) > (kMaxSharpeIndependent * stdev_orig_indep_[indep_index_])))) {
      if (!avoid_high_sharpe_check_vector_[indep_index_])
        in_included_[indep_index_] = -1;  // marking high sharpe indicators as ineligible for selection
      // else
      //   std::cerr << "skipped high_sharpe_indep check for index " << indep_index_ << std::endl;

      double sharpe_ = 0;
      if (stdev_orig_indep_[indep_index_] > 0) {
        sharpe_ = (fabs(mean_orig_indep_[indep_index_]) / stdev_orig_indep_[indep_index_]);
        if (std::isnan(sharpe_)) {
          sharpe_ = 0;
        } else {
          std::cerr << " Sharpe of indep ( " << indep_index_ << " ) is " << sharpe_ << " more than "
                    << kMaxSharpeIndependent << " since fabs(mean) " << fabs(mean_orig_indep_[indep_index_])
                    << " > kMaxSharpeIndependent * stdev = " << stdev_orig_indep_[indep_index_] << std::endl;
        }
      } else {
        in_included_[indep_index_] = -1;  // marking zero stdev indicators as ineligible for selection
      }
    }
  }

  if (NextNotIncluded(in_included_) == kInvalidArrayIndex) {
    std::cerr << "No valid independent left after eliminating high sharpe ones" << std::endl;
    exit(kCallIterativeRegressHighSharpeIndep);
  }
  NormalizeNonZeroDataVec(train_data_independents_, mean_orig_indep_, stdev_orig_indep_, in_included_);

  // making a copy of this to compute model statistics after regression, since the vector passed will be changed
  // But remember that this is the normalized copy i.e x-mu/sigma copy

  std::vector<int> model_indep_indx_(train_data_dependent_.size(), -1);
  std::cout << "before calling logistic regression\n";
  std::vector<double> w_col(3, 0.0);
  model_.push_back(w_col);
  FS_Logistic_Regression(discrete_train_data_dependent_, train_data_independents_, in_included_, max_indep_correlation_,
                         model_, model_indep_indx_, max_model_size_);

  return 0;
}
