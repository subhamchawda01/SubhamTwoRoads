/**
    \file MTools/data_processing.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#ifndef BASE_MTOOLS_DATA_PROCESSING_H
#define BASE_MTOOLS_DATA_PROCESSING_H

#include <vector>
#include <stdio.h>
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"

#define ZERO 0.000001

typedef unsigned int uint32_t;

namespace HFSAT {

typedef struct {
  unsigned int origindex_;
  double coeff_;
  double tstat_residual_dependant_;
  double tstat_;
  double RSS_;
  double SXX_;
} IndexCoeffPair_t;

typedef struct {
  unsigned int origindex_;
  double coeff_;
} IndexCoeffPairSimple_t;

typedef struct {
  int origindex_;
  double alpha_;
  double beta_;
  double mse_;
} IndexCoeffPairSigmoid_t;

void ApplyModelCoeffCorrection(std::vector<IndexCoeffPair_t>& finmodel_,
                               const std::vector<std::vector<double> >& indep_chosen_beta_);

void ComputeAccurateTstat(const std::vector<std::vector<double> >& indep_, double rss_,
                          std::vector<IndexCoeffPair_t>& model_);

int NextNotIncluded(const std::vector<int>& included_);

void NormalizeDataVec(std::vector<std::vector<double> >& train_data_independants_,
                      const std::vector<double>& mean_indep_, const std::vector<double>& stdev_indep_);

void NormalizeDataVec(std::vector<std::vector<double> >& train_data_independants_,
                      const std::vector<double>& mean_indep_, const std::vector<double>& stdev_indep_,
                      const std::vector<int>& included_);
void NormalizeNonZeroDataVec(std::vector<std::vector<double> >& train_data_independants_,
                             const std::vector<double>& mean_indep_, const std::vector<double>& stdev_indep_,
                             const std::vector<int>& included_);

void CalcMeanStdevNormalizeDataVec(std::vector<std::vector<double> >& train_data_independants_,
                                   std::vector<double>& mean_indep_, std::vector<double>& stdev_indep_,
                                   const std::vector<int>& included_);

void ComputeCorrelationsOfNormalizedDataVec(const std::vector<double>& train_data_dependant_,
                                            const std::vector<std::vector<double> >& train_data_independants_,
                                            std::vector<double>& initial_correlations_,
                                            const std::vector<int>& included_);

void CalcNoMeanSums(const std::vector<double>& train_data_dependant_,
                    const std::vector<std::vector<double> >& train_data_independants_,
                    const std::vector<int>& included_, std::vector<double>& sum_square_independants_,
                    std::vector<double>& sum_dep_independants_);
double CalcAutoCorrelation(const std::vector<double> residual_error_, const int window_size_);

/// Returns the linear regression coefficient of indep_ towards dep_
double GetSLRCoeff(const std::vector<double>& dep_, const std::vector<double>& indep_);

/// Returns the linear regression coefficient of indep_ towards dep_, assuming both are mean-zero
double GetSLRCoeffNoMean(const std::vector<double>& dep_, const std::vector<double>& indep_);

/// Returns the linear regression coefficient of indep_ towards dep_, only on High-Value-Indep lines, assuming both are
/// mean-zero
double GetSHLRCoeffNoMean(const std::vector<double>& dep_, const std::vector<double>& indep_, double t_thresh_value_);

/// Returns the linear regression coefficient of indep_ towards dep_, only on High-Value-Dep lines, assuming both are
/// mean-zero
double GetSHDLRCoeffNoMean(const std::vector<double>& dep_, const std::vector<double>& indep_, double t_thresh_value_);

/// Returns the linear ridge regression coefficient of indep_ towards dep_, for the given regularization_coeff_
double GetSRRCoeff(const std::vector<double>& dep_, const std::vector<double>& indep_,
                   const double& regularization_coeff_);

/// Returns the linear ridge regression coefficient of NoMean indep_ towards dep_, for the given regularization_coeff_
double GetSRRCoeffNoMean(const std::vector<double>& dep_, const std::vector<double>& indep_,
                         const double& regularization_coeff_);

/// Function to compute the correlation and SRR coefficient of independant with dependant, assuming both dep_ and indep_
/// are mean-zero series
void GetSRRCoeffCorrelationNoMean(const std::vector<double>& dep_, const std::vector<double>& indep_,
                                  const double& regularization_coeff_, double& coeff_, double& correlation_);

/// Returns the reduction in variance of the dep_ after including the given indep_ with given mult_factor_
double GetLossReduction(const std::vector<double>& dep_, const std::vector<double>& indep_, const double& mult_factor_);

/// Returns the reduction in variance of the dep_ after including the given indep_ with given mult_factor_. Assuming bot
/// hdep_ and indep_ are NoMean
double GetLossReductionNoMean(const std::vector<double>& dep_, const std::vector<double>& indep_,
                              const double& mult_factor_);

/// Returns the reduction in variance of the dep_ after including the given indep_ with given mult_factor_. Assuming bot
/// hdep_ and indep_ are NoMean
double GetLossReductionNoMeanIndexed(const std::vector<double>& dep_, const std::vector<double>& indep_,
                                     const double& mult_factor_, unsigned int start_index_, unsigned int end_index_);

/// Computes the correlation of indep_ with dep_
double GetCorrelation(const std::vector<double>& dep_, const std::vector<double>& indep_);

/// Computes the correlation of indep_ with dep_, assuming bot hare mean-zero series
double GetCorrelationNoMean(const std::vector<double>& dep_, const std::vector<double>& indep_);

/// Internal Function used by GetCorrelationNoMean and GetMedianNoMeanCorrelation
double GetCorrelationNoMeanIndexed(const std::vector<double>& dep_, const std::vector<double>& indep_,
                                   unsigned int start_index_, unsigned int end_index_);

/// Computes the correlation of indep_ with dep_ only on samples where the fabs-indep-value exceeds given threshold,
/// assuming both are mean-zero series
double GetHVCorrelationNoMean(const std::vector<double>& dep_, const std::vector<double>& indep_,
                              double t_thresh_value_);

/// Computes model correlation over high values of chosen indeps only
double GetModelHVCorrelationNoMean(const std::vector<double>& dep_orig_, const std::vector<double>& dep_new_,
                                   const std::vector<std::vector<double> >& indeps_,
                                   const std::vector<IndexCoeffPair_t>& t_finmodel_, double t_thresh_factor_);

/// Computes the correlation of indep_ with dep_ only on samples where the fabs-dep-value exceeds given threshold,
/// assuming both are mean-zero series
double GetHDVCorrelationNoMean(const std::vector<double>& dep_, const std::vector<double>& indep_,
                               double t_thresh_value_);

/// Computes the median of the num_folds_ correlations between Y, X
double GetMedianNoMeanCorrelation(const std::vector<double>& dep_, const std::vector<double>& indep_,
                                  unsigned int num_folds_);

double GetNoMeanCorrelationProdMeanSharpe(const std::vector<double>& dep_, const std::vector<double>& indep_,
                                          unsigned int num_folds_);

double GetNoMeanCorrelationProdMeanFracSameSign(const std::vector<double>& dep_, const std::vector<double>& indep_,
                                                unsigned int num_folds_);

double GetStableDepVarianceReduced(const std::vector<double>& dep_, const std::vector<double>& indep_,
                                   const double this_beta_, unsigned int num_folds_);

/// Function to compute the correlation and SLR coefficient of independant with dependant, assuming both dep_ and indep_
/// are mean zero series
void GetSLRCoeffCorrelationNoMean(const std::vector<double>& dep_, const std::vector<double>& indep_, double& coeff_,
                                  double& correlation_);

/// adjust precomputed coeffs based on the new coeff and the dependancy matrix
void ModifyPreComputedCoeffs(std::vector<IndexCoeffPair_t>& finmodel_,
                             const std::vector<std::vector<double> >& coeff_dependancy_matrix_,
                             unsigned int chosen_index_, double dep_chosen_coeff_);
void RevertPreComputedCoeffs(std::vector<IndexCoeffPair_t>& finmodel_,
                             const std::vector<std::vector<double> >& coeff_dependancy_matrix_,
                             unsigned int chosen_index_, double dep_chosen_coeff_);

/// coeff_dependancy_matrix_[chosen_index_ ][new_var_index_]says by how much coeff of variable chosen_index_ should by
/// incremented for unit addition to coeff of variable new_var_index_
/// since we are orthogonalizing indep_index_ against chosen_index_ update the coeff_dependancy_matrix_
void IncrementDependancyMatrix(const std::vector<IndexCoeffPair_t>& finmodel_,
                               std::vector<std::vector<double> >& coeff_dependancy_matrix_, unsigned int indep_index_,
                               unsigned int chosen_index_, const double& indep_chosen_beta_);

/// figure out how much Sum ( coeff^2 ) would increase by addition of this_SRR_coeff_ to coeff of independant ( or
/// indicator ) at indep_index_ [ ASSUMING the current coeff of indep_index_ = 0 ]
double GetCoeffLossReduced(const std::vector<int>& included_, const std::vector<IndexCoeffPair_t>& finmodel_,
                           const std::vector<std::vector<double> >& coeff_dependancy_matrix_,
                           const unsigned int indep_index_, double this_SRR_coeff_);

/// assuming that the data was normalized makes calculating rsquared simple
void ComputeModelStatisticsOrigNormalized(const std::vector<double>& orig_values_,
                                          const std::vector<double>& final_values_, double& model_rsquared_,
                                          double& model_correlation_, double& stdev_final_dependant_,
                                          double& stdev_model_);
}
#endif  // BASE_MTOOLS_DATA_PROCESSING_H
