/**
   \file MToolsCode/fslr_so_eff.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
*/
#include <algorithm>
#include "dvccode/CDef/math_utils.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "basetrade/MTools/data_processing.hpp"
#include "basetrade/MTools/iterative_regress.hpp"

namespace HFSAT {

struct mystruc {
  unsigned int indep_index_;
  double corr_with_dep_;
  bool operator==(const mystruc& _newmystruc_) { return (indep_index_ == _newmystruc_.indep_index_); }
  //  bool operator < ( const mystruc & _newmystruc_ ) const { return ( indep_index_ < _newmystruc_.indep_index_ ) ; }
  static bool mycompare(const mystruc& _s1, const mystruc& _s2) { return (_s1.corr_with_dep_ > _s2.corr_with_dep_); }
};

std::vector<unsigned int> GetSortedIndicesToConsider(SquareMatrix<double>& corr_matrix_, std::vector<int>& included_,
                                                     const double kMinCorrelationIncrement) {
  std::vector<mystruc> v;
  for (auto i = 0u; i < included_.size(); i++) {
    if ((included_[i] == 0) && (fabs(corr_matrix_(0, i + 1)) > kMinCorrelationIncrement)) {
      mystruc ms;
      ms.indep_index_ = i;
      ms.corr_with_dep_ = fabs(corr_matrix_(0, i + 1));

      v.push_back(ms);
    } else if (fabs(corr_matrix_(0, i + 1)) <= kMinCorrelationIncrement) {
      included_[i] = -1;
    }
  }
  std::vector<unsigned int> sorted_indices_;
  if (!v.empty()) {
    std::sort(v.begin(), v.end(), mystruc::mycompare);
    for (auto i = 0u; i < v.size(); i++) {
      sorted_indices_.push_back(v[i].indep_index_);
    }
  }
  return sorted_indices_;
}

// this function performs forward stagewise linear regression of non-NoMean data by successive orthogonalization of
// independants
// pruning independants that drop below kMinCorrelationIncrement threshold along the way
void FSLR_SO_eff(SquareMatrix<double>& covar_matrix_, SquareMatrix<double>& corr_matrix_, std::vector<int>& included_,
                 std::vector<IndexCoeffPair_t>& finmodel_, const double kMinCorrelationIncrement,
                 const double kMaxIndepCorrelation, const unsigned int max_model_size_,
                 const unsigned int count_lines_) {
  if (finmodel_.size() >= max_model_size_) return;

  if (NextNotIncluded(included_) != kInvalidArrayIndex) {  // there are still indicators eligible to be selected

    std::vector<unsigned int> sorted_indices_to_consider_ =
        GetSortedIndicesToConsider(corr_matrix_, included_, kMinCorrelationIncrement);

    if (sorted_indices_to_consider_.empty()) return;

    unsigned int chosen_indep_index_ = sorted_indices_to_consider_[0];
    unsigned int chosen_covar_index_ = chosen_indep_index_ + 1u;

    included_[chosen_indep_index_] = 1;  // marking it ineligible for future

    // std::cout << " This guy got chosen.."<< chosen_indep_index_ << std::endl;

    // SLR coeff for nomean data = Sum_j ( x_ci_j * y_j ) / Sum_j ( x_ci_j * x_ci_j )
    double dep_chosen_coeff_ =
        covar_matrix_(0, chosen_covar_index_) / covar_matrix_(chosen_covar_index_, chosen_covar_index_);

    IndexCoeffPair_t icp_t_;
    icp_t_.origindex_ = chosen_indep_index_;
    icp_t_.coeff_ = dep_chosen_coeff_;

    double n_2_ = count_lines_ - 2;
    double n_1_ = count_lines_ - 1;
    double RSS_ = n_1_ * (covar_matrix_(0, 0) - (GetSquareOf(covar_matrix_(0, chosen_covar_index_)) /
                                                 covar_matrix_(chosen_covar_index_, chosen_covar_index_)));
    double SXX_ = n_1_ * (covar_matrix_(chosen_covar_index_, chosen_covar_index_));
    icp_t_.tstat_residual_dependant_ = dep_chosen_coeff_ / sqrt((RSS_ / n_2_) / SXX_);

    icp_t_.tstat_ = dep_chosen_coeff_ / sqrt((RSS_ / n_2_) / n_1_);

    icp_t_.RSS_ = RSS_;
    icp_t_.SXX_ = SXX_;
    // std::cout << " TSTAT: "<< icp_t_.tstat_ << std::endl;
    // std::cout << " INDX: "<< icp_t_.origindex_ << std::endl;
    // std::cout << " COEFF: "<< icp_t_.coeff_ << std::endl;
    // std::cout << " RSS: "<< icp_t_.RSS_ << std::endl;
    // std::cout << " SXX_: "<< icp_t_.SXX_ << std::endl;
    // std::cout << " TST RES: "<< dep_chosen_coeff_ / sqrt ( ( RSS_ / n_2_ ) / SXX_ )<< std::endl;

    unsigned int this_finmodel_index_ = finmodel_.size();
    finmodel_.push_back(icp_t_);

    // change the covar_matrix_ for inclusion of chosen_indep_index_
    // covar_matrix_(0,0) = covar_matrix_(0,0) - ( GetSquareOf ( covar_matrix_(0,chosen_covar_index_) ) / covar_matrix_
    // ( chosen_covar_index_, chosen_covar_index_ ) ) ; // now moved to loop ahead
    for (auto i = 0u; i < covar_matrix_.row_count(); i++) {
      for (unsigned int j = i; j < covar_matrix_.row_count(); j++) {
        if (((i == 0) || (included_[i - 1] == 0)) && ((j == 0) || (included_[j - 1] == 0))) {
          covar_matrix_(i, j) =
              covar_matrix_(i, j) - (covar_matrix_(i, chosen_covar_index_) * covar_matrix_(j, chosen_covar_index_) /
                                     covar_matrix_(chosen_covar_index_, chosen_covar_index_));
          if (i != j) {
            covar_matrix_(j, i) = covar_matrix_(i, j);
            // double denom = sqrt ( covar_matrix_(i,i) * covar_matrix_(j,j) );
            // if ( denom > 0 )
            //   {
            //     corr_matrix_(i,j) = covar_matrix_(i,j)/denom;
            //     corr_matrix_(j,i) = corr_matrix_(i,j);
            //   }
            // else
            //   {
            //     corr_matrix_(j,i) = corr_matrix_(i,j) = 0;
            //   }
          }
        }
      }
    }
    // Recursive Step
    if (NextNotIncluded(included_) != kInvalidArrayIndex) {  // there are still indicators eligible to be selected

      // remove effect of chosen_indep_index_ from INDEPs not yet included
      // and store the std::vector of betas to calc the coeff when the recursive call completes.
      std::vector<double> indep_chosen_beta_(included_.size(), 0);
      for (unsigned int indep_index_ = 0; indep_index_ < included_.size(); indep_index_++) {
        if (included_[indep_index_] == 0) {  // this indicator is still eligible to be picked
          double this_indep_correlation_ = corr_matrix_(indep_index_ + 1u, chosen_covar_index_);
          if (fabs(this_indep_correlation_) >= kMaxIndepCorrelation) {
            included_[indep_index_] = -1;
          } else {
            indep_chosen_beta_[indep_index_] = covar_matrix_(indep_index_ + 1u, chosen_covar_index_) /
                                               covar_matrix_(chosen_covar_index_, chosen_covar_index_);
          }
        }
      }

      for (auto i = 0u; i < covar_matrix_.row_count(); i++) {
        for (unsigned int j = i; j < covar_matrix_.row_count(); j++) {
          if (((i == 0) || (included_[i - 1] == 0)) && ((j == 0) || (included_[j - 1] == 0))) {
            double denom = sqrt(covar_matrix_(i, i) * covar_matrix_(j, j));
            if (denom > 0) {
              double prev_corr = corr_matrix_(i, j);
              corr_matrix_(i, j) = covar_matrix_(i, j) / denom;
              corr_matrix_(j, i) = corr_matrix_(i, j);
              // Only for the first row, interested to find out column vectors
              // who have changed their correlation sign with the dependent vector
              if (prev_corr * corr_matrix_(i, j) < 0 && j > 0 && i == 0) {
                included_[j - 1] = -1;
                // included_[j -1] = -1;
              }
            } else {
              corr_matrix_(j, i) = corr_matrix_(i, j) = 0;
            }
          }
        }
      }

      FSLR_SO_eff(covar_matrix_, corr_matrix_, included_, finmodel_, kMinCorrelationIncrement, kMaxIndepCorrelation,
                  max_model_size_, count_lines_);

      for (unsigned int i = (this_finmodel_index_ + 1); i < finmodel_.size();
           i++) {  // look at the indicators chosen after this one
        int thisvarindex_ = finmodel_[i].origindex_;
        if ((thisvarindex_ >= 0) && (thisvarindex_ < (int)included_.size())) {
          double thisvarcoeff_ = finmodel_[i].coeff_;
          double thisbeta_ = indep_chosen_beta_[thisvarindex_];  // this was the beta used to remove the effect of
                                                                 // chosen_indep_index_ column from this indicator
          double c_thisbeta_ = covar_matrix_(thisvarindex_ + 1u, chosen_covar_index_) /
                               covar_matrix_(chosen_covar_index_, chosen_covar_index_);
          if (c_thisbeta_ != thisbeta_) {
            std::cerr << "should have been same!" << std::endl;
            exit(0);
          }
          finmodel_[this_finmodel_index_].coeff_ += -thisbeta_ * thisvarcoeff_;
        }
      }
    }
  }
}
}
