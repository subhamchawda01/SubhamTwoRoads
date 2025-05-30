/**
    \file MToolsExe/call_nultiple_fslr.cpp

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
#include "basetrade/MToolsExe/mtools_utils.hpp"
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
  std::vector<std::vector<double> > train_data_dependants_;
  std::vector<std::vector<double> > train_data_independants_;

  unsigned int must_add_next_k_indeps_ = 0u;

  std::vector<IndexCoeffPair_t> finmodel_;

  std::string infilename_ = "";
  std::string regression_output_filename_ = "";

  unsigned int num_deps_ = 4u;
  double min_correlation_ = 0.01;
  // const double kMaxSharpeDependant = 0.20 ;
  const double kMaxSharpeIndependant = 0.15;
  double max_indep_correlation_ = 0.75;
  unsigned int max_model_size_ = 30u;
  double target_stdev_model_ = 0;
  double tstat_cutoff = 5.00;  // TODO take this as an argument
  // double pred_duration_ = 32; //default value
  // double datagen_timeout_ = 3000;

  std::string avoid_high_sharpe_check_vector_filename_ = "";

  bool first_indep_is_weight_ = false;

  // command line processing
  if (argc < 9) {
    std::cerr << "USAGE: " << argv[0]
              << " input_reg_data_file_name  num_deps_  min_correlation  must_include_first_k_independants  "
                 "max_indep_correlation  regression_output_filename  max_model_size  avoid_sharpe_check_filename  "
                 "[indicator_correlations_]" << std::endl;
    exit(0);
  }

  infilename_ = argv[1];
  num_deps_ = std::max(1u, std::min(5u, (unsigned int)atoi(argv[2])));
  min_correlation_ = atof(argv[3]);
  must_add_next_k_indeps_ = (unsigned int)std::max(0, atoi(argv[4]));
  max_indep_correlation_ = atof(argv[5]);
  regression_output_filename_ = argv[6];
  max_model_size_ = std::min(30u, std::max(1u, (unsigned int)atoi(argv[7])));
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

  train_data_dependants_.resize(num_deps_);
  // read data
  ReadMultDepData(infilename_, num_deps_, train_data_dependants_, train_data_independants_);

  // {
  //   std::vector< std::string > tokens_;
  //   {
  //     std::istringstream iss(infilename_);
  //     std::string token;
  //     while ( getline(iss,token,'_') )
  // 	{
  // 	  tokens_.push_back(token.c_str());
  // 	}
  //   }

  //   for ( unsigned int i=0; i< tokens_.size(); i++  )
  //     {
  // 	if(!tokens_[i].compare("na") || !tokens_[i].compare("ac"))
  // 	  {
  // if ( ( ( i - 1 ) >= 0 ) && ( ( i - 1 ) < tokens_.size ( ) ) ) {
  //   pred_duration_ = atof(tokens_[i-1].c_str());
  // }
  // if ( ( ( i + 8 ) >= 0 ) && ( ( i + 8 ) < tokens_.size ( ) ) ) {
  //   datagen_timeout_ = atof(tokens_[i+8].c_str());
  // }
  // 	  }
  //     }
  // }
  // double window_size_ =  ( pred_duration_ * 1000 ) / datagen_timeout_;
  std::vector<int> in_included_(train_data_independants_.size(), 0);  ///< included [ i ] = 0 if eligible to be selected
                                                                      ///, 1 if the indep(i) is already in the model, -1
  /// if it is ineligible to be selected
  // mark all indices eligible to be selected

  std::vector<int> avoid_high_sharpe_check_vector_(train_data_independants_.size(), 0);
  if (avoid_high_sharpe_check_vector_filename_.length() > 0) {
    HFSAT::MTOOLS_UTILS::LoadAvoidCheckfile(avoid_high_sharpe_check_vector_filename_, avoid_high_sharpe_check_vector_);
  }

  if (first_indep_is_weight_) {
    in_included_[0] = -3;  // mark this index as ineligible to be selected as a chosen independant in the model
    for (unsigned int dep_index_ = 0u; dep_index_ < train_data_dependants_.size(); dep_index_++) {
      for (auto i = 0u; i < train_data_dependants_[dep_index_].size(); i++) {
        train_data_dependants_[dep_index_][i] *= train_data_independants_[0][i];
      }
    }
    for (unsigned int indep_index_ = 1u; indep_index_ < train_data_independants_.size(); indep_index_++) {
      for (auto i = 0u; i < train_data_independants_[indep_index_].size(); i++) {
        train_data_independants_[indep_index_][i] *= train_data_independants_[0][i];
      }
    }
  }

  // computing mean and stdev ... to filter on high sharpe
  // normalize data and filter ( mark ineligible for selection )
  std::vector<double> mean_orig_dependant_vec_(train_data_dependants_.size(), 0.0);
  std::vector<double> stdev_orig_dependant_vec_(train_data_dependants_.size(), 1.0);

  VectorUtils::CalcMeanStdevVec(train_data_dependants_, mean_orig_dependant_vec_, stdev_orig_dependant_vec_);
  NormalizeDataVec(train_data_dependants_, mean_orig_dependant_vec_, stdev_orig_dependant_vec_);

  if (target_stdev_model_ <= 0.00000001) {  // finally the model has to be caliberated to have a standard deviation
    target_stdev_model_ = VectorUtils::GetMean(stdev_orig_dependant_vec_);
  }

  std::vector<double> mean_orig_indep_(train_data_independants_.size(), 0.0);
  std::vector<double> stdev_orig_indep_(train_data_independants_.size(), 1.0);

  VectorUtils::CalcNonZeroMeanStdevVec(train_data_independants_, mean_orig_indep_, stdev_orig_indep_);

  for (unsigned int indep_index_ = 0; indep_index_ < train_data_independants_.size(); indep_index_++) {
    if ((in_included_[indep_index_] == 0) &&
        ((stdev_orig_indep_[indep_index_] <= 0) ||
         (fabs(mean_orig_indep_[indep_index_]) > (kMaxSharpeIndependant * stdev_orig_indep_[indep_index_])))) {
      if (!avoid_high_sharpe_check_vector_[indep_index_]) {
        in_included_[indep_index_] = -3;  // marking high sharpe indicators as ineligible for selection
      }

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
        in_included_[indep_index_] = -3;  // marking zero stdev indicators as ineligible for selection
      }
    }
  }

  NormalizeNonZeroDataVec(train_data_independants_, mean_orig_indep_, stdev_orig_indep_, in_included_);

  if (NextNotIncluded(in_included_) == kInvalidArrayIndex) {
    std::cerr << "No valid independant left after eliminating high sharpe ones" << std::endl;
    exit(kCallIterativeRegressHighSharpeIndep);
  }

  // computing initial correlation to filter on min correlation
  std::vector<double> initial_correlations_(train_data_independants_.size(),
                                            0);  // sum_j ( x_i_j * y_j ) = corr ( x_i, y ) // since all data normalized
  for (unsigned int dep_index_ = 0u; dep_index_ < train_data_dependants_.size(); dep_index_++) {
    std::vector<double> this_dep_correlations_(train_data_independants_.size(), 0.0);
    ComputeCorrelationsOfNormalizedDataVec(train_data_dependants_[dep_index_], train_data_independants_,
                                           this_dep_correlations_, in_included_);
    VectorUtils::ScaledVectorAddition(initial_correlations_, this_dep_correlations_,
                                      1.0 / (double)train_data_dependants_.size());
  }

  std::vector<double> average_train_data_dependant_(train_data_dependants_[0].size(), 0.0);
  for (unsigned int dep_index_ = 0u; dep_index_ < train_data_dependants_.size(); dep_index_++) {
    VectorUtils::ScaledVectorAddition(average_train_data_dependant_, train_data_dependants_[dep_index_],
                                      1.0 / (double)train_data_dependants_.size());
  }
  // making a copy of this to compute model statistics after regression, since the vector passed will be changed
  // But remember that this is the normalized copy i.e x-mu/sigma copy
  std::vector<double> train_data_orig_dependant_ = average_train_data_dependant_;

  // we have to send a yhat to regression to compute it incrementally
  // and cannot calculate it later since
  // we change the indicators while computing coefficients
  std::vector<double> predicted_values_(train_data_dependants_[0].size(), 0.0);

  // square matrix of num_indiccators with 0.0
  std::vector<std::vector<double> > independant_orthogonalization_matrix_(train_data_independants_.size());
  for (unsigned int row_idx_ = 0u; row_idx_ < independant_orthogonalization_matrix_.size(); row_idx_++) {
    for (unsigned int col_idx_ = 0u; col_idx_ < train_data_independants_.size(); col_idx_++) {
      independant_orthogonalization_matrix_[row_idx_].push_back(0.0);
    }
  }
  std::vector<std::vector<double> > train_data_independants_original_ = train_data_independants_;

  std::map<unsigned int, unsigned int> round_index_added_in_;

  for (unsigned int dep_index_ = 0u; dep_index_ < train_data_dependants_.size(); dep_index_++) {
    VectorUtils::ScaledVectorAddition(train_data_dependants_[dep_index_], predicted_values_,
                                      -1.00);  // remove the yhat computed so far

    // in case the independant was marked ineligible for low correlation ... reallowing it in the next round
    for (unsigned int indep_index_ = 0u; indep_index_ < in_included_.size(); indep_index_++) {
      if (in_included_[indep_index_] == -1) {
        in_included_[indep_index_] = 0;
      }
    }

    HFSAT::MTOOLS_UTILS::filterIndicatorsBasedOnHistoricalCorrelations(
        train_data_independants_.size(), initial_correlations_, min_correlation_, historical_indicator_correlations_,
        in_included_, false);

    if (check_semantic_correlations_) {
      HFSAT::MTOOLS_UTILS::filterIndicatorsBasedOnSemantics(train_data_independants_.size(), initial_correlations_,
                                                            semantic_indicator_correlations_, in_included_, true);
    }

    unsigned int current_finmodel_size_ = finmodel_.size();

    // note that since in the following step we are chaning the indicators
    // in future calls they will not be stdev 1
    FSLR_SO_NoMeanData(train_data_dependants_[dep_index_], train_data_independants_, must_add_next_k_indeps_,
                       in_included_, finmodel_, min_correlation_, max_indep_correlation_, initial_correlations_,
                       max_model_size_, predicted_values_, independant_orthogonalization_matrix_,
                       train_data_independants_original_, tstat_cutoff);

    // adjust previously computed coefficients
    // need to verify logic
    if ((current_finmodel_size_ >= 1) &&
        (finmodel_.size() >
         current_finmodel_size_)) {  // if previously the finmodel was nonempty and new indicators added in this round
      std::vector<double> additions_in_this_round_(train_data_independants_.size(), 0.0);
      for (unsigned int fm_idx_this_round_ = current_finmodel_size_; fm_idx_this_round_ < finmodel_.size();
           fm_idx_this_round_++) {
        round_index_added_in_[finmodel_[fm_idx_this_round_].origindex_] = dep_index_;
        additions_in_this_round_[finmodel_[fm_idx_this_round_].origindex_] += finmodel_[fm_idx_this_round_].coeff_;
      }

      for (unsigned int fm_idx_to_normalize_ = current_finmodel_size_ - 1;
           (fm_idx_to_normalize_ < current_finmodel_size_) && (fm_idx_to_normalize_ >= 0);
           fm_idx_to_normalize_--) {  // for each indicator added to the model before this round
        // in reverse order

        unsigned int index_in_independant_array_ = finmodel_[fm_idx_to_normalize_].origindex_;
        for (unsigned int this_indep_index_ = 0; this_indep_index_ < train_data_independants_.size();
             this_indep_index_++) {
          additions_in_this_round_[index_in_independant_array_] +=
              -independant_orthogonalization_matrix_[index_in_independant_array_][this_indep_index_] *
              additions_in_this_round_[this_indep_index_];
        }
        finmodel_[fm_idx_to_normalize_].coeff_ += additions_in_this_round_[index_in_independant_array_];
      }
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
        (finmodel_[i].coeff_ * target_stdev_model_) / (stdev_orig_indep_[finmodel_[i].origindex_]);
    regression_output_file_ << "OutCoeff " << finmodel_[i].origindex_ << ' ' << stdev_readjusted_coeff_
                            << " InitCorrelation " << initial_correlations_[finmodel_[i].origindex_] << " Tstat "
                            << finmodel_[i].tstat_ << " TstatSelect " << finmodel_[i].tstat_residual_dependant_
                            << " DepIndex " << round_index_added_in_[finmodel_[i].origindex_] << std::endl;
  }

  double model_rsquared_ = 0;
  double model_correlation_ = 0;
  double stdev_final_dependant_ = 0;
  double stdev_model_ = 0;

  VectorUtils::ScaledVectorAddition(average_train_data_dependant_, predicted_values_, -1.00);
  ComputeModelStatisticsOrigNormalized(train_data_orig_dependant_, average_train_data_dependant_, model_rsquared_,
                                       model_correlation_, stdev_final_dependant_, stdev_model_);

  regression_output_file_ << "RSquared " << model_rsquared_ << std::endl;
  // double adjusted_model_rsquared_ = 1 - ( ( 1 - model_rsquared_ ) * ( ( train_data_dependant_.size ( ) - 1 ) / (
  // train_data_dependant_.size ( ) - finmodel_.size() - 1 ) ) ) ;
  // regression_output_file_ << "Adjustedrsquared " << adjusted_model_rsquared_ << std::endl ;
  regression_output_file_ << "Correlation " << model_correlation_ << std::endl;
  regression_output_file_ << "StdevDependant " << target_stdev_model_ << std::endl;
  // regression_output_file_ << "StdevFINALDependant " << stdev_final_dependant_ << std::endl ;
  regression_output_file_ << "StdevResidual " << (target_stdev_model_ * stdev_final_dependant_) << std::endl;
  regression_output_file_ << "StdevModel " << (target_stdev_model_ * stdev_model_) << std::endl;
  // regression_output_file_ << "ErrorAutoCorrelation " << error_auto_corr_ << std::endl;
  // regression_output_file_ << "PredictionAutoCorrelation "<< pred_auto_corr_ << std::endl;

  regression_output_file_.close();

  return 0;
}
