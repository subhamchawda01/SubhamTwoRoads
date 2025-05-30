/**
   \file MToolsCode/fsrr_so_nomeandata.cpp

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

// this function performs forward stagewise ridge regression of nomean data by successive orthogonalization of
// independants
void FSRR_SO_NoMeanData(std::vector<double>& train_data_dependant_,
                        std::vector<std::vector<double> >& train_data_independants_, int must_add_next_k_indeps_,
                        std::vector<int>& included_, std::vector<IndexCoeffPair_t>& finmodel_,
                        std::vector<std::vector<double> >& coeff_dependancy_matrix_,
                        const double kMinCorrelationIncrement, const double regularization_coeff_,
                        const double kMaxIndepCorrelation, const std::vector<double>& initial_correlations_,
                        const unsigned int max_model_size_,
                        const std::vector<std::vector<double> >& train_data_independants_original_,
                        double tstat_cutoff_, bool ignore_zeros_) {
  if (finmodel_.size() >= max_model_size_) return;

  HFSAT::CovMatInv::GetUniqueInstance(train_data_independants_original_);

  if (NextNotIncluded(included_) != kInvalidArrayIndex) {  // there are still indicators eligible to be selected

    int chosen_index_ = kInvalidArrayIndex;

    if (must_add_next_k_indeps_ > 0) {
      chosen_index_ = NextNotIncluded(
          included_);  // should this be done before some indices are marked -1 ... or not mark the first k indices -1 ?
      must_add_next_k_indeps_--;
    } else {
      double max_ridge_variance_reduced_ = 0;
      double max_fabs_correlation_ = 0;
      for (unsigned int indep_index_ = 0; indep_index_ < train_data_independants_.size(); indep_index_++) {
        if (included_[indep_index_] == 0) {
          double this_SRR_coeff_ = 0;
          double this_correlation_ = 0;
          GetSRRCoeffCorrelationNoMean(train_data_dependant_, train_data_independants_[indep_index_],
                                       regularization_coeff_, this_SRR_coeff_, this_correlation_);

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

          double t_dep_chosen_coeff_ =
              GetSRRCoeffNoMean(train_data_dependant_, train_data_independants_[indep_index_], regularization_coeff_);
          VectorUtils::ScaledVectorAddition(train_data_dependant_, train_data_independants_[indep_index_],
                                            -t_dep_chosen_coeff_);

          double t_RSS_ = VectorUtils::CalcL2Norm(train_data_dependant_);
          double t_SXX_ = VectorUtils::CalcL2Norm(train_data_independants_[indep_index_]);

          bool chosenIndexAdded =
              CheckTstat(train_data_dependant_, train_data_independants_original_, indep_index_, t_dep_chosen_coeff_,
                         t_RSS_, t_SXX_, coeff_dependancy_matrix_, tstat_cutoff_, finmodel_, true);

          VectorUtils::ScaledVectorAddition(train_data_dependant_, train_data_independants_[indep_index_],
                                            t_dep_chosen_coeff_);

          if (chosenIndexAdded == false) {
            included_[indep_index_] = -2;
            continue;
          }

          if (ignore_zeros_ == true) {
            if ((chosen_index_ == kInvalidArrayIndex) || (fabs(this_correlation_) > max_fabs_correlation_)) {
              chosen_index_ = indep_index_;
              max_fabs_correlation_ = fabs(this_correlation_);
            }
          } else {
            double this_dep_loss_reduced_ =
                GetLossReductionNoMean(train_data_dependant_, train_data_independants_[indep_index_], this_SRR_coeff_);
            double this_coeff_loss_reduced_ =
                GetCoeffLossReduced(included_, finmodel_, coeff_dependancy_matrix_, indep_index_, this_SRR_coeff_);
            double this_ridge_variance_reduced_ =
                this_dep_loss_reduced_ + regularization_coeff_ * this_coeff_loss_reduced_;
            if ((chosen_index_ == kInvalidArrayIndex) || (this_ridge_variance_reduced_ > max_ridge_variance_reduced_)) {
              chosen_index_ = indep_index_;
              max_ridge_variance_reduced_ = this_ridge_variance_reduced_;
            }
          }
        }
      }
    }

    // checking eligibility
    if ((chosen_index_ == kInvalidArrayIndex) || (included_[chosen_index_] != 0)) {
      return;
    }

    included_[chosen_index_] = 1;

    double dep_chosen_coeff_ =
        GetSRRCoeffNoMean(train_data_dependant_, train_data_independants_[chosen_index_], regularization_coeff_);
    // Remove effect of chosen_index_ from DEP . Undo if tstat check fails
    VectorUtils::ScaledVectorAddition(train_data_dependant_, train_data_independants_[chosen_index_],
                                      -dep_chosen_coeff_);
    double RSS_ = VectorUtils::CalcL2Norm(train_data_dependant_);
    double SXX_ = VectorUtils::CalcL2Norm(train_data_independants_[chosen_index_]);

    bool chosenIndexAdded = CheckTstatAndAddToModel(train_data_dependant_, train_data_independants_original_,
                                                    chosen_index_, dep_chosen_coeff_, RSS_, SXX_,
                                                    coeff_dependancy_matrix_, tstat_cutoff_, finmodel_, true);
    if (chosenIndexAdded == false) {
      included_[chosen_index_] = -1;
      VectorUtils::ScaledVectorAddition(train_data_dependant_, train_data_independants_[chosen_index_],
                                        dep_chosen_coeff_);
      return;
    }

    // Recursive Step
    if (NextNotIncluded(included_) != kInvalidArrayIndex) {  // there are still indicators eligible to be selected

      // remove effect of chosen_index_ from INDEPs still eligible to be included
      // this introduces a dependancy in terms of the coeff_dependancy_matrix_
      for (unsigned int indep_index_ = 0; indep_index_ < train_data_independants_.size(); indep_index_++) {
        if (included_[indep_index_] == 0) {  // still eligible to be selected

          // although ridge regression takes care of collinearity
          // this part is to explicitly eliminate extremely correlated indicators
          double this_indep_correlation_ =
              GetCorrelation(train_data_independants_[indep_index_], train_data_independants_[chosen_index_]);
          if (fabs(this_indep_correlation_) >= kMaxIndepCorrelation) {
            included_[indep_index_] = -1;
          } else {
            double indep_chosen_beta_indep_index_ =
                GetSLRCoeffNoMean(train_data_independants_[indep_index_], train_data_independants_[chosen_index_]);
            VectorUtils::ScaledVectorAddition(train_data_independants_[indep_index_],
                                              train_data_independants_[chosen_index_], -indep_chosen_beta_indep_index_);

            // now onwards addition of X to coeff of indep_index_ will cause change in coeff of chosen_index_ by
            // (-indep_chosen_beta_indep_index_)
            // since train_data_independants_[ indep_index_ ] has train_data_independants_ [ chosen_index_ ] * (
            // -indep_chosen_beta_indep_index_ ) in it
            // and similarly if some other coeff, say coeff[j] depends on coeff[chosen_index_] by factor f, then the
            // dependancy of coeff[j] on coeff [ indep_index_ ] increases by f * ( -indep_chosen_beta_indep_index_ )
            IncrementDependancyMatrix(finmodel_, coeff_dependancy_matrix_, indep_index_, chosen_index_,
                                      -indep_chosen_beta_indep_index_);
          }
        }
      }

      FSRR_SO_NoMeanData(train_data_dependant_, train_data_independants_, must_add_next_k_indeps_, included_, finmodel_,
                         coeff_dependancy_matrix_, kMinCorrelationIncrement, regularization_coeff_,
                         kMaxIndepCorrelation, initial_correlations_, max_model_size_,
                         train_data_independants_original_, tstat_cutoff_, ignore_zeros_);
    }
  }
}
}
