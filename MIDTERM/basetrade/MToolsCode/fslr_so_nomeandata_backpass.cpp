/**
   \file MToolsCode/fslr_so_nomeandata_backpass.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "basetrade/MTools/data_processing.hpp"
#include "basetrade/MTools/iterative_regress.hpp"

namespace HFSAT {

// Can be optimized, tradeoff b/w space and time. Save multiple copies of training data to avoid running regression
// again.
void Backward_Pass(const std::vector<double>& train_data_orig_dependant_,
                   const std::vector<std::vector<double> >& train_data_orig_independants_,
                   std::vector<IndexCoeffPair_t>& fwd_finmodel_, const double kMinCorrelationIncrement,
                   const double kMaxIndepCorrelation, const std::vector<double>& initial_correlations_,
                   const unsigned int max_model_size_, double tstat_cutoff_, bool ignore_zeros_) {
  if (fwd_finmodel_.size() < 2) {
    return;
  }
  std::vector<double> GCV;  // Generalized cross validation score, lower score is better
  double min_GCV = -1.0;
  double min_GCV_index = -1;
  // double N_inv = 1.0/N;
  // double GCV = ( RSS_ ) / ( (double)(N) * ( 1.0 - param_no*N_inv ) * ( 1.0 - param_no*N_inv ) ) ;
  // denominator is common, so use only numerator

  for (auto i = 0u; i < fwd_finmodel_.size(); i++) {
    std::vector<int> temp_in_included_(train_data_orig_independants_.size(), -1);
    for (unsigned int j = 0; j < fwd_finmodel_.size(); j++)
      temp_in_included_[fwd_finmodel_[j].origindex_] =
          0;  // mark all indicators ineligible for selection except the ones that were selected by fwd pass
    temp_in_included_[fwd_finmodel_[i].origindex_] = -1;                     // remove one indicator from fwd_pass_model
    std::vector<double> train_data_dependant_ = train_data_orig_dependant_;  // create copy which will be modified
    std::vector<std::vector<double> > train_data_independants_ = train_data_orig_independants_;
    std::vector<IndexCoeffPair_t> temp_finmodel_;

    // the following two blocks of code are only for being to call the function

    // we have to send a yhat to regression to compute it incrementally
    // and cannot calculate it later since
    // we change the indicators while computing coefficients
    std::vector<double> predicted_values_(train_data_independants_[0].size(), 0.0);

    // square matrix of num_indiccators with 0.0
    std::vector<std::vector<double> > independant_orthogonalization_matrix_(train_data_independants_.size());
    for (unsigned int row_idx_ = 0u; row_idx_ < independant_orthogonalization_matrix_.size(); row_idx_++) {
      for (unsigned int col_idx_ = 0u; col_idx_ < train_data_independants_.size(); col_idx_++) {
        independant_orthogonalization_matrix_[row_idx_].push_back(0.0);
      }
    }
    std::vector<std::vector<double> > train_data_independants_original_ =
        train_data_independants_;  // make a copy //TODO clean varnames
    FSLR_SO_NoMeanData(train_data_dependant_, train_data_independants_, 0, temp_in_included_, temp_finmodel_,
                       kMinCorrelationIncrement, kMaxIndepCorrelation, initial_correlations_, max_model_size_,
                       predicted_values_, independant_orthogonalization_matrix_, train_data_independants_original_,
                       tstat_cutoff_, ignore_zeros_);

    double temp_GCV = temp_finmodel_[temp_finmodel_.size() - 1].RSS_;
    GCV.push_back(temp_GCV);
    if ((min_GCV < 0) || (min_GCV > temp_GCV)) {
      min_GCV = temp_GCV;
      min_GCV_index = i;
    }
  }
  std::cerr << "minGCV: " << min_GCV << "@" << min_GCV_index << "\t"
            << "fwdModel_GCV" << fwd_finmodel_[fwd_finmodel_.size() - 1].RSS_ << "\n";
  if (min_GCV < fwd_finmodel_[fwd_finmodel_.size() - 1].RSS_) {
    std::cerr << "Removing indicator " << fwd_finmodel_[min_GCV_index].origindex_ << "\n";
    fwd_finmodel_.erase(fwd_finmodel_.begin() + min_GCV_index);
    Backward_Pass(train_data_orig_dependant_, train_data_orig_independants_, fwd_finmodel_, kMinCorrelationIncrement,
                  kMaxIndepCorrelation, initial_correlations_, max_model_size_, tstat_cutoff_, ignore_zeros_);
  }
}

// this function performs forward stagewise linear regression of nomean data by successive orthogonalization of
// independants
void FSLR_SO_NoMeanData_Backpass(const std::vector<double>& train_data_orig_dependant_,
                                 const std::vector<std::vector<double> >& train_data_orig_independants_,
                                 std::vector<double>& train_data_dependant_,
                                 std::vector<std::vector<double> >& train_data_independants_,
                                 int must_add_next_k_indeps_, std::vector<int>& included_,
                                 std::vector<IndexCoeffPair_t>& finmodel_, const double kMinCorrelationIncrement,
                                 const double kMaxIndepCorrelation, const std::vector<double>& initial_correlations_,
                                 const unsigned int max_model_size_, double tstat_cutoff_, bool ignore_zeros_) {
  if (finmodel_.size() >= max_model_size_) return;

  if (NextNotIncluded(included_) != kInvalidArrayIndex) {  // there are still indicators eligible to be selected
    // precomputing variables for faster computation of loss reduction and correlation

    std::vector<double> sum_square_independants_(train_data_independants_.size(),
                                                 -1);  ///< ith value = Sum_j ( x_i_j * x_i_j )
    std::vector<double> sum_dep_independants_(train_data_independants_.size(),
                                              -1);  ///< ith value = Sum_j ( x_i_j * y_j )

    double sum_square_dependant_ = VectorUtils::CalcL2Norm(train_data_dependant_);
    if (sum_square_dependant_ <= 0) return;

    int chosen_index_ = kInvalidArrayIndex;
    double sum_dep_independants_chosen_index_ = 0;     ///< Sum_j ( x_ci_j * y_j )
    double sum_square_independants_chosen_index_ = 0;  ///< Sum_j ( x_ci_j * x_ci_j )

    if (must_add_next_k_indeps_ > 0) {
      chosen_index_ = NextNotIncluded(
          included_);  // should this be done before some indices are marked -1 ... or not mark the first k indices -1 ?
      must_add_next_k_indeps_--;

      // compute variables needed later in orthogonalization
      sum_dep_independants_[chosen_index_] =
          VectorUtils::CalcDotProduct(train_data_dependant_, train_data_independants_[chosen_index_]);
      sum_square_independants_[chosen_index_] = VectorUtils::CalcL2Norm(train_data_independants_[chosen_index_]);
      sum_dep_independants_chosen_index_ = sum_dep_independants_[chosen_index_];
      sum_square_independants_chosen_index_ = sum_square_independants_[chosen_index_];
    } else {
      CalcNoMeanSums(train_data_dependant_, train_data_independants_, included_, sum_square_independants_,
                     sum_dep_independants_);

      // choosing index which minimizes squared error most ... equivalent to having maximum fabs ( correlation )
      double max_variance_reduced_ = 0;
      double max_fabs_correlation_ = 0;
      for (unsigned int indep_index_ = 0; indep_index_ < train_data_independants_.size(); indep_index_++) {
        if (included_[indep_index_] == 0) {  // still eligible to be selected
          double this_correlation_ = sum_dep_independants_[indep_index_] /
                                     (sqrt(sum_square_dependant_) * sqrt(sum_square_independants_[indep_index_]));

          if (this_correlation_ * initial_correlations_[indep_index_] <
              0) {  // the sign of correlation has changed from initial correlation
            included_[indep_index_] = -1;
            continue;
          }

          if (fabs(this_correlation_) <= kMinCorrelationIncrement) {  // pruning independants that drop below
                                                                      // kMinCorrelationIncrement threshold along the
                                                                      // way
            included_[indep_index_] = -1;
            continue;
          }

          if (ignore_zeros_ == true) {
            if ((chosen_index_ == kInvalidArrayIndex) || (fabs(this_correlation_) > max_fabs_correlation_)) {
              chosen_index_ = indep_index_;
              max_fabs_correlation_ = fabs(this_correlation_);
            }
          } else {
            double this_dep_loss_reduced_ =
                GetSquareOf(sum_dep_independants_[indep_index_]) /
                sum_square_independants_[indep_index_];  ///< ( Sum_j ( x_i_j * y_j ) )^2 / Sum_j ( (x_i_j)^2 )
            if ((chosen_index_ == kInvalidArrayIndex) ||
                (this_dep_loss_reduced_ > max_variance_reduced_)) {  // store index and value of current best option
              chosen_index_ = indep_index_;
              max_variance_reduced_ = this_dep_loss_reduced_;
            }
          }
        }
      }

      if (chosen_index_ == kInvalidArrayIndex) {
        return;
      }

      sum_dep_independants_chosen_index_ = sum_dep_independants_[chosen_index_];
      sum_square_independants_chosen_index_ = sum_square_independants_[chosen_index_];
    }

    // checking eligibility
    if ((chosen_index_ == kInvalidArrayIndex) || (included_[chosen_index_] != 0)) {
      return;
    }

    included_[chosen_index_] = 1;  // mark as selected

    // std::cout << " This guy got chosen.."<< chosen_index_ << std::endl;

    // SLR coeff for nomean data = Sum_j ( x_ci_j * y_j ) / Sum_j ( x_ci_j * x_ci_j )
    double dep_chosen_coeff_ = sum_dep_independants_chosen_index_ / sum_square_independants_chosen_index_;

    // Remove effect of chosen_index_ from DEP .
    // Doing this before pushing to finmodel_ to compute RSS_
    // which requires the dependant after including this indicator .
    VectorUtils::ScaledVectorAddition(train_data_dependant_, train_data_independants_[chosen_index_],
                                      -dep_chosen_coeff_);
    // after this step stdev ( train_data_dependant_ ) != 1
    // hence the data is not normalized but just nomean

    IndexCoeffPair_t icp_t_;
    icp_t_.origindex_ = chosen_index_;
    icp_t_.coeff_ = dep_chosen_coeff_;

    double RSS_ = VectorUtils::CalcL2Norm(train_data_dependant_);  // this assumes ScaledVectorAddition above
    double n_2_ = train_data_dependant_.size() - 2;
    double n_1_ = train_data_dependant_.size() - 1;
    double SXX_ = VectorUtils::CalcL2Norm(train_data_independants_[chosen_index_]);
    icp_t_.tstat_residual_dependant_ = dep_chosen_coeff_ / sqrt((RSS_ / n_2_) / SXX_);
    icp_t_.tstat_ = dep_chosen_coeff_ / sqrt((RSS_ / n_2_) / n_1_);  // yet to be computed accurately but this should be
                                                                     // an okay estimate, since SXX is n_1_ since data
                                                                     // was normalized. The major change will come from
                                                                     // computation of dep_chosen_coeff_ finally

    icp_t_.RSS_ = RSS_;
    icp_t_.SXX_ = SXX_;

    // std::cout << " TSTAT: "<< icp_t_.tstat_ << std::endl;
    // std::cout << " INDX: "<< icp_t_.origindex_ << std::endl;
    // std::cout << " COEFF: "<< icp_t_.coeff_ << std::endl;
    // std::cout << " RSS: "<< icp_t_.RSS_ << std::endl;
    // std::cout << " SXX_: "<< icp_t_.SXX_ << std::endl;

    unsigned int this_finmodel_index_ = finmodel_.size();
    finmodel_.push_back(icp_t_);

    Backward_Pass(train_data_orig_dependant_, train_data_orig_independants_, finmodel_, kMinCorrelationIncrement,
                  kMaxIndepCorrelation, initial_correlations_, max_model_size_, tstat_cutoff_,
                  ignore_zeros_);  // Try removing some variables from the model created thus far.

    // Recursive Step
    if (NextNotIncluded(included_) != kInvalidArrayIndex) {  // there are still indicators eligible to be selected

      // remove effect of chosen_index_ from INDEPs not yet included
      // and store the std::vector of betas to calc the coeff when the recursive call completes.
      std::vector<double> indep_chosen_beta_(train_data_independants_.size(), 0);
      int eligible_indep_count = 0;  ///< added variable to make sure that must_add_next_k_indeps_ takes precedence over
      /// correlation based seiving of indicators
      for (unsigned int indep_index_ = 0; indep_index_ < train_data_independants_.size(); indep_index_++) {
        if (included_[indep_index_] == 0) {  // still eligible to be selected
          double sum_chosen_indep_ = 0;
          for (unsigned int dataline_num_ = 0; dataline_num_ < train_data_independants_[chosen_index_].size();
               dataline_num_++) {
            sum_chosen_indep_ += (train_data_independants_[chosen_index_][dataline_num_]) *
                                 (train_data_independants_[indep_index_][dataline_num_]);
          }

          // also adding correlation based removal of independants not selected yet
          if (sum_square_independants_[indep_index_] <= 0) {  // not computed yet
            sum_square_independants_[indep_index_] = VectorUtils::CalcL2Norm(train_data_independants_[indep_index_]);
          }
          double corr_chosen_indep_ = sum_chosen_indep_ / (sqrt(sum_square_independants_chosen_index_) *
                                                           sqrt(sum_square_independants_[indep_index_]));
          if (fabs(corr_chosen_indep_) > kMaxIndepCorrelation) {  // marking very correlated indicators to the one that
                                                                  // was just selected, as ineligible
            if (eligible_indep_count < must_add_next_k_indeps_) {
              std::cerr << " Although in must_add_next_k_indeps_, marked " << indep_index_
                        << " ineligible since correlation with " << chosen_index_ << " = " << corr_chosen_indep_
                        << std::endl;
            }
            included_[indep_index_] = -1;
            // std::cout << " Marked " << indep_index_ << " ineligible since correlation with " << chosen_index_ << " =
            // " << corr_chosen_indep_ << std::endl;
          } else {
            indep_chosen_beta_[indep_index_] = sum_chosen_indep_ / sum_square_independants_chosen_index_;
            // std::cout << " Orthogonalization of " << indep_index_ << " against " << chosen_index_ << " coeff = " <<
            // indep_chosen_beta_ [ indep_index_ ] << std::endl;
            VectorUtils::ScaledVectorAddition(train_data_independants_[indep_index_],
                                              train_data_independants_[chosen_index_],
                                              -indep_chosen_beta_[indep_index_]);
            // even if coming in stdev ( train_data_independants_ [ indep_index_ ] ) == 1
            // after this step stdev ( train_data_independants_ [ indep_index_ ] ) != 1
            // hence the data is not mean=0+stdev=one but just mean=0
          }
          eligible_indep_count++;
        }
      }

      // double l2_dep = VectorUtils::CalcL2Norm ( train_data_dependant_ );
      // double l2_chosen = VectorUtils::CalcL2Norm (  train_data_independants_[chosen_index_]);
      // for ( unsigned int indep_index_ = 0 ; indep_index_ < train_data_independants_.size ( ) ; indep_index_ ++ )
      //   {
      // 	 double l2_indep = VectorUtils::CalcL2Norm ( train_data_independants_[indep_index_] );

      // 	 double sum_dep_indep_ = 0;
      // 	 for ( unsigned int dataline_num_ = 0 ; dataline_num_ < train_data_independants_ [ chosen_index_ ].size
      // ( ) ; dataline_num_ ++ )
      // 	   {
      // 	     sum_dep_indep_ += (train_data_dependant_[dataline_num_]) *
      // (train_data_independants_[indep_index_][dataline_num_]) ;
      // 	   }
      // 	 if ( included_[indep_index_] == 0 )
      // 	   std::cout << " COVAR (0," << indep_index_ + 1  <<" ): " << sum_dep_indep_/
      // (train_data_dependant_.size ( ) - 1 )
      //   // << " COVAR (0, 0 ) : " << l2_dep/ (train_data_dependant_.size ( ) - 1 )
      //   //	      << " COVAR:(c,c) " << chosen_index_<< " : "<< l2_chosen/(train_data_dependant_.size ( ) - 1 )
      //   //	      << " COVAR: ("<< indep_index_+1 <<" , " << indep_index_+1 << ") :" << VectorUtils::CalcL2Norm (
      //   train_data_independants_[indep_index_])/ (train_data_dependant_.size ( ) - 1 )
      // 	      << " INCLD: "<< included_[indep_index_]
      // 	      << " CORR (0, " << indep_index_ + 1<< ") : "<<   sum_dep_indep_ / ( sqrt ( l2_dep * l2_indep ) )
      // << std::endl;

      //   }

      FSLR_SO_NoMeanData_Backpass(train_data_orig_dependant_, train_data_orig_independants_, train_data_dependant_,
                                  train_data_independants_, must_add_next_k_indeps_, included_, finmodel_,
                                  kMinCorrelationIncrement, kMaxIndepCorrelation, initial_correlations_,
                                  max_model_size_, tstat_cutoff_, ignore_zeros_);

      // std::cout << "Final model..Size "<< this_finmodel_index_ + 1 << " : "<< finmodel_.size ( )<<std::endl;
      for (unsigned int i = (this_finmodel_index_ + 1); i < finmodel_.size();
           i++) {  // look at the indicators chosen after this one
        int thisvarindex_ = finmodel_[i].origindex_;
        double thisvarcoeff_ = finmodel_[i].coeff_;
        double thisbeta_ = indep_chosen_beta_[thisvarindex_];  // this was the beta used to remove the effect of
                                                               // chosen_index_ column from this indicator

        // std::cout << " Modifying coeff of " << thisvarindex_ << " thisvarcoeff_ " << thisvarcoeff_ << " thisbeta_ "
        // << thisbeta_ << " from " << finmodel_ [ this_finmodel_index_ ].coeff_ ;
        finmodel_[this_finmodel_index_].coeff_ += -thisbeta_ * thisvarcoeff_;
        // std::cout << " to " << finmodel_ [ this_finmodel_index_ ].coeff_ << std::endl;
      }
    }
  }
}
}
