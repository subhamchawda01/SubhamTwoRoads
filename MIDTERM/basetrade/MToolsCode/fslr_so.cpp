/**
   \file MToolsCode/fslr_so.cpp

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

// this function performs forward stagewise linear regression of non-NoMean data by successive orthogonalization of
// independants
// pruning independants that drop below kMinCorrelationIncrement threshold along the way
void FSLR_SO(std::vector<double>& train_data_dependant_, std::vector<std::vector<double> >& train_data_independants_,
             int must_add_next_k_indeps_, std::vector<int>& included_, std::vector<IndexCoeffPair_t>& finmodel_,
             const double kMinCorrelationIncrement, const double kMaxIndepCorrelation,
             const std::vector<double>& initial_correlations_, const unsigned int max_model_size_,
             const std::vector<std::vector<double> >& train_data_independants_original_) {
  if (finmodel_.size() >= max_model_size_) return;

  if (NextNotIncluded(included_) != kInvalidArrayIndex) {  // there are still indicators eligible to be selected

    int chosen_index_ = kInvalidArrayIndex;

    if (must_add_next_k_indeps_ > 0) {
      chosen_index_ = NextNotIncluded(
          included_);  // should this be done before some indices are marked -1 ... or not mark the first k indices -1 ?
      must_add_next_k_indeps_--;
      // std::cerr << " Due to must_add_next_k_indeps_ chosen_index_ = " << chosen_index_ << " remaining
      // must_add_next_k_indeps_ = " << must_add_next_k_indeps_ << std::endl;
    } else {
      double chosen_indep_fabs_correlation_ = 0;
      // choosing index which has maximum fabs ( correlation )
      for (unsigned int indep_index_ = 0; indep_index_ < train_data_independants_.size(); indep_index_++) {
        if (included_[indep_index_] == 0) {
          double this_correlation_ = GetCorrelation(train_data_dependant_, train_data_independants_[indep_index_]);

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

          double this_fabs_correlation_ = fabs(this_correlation_);
          if ((chosen_index_ == kInvalidArrayIndex) || (this_fabs_correlation_ > chosen_indep_fabs_correlation_)) {
            chosen_index_ = indep_index_;
            chosen_indep_fabs_correlation_ = this_fabs_correlation_;
          }
        }
      }

      if (chosen_index_ == kInvalidArrayIndex) {
        return;
      }
    }

    // checking eligibility ... should never be used
    if ((chosen_index_ == kInvalidArrayIndex) || (included_[chosen_index_] != 0)) {
      return;
    }

    included_[chosen_index_] = 1;

    // SLR coeff for nomean data = Sum_j ( x_ci_j * y_j ) / Sum_j ( x_ci_j * x_ci_j )
    double dep_chosen_coeff_ = GetSLRCoeff(train_data_dependant_, train_data_independants_[chosen_index_]);

    // Remove effect of chosen_index_ from DEP .
    // Doing this before pushing to finmodel_ to compute RSS_
    // which requires the dependant after including this indicator .
    VectorUtils::ScaledVectorAddition(train_data_dependant_, train_data_independants_[chosen_index_],
                                      -dep_chosen_coeff_);

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

    unsigned int this_finmodel_index_ = finmodel_.size();
    finmodel_.push_back(icp_t_);

    // Recursive Step
    if (NextNotIncluded(included_) != kInvalidArrayIndex) {  // there are still indicators eligible to be selected

      // remove effect of chosen_index_ from INDEPs not yet included
      // and store the std::vector of betas to calc the coeff when the recursive call completes.
      std::vector<double> indep_chosen_beta_(train_data_independants_.size(), 0);
      for (unsigned int indep_index_ = 0; indep_index_ < train_data_independants_.size(); indep_index_++) {
        if (included_[indep_index_] == 0) {  // this indicator is still eligible to be picked
          double this_indep_correlation_ =
              GetCorrelation(train_data_independants_[indep_index_], train_data_independants_[chosen_index_]);
          if (fabs(this_indep_correlation_) >= kMaxIndepCorrelation) {
            included_[indep_index_] = -1;
          } else {
            indep_chosen_beta_[indep_index_] =
                GetSLRCoeff(train_data_independants_[indep_index_], train_data_independants_[chosen_index_]);
            VectorUtils::ScaledVectorAddition(train_data_independants_[indep_index_],
                                              train_data_independants_[chosen_index_],
                                              -indep_chosen_beta_[indep_index_]);
          }
        }
      }

      FSLR_SO(train_data_dependant_, train_data_independants_, must_add_next_k_indeps_, included_, finmodel_,
              kMinCorrelationIncrement, kMaxIndepCorrelation, initial_correlations_, max_model_size_,
              train_data_independants_original_);

      for (unsigned int i = (this_finmodel_index_ + 1); i < finmodel_.size();
           i++) {  // look at the indicators chosen after this one
        int thisvarindex_ = finmodel_[i].origindex_;
        double thisvarcoeff_ = finmodel_[i].coeff_;
        double thisbeta_ = indep_chosen_beta_[thisvarindex_];  // this was the beta used to remove the effect of
                                                               // chosen_index_ column from this indicator

        finmodel_[this_finmodel_index_].coeff_ += -thisbeta_ * thisvarcoeff_;
      }
    }
  }
}
}
