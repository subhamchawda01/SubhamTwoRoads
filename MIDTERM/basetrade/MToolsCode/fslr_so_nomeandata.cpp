/**
   \file MToolsCode/fslr_so_nomeandata.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
*/

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "basetrade/MTools/data_processing.hpp"
#include "basetrade/MTools/iterative_regress.hpp"
//#include "dvccode/CDef/math_utils.hpp"

namespace HFSAT {

// this function performs forward stagewise linear regression of nomean data by successive orthogonalization of
// independants
void FSLR_SO_NoMeanData(std::vector<double>& train_data_dependant_,
                        std::vector<std::vector<double> >& train_data_independants_,
                        unsigned int must_add_next_k_indeps_, std::vector<int>& included_,
                        std::vector<IndexCoeffPair_t>& finmodel_, const double kMinCorrelationIncrement,
                        const double kMaxIndepCorrelation, const std::vector<double>& initial_correlations_,
                        const unsigned int max_model_size_, std::vector<double>& predicted_values_,
                        std::vector<std::vector<double> >& independant_orthogonalization_matrix_,
                        const std::vector<std::vector<double> >& train_data_independants_original_,
                        double tstat_cutoff_, bool ignore_zeros_) {
  if (finmodel_.size() >= max_model_size_) {
    return;
  }

  if (NextNotIncluded(included_) != kInvalidArrayIndex) {  // there are still indicators eligible to be selected
    // precomputing variables for faster computation of loss reduction and correlation

    std::vector<double> sum_square_independants_(train_data_independants_.size(),
                                                 -1);  ///< ith value = Sum_j ( x_i_j * x_i_j )
    std::vector<double> sum_dep_independants_(train_data_independants_.size(),
                                              -1);  ///< ith value = Sum_j ( x_i_j * y_j )

    double sum_square_dependant_ = VectorUtils::CalcL2Norm(train_data_dependant_);
    if (sum_square_dependant_ <= 0) {
      return;
    }

    int chosen_index_ = kInvalidArrayIndex;
    double sum_square_independants_chosen_index_ = 0;  ///< Sum_j ( x_ci_j * x_ci_j )

    if (must_add_next_k_indeps_ > 0) {
      chosen_index_ = NextNotIncluded(
          included_);  // should this be done before some indices are marked -1 ... or not mark the first k indices -1 ?
      must_add_next_k_indeps_--;

      // compute variables needed later in orthogonalization

      sum_dep_independants_[chosen_index_] =
          VectorUtils::CalcDotProduct(train_data_dependant_, train_data_independants_[chosen_index_]);
      sum_square_independants_[chosen_index_] = VectorUtils::CalcL2Norm(train_data_independants_[chosen_index_]);
      sum_square_independants_chosen_index_ = sum_square_independants_[chosen_index_];

    } else {
      CalcNoMeanSums(train_data_dependant_, train_data_independants_, included_, sum_square_independants_,
                     sum_dep_independants_);

      // choosing index which minimizes squared error most ... equivalent to
      // having maximum fabs ( correlation )
      double max_variance_reduced_ = 0;
      double max_fabs_correlation_ = 0;
      for (unsigned int indep_index_ = 0; indep_index_ < train_data_independants_.size(); indep_index_++) {
        if (included_[indep_index_] == 0) {  // still eligible to be selected
          double this_SLR_coeff_ = 0;
          double this_correlation_ = 0;
          GetSLRCoeffCorrelationNoMean(train_data_dependant_, train_data_independants_[indep_index_], this_SLR_coeff_,
                                       this_correlation_);
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

          double t_dep_chosen_coeff_ = GetSLRCoeffNoMean(train_data_dependant_, train_data_independants_[indep_index_]);
          // Remove effect of indep_index_ from DEP . Undo it after tstat check. Not sure if this is neccessary but it
          // can slow regression
          VectorUtils::ScaledVectorAddition(train_data_dependant_, train_data_independants_[indep_index_],
                                            -t_dep_chosen_coeff_);
          // don't think this will effect tstatcheck
          // VectorUtils::ScaledVectorAddition ( predicted_values_, train_data_independants_ [ chosen_index_ ],
          // dep_chosen_coeff_ ) ;
          double t_RSS_ = VectorUtils::CalcL2Norm(train_data_dependant_);
          double t_SXX_ = VectorUtils::CalcL2Norm(train_data_independants_[indep_index_]);

          bool chosenIndexAdded =
              CheckTstat(train_data_dependant_, train_data_independants_original_, indep_index_, t_dep_chosen_coeff_,
                         t_RSS_, t_SXX_, independant_orthogonalization_matrix_, tstat_cutoff_, finmodel_);
          // Undoing addition we did for tstat check.
          VectorUtils::ScaledVectorAddition(train_data_dependant_, train_data_independants_[indep_index_],
                                            t_dep_chosen_coeff_);

          if (chosenIndexAdded == false) {
            included_[indep_index_] = -2;
            continue;
          }
          if (ignore_zeros_ == true) {
            if ((chosen_index_ == kInvalidArrayIndex) ||
                (fabs(this_correlation_) > max_fabs_correlation_)) {  // store index and value of current best option
              chosen_index_ = indep_index_;
              //	std::cerr << this_correlation_;
              max_fabs_correlation_ = fabs(this_correlation_);
            }
          } else {
            double this_dep_loss_reduced_ =
                GetLossReductionNoMean(train_data_dependant_, train_data_independants_[indep_index_], this_SLR_coeff_);
            if ((chosen_index_ == kInvalidArrayIndex) ||
                (this_dep_loss_reduced_ > max_variance_reduced_)) {  // store index and value of current best option
              chosen_index_ = indep_index_;
              // std::cerr << this_correlation_;
              max_variance_reduced_ = this_dep_loss_reduced_;
            }
          }
        }
      }

      if (chosen_index_ == kInvalidArrayIndex) {
        return;
      }

      sum_square_independants_chosen_index_ = sum_square_independants_[chosen_index_];
    }

    // checking eligibility
    if ((chosen_index_ == kInvalidArrayIndex) || (included_[chosen_index_] != 0)) {
      return;
    }

    included_[chosen_index_] = 1;  // mark as selected
    double dep_chosen_coeff_ = GetSLRCoeffNoMean(train_data_dependant_, train_data_independants_[chosen_index_]);
    // Remove effect of chosen_index_ from DEP . Undo if tstat check fails
    VectorUtils::ScaledVectorAddition(train_data_dependant_, train_data_independants_[chosen_index_],
                                      -dep_chosen_coeff_);
    VectorUtils::ScaledVectorAddition(predicted_values_, train_data_independants_[chosen_index_], dep_chosen_coeff_);
    double RSS_ = VectorUtils::CalcL2Norm(train_data_dependant_);
    double SXX_ = VectorUtils::CalcL2Norm(train_data_independants_[chosen_index_]);

    // just to add to finmodel, tstat check already done
    bool chosenIndexAdded = CheckTstatAndAddToModel(train_data_dependant_, train_data_independants_original_,
                                                    chosen_index_, dep_chosen_coeff_, RSS_, SXX_,
                                                    independant_orthogonalization_matrix_, tstat_cutoff_, finmodel_);
    if (chosenIndexAdded == false) {
      // should never reach here
      std::cerr << "tstat shouldn't fail for chosen index\n";
      exit(1);
      included_[chosen_index_] = -2;
      VectorUtils::ScaledVectorAddition(train_data_dependant_, train_data_independants_[chosen_index_],
                                        dep_chosen_coeff_);
      VectorUtils::ScaledVectorAddition(predicted_values_, train_data_independants_[chosen_index_], -dep_chosen_coeff_);
      return;
    }

    // Recursive Step
    if (NextNotIncluded(included_) != kInvalidArrayIndex) {  // there are still indicators eligible to be selected

      // remove effect of chosen_index_ from INDEPs not yet included
      // and store the std::vector of betas to calc the coeff when the recursive call completes
      // in independant_orthogonalization_matrix_ ( chosen_index_, : )

      unsigned int eligible_indep_count = 0u;  ///< added variable to make sure that must_add_next_k_indeps_ takes
      /// precedence over correlation based seiving of indicators
      for (unsigned int indep_index_ = 0u; indep_index_ < train_data_independants_.size(); indep_index_++) {
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
            included_[indep_index_] = -2;
          } else {
            independant_orthogonalization_matrix_[chosen_index_][indep_index_] =
                sum_chosen_indep_ / sum_square_independants_chosen_index_;

            // This is the only line where we are modifying the input indicators
            VectorUtils::ScaledVectorAddition(train_data_independants_[indep_index_],
                                              train_data_independants_[chosen_index_],
                                              -independant_orthogonalization_matrix_[chosen_index_][indep_index_]);
            // even if coming in stdev ( train_data_independants_ [ indep_index_ ] ) == 1
            // after this step stdev ( train_data_independants_ [ indep_index_ ] ) != 1
            // hence the data is not mean=0+stdev=one but just mean=0
          }
          eligible_indep_count++;
        }
      }
      FSLR_SO_NoMeanData(train_data_dependant_, train_data_independants_, must_add_next_k_indeps_, included_, finmodel_,
                         kMinCorrelationIncrement, kMaxIndepCorrelation, initial_correlations_, max_model_size_,
                         predicted_values_, independant_orthogonalization_matrix_, train_data_independants_original_,
                         tstat_cutoff_, ignore_zeros_);
    }
  }
}
}
