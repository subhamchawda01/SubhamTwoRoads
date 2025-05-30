/**
   \file MToolsCode/fshdvlr_so_nomeandata.cpp

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
void Backward_Pass_FSHDVLR(const std::vector<double>& train_data_orig_dependant_,
                           const std::vector<std::vector<double> >& train_data_orig_independants_,
                           std::vector<IndexCoeffPair_t>& fwd_finmodel_, const double kMinCorrelationIncrement,
                           const double kMaxIndepCorrelation, const std::vector<double>& initial_correlations_,
                           const unsigned int max_model_size_, const double thresh_factor_, double tstat_cutoff_) {
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

    FSHDVLR_SO_NoMeanData(train_data_dependant_, train_data_independants_, 0, temp_in_included_, temp_finmodel_,
                          kMinCorrelationIncrement, kMaxIndepCorrelation, initial_correlations_, max_model_size_,
                          thresh_factor_, train_data_orig_independants_, tstat_cutoff_);
    if (temp_finmodel_.size() <= 0) continue;  // all indicators removed
    double temp_GCV = temp_finmodel_[temp_finmodel_.size() - 1].RSS_;
    GCV.push_back(temp_GCV);
    if ((min_GCV < 0) || (min_GCV > temp_GCV)) {
      min_GCV = temp_GCV;
      min_GCV_index = i;
    }
  }
  if (min_GCV < 0.0) {
    std::cerr << "All indicators removed by backpass in this iteration. Retaining fwdpass model";
    return;
  }

  std::cerr << "minGCV: " << min_GCV << "@" << min_GCV_index << "\t"
            << "fwdModel_GCV" << fwd_finmodel_[fwd_finmodel_.size() - 1].RSS_ << "\n";
  if (min_GCV < fwd_finmodel_[fwd_finmodel_.size() - 1].RSS_) {
    std::cerr << "Removing indicator " << fwd_finmodel_[min_GCV_index].origindex_ << "\n";
    fwd_finmodel_.erase(fwd_finmodel_.begin() + min_GCV_index);
    Backward_Pass_FSHDVLR(train_data_orig_dependant_, train_data_orig_independants_, fwd_finmodel_,
                          kMinCorrelationIncrement, kMaxIndepCorrelation, initial_correlations_, max_model_size_,
                          thresh_factor_, tstat_cutoff_);
  }
}

// this function performs forward stagewise linear regression of nomean data by successive orthogonalization of
// independants
void FSHDVLR_SO_NoMeanData_Backpass(const std::vector<double>& train_data_orig_dependant_,
                                    const std::vector<std::vector<double> >& train_data_orig_independants_,
                                    std::vector<double>& train_data_dependant_,
                                    std::vector<std::vector<double> >& train_data_independants_,
                                    int must_add_next_k_indeps_, std::vector<int>& included_,
                                    std::vector<IndexCoeffPair_t>& finmodel_, const double kMinCorrelationIncrement,
                                    const double kMaxIndepCorrelation, const std::vector<double>& initial_correlations_,
                                    const unsigned int max_model_size_, const double thresh_factor_,
                                    double tstat_cutoff_, bool ignore_zeros_) {
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

    if (must_add_next_k_indeps_ > 0) {
      chosen_index_ = NextNotIncluded(
          included_);  // should this be done before some indices are marked -1 ... or not mark the first k indices -1 ?
      must_add_next_k_indeps_--;

      // compute variables needed later in orthogonalization
      sum_dep_independants_[chosen_index_] =
          VectorUtils::CalcDotProduct(train_data_dependant_, train_data_independants_[chosen_index_]);
      sum_square_independants_[chosen_index_] = VectorUtils::CalcL2Norm(train_data_independants_[chosen_index_]);
    } else {
      CalcNoMeanSums(train_data_dependant_, train_data_independants_, included_, sum_square_independants_,
                     sum_dep_independants_);

      // choosing index which minimizes squared error most ... equivalent to having maximum fabs ( correlation )
      double max_fabs_hdv_correlation_ = 0;
      double max_fabs_correlation_ = 0;
      for (unsigned int indep_index_ = 0; indep_index_ < train_data_independants_.size(); indep_index_++) {
        if (included_[indep_index_] == 0) {  // still eligible to be selected

          if (sum_square_independants_[indep_index_] <= 0) {  // this indep is a constant, so no point
            included_[indep_index_] = -1;
            continue;
          }
          double this_SLR_coeff_ = 0;
          double this_correlation_ = 0;
          GetSLRCoeffCorrelationNoMean(train_data_dependant_, train_data_independants_[indep_index_], this_SLR_coeff_,
                                       this_correlation_);
          {
            //  double this_correlation_ = sum_dep_independants_[indep_index_] /
            //                           (sqrt(sum_square_dependant_) * sqrt(sum_square_independants_[indep_index_]));
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
          }

          double t_thresh_value_ =
              thresh_factor_ * sqrt(sum_square_dependant_ / (double)(train_data_dependant_.size() - 1));
          double this_hdv_correlation_ =
              GetHDVCorrelationNoMean(train_data_dependant_, train_data_independants_[indep_index_], t_thresh_value_);
          double this_fabs_hdv_correlation_ = fabs(this_hdv_correlation_);
          // std::cout << "DEBUG : " << indep_index_ << " partcorr: " << this_hdv_correlation_ << std::endl;
          if (ignore_zeros_ == true) {
            if ((chosen_index_ == kInvalidArrayIndex) ||
                (fabs(this_correlation_) > max_fabs_correlation_)) {  // store index and value of current best option
              chosen_index_ = indep_index_;
              //      std::cerr << this_correlation_;
              max_fabs_correlation_ = fabs(this_correlation_);
            }
          } else {
            if ((chosen_index_ == kInvalidArrayIndex) ||
                (this_fabs_hdv_correlation_ >
                 max_fabs_hdv_correlation_)) {  // store index and value of current best option
              chosen_index_ = indep_index_;
              max_fabs_hdv_correlation_ = this_fabs_hdv_correlation_;
              // std::cout << "DEBUG : new_chosen_index_ " << chosen_index_ << std::endl;
            }
          }
        }
      }
      // std::cout << "DEBUG : Finalchosen_index_ " << chosen_index_ << std::endl;

      if (chosen_index_ == kInvalidArrayIndex) {
        return;
      }
    }

    // checking eligibility
    if ((chosen_index_ == kInvalidArrayIndex) || (included_[chosen_index_] != 0)) {
      return;
    }

    included_[chosen_index_] = 1;  // mark as selected

    if (sum_square_independants_[chosen_index_] <= 0) {  // not computed yet
      sum_square_independants_[chosen_index_] = VectorUtils::CalcL2Norm(train_data_independants_[chosen_index_]);
    }
    double sum_square_independants_chosen_index_ = sum_square_independants_[chosen_index_];

    // SHDLR coeff for nomean data = Sum_j_lines_with_(fabs(x_ci_j) > stdev(x_ci)) [ ( x_ci_j * y_j ) / Sum_j ( x_ci_j *
    // x_ci_j ) ]
    // which is the same as SLR Coeff for nomean data if the dataset is restricted to samples where the fabs_dep_value
    // >= stdev_dep
    double t_thresh_value_ = thresh_factor_ * sqrt(sum_square_dependant_ / (double)(train_data_dependant_.size() - 1));
    double dep_chosen_coeff_ =
        GetSHDLRCoeffNoMean(train_data_dependant_, train_data_independants_[chosen_index_], t_thresh_value_);

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

    unsigned int this_finmodel_index_ = finmodel_.size();
    if (RSS_ >= sum_square_dependant_) {  // Gained nothing by adding this indicator...
      // Undoing the changes made already...
      included_[chosen_index_] = -1;
      VectorUtils::ScaledVectorAddition(train_data_dependant_, train_data_independants_[chosen_index_],
                                        +dep_chosen_coeff_);
    } else {
      finmodel_.push_back(icp_t_);
    }

    Backward_Pass_FSHDVLR(train_data_orig_dependant_, train_data_orig_independants_, finmodel_,
                          kMinCorrelationIncrement, kMaxIndepCorrelation, initial_correlations_, max_model_size_,
                          thresh_factor_,
                          tstat_cutoff_);          // Try removing some variables from the model created thus far.
    if (finmodel_.size() <= this_finmodel_index_)  // some indicator pruned in backpass
    {                                              // create training data again
      std::vector<int> temp_in_included_(train_data_orig_independants_.size(), -1);
      for (unsigned int j = 0; j < finmodel_.size(); j++)
        temp_in_included_[finmodel_[j].origindex_] =
            0;  // mark all indicators ineligible for selection except the ones that were retained by backpass pass
      train_data_dependant_ = train_data_orig_dependant_;  // overwrite copy which will be modified
      train_data_independants_ = train_data_orig_independants_;
      std::vector<IndexCoeffPair_t> temp_finmodel_;
      FSHDVLR_SO_NoMeanData(train_data_dependant_, train_data_independants_, 0, temp_in_included_, temp_finmodel_,
                            kMinCorrelationIncrement, kMaxIndepCorrelation, initial_correlations_, max_model_size_,
                            thresh_factor_, train_data_orig_independants_, tstat_cutoff_);
      finmodel_ = temp_finmodel_;
    }

    // Recursive Step
    if (NextNotIncluded(included_) != kInvalidArrayIndex) {  // there are still indicators eligible to be selected

      // remove effect of chosen_index_ from INDEPs not yet included
      // and store the std::vector of betas to calc the coeff when the recursive call completes.
      std::vector<double> indep_chosen_beta_(train_data_independants_.size(), 0);
      int eligible_indep_count = 0;  ///< added variable to make sure that must_add_next_k_indeps_ takes precedence over
      /// correlation based seiving of indicators
      if (included_[chosen_index_] == 1)
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
            if (fabs(corr_chosen_indep_) > kMaxIndepCorrelation) {  // marking very correlated indicators to the one
                                                                    // that was just selected, as ineligible
              if (eligible_indep_count < must_add_next_k_indeps_) {
                std::cerr << " Although in must_add_next_k_indeps_, marked " << indep_index_
                          << " ineligible since correlation with " << chosen_index_ << " = " << corr_chosen_indep_
                          << std::endl;
              }
              included_[indep_index_] = -1;
              // std::cout << " DBEUG : Marked " << indep_index_ << " ineligible since correlation with " <<
              // chosen_index_ << " = " << corr_chosen_indep_ << std::endl;
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

      if (finmodel_.size() < 8u) {  // only first 8 indicators HVcorr
        FSHDVLR_SO_NoMeanData_Backpass(train_data_orig_dependant_, train_data_orig_independants_, train_data_dependant_,
                                       train_data_independants_, must_add_next_k_indeps_, included_, finmodel_,
                                       kMinCorrelationIncrement, kMaxIndepCorrelation, initial_correlations_,
                                       max_model_size_, thresh_factor_, tstat_cutoff_, ignore_zeros_);
      } else {
        FSLR_SO_NoMeanData_Backpass(train_data_orig_dependant_, train_data_orig_independants_, train_data_dependant_,
                                    train_data_independants_, must_add_next_k_indeps_, included_, finmodel_,
                                    kMinCorrelationIncrement, kMaxIndepCorrelation, initial_correlations_,
                                    max_model_size_, tstat_cutoff_, ignore_zeros_);
      }

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
