/**
    \file MTools/iterative_regress.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#ifndef BASE_MTOOLS_ITERATIVE_REGRESS_H
#define BASE_MTOOLS_ITERATIVE_REGRESS_H

#include <vector>
#include "dvccode/CDef/math_utils.hpp"
#include "basetrade/Math/gen_utils.hpp"
#include "basetrade/Math/matrix_utils.hpp"
#include "basetrade/MTools/data_processing.hpp"
#include "basetrade/MTools/earth_model_stats.hpp"
#include "basetrade/MTools/covariance_mat_inverse.hpp"

namespace HFSAT {

/// this function performs forward stagewise high-value linear regression of nomean data
/// by successive orthogonalization of independants,
/// pruning independants that drop below kMinCorrelationIncrement threshold along the way
/// only looking at High ABS Indep Value lines for computation of correlation
void FSHLR_SO_NoMeanData(std::vector<double>& train_data_dependant_,
                         std::vector<std::vector<double> >& train_data_independants_, int must_add_next_k_indeps_,
                         std::vector<int>& included_, std::vector<IndexCoeffPair_t>& finmodel_,
                         const double kMinCorrelationIncrement, const double kMaxIndepCorrelation,
                         const std::vector<double>& initial_correlations_, const unsigned int max_model_size_,
                         const double thresh_factor_,
                         const std::vector<std::vector<double> >& train_data_independants_original_,
                         double tstat_cutoff_, bool ignore_zeros_ = false);

/// this function performs forward stagewise high-value linear regression of nomean data
/// by successive orthogonalization of independants,
/// pruning independants that drop below kMinCorrelationIncrement threshold along the way
/// only looking at High ABS Dep Value lines for computation of correlation
void FSHDVLR_SO_NoMeanData(std::vector<double>& train_data_dependant_,
                           std::vector<std::vector<double> >& train_data_independants_, int must_add_next_k_indeps_,
                           std::vector<int>& included_, std::vector<IndexCoeffPair_t>& finmodel_,
                           const double kMinCorrelationIncrement, const double kMaxIndepCorrelation,
                           const std::vector<double>& initial_correlations_, const unsigned int max_model_size_,
                           const double thresh_factor_,
                           const std::vector<std::vector<double> >& train_data_independants_original_,
                           double tstat_cutoff_, bool ignore_zeros_ = false);

/// this function performs forward stagewise high-value linear regression of nomean data
/// by successive orthogonalization of independants,
/// pruning independants that drop below kMinCorrelationIncrement threshold along the way
/// only looking at High ABS Dep Value lines for computation of correlation
/// Also does backward pass after each forward pass
void FSHDVLR_SO_NoMeanData_Backpass(const std::vector<double>& train_data_orig_dependant_,
                                    const std::vector<std::vector<double> >& train_data_orig_independants_,
                                    std::vector<double>& train_data_dependant_,
                                    std::vector<std::vector<double> >& train_data_independants_,
                                    int must_add_next_k_indeps_, std::vector<int>& included_,
                                    std::vector<IndexCoeffPair_t>& finmodel_, const double kMinCorrelationIncrement,
                                    const double kMaxIndepCorrelation, const std::vector<double>& initial_correlations_,
                                    const unsigned int max_model_size_, const double thresh_factor_,
                                    double tstat_cutoff_, bool ignore_zeros_ = false);

/// this function performs forward stagewise linear regression of nomean data
/// by successive orthogonalization of independants
/// pruning independants that drop below kMinCorrelationIncrement threshold along the way
void FSLR_SO_NoMeanData(std::vector<double>& train_data_dependant_,
                        std::vector<std::vector<double> >& train_data_independants_,
                        unsigned int must_add_next_k_indeps_, std::vector<int>& included_,
                        std::vector<IndexCoeffPair_t>& finmodel_, const double kMinCorrelationIncrement,
                        const double kMaxIndepCorrelation, const std::vector<double>& initial_correlations_,
                        const unsigned int max_model_size_, std::vector<double>& predicted_values_,
                        std::vector<std::vector<double> >& independant_orthogonalization_matrix_,
                        const std::vector<std::vector<double> >& train_data_independants_original_,
                        double tstat_threshold_, bool ignore_zeros_ = false);

/// this function performs forward stagewise linear regression of nomean data
/// by successive orthogonalization of independants
/// pruning independants that drop below kMinCorrelationIncrement threshold along the way
/// Also does backward pass after each forward pass
void FSLR_SO_NoMeanData_Backpass(const std::vector<double>& train_data_orig_dependant_,
                                 const std::vector<std::vector<double> >& train_data_orig_independants_,
                                 std::vector<double>& train_data_dependant_,
                                 std::vector<std::vector<double> >& train_data_independants_,
                                 int must_add_next_k_indeps_, std::vector<int>& included_,
                                 std::vector<IndexCoeffPair_t>& finmodel_, const double kMinCorrelationIncrement,
                                 const double kMaxIndepCorrelation, const std::vector<double>& initial_correlations_,
                                 const unsigned int max_model_size_, double tstat_cutoff_, bool ignore_zeros_ = false);

/// this function performs forward stagewise linear regression of non-nomean data
/// by successive orthogonalization of independants
/// pruning independants that drop below kMinCorrelationIncrement threshold along the way
void FSLR_SO(std::vector<double>& train_data_dependant_, std::vector<std::vector<double> >& train_data_independants_,
             int must_add_next_k_indeps_, std::vector<int>& included_, std::vector<IndexCoeffPair_t>& finmodel_,
             const double kMinCorrelationIncrement, const double kMaxIndepCorrelation,
             const std::vector<double>& initial_correlations_, const unsigned int max_model_size_,
             const std::vector<std::vector<double> >& train_data_independants_original_);

void FSLR_SO_eff(HFSAT::SquareMatrix<double>& covar_matrix_, HFSAT::SquareMatrix<double>& corr_matrix_,
                 std::vector<int>& included_, std::vector<IndexCoeffPair_t>& finmodel_,
                 const double kMinCorrelationIncrement, const double kMaxIndepCorrelation,
                 const unsigned int max_model_size_, const unsigned int count_lines_);

/// this function performs forward stagewise ridge regression of non-normalized data
/// by successive orthogonalization of independants
/// pruning independants that drop below kMinCorrelationIncrement threshold along the way
void FSRR_SO_NoMeanData(std::vector<double>& train_data_dependant_,
                        std::vector<std::vector<double> >& train_data_independants_, int must_add_next_k_indeps_,
                        std::vector<int>& included_, std::vector<IndexCoeffPair_t>& finmodel_,
                        std::vector<std::vector<double> >& coeff_dependancy_matrix_,
                        const double kMinCorrelationIncrement, const double regularization_coeff_,
                        const double kMaxIndepCorrelation, const std::vector<double>& initial_correlations_,
                        const unsigned int max_model_size_,
                        const std::vector<std::vector<double> >& train_data_independants_original_, double tsat_cutoff_,
                        bool ignore_zeros_ = false);

// function for forward stagewise logistic regression
void FS_Logistic_Regression(std::vector<int>& train_data_dependant_,
                            std::vector<std::vector<double> >& train_data_independants_, std::vector<int>& included_,
                            const double kMaxIndepCorrelation, std::vector<std::vector<double> >& model_,
                            std::vector<int>& model_indep_indx_, const unsigned int max_model_size_);

/// this function performs forward stagewise "robust" linear regression of nomean data
/// by successive orthogonalization of independants
/// pruning independants that drop below kMinCorrelationIncrement threshold along the way.
/// Only difference from FSLR is that all correlations are replaced with median correlation.
void FSRLM_NoMeanData(std::vector<double>& train_data_dependant_,
                      std::vector<std::vector<double> >& train_data_independants_, int must_add_next_k_indeps_,
                      std::vector<int>& included_, std::vector<IndexCoeffPair_t>& finmodel_,
                      const unsigned int num_folds_, const double kMinCorrelationIncrement,
                      const double kMaxIndepCorrelation, const std::vector<double>& initial_correlations_,
                      const unsigned int max_model_size_);

/// this function performs forward stagewise "robust" linear regression of nomean data
/// by successive orthogonalization of independants
/// pruning independants that drop below kMinCorrelationIncrement threshold along the way.
/// Only difference from FSLR is that all correlations are replaced with mean * sharpe of correlation of folds
void FSR_Mean_Sharpe_Corr(std::vector<double>& train_data_dependant_,
                          std::vector<std::vector<double> >& train_data_independants_, int must_add_next_k_indeps_,
                          std::vector<int>& included_, std::vector<IndexCoeffPair_t>& finmodel_,
                          const unsigned int num_folds_, const double kMinCorrelationIncrement,
                          const double kMaxIndepCorrelation, const std::vector<double>& initial_correlations_,
                          const unsigned int max_model_size_);

/// this function performs forward stagewise "robust" linear regression of nomean data
/// by successive orthogonalization of independants
/// pruning independants that drop below kMinCorrelationIncrement threshold along the way.
/// Only difference from FSLR is that the sorting criteria is mean * abs(sharpe) of variance reduced
/// in folds of data
void FSR_sharpe_rsq(std::vector<double>& train_data_dependant_,
                    std::vector<std::vector<double> >& train_data_independants_, int must_add_next_k_indeps_,
                    std::vector<int>& included_, std::vector<IndexCoeffPair_t>& finmodel_,
                    const unsigned int num_folds_, const double kMinCorrelationIncrement,
                    const double kMaxIndepCorrelation, const std::vector<double>& initial_correlations_,
                    const unsigned int max_model_size_);

/// this function performs forward stagewise "robust" linear regression of nomean data
/// by successive orthogonalization of independants
/// pruning independants that drop below kMinCorrelationIncrement threshold along the way.
/// Only difference from FSLR is that all correlations are replaced with mean * frac_same_sign of correlation of folds
void FSR_Mean_FSS_Corr(std::vector<double>& train_data_dependant_,
                       std::vector<std::vector<double> >& train_data_independants_, int must_add_next_k_indeps_,
                       std::vector<int>& included_, std::vector<IndexCoeffPair_t>& finmodel_,
                       const unsigned int num_folds_, const double kMinCorrelationIncrement,
                       const double kMaxIndepCorrelation, const std::vector<double>& initial_correlations_,
                       const unsigned int max_model_size_);

void PCA_REG(std::vector<double>& train_data_dependant_, std::vector<std::vector<double> >& train_data_independants_,
             std::vector<int>& included_, std::vector<IndexCoeffPairSimple_t>& finmodel_,
             const double kMinCorrelationIncrement, const std::vector<double>& initial_correlations_,
             const unsigned int max_model_size_, bool ignore_zeros_ = false);

void FSLRSQ_NM(std::vector<double>& train_data_dependant_, std::vector<std::vector<double> >& train_data_independants_,
               unsigned int must_add_next_k_indeps_, std::vector<int>& included_,
               std::vector<IndexCoeffPair_t>& finmodel_, const double kMinRsquaredIncrement,
               const unsigned int SortCriterion, const double variance_orig_dependant_,
               const double variance_dependant_,
               std::vector<double>& max_abs_correlations_sel_,  // this refers to the maximum abs ( corr ) of remaining
                                                                // indicators against already selected ones
               const std::vector<double>& initial_correlations_, const unsigned int max_model_size_,
               std::vector<double>& predicted_values_,
               std::vector<std::vector<double> >& independant_orthogonalization_matrix_);

inline bool CheckTstatAndAddToModel(const std::vector<double>& train_data_dependant_,
                                    const std::vector<std::vector<double> > train_data_independants_original_,
                                    size_t chosen_index_, double dep_chosen_coeff_, double RSS_, double SXX_,
                                    const std::vector<std::vector<double> >& coeff_dependancy_matrix_,
                                    double tstat_cutoff_, std::vector<IndexCoeffPair_t>& finmodel_,
                                    bool isRidgeRegression = false) {
  HFSAT::CovMatInv& covMat = HFSAT::CovMatInv::GetUniqueInstance(train_data_independants_original_);

  bool rejectForTstat = false;
  double deg_freedom = train_data_dependant_.size() - (finmodel_.size() + 1);
  // Residual standard error square
  double sigma2 = RSS_ / deg_freedom;

  covMat.AddIndex(chosen_index_);
  std::vector<double> covInvDiag = covMat.inv_mat.diagonal();
  std::vector<double> updatedCoeff(finmodel_.size(), 0);
  std::vector<double> updatedTstat(finmodel_.size(), 0);

  if (isRidgeRegression) {
    for (auto i = 0u; i < finmodel_.size(); ++i) {
      updatedCoeff[i] =
          finmodel_[i].coeff_ + coeff_dependancy_matrix_[finmodel_[i].origindex_][chosen_index_] * dep_chosen_coeff_;
    }
  } else {
    double delta = dep_chosen_coeff_;
    for (unsigned int i = finmodel_.size() - 1; 1 + i; --i) {
      delta *= coeff_dependancy_matrix_[finmodel_[i].origindex_][chosen_index_];
      updatedCoeff[i] = finmodel_[i].coeff_ - delta;
    }
  }
  for (auto i = 0u; i < finmodel_.size() && rejectForTstat == false; ++i) {
    updatedTstat[i] = updatedCoeff[i] / sqrt(covInvDiag[i] * sigma2);
    if (fabs(updatedTstat[i]) < tstat_cutoff_) {
      std::cout << "Updated tstat for coeff " << finmodel_[i].origindex_ << "falls below threshold " << tstat_cutoff_
                << "\n";
      rejectForTstat = true;
    }
    if (IS_SAME_SIGN(updatedCoeff[i], finmodel_[i].coeff_) == false) {
      std::cout << "Updated coeff is of different sign for " << finmodel_[i].origindex_ << "\n";
      rejectForTstat = true;
    }
  }
  double dep_chosen_tstat_ = dep_chosen_coeff_ / sqrt(covInvDiag.back() * sigma2);
  // std::cerr << "chosen_index_ : "<< chosen_index_ << ", sigma2 : " << sigma2 << ", tstat : " << dep_chosen_tstat_ <<
  // ", dep_chosen_coeff_ : " << dep_chosen_coeff_ << ", RSS_ : " << RSS_ << ", SXX_ : " << SXX_ << ", add_success_ :"<<
  // add_success_ << ", covInvDiag.back() : " << covInvDiag.back() << "\n";
  if (fabs(dep_chosen_tstat_) < tstat_cutoff_) {
    std::cout << "tstat for coeff " << chosen_index_ << "falls below threshold " << tstat_cutoff_ << "\n";
    rejectForTstat = true;
  }
  if (rejectForTstat) {
    covMat.UndoLastAdd();
    return false;
  }
  // update coeff and tstat of existing components
  for (size_t i = 0; i < finmodel_.size(); ++i) {
    finmodel_[i].tstat_ = updatedTstat[i];
    finmodel_[i].coeff_ = updatedCoeff[i];
  }

  // Add the new component to the model
  IndexCoeffPair_t icp_t_;
  icp_t_.origindex_ = chosen_index_;
  icp_t_.coeff_ = dep_chosen_coeff_;
  icp_t_.tstat_residual_dependant_ = dep_chosen_coeff_ / sqrt(sigma2 / SXX_);
  icp_t_.tstat_ = dep_chosen_tstat_;

  // unsigned int this_finmodel_index_ = finmodel_.size ( ) ;
  finmodel_.push_back(icp_t_);

  return true;
}

inline bool CheckTstat(const std::vector<double>& train_data_dependant_,
                       const std::vector<std::vector<double> > train_data_independants_original_, size_t chosen_index_,
                       double dep_chosen_coeff_, double RSS_, double SXX_,
                       const std::vector<std::vector<double> >& coeff_dependancy_matrix_, double tstat_cutoff_,
                       std::vector<IndexCoeffPair_t>& finmodel_, bool isRidgeRegression = false) {
  HFSAT::CovMatInv& covMat = HFSAT::CovMatInv::GetUniqueInstance(train_data_independants_original_);

  bool rejectForTstat = false;
  double deg_freedom = train_data_dependant_.size() - (finmodel_.size() + 1);
  // Residual standard error square
  double sigma2 = RSS_ / deg_freedom;

  std::vector<double> covInvDiag = covMat.NoAddIndex(chosen_index_);
  // std::vector<double> covInvDiag = covMat.inv_mat.diagonal();
  std::vector<double> updatedCoeff(finmodel_.size(), 0);
  std::vector<double> updatedTstat(finmodel_.size(), 0);

  if (isRidgeRegression) {
    for (auto i = 0u; i < finmodel_.size(); ++i) {
      updatedCoeff[i] =
          finmodel_[i].coeff_ + coeff_dependancy_matrix_[finmodel_[i].origindex_][chosen_index_] * dep_chosen_coeff_;
    }
  } else {
    double delta = dep_chosen_coeff_;
    for (unsigned int i = finmodel_.size() - 1; 1 + i; --i) {
      delta *= coeff_dependancy_matrix_[finmodel_[i].origindex_][chosen_index_];
      updatedCoeff[i] = finmodel_[i].coeff_ - delta;
    }
  }
  for (auto i = 0u; i < finmodel_.size() && rejectForTstat == false; ++i) {
    updatedTstat[i] = updatedCoeff[i] / sqrt(covInvDiag[i] * sigma2);
    if (fabs(updatedTstat[i]) < tstat_cutoff_) {
      std::cout << "Updated tstat for coeff " << finmodel_[i].origindex_ << "falls below threshold " << tstat_cutoff_
                << "\n";
      rejectForTstat = true;
    }
    if (IS_SAME_SIGN(updatedCoeff[i], finmodel_[i].coeff_) == false) {
      std::cout << "Updated coeff is of different sign for " << finmodel_[i].origindex_ << "\n";
      rejectForTstat = true;
    }
  }
  double dep_chosen_tstat_ = dep_chosen_coeff_ / sqrt(covInvDiag.back() * sigma2);
  // std::cerr << "chosen_index_ : "<< chosen_index_ << ", sigma2 : " << sigma2 << ", tstat : " << dep_chosen_tstat_ <<
  // ", dep_chosen_coeff_ : " << dep_chosen_coeff_ << ", RSS_ : " << RSS_ << ", SXX_ : " << SXX_ << ", add_success_ :"<<
  // add_success_ << ", covInvDiag.back() : " << covInvDiag.back() << "\n";
  if (fabs(dep_chosen_tstat_) < tstat_cutoff_) {
    std::cout << "tstat for coeff " << chosen_index_ << "falls below threshold " << tstat_cutoff_ << "\n";
    rejectForTstat = true;
  }

  if (rejectForTstat) {
    covMat.UndoLastAdd();
    return false;
  }
  return true;
}

// Multivariate Adaptive Regression Splines
#ifdef MARS_DEG_2_TERMS  // should be defined or not in included header file earth_models_stats.hpp
void MARS_SO(std::vector<double>& train_data_dependant_, std::vector<std::vector<double> >& train_data_independants_,
             const unsigned int max_model_size_, std::vector<double>& initial_correlations_, std::vector<Term1>& Model,
             ModelStats_t& model_stats_);
#else
void MARS_SO(std::vector<double>& train_data_dependant_, std::vector<std::vector<double> >& train_data_independants_,
             const unsigned int max_model_size_, std::vector<double>& initial_correlations_, std::vector<Term>& Model,
             ModelStats_t& model_stats_);
#endif
}
#endif  // BASE_MTOOLS_ITERATIVE_REGRESS_H
