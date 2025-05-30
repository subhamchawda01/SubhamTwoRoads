/**
    \file MToolsExe/call_FSLR_nomem.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/

#include "dvccode/Profiler/cpucycle_profiler.hpp"

#include "basetrade/Math/gen_utils.hpp"
#include "basetrade/Math/matrix_utils.hpp"
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
  // following variables are used to compute the covar matrix
  unsigned int count_lines_ = 0u;
  std::vector<double> l1_sum_vec_;
  std::vector<double> l2_sum_vec_;

  HFSAT::SquareMatrix<double> covar_matrix_;

  // int must_add_next_k_indeps_ = 0;

  std::vector<IndexCoeffPair_t> finmodel_;

  std::string infilename_ = "";
  std::string regression_output_filename_ = "";

  double min_correlation_ = 0.01;
  const double kMaxSharpeDependant = 0.20;
  const double kMaxSharpeIndependant = 0.22;
  double max_indep_correlation_ = 0.7;
  unsigned int max_model_size_ = 20;

  // bool first_indep_is_weight_ = false;

  // command line processing
  if (argc < 8) {
    std::cerr << "USAGE: " << argv[0]
              << " input_file_name  min_correlation  first_indep_is_weight  must_include_first_k_independants  "
                 "max_indep_correlation  regression_output_filename  max_model_size" << std::endl;
    exit(0);
  }

  infilename_ = argv[1];
  min_correlation_ = atof(argv[2]);
  // first_indep_is_weight_ = ( atoi ( argv[3] ) != 0 ) ;
  // must_add_next_k_indeps_ = atoi ( argv[4] );
  max_indep_correlation_ = atof(argv[5]);
  regression_output_filename_ = argv[6];
  max_model_size_ = (unsigned int)std::min(30, std::max(1, atoi(argv[7])));

  // read data
  std::ifstream infile_;
  infile_.open(infilename_.c_str());
  if (!infile_.is_open()) {
    std::cerr << " Input data file " << infilename_ << " did not open " << std::endl;
    exit(kCallIterativeRegressInFileOpenError);
  }

  const unsigned int kLineBufferLen = 10240;
  char readline_buffer_[kLineBufferLen];
  bzero(readline_buffer_, kLineBufferLen);
  unsigned int num_independants_ = 0;

  while (infile_.good()) {
    bzero(readline_buffer_, kLineBufferLen);
    infile_.getline(readline_buffer_, kLineBufferLen);
    PerishableStringTokenizer st_(readline_buffer_, kLineBufferLen);
    const std::vector<const char*>& tokens_ = st_.GetTokens();

    // simple file format : "DEPENDANT INDEP_1 INDEP_2 ... INDEP_V"

    if (tokens_.size() <= 1) continue;

    if (num_independants_ == 0) {              // first line
      num_independants_ = tokens_.size() - 1;  // given tokens_.size() > 1
      if (num_independants_ < 1) {
        infile_.close();

        std::cerr << "First line has very few tokens " << tokens_.size() << " should be at least 2 " << std::endl;
        exit(kCallIterativeRegressFewTokens);
      }
      l1_sum_vec_.resize(tokens_.size());
      l2_sum_vec_.resize(tokens_.size());
      covar_matrix_.Init(tokens_.size());
    }

    if ((1 + num_independants_) !=
        (unsigned int)(tokens_.size()))  // make sure this line has the number of words we are expecting
    {
      infile_.close();
      std::cerr << " A line in the input data has a different number of tokens " << tokens_.size() << " than the first "
                << (1 + num_independants_) << std::endl;
      exit(kCallIterativeRegressDiffNumTokensError);
    } else {
      // for lines that are to be included ... ( right now all lines )
      count_lines_++;

      for (register auto i = 0u; i < tokens_.size(); i++) {
        register double i_value_ = atof(tokens_[i]);
        l1_sum_vec_[i] += i_value_;
        l2_sum_vec_[i] += i_value_ * i_value_;
        for (register unsigned int j = i; j < tokens_.size(); j++) {
          register double j_value_ = atof(tokens_[j]);
          covar_matrix_(i, j) += i_value_ * j_value_;
        }
      }
    }
  }

  infile_.close();
  for (register auto i = 0u; i < num_independants_ + 1; i++) {
    for (register unsigned int j = 0; j < num_independants_ + 1; j++) {
      if (i > j) {
        covar_matrix_(i, j) = covar_matrix_(j, i);
      }
    }
  }

  const unsigned int kMinLinesNeededIterativeRegress = 10;
  if (count_lines_ < kMinLinesNeededIterativeRegress) {
    std::cerr << "Less than " << kMinLinesNeededIterativeRegress << " (" << count_lines_
              << ") lines in input data file: " << infilename_ << std::endl;
    exit(kCallIterativeRegressVeryFewLines);
  }

  // finish up calcing mean,stdev,covar_matrix, corr_matrix

  // computing mean and stdev ... to filter on high sharpe
  // normalize data and filter ( mark ineligible for selection )
  double mean_orig_dependant_ = 0;
  double stdev_orig_dependant_ = 1;

  std::vector<double> mean_orig_indep_(num_independants_, 0);
  std::vector<double> stdev_orig_indep_(num_independants_, 1);

  std::vector<int> in_included_(num_independants_, 0);  ///< included [ i ] = 0 if eligible to be selected , 1 if the
  /// indep(i) is already in the model, -1 if it is ineligible to
  /// be selected
  // mark all indices eligible to be selected

  mean_orig_dependant_ = l1_sum_vec_[0] / (double)count_lines_;
  stdev_orig_dependant_ = HFSAT::Math::GetStdev(l1_sum_vec_[0], l2_sum_vec_[0], count_lines_);
  for (auto i = 0u; i < mean_orig_indep_.size(); i++) {
    mean_orig_indep_[i] = l1_sum_vec_[i + 1] / (double)count_lines_;
    stdev_orig_indep_[i] = HFSAT::Math::GetStdev(l1_sum_vec_[i + 1], l2_sum_vec_[i + 1], count_lines_);
  }

  // V(i,j) = ( SXY(i,j) - ( SX(i)*SX(j)/n ) )/(n-1)
  for (auto i = 0u; i < covar_matrix_.row_count(); i++) {
    for (unsigned int j = 0; j < covar_matrix_.row_count(); j++) {
      covar_matrix_(i, j) =
          (covar_matrix_(i, j) - (l1_sum_vec_[i] * l1_sum_vec_[j] / (double)count_lines_)) / (double)(count_lines_ - 1);
    }
  }

  HFSAT::SquareMatrix<double> corr_matrix_ = covar_matrix_;
  HFSAT::MatrixUtils::CovarToCorrMatrix(corr_matrix_);

  if ((stdev_orig_dependant_ <= 0) || (fabs(mean_orig_dependant_) > (kMaxSharpeDependant * stdev_orig_dependant_))) {
    double sharpe_ = (fabs(mean_orig_dependant_) / stdev_orig_dependant_);
    if (std::isnan(sharpe_)) {
      sharpe_ = 0;
    } else {
      // std::cerr << " Sharpe of the dependant is " << sharpe_
      // 	    << " more than " << kMaxSharpeDependant << " since fabs(mean) " << fabs ( mean_orig_dependant_ )
      // 	    << " > kMaxSharpeDependant * stdev = " << stdev_orig_dependant_ << std::endl ;
    }
    // not stopping if dependant has a high sharpe
    // just reporting it, and removing mean like normal
    // exit ( kCallIterativeRegressHighSharpeDependant );
  }

  for (unsigned int indep_index_ = 0; indep_index_ < num_independants_; indep_index_++) {
    if ((in_included_[indep_index_] == 0) &&
        ((stdev_orig_indep_[indep_index_] <= 0) ||  // no variability in this indicator
         (fabs(mean_orig_indep_[indep_index_]) >
          (kMaxSharpeIndependant * stdev_orig_indep_[indep_index_]))))  // high sharpe
    {
      // in_included_ [ indep_index_ ] = -1; // marking constant / high sharpe indicators as ineligible for selection

      double sharpe_ = 0;
      if (stdev_orig_indep_[indep_index_] > 0) {
        sharpe_ = (fabs(mean_orig_indep_[indep_index_]) / stdev_orig_indep_[indep_index_]);
        if (std::isnan(sharpe_)) {
          sharpe_ = 0;
        } else {
          // std::cerr << " Sharpe of indep ( " << indep_index_ << " ) is " << sharpe_
          // 	    << " more than " << kMaxSharpeDependant << " since fabs(mean) " << fabs (
          // mean_orig_indep_[indep_index_] )
          // 	    << " > kMaxSharpeDependant * stdev = " << stdev_orig_indep_[indep_index_] << std::endl ;
        }
      }
    }
  }

  if (NextNotIncluded(in_included_) == kInvalidArrayIndex) {
    std::cerr << "No valid independant left after eliminating high sharpe ones" << std::endl;
    exit(kCallIterativeRegressHighSharpeIndep);
  }

  //  Passing covar_mat same as corr_mat to be consistent with other model
  for (auto i = 0u; i < covar_matrix_.row_count(); i++) {
    for (unsigned int j = 0; j < covar_matrix_.row_count(); j++) {
      covar_matrix_(i, j) = corr_matrix_(i, j);
    }
  }

  HFSAT::FSLR_SO_eff(covar_matrix_, corr_matrix_, in_included_, finmodel_, min_correlation_, max_indep_correlation_,
                     max_model_size_, count_lines_);

  // TODO ... figure out how to compute stats

  // compute final tstat_
  {
    // Previously WithMem version Train_data got beta subtracted say p times where
    // p indices were chosen
    double RSS_ = covar_matrix_(0, 0);  // VectorUtils::CalcL2Norm ( train_data_dependant_ );
    double n_2_ = count_lines_ - 2;     // train_data_dependant_.size() - 2;
    double n_1_ = count_lines_ - 1;     // train_data_dependant_.size() - 1;
    double denom_ = sqrt((RSS_ / n_2_) / n_1_);
    for (auto i = 0u; i < finmodel_.size(); i++) {
      finmodel_[i].tstat_ = finmodel_[i].coeff_ / denom_;
    }
  }

  std::ofstream regression_output_file_(regression_output_filename_.c_str(), std::ofstream::out);
  // ignoring constant for now, since probability of the constant in the dependant to be predictable is low.
  // double would_be_constant_term_ = 0;
  regression_output_file_
      << "OutConst " << 0 << " " << 0
      << std::endl;  // assuming that the best we can do is assume everything is mean 0 in unseen data
  for (auto i = 0u; i < finmodel_.size(); i++) {
    double stdev_readjusted_coeff_ =
        (finmodel_[i].coeff_ * stdev_orig_dependant_) / (stdev_orig_indep_[finmodel_[i].origindex_]);
    regression_output_file_ << "OutCoeff " << finmodel_[i].origindex_ << ' ' << stdev_readjusted_coeff_
                            // << " InitCorrelation " << initial_correlations_ [ finmodel_[i].origindex_
                            // ]
                            << " RSS " << finmodel_[i].RSS_ << " SXX " << finmodel_[i].SXX_ << " Tstat "
                            << finmodel_[i].tstat_ << " TstatSelect " << finmodel_[i].tstat_residual_dependant_
                            << std::endl;
  }

  double model_rsquared_ = 0;
  double model_correlation_ = 0;
  double stdev_final_dependant_ = 0;

  // Origin normalized_l2_sum
  double dep_l2_norm = (l2_sum_vec_[0] - (l1_sum_vec_[0] * l1_sum_vec_[0] / count_lines_)) /
                       (stdev_orig_dependant_ * stdev_orig_dependant_);
  // FInal normalized norm
  double ff_l2_norm = covar_matrix_(0, 0) * (count_lines_ - 1);
  regression_output_file_ << "INIT: " << dep_l2_norm << " : " << ff_l2_norm << std::endl;
  model_rsquared_ = (dep_l2_norm - ff_l2_norm) / dep_l2_norm;
  model_correlation_ = sqrt(std::max(0.0, model_rsquared_));
  stdev_final_dependant_ = sqrt(std::max(0.0, covar_matrix_(0, 0)));
  regression_output_file_ << "RSquared " << model_rsquared_ << std::endl;
  double adjusted_model_rsquared_ =
      1 - ((1 - model_rsquared_) * ((count_lines_ - 1) / (count_lines_ - finmodel_.size() - 1)));
  regression_output_file_ << "Adjustedrsquared " << adjusted_model_rsquared_ << std::endl;
  regression_output_file_ << "Correlation " << model_correlation_ << std::endl;
  regression_output_file_ << "StdevDependant " << stdev_orig_dependant_ << std::endl;
  regression_output_file_ << "StdevFINALDependant " << stdev_final_dependant_ << std::endl;
  regression_output_file_ << "StdevResidual " << (stdev_orig_dependant_ * stdev_final_dependant_) << std::endl;

  regression_output_file_.close();

  return 0;
}
