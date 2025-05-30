/**
    \file MToolsExe/call_FSHDVLR.cpp

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

/// Exec used to call Forward Stagewise High-value Linear Regression
/// Reads command line arguments
/// Compute mean & stdev of dependant
/// Compute mean & stdev of all independants
/// Normalize dependant
/// Normalize independants
/// Compute initial correlations and mark the columns ineligible that do not make the min_correlation_ cut
/// make a copy of dependant column ( to compute stats later on )
/// call FSHDVLR ( on data that is mean zero to save needless mean calcing operations )
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
  unsigned int max_model_size_ = 16u;
  double target_stdev_model_ = 0;
  double pred_duration_ = 32;  // default value
  double datagen_timeout_ = 3000;
  double tstat_thresh_ = 4.0;
  std::string avoid_high_sharpe_check_vector_filename_ = "";

  bool first_indep_is_weight_ = false;

  // command line processing
  if (argc < 8) {
    std::cerr << "USAGE: " << argv[0]
              << " input_reg_data_file_name  min_correlation  first_indep_is_weight  mult_include_first_k_independants "
                 " max_indep_correlation regression_output_filename  max_model_size avoid_sharpe_check_filename [ "
                 "historical_indicator_correlations_ ]" << std::endl;
    exit(0);
  }

  infilename_ = argv[1];
  min_correlation_ = atof(argv[2]);
  first_indep_is_weight_ = (atoi(argv[3]) != 0);
  must_add_next_k_indeps_ = atoi(argv[4]);
  max_indep_correlation_ = atof(argv[5]);
  //  tstat_thresh_ =atof ( argv[6] );
  regression_output_filename_ = argv[6];
  max_model_size_ = (unsigned int)std::min(30, std::max(1, atoi(argv[7])));
  if (argc >= 9) {
    avoid_high_sharpe_check_vector_filename_ = argv[8];
  }

  std::vector<double>
      historical_indicator_correlations_;  //-1 for negative, 0 for no info or actually ZERO, 1 for positive
  std::vector<int> semantic_indicator_correlations_;
  if (argc >= 10) {
    HFSAT::MTOOLS_UTILS::loadIndicatorHistoricalCorrelations(argv[9], historical_indicator_correlations_);
    HFSAT::MTOOLS_UTILS::loadIndicatorSemantics(argv[9], semantic_indicator_correlations_);
  }

  bool check_semantic_correlations_ = true;
  if (argc >= 11 && *argv[10] == 'N') {
    std::cerr << "muting semantic correlations\n";
    check_semantic_correlations_ = false;
  }

  bool ignore_zeros_ = false;
  if (argc >= 12 && *argv[11] == 'Y') {
    std::cerr << "Ignoring zeros for selecting indicators\n";
    ignore_zeros_ = true;
  }

  // read data
  ReadData(infilename_, train_data_dependant_, train_data_independants_);

  {
    std::vector<std::string> tokens_;
    std::istringstream iss(infilename_);
    std::string token;
    while (getline(iss, token, '_')) {
      tokens_.push_back(token.c_str());
    }

    for (auto i = 0u; i < tokens_.size(); i++) {
      if (!tokens_[i].compare("na") || !tokens_[i].compare("ac")) {
        if (((i - 1) >= 0) && ((i - 1) < tokens_.size())) {
          pred_duration_ = atof(tokens_[i - 1].c_str());
        }
        if (((i + 8) >= 0) && ((i + 8) < tokens_.size())) {
          datagen_timeout_ = atof(tokens_[i + 8].c_str());
        }
      }
    }
  }
  double window_size_ = (pred_duration_ * 1000) / datagen_timeout_;

  std::vector<int> in_included_(train_data_independants_.size(), 0);  ///< included [ i ] = 0 if eligible to be selected
                                                                      ///, 1 if the indep(i) is already in the model, -1
  /// if it is ineligible to be selected
  // mark all indices eligible to be selected

  std::vector<int> avoid_high_sharpe_check_vector_(train_data_independants_.size(), 0);
  HFSAT::MTOOLS_UTILS::LoadAvoidCheckfile(avoid_high_sharpe_check_vector_filename_, avoid_high_sharpe_check_vector_);

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

  // removing indicators from consideration whose initial correlation ois also lower thant the min_correlation_ required
  // to add an idnicator
  HFSAT::MTOOLS_UTILS::filterIndicatorsBasedOnHistoricalCorrelations(
      train_data_independants_.size(), initial_correlations_, min_correlation_, historical_indicator_correlations_,
      in_included_, false);

  if (check_semantic_correlations_) {
    HFSAT::MTOOLS_UTILS::filterIndicatorsBasedOnSemantics(train_data_independants_.size(), initial_correlations_,
                                                          semantic_indicator_correlations_, in_included_, true);
  }

  // making a copy of this to compute model statistics after regression, since the vector passed will be changed
  // But remember that this is the normalized copy i.e x-mu/sigma copy
  std::vector<double> train_data_orig_dependant_ = train_data_dependant_;
  std::vector<std::vector<double> > train_data_independants_original_ = train_data_independants_;

  FSHDVLR_SO_NoMeanData(train_data_dependant_, train_data_independants_, must_add_next_k_indeps_, in_included_,
                        finmodel_, min_correlation_, max_indep_correlation_, initial_correlations_, max_model_size_,
                        1.0, train_data_independants_original_, tstat_thresh_, ignore_zeros_);

  // compute final tstat_
  {
    double RSS_ = VectorUtils::CalcL2Norm(train_data_dependant_);
    double n_2_ = train_data_dependant_.size() - 2;
    double n_1_ = train_data_dependant_.size() - 1;
    double denom_ = sqrt((RSS_ / n_2_) / n_1_);
    for (auto i = 0u; i < finmodel_.size(); i++) {
      finmodel_[i].tstat_ = finmodel_[i].coeff_ / denom_;
    }
  }

  std::ofstream regression_output_file_(regression_output_filename_.c_str(), std::ofstream::out);
  // ignoring constant for now, since probability of the constant in the dependant to be predictable is low.
  // double would_be_constant_term_ = 0;

  // computing autocorrelation for indeps
  std::vector<double> indep_auto_corrs_(finmodel_.size(), 0.0);
  for (auto i = 0u; i < finmodel_.size(); i++) {
    indep_auto_corrs_[i] = CalcAutoCorrelation(train_data_independants_[finmodel_[i].origindex_], window_size_);
  }

  regression_output_file_
      << "OutConst " << 0 << " " << 0
      << std::endl;  // assuming that the best we can do is assume everything is mean 0 in unseen data
  for (auto i = 0u; i < finmodel_.size(); i++) {
    double stdev_readjusted_coeff_ =
        (finmodel_[i].coeff_ * target_stdev_model_) / (stdev_orig_indep_[finmodel_[i].origindex_]);
    regression_output_file_ << "OutCoeff " << finmodel_[i].origindex_ << ' ' << stdev_readjusted_coeff_
                            << " InitCorrelation " << initial_correlations_[finmodel_[i].origindex_] << " Tstat "
                            << finmodel_[i].tstat_ << " AutoCorrelation " << indep_auto_corrs_[i] << " TstatSelect "
                            << finmodel_[i].tstat_residual_dependant_ << std::endl;
  }

  double model_rsquared_ = 0;
  double model_correlation_ = 0;
  double stdev_final_dependant_ = 0;
  double stdev_model_ = 0;

  double error_auto_corr_ = CalcAutoCorrelation(train_data_dependant_, window_size_);
  std::vector<double> predictions_(train_data_dependant_.size(), 0.0);

  for (auto i = 0u; i < predictions_.size(); i++) {
    predictions_[i] = train_data_orig_dependant_[i] - train_data_dependant_[i];
  }
  double pred_auto_corr_ = CalcAutoCorrelation(predictions_, window_size_);

  ComputeModelStatisticsOrigNormalized(train_data_orig_dependant_, train_data_dependant_, model_rsquared_,
                                       model_correlation_, stdev_final_dependant_, stdev_model_);

  regression_output_file_ << "RSquared " << model_rsquared_ << std::endl;
  double adjusted_model_rsquared_ =
      1 - ((1 - model_rsquared_) *
           ((train_data_dependant_.size() - 1) / (train_data_dependant_.size() - finmodel_.size() - 1)));
  regression_output_file_ << "Adjustedrsquared " << adjusted_model_rsquared_ << std::endl;
  regression_output_file_ << "Correlation " << model_correlation_ << std::endl;
  regression_output_file_ << "HDVCorr " << GetHDVCorrelationNoMean(train_data_orig_dependant_, train_data_dependant_,
                                                                   -1.0) << std::endl;
  regression_output_file_ << "StdevDependant " << target_stdev_model_ << std::endl;
  // regression_output_file_ << "StdevFINALDependant " << stdev_final_dependant_ << std::endl ;
  regression_output_file_ << "StdevResidual " << (target_stdev_model_ * stdev_final_dependant_) << std::endl;
  regression_output_file_ << "StdevModel " << (target_stdev_model_ * stdev_model_) << std::endl;
  regression_output_file_ << "ErrorAutoCorrelation " << error_auto_corr_ << std::endl;
  regression_output_file_ << "PredictionAutoCorrelation " << pred_auto_corr_ << std::endl;

  regression_output_file_.close();

  return 0;
}
