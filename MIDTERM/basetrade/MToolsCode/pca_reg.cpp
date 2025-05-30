/**
   \file MToolsCode/pca_reg.cpp

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

// this function assumes the independants are orthogonal and hence does piecewise linear regression
void PCA_REG(std::vector<double>& train_data_dependant_, std::vector<std::vector<double> >& train_data_independants_,
             std::vector<int>& included_, std::vector<IndexCoeffPairSimple_t>& finmodel_,
             const double kMinCorrelationIncrement, const std::vector<double>& initial_correlations_,
             const unsigned int max_model_size_, bool ignore_zeros_) {
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
    double sum_dep_independants_chosen_index_ = 0;     ///< Sum_j ( x_ci_j * y_j )
    double sum_square_independants_chosen_index_ = 0;  ///< Sum_j ( x_ci_j * x_ci_j )

    CalcNoMeanSums(train_data_dependant_, train_data_independants_, included_, sum_square_independants_,
                   sum_dep_independants_);

    // choosing index which minimizes squared error most ... equivalent to having maximum fabs ( correlation )
    double max_variance_reduced_ = 0;
    double max_fabs_correlation_ = 0;
    for (unsigned int indep_index_ = 0; indep_index_ < train_data_independants_.size(); indep_index_++) {
      if (included_[indep_index_] == 0) {  // still eligible to be selected
        double this_correlation_ = sum_dep_independants_[indep_index_] /
                                   (sqrt(sum_square_dependant_) * sqrt(sum_square_independants_[indep_index_]));

        if (fabs(this_correlation_) <= kMinCorrelationIncrement) {  // pruning independants that drop below
                                                                    // kMinCorrelationIncrement threshold along the way
          included_[indep_index_] = -1;
          continue;
        }
        if (ignore_zeros_ == true) {
          if ((chosen_index_ == kInvalidArrayIndex) ||
              (fabs(this_correlation_) > max_fabs_correlation_)) {  // store index and value of current best option
            chosen_index_ = indep_index_;
            //      std::cerr << this_correlation_;
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
    // after this step stdev ( train_data_dependant_ ) != 1
    // hence the data is not normalized but just nomean

    IndexCoeffPairSimple_t icp_t_;
    icp_t_.origindex_ = chosen_index_;
    icp_t_.coeff_ = dep_chosen_coeff_;
    finmodel_.push_back(icp_t_);

    // Recursive Step
    if (NextNotIncluded(included_) != kInvalidArrayIndex) {  // there are still indicators eligible to be selected

      // remove effect of chosen_index_ from INDEPs not yet included
      // and store the std::vector of betas to calc the coeff when the recursive call completes
      // in independant_orthogonalization_matrix_ ( chosen_index_, : )
      PCA_REG(train_data_dependant_, train_data_independants_, included_, finmodel_, kMinCorrelationIncrement,
              initial_correlations_, max_model_size_, ignore_zeros_);
    }
  }
}
}
