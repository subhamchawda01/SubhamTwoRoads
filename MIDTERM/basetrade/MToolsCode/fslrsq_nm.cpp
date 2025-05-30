/**
   \file MToolsCode/fslrsq_nm.cpp

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
// For subset selection it uses a cost benefit tradeoff
// benefit is the decrease in variance on adding an indicator
// cost is
//   (a) the square max abs correlation that indicator has with anything that has been chosen already
//   (b) variance of variance reduced // TODO
void FSLRSQ_NM(std::vector<double>& train_data_dependant_, std::vector<std::vector<double> >& train_data_independants_,
               unsigned int must_add_next_k_indeps_, std::vector<int>& included_,
               std::vector<IndexCoeffPair_t>& finmodel_, const double kMinRsquaredIncrement,
               const unsigned int SortCriterion,                // unused right now
               std::vector<double>& max_abs_correlations_sel_,  // this refers to the maximum abs ( corr ) of remaining
                                                                // indicators against already selected ones
               const std::vector<double>& initial_correlations_, const unsigned int max_model_size_,
               std::vector<double>& predicted_values_,
               std::vector<std::vector<double> >& independant_orthogonalization_matrix_) {
  if (finmodel_.size() >= max_model_size_) {
    return;
  }

  if (NextNotIncluded(included_) != kInvalidArrayIndex) {  // there are still indicators eligible to be selected
    // precomputing variables for faster computation of loss reduction and correlation

    std::vector<double> sum_square_independants_(train_data_independants_.size(),
                                                 -1);  ///< ith value = Sum_j ( x_i_j * x_i_j )
    std::vector<double> sum_dep_independants_(train_data_independants_.size(),
                                              -1);  ///< ith value = Sum_j ( x_i_j * y_j )

    double sum_square_dependant_ = VectorUtils::CalcL2Norm(train_data_dependant_);  // variance of mean zero dependant
    if (sum_square_dependant_ <= 0) {
      return;
    }

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
      for (unsigned int indep_index_ = 0; indep_index_ < train_data_independants_.size(); indep_index_++) {
        if (included_[indep_index_] == 0) {  // still eligible to be selected
          double this_correlation_ = sum_dep_independants_[indep_index_] /
                                     (sqrt(sum_square_dependant_) * sqrt(sum_square_independants_[indep_index_]));

          if (this_correlation_ * initial_correlations_[indep_index_] <
              0) {  // the sign of correlation has changed from initial correlation
            included_[indep_index_] = -1;
            continue;
          }

          // the "reward" of choosing this indicator is that it will reduce the followng variance on training data
          double this_dep_loss_reduced_ =
              GetSquareOf(sum_dep_independants_[indep_index_]) /
              sum_square_independants_[indep_index_];  ///< ( Sum_j ( x_i_j * y_j ) )^2 / Sum_j ( (x_i_j)^2 )
#define BASE_CORR_VALUE 0.1                            // to have non zero denominator
          double this_cost_ = GetSquareOf(BASE_CORR_VALUE + max_abs_correlations_sel_[indep_index_]);
#undef BASE_CORR_VALUE
          double risk_adjusted_reward_ = this_dep_loss_reduced_ / this_cost_;

          if (this_dep_loss_reduced_ / sum_square_dependant_ <
              kMinRsquaredIncrement) {  // pruning independants that drop below kMinRsquaredIncrement threshold along
                                        // the way
            included_[indep_index_] = -1;
            continue;
          }

          if ((chosen_index_ == kInvalidArrayIndex) ||
              (risk_adjusted_reward_ > max_variance_reduced_)) {  // store index and value of current best option
            chosen_index_ = indep_index_;
            max_variance_reduced_ = risk_adjusted_reward_;
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

    // SLR coeff for nomean data = Sum_j ( x_ci_j * y_j ) / Sum_j ( x_ci_j * x_ci_j )
    double dep_chosen_coeff_ = sum_dep_independants_chosen_index_ / sum_square_independants_chosen_index_;

    // Remove effect of chosen_index_ from DEP .
    // Doing this before pushing to finmodel_ to compute RSS_
    // which requires the dependant after including this indicator .
    VectorUtils::ScaledVectorAddition(train_data_dependant_, train_data_independants_[chosen_index_],
                                      -dep_chosen_coeff_);
    VectorUtils::ScaledVectorAddition(predicted_values_, train_data_independants_[chosen_index_], dep_chosen_coeff_);
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
      // and store the std::vector of betas to calc the coeff when the recursive call completes
      // in independant_orthogonalization_matrix_ ( chosen_index_, : )

      // unsigned int eligible_indep_count = 0u; ///< added variable to make sure that must_add_next_k_indeps_ takes
      // precedence over correlation based seiving of indicators
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
          if (max_abs_correlations_sel_[indep_index_] <
              fabs(corr_chosen_indep_)) {  // recalculating the cost of selecting this indicator
            max_abs_correlations_sel_[indep_index_] = fabs(corr_chosen_indep_);
          }

          {
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
          // eligible_indep_count ++;
        }
      }

      FSLRSQ_NM(train_data_dependant_, train_data_independants_, must_add_next_k_indeps_, included_, finmodel_,
                kMinRsquaredIncrement, SortCriterion, max_abs_correlations_sel_, initial_correlations_, max_model_size_,
                predicted_values_, independant_orthogonalization_matrix_);

      for (unsigned int i = (this_finmodel_index_ + 1); i < finmodel_.size();
           i++) {  // look at the indicators chosen after this one
        int thisvarindex_ = finmodel_[i].origindex_;
        double thisvarcoeff_ = finmodel_[i].coeff_;
        double thisbeta_ =
            independant_orthogonalization_matrix_[chosen_index_][thisvarindex_];  // this was the beta used to remove
                                                                                  // the effect of chosen_index_ column
                                                                                  // from this indicator
        finmodel_[this_finmodel_index_].coeff_ += -thisbeta_ * thisvarcoeff_;
      }
    }
  }
}
}
