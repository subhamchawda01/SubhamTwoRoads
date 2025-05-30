/**
   \file MToolsCode/fsr_sharpe_rsq.cpp

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

// this function performs forward stagewise linear regression of nomean data by successive orthogonalization of
// independants
void FSR_sharpe_rsq(std::vector<double>& train_data_dependant_,
                    std::vector<std::vector<double> >& train_data_independants_, int must_add_next_k_indeps_,
                    std::vector<int>& included_, std::vector<IndexCoeffPair_t>& finmodel_,
                    const unsigned int num_folds_, const double kMinCorrelationIncrement,
                    const double kMaxIndepCorrelation, const std::vector<double>& initial_correlations_,
                    const unsigned int max_model_size_) {
  if (finmodel_.size() >= max_model_size_) {
    return;
  }

  if (NextNotIncluded(included_) != kInvalidArrayIndex) {  // there are still indicators eligible to be selected
    // precomputing variables for faster computation of loss reduction and correlation

    unsigned int chosen_index_ = train_data_independants_.size();  // invalid
    if (must_add_next_k_indeps_ > 0) {
      chosen_index_ = NextNotIncluded(
          included_);  // should this be done before some indices are marked -1 ... or not mark the first k indices -1 ?
      must_add_next_k_indeps_--;
    } else {
      // choosing index which has maximum fabs ( median ( correlation ) )
      double max_stable_rsq_ = 0;
      for (unsigned int indep_index_ = 0; indep_index_ < train_data_independants_.size(); indep_index_++) {
        if (included_[indep_index_] == 0) {  // still eligible to be selected

          // get the regression beta on the entre data
          double this_beta_ = GetSLRCoeffNoMean(train_data_dependant_, train_data_independants_[indep_index_]);

          if (this_beta_ * initial_correlations_[indep_index_] <
              0) {  // the sign of correlation has changed from initial correlation
            included_[indep_index_] = -1;
            continue;
          }

          // then get mean * sharpe of variance reduction on folds
          double this_stable_rsq_ = GetStableDepVarianceReduced(
              train_data_dependant_, train_data_independants_[indep_index_], this_beta_, num_folds_);
          if ((chosen_index_ == train_data_independants_.size()) ||
              (this_stable_rsq_ > max_stable_rsq_)) {  // if this stability adjusted variance reduction
            if (fabs(GetCorrelationNoMean(train_data_dependant_, train_data_independants_[indep_index_])) <=
                kMinCorrelationIncrement) {  // pruning independants that drop below kMinCorrelationIncrement threshold
                                             // along the way
              included_[indep_index_] = -1;
              continue;
            }

            // store index and value of current best option
            chosen_index_ = indep_index_;
            max_stable_rsq_ = this_stable_rsq_;
          }
        }
      }
    }

    // checking eligibility
    if ((chosen_index_ == train_data_independants_.size()) || (included_[chosen_index_] != 0)) {
      return;
    }

    included_[chosen_index_] = 1;  // mark as selected

    double chosen_indicator_stdev_ = VectorUtils::GetStdevNoMean(train_data_independants_[chosen_index_]);
    // RLM coeff for nomean data = Median Corr * std ( Y ) / std ( X_i )
    // double dep_chosen_coeff_ = GetMedianNoMeanCorrelation ( train_data_dependant_, train_data_independants_ [
    // chosen_index_ ], num_folds_ ) * VectorUtils::GetStdevNoMean ( train_data_dependant_ ) / chosen_indicator_stdev_ ;
    double dep_chosen_coeff_ = GetCorrelationNoMean(train_data_dependant_, train_data_independants_[chosen_index_]) *
                               VectorUtils::GetStdevNoMean(train_data_dependant_) / chosen_indicator_stdev_;

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

    unsigned int this_finmodel_index_ = finmodel_.size();
    finmodel_.push_back(icp_t_);

    // Recursive Step
    if (NextNotIncluded(included_) != kInvalidArrayIndex) {  // there are still indicators eligible to be selected

      // remove effect of chosen_index_ from INDEPs not yet included
      // and store the std::vector of betas to calc the coeff when the recursive call completes.
      std::vector<double> indep_chosen_beta_(train_data_independants_.size(), 0);
      int eligible_indep_count = 0;  ///< added variable to make sure that must_add_next_k_indeps_ takes precedence over
      /// correlation based seiving of indicators
      for (unsigned int indep_index_ = 0; indep_index_ < train_data_independants_.size(); indep_index_++) {
        if (included_[indep_index_] == 0) {  // still eligible to be selected
                                             //		    double corr_chosen_indep_ = GetMedianNoMeanCorrelation (
          // train_data_independants_[indep_index_],
          // train_data_independants_[chosen_index_], num_folds_ );
          double corr_chosen_indep_ =
              GetCorrelationNoMean(train_data_independants_[indep_index_], train_data_independants_[chosen_index_]);
          if (fabs(corr_chosen_indep_) > kMaxIndepCorrelation) {  // marking very correlated indicators to the one that
                                                                  // was just selected, as ineligible
            if (eligible_indep_count < must_add_next_k_indeps_) {
              std::cerr << " Although in must_add_next_k_indeps_, marked " << indep_index_
                        << " ineligible since correlation with " << chosen_index_ << " = " << corr_chosen_indep_
                        << std::endl;
            }
            included_[indep_index_] = -1;
          } else {
            indep_chosen_beta_[indep_index_] = corr_chosen_indep_ *
                                               VectorUtils::GetStdevNoMean(train_data_independants_[indep_index_]) /
                                               chosen_indicator_stdev_;
            VectorUtils::ScaledVectorAddition(train_data_independants_[indep_index_],
                                              train_data_independants_[chosen_index_],
                                              -indep_chosen_beta_[indep_index_]);
          }
          eligible_indep_count++;
        }
      }

      FSR_sharpe_rsq(train_data_dependant_, train_data_independants_, must_add_next_k_indeps_, included_, finmodel_,
                     num_folds_, kMinCorrelationIncrement, kMaxIndepCorrelation, initial_correlations_,
                     max_model_size_);

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
