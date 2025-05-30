/**
    \file MToolsCode/data_processing.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#include <assert.h>
#include "basetrade/Math/gen_utils.hpp"
#include "basetrade/MTools/data_processing.hpp"
#include "dvctrade/linal/linal_util.hpp"

namespace HFSAT {

int NextNotIncluded(const std::vector<int>& included_) {
  for (auto i = 0u; i < included_.size(); i++) {
    if (included_[i] == 0) {
      return (int)i;
    }
  }
  return (int)kInvalidArrayIndex;
}

void ApplyModelCoeffCorrection(std::vector<IndexCoeffPair_t>& finmodel_,
                               const std::vector<std::vector<double> >& indep_chosen_beta_) {
  for (int k = finmodel_.size() - 1; k >= 0; ++k) {
    int chosen_index = finmodel_[k].origindex_;
    for (unsigned int i = (k + 1); i < finmodel_.size(); i++) {  // look at the indicators chosen after this one
      int thisvarindex_ = finmodel_[i].origindex_;
      double thisvarcoeff_ = finmodel_[i].coeff_;
      double thisbeta_ = indep_chosen_beta_[chosen_index][thisvarindex_];  // this was the beta used to remove the
                                                                           // effect of chosen_index_ column from this
                                                                           // indicator
      finmodel_[k].coeff_ += -thisbeta_ * thisvarcoeff_;
    }
  }
}

void ComputeAccurateTstat(const std::vector<std::vector<double> >& indep_, double rss_,
                          std::vector<IndexCoeffPair_t>& model_) {
  LINAL::Matrix indep_matrix_trans(model_.size(), indep_[0].size());
  for (size_t i = 0; i < indep_matrix_trans.getRowDimension(); ++i) {
    for (size_t j = 0; j < indep_matrix_trans.getColumnDimension(); ++j) {
      indep_matrix_trans.set(i, j, indep_[model_[i].origindex_][j]);
    }
  }
  LINAL::Matrix std_err_mat_ = LINAL::getPINV(indep_matrix_trans.times(indep_matrix_trans.transpose()));
  double freedom_degree = (signed)(indep_[0].size() - model_.size());
  for (auto i = 0u; i < model_.size(); i++)
    model_[i].tstat_ = model_[i].coeff_ / sqrt((rss_ / freedom_degree) * std_err_mat_(i, i));
}

void NormalizeDataVec(std::vector<std::vector<double> >& train_data_independants_,
                      const std::vector<double>& mean_indep_, const std::vector<double>& stdev_indep_) {
  for (unsigned int indep_index_ = 0; indep_index_ < train_data_independants_.size(); indep_index_++) {
    VectorUtils::NormalizeData(train_data_independants_[indep_index_], mean_indep_[indep_index_],
                               stdev_indep_[indep_index_]);
  }
}

void NormalizeNonZeroDataVec(std::vector<std::vector<double> >& train_data_independants_,
                             const std::vector<double>& mean_indep_, const std::vector<double>& stdev_indep_,
                             const std::vector<int>& included_) {
  for (unsigned int indep_index_ = 0; indep_index_ < train_data_independants_.size(); indep_index_++) {
    if (included_[indep_index_] == 0) {
      VectorUtils::NormalizeNonZeroData(train_data_independants_[indep_index_], mean_indep_[indep_index_],
                                        stdev_indep_[indep_index_]);
    }
  }
}

void NormalizeDataVec(std::vector<std::vector<double> >& train_data_independants_,
                      const std::vector<double>& mean_indep_, const std::vector<double>& stdev_indep_,
                      const std::vector<int>& included_) {
  for (unsigned int indep_index_ = 0; indep_index_ < train_data_independants_.size(); indep_index_++) {
    if (included_[indep_index_] == 0) {
      VectorUtils::NormalizeData(train_data_independants_[indep_index_], mean_indep_[indep_index_],
                                 stdev_indep_[indep_index_]);
    }
  }
}

void CalcMeanStdevNormalizeDataVec(std::vector<std::vector<double> >& train_data_independants_,
                                   std::vector<double>& mean_indep_, std::vector<double>& stdev_indep_,
                                   const std::vector<int>& included_) {
  mean_indep_.resize(train_data_independants_.size());
  stdev_indep_.resize(train_data_independants_.size());
  assert(included_.size() == train_data_independants_.size());

  for (unsigned int indep_index_ = 0; indep_index_ < train_data_independants_.size(); indep_index_++) {
    if (included_[indep_index_] == 0) {
      VectorUtils::CalcNonZeroMeanStdevNormalizeData(train_data_independants_[indep_index_], mean_indep_[indep_index_],
                                                     stdev_indep_[indep_index_]);
    }
  }
}

void ComputeCorrelationsOfNormalizedDataVec(const std::vector<double>& train_data_dependant_,
                                            const std::vector<std::vector<double> >& train_data_independants_,
                                            std::vector<double>& initial_correlations_,
                                            const std::vector<int>& included_) {
  for (unsigned int indep_index_ = 0; indep_index_ < train_data_independants_.size(); indep_index_++) {
    if (included_[indep_index_] == 0) {
      // first,compute the non zero values for this indicator
      unsigned num_nonzero_ = 0;
      for (unsigned idx_ = 0; idx_ < train_data_dependant_.size(); idx_++) {
        if (double(fabs(train_data_independants_[indep_index_][idx_])) > 0) {
          num_nonzero_++;
        }
      }
      if (num_nonzero_ == 0)  // encountered a bad indicator,the other part of the algorithm is good enough to take care
                              // of this situation
      {
        num_nonzero_ = train_data_dependant_.size();
      }
      // should be (X-muX.Y-muY)/(n-1 * std(X)* std(Y)) For normalized std(X)=std(Y) = 1
      initial_correlations_[indep_index_] =
          (VectorUtils::CalcDotProduct(train_data_dependant_, train_data_independants_[indep_index_])) /
          (double(num_nonzero_) - 1);
    }
  }
}

void CalcNoMeanSums(const std::vector<double>& train_data_dependant_,
                    const std::vector<std::vector<double> >& train_data_independants_,
                    const std::vector<int>& included_, std::vector<double>& sum_square_independants_,
                    std::vector<double>& sum_dep_independants_) {
  for (unsigned int indep_index_ = 0; indep_index_ < train_data_independants_.size(); indep_index_++) {
    if (included_[indep_index_] == 0) {            // still eligible to be selected
      sum_square_independants_[indep_index_] = 0;  // since it might have been set to -1 earlier
      sum_dep_independants_[indep_index_] = 0;
      for (unsigned int dataline_num_ = 0; dataline_num_ < train_data_dependant_.size(); dataline_num_++) {
        sum_square_independants_[indep_index_] += train_data_independants_[indep_index_][dataline_num_] *
                                                  train_data_independants_[indep_index_][dataline_num_];
        sum_dep_independants_[indep_index_] +=
            train_data_independants_[indep_index_][dataline_num_] * train_data_dependant_[dataline_num_];
      }
    } else {
      sum_square_independants_[indep_index_] = 0;
      sum_dep_independants_[indep_index_] = 0;
    }
  }
}

double CalcAutoCorrelation(const std::vector<double> residual_error_, const int window_size_) {
  double sum_square_residuals_ = 0.0;
  double t_sum_ = 0.0;
  unsigned int w_window_size_ = std::max(window_size_, 1);
  std::vector<double> window_avgd_val_;
  double _this_window_avg_ = 0.0;

  for (auto i = 0u; i < residual_error_.size(); i++) {
    _this_window_avg_ += residual_error_[i] / w_window_size_;
    if (i % w_window_size_ == (w_window_size_ - 1)) {
      window_avgd_val_.push_back(_this_window_avg_);
      _this_window_avg_ = 0;
    }
  }

  for (int i = 0; i < (signed)(window_avgd_val_.size() - 1); i++) {
    t_sum_ += window_avgd_val_[i] * window_avgd_val_[i + 1];
  }

  for (auto i = 0u; i < window_avgd_val_.size(); i++) {
    sum_square_residuals_ += window_avgd_val_[i] * window_avgd_val_[i];
  }
  if (sum_square_residuals_ == 0.0) sum_square_residuals_ = (t_sum_ - 2);
  return t_sum_ / sum_square_residuals_;
}

double GetSLRCoeff(const std::vector<double>& dep_, const std::vector<double>& indep_) {
  if (dep_.empty() || indep_.empty() || (dep_.size() != indep_.size())) return 0;

  double mean_dep_ = VectorUtils::GetMean(dep_);
  double mean_indep_ = VectorUtils::GetMean(indep_);

  double covariance_ = 0;
  double invariance_ = 0;

  for (auto i = 0u; i < dep_.size(); i++) {
    covariance_ += (indep_[i] - mean_indep_) * (dep_[i] - mean_dep_);
    invariance_ += GetSquareOf(indep_[i] - mean_indep_);
  }
  return (covariance_ / invariance_);
}

double GetSLRCoeffNoMean(const std::vector<double>& dep_, const std::vector<double>& indep_) {
  if (dep_.empty() || indep_.empty() || (dep_.size() != indep_.size())) {
    return 0;
  }
  double covariance_ = 0;
  double invariance_ = 0;
  for (auto i = 0u; i < dep_.size(); i++) {
    covariance_ += (indep_[i]) * (dep_[i]);
    invariance_ += GetSquareOf(indep_[i]);
  }
  return (covariance_ / invariance_);
}

double GetSHLRCoeffNoMean(const std::vector<double>& dep_, const std::vector<double>& indep_,
                          double t_thresh_value_ = -1) {
  if (dep_.empty() || indep_.empty() || (dep_.size() != indep_.size())) return 0;

  if (t_thresh_value_ < 0) {
    t_thresh_value_ = VectorUtils::GetStdevNoMean(indep_);
  }
  if (t_thresh_value_ <= 0) {
    return 0;
  }

  double covariance_ = 0;
  double invariance_ = 0;

  for (auto i = 0u; i < dep_.size(); i++) {
    if (fabs(indep_[i]) > t_thresh_value_) {
      covariance_ += (indep_[i]) * (dep_[i]);
      invariance_ += GetSquareOf(indep_[i]);
    }
  }

  if (invariance_ <= 0) return 0;

  return (covariance_ / invariance_);
}

double GetSHDLRCoeffNoMean(const std::vector<double>& dep_, const std::vector<double>& indep_,
                           double t_thresh_value_ = -1) {
  if (dep_.empty() || indep_.empty() || (dep_.size() != indep_.size())) return 0;

  if (t_thresh_value_ < 0) {
    t_thresh_value_ = VectorUtils::GetStdevNoMean(dep_);
  }
  if (t_thresh_value_ <= 0) {
    return 0;
  }

  double covariance_ = 0;
  double invariance_ = 0;

  for (auto i = 0u; i < dep_.size(); i++) {
    if (fabs(dep_[i]) > t_thresh_value_) {
      covariance_ += (indep_[i]) * (dep_[i]);
      invariance_ += GetSquareOf(indep_[i]);
    }
  }

  if (invariance_ <= 0) return 0;

  return (covariance_ / invariance_);
}

double GetSRRCoeff(const std::vector<double>& dep_, const std::vector<double>& indep_,
                   const double& regularization_coeff_) {
  if (regularization_coeff_ < 0) return 0;

  double mean_dep_ = VectorUtils::GetMean(dep_);
  double mean_indep_ = VectorUtils::GetMean(indep_);

  double covariance_ = 0;
  double invariance_ = 0;

  for (auto i = 0u; i < dep_.size(); i++) {
    covariance_ += (indep_[i] - mean_indep_) * (dep_[i] - mean_dep_);
    invariance_ += GetSquareOf(indep_[i] - mean_indep_);
  }

  if (invariance_ + regularization_coeff_ <= 0) return 0;

  return (covariance_ / (invariance_ + regularization_coeff_));
}

double GetSRRCoeffNoMean(const std::vector<double>& dep_, const std::vector<double>& indep_,
                         const double& regularization_coeff_) {
  if (regularization_coeff_ < 0) return 0;

  double covariance_ = 0;
  double invariance_ = 0;

  for (auto i = 0u; i < dep_.size(); i++) {
    covariance_ += (indep_[i]) * (dep_[i]);
    invariance_ += GetSquareOf(indep_[i]);
  }

  if (invariance_ + regularization_coeff_ <= 0) return 0;

  return (covariance_ / (invariance_ + regularization_coeff_));
}

void GetSRRCoeffCorrelationNoMean(const std::vector<double>& dep_, const std::vector<double>& indep_,
                                  const double& regularization_coeff_, double& coeff_, double& correlation_) {
  if (regularization_coeff_ < 0) {
    coeff_ = 0;
    correlation_ = 0;
    return;
  }

  double covariance_ = 0;
  double invariance_ = 0;
  double depvariance_ = 0;

  for (auto i = 0u; i < dep_.size(); i++) {
    if (double(fabs(indep_[i])) > 0) {
      covariance_ += (indep_[i]) * (dep_[i]);
      invariance_ += GetSquareOf(indep_[i]);
      depvariance_ += GetSquareOf(dep_[i]);
    }
  }

  coeff_ = (covariance_ / (invariance_ + regularization_coeff_));
  correlation_ = (covariance_ / (sqrt(depvariance_ * invariance_)));
}

double GetLossReduction(const std::vector<double>& dep_, const std::vector<double>& indep_,
                        const double& mult_factor_) {
  double mean_dep_ = VectorUtils::GetMean(dep_);
  double mean_indep_ = VectorUtils::GetMean(indep_);
  register double lossreduction_ = 0;
  for (auto i = 0u; i < dep_.size(); i++) {
    lossreduction_ += GetSquareOf(dep_[i] - mean_dep_) -
                      GetSquareOf((dep_[i] - mean_dep_) - (mult_factor_ * (indep_[i] - mean_indep_)));
  }
  return lossreduction_;
}

double GetLossReductionNoMean(const std::vector<double>& dep_, const std::vector<double>& indep_,
                              const double& mult_factor_) {
  register double lossreduction_ = 0;
  for (auto i = 0u; i < dep_.size(); i++) {
    lossreduction_ += GetSquareOf(dep_[i]) - GetSquareOf(dep_[i] - (mult_factor_ * indep_[i]));
  }
  return lossreduction_;
}

double GetLossReductionNoMeanIndexed(const std::vector<double>& dep_, const std::vector<double>& indep_,
                                     const double& mult_factor_, unsigned int start_index_, unsigned int end_index_) {
  register double lossreduction_ = 0;
  if (dep_.size() == indep_.size() && dep_.size() > 0) {
    if (end_index_ > dep_.size() - 1) {
      end_index_ = dep_.size() - 1;
    }
    for (unsigned int i = start_index_; i <= end_index_; i++) {
      lossreduction_ += GetSquareOf(dep_[i]) - GetSquareOf(dep_[i] - (mult_factor_ * indep_[i]));
    }
  }
  return lossreduction_;
}

double GetCorrelation(const std::vector<double>& dep_, const std::vector<double>& indep_) {
  if (dep_.empty() || indep_.empty() || (dep_.size() != indep_.size())) return 0;

  double mean_dep_ = VectorUtils::GetMean(dep_);
  double mean_indep_ = VectorUtils::GetMean(indep_);

  double covariance_ = 0;
  double invariance_ = 0;
  double depvariance_ = 0;

  for (auto i = 0u; i < dep_.size(); i++) {
    if (double(fabs(indep_[i])) > 0) {
      covariance_ += (indep_[i] - mean_indep_) * (dep_[i] - mean_dep_);
      depvariance_ += GetSquareOf(dep_[i] - mean_dep_);
      invariance_ += GetSquareOf(indep_[i] - mean_indep_);
    }
  }

  if (depvariance_ * invariance_ <= 0) return 0;

  return (covariance_ / (sqrt(depvariance_ * invariance_)));
}

double GetCorrelationNoMean(const std::vector<double>& dep_, const std::vector<double>& indep_) {
  if (dep_.empty() || indep_.empty() || (dep_.size() != indep_.size())) {
    return 0;
  }
  return GetCorrelationNoMeanIndexed(dep_, indep_, 0, dep_.size() - 1);
}

double GetCorrelationNoMeanIndexed(const std::vector<double>& dep_, const std::vector<double>& indep_,
                                   unsigned int start_index_, unsigned int end_index_) {
  double covariance_ = 0;
  double invariance_ = 0;
  double depvariance_ = 0;
  for (unsigned int i = start_index_; i <= end_index_; i++) {
    if (double(fabs(indep_[i])) > 0) {
      covariance_ += (indep_[i]) * (dep_[i]);
      depvariance_ += GetSquareOf(dep_[i]);
      invariance_ += GetSquareOf(indep_[i]);
    }
  }

  if (depvariance_ * invariance_ <= 0) {
    return 0;
  }

  return (covariance_ / (sqrt(depvariance_ * invariance_)));
}

double GetHVCorrelationNoMean(const std::vector<double>& dep_, const std::vector<double>& indep_,
                              double t_thresh_value_ = -1) {
  if (dep_.empty() || indep_.empty() || (dep_.size() != indep_.size())) return 0;

  if (t_thresh_value_ < 0) {
    t_thresh_value_ = VectorUtils::GetStdevNoMean(indep_);
  }
  if (t_thresh_value_ <= 0) {
    return 0;
  }

  double covariance_ = 0;
  double invariance_ = 0;
  double depvariance_ = 0;

  for (auto i = 0u; i < dep_.size(); i++) {
    if (fabs(indep_[i]) > t_thresh_value_) {
      covariance_ += (dep_[i]) * (indep_[i]);
      depvariance_ += GetSquareOf(dep_[i]);
      invariance_ += GetSquareOf(indep_[i]);
    }
  }

  if (depvariance_ * invariance_ <= 0) return 0;

  return (covariance_ / (sqrt(depvariance_ * invariance_)));
}

double GetModelHVCorrelationNoMean(const std::vector<double>& dep_orig_, const std::vector<double>& dep_new_,
                                   const std::vector<std::vector<double> >& indeps_,
                                   const std::vector<IndexCoeffPair_t>& t_finmodel_, double t_thresh_factor_) {
  if (dep_orig_.empty() || dep_new_.empty() || (dep_orig_.size() != dep_new_.size())) return 0;

  std::vector<double> t_thresh_value_;
  for (unsigned int j = 0; j < t_finmodel_.size(); ++j) {
    t_thresh_value_.push_back(t_thresh_factor_ * VectorUtils::GetStdevNoMean(indeps_[t_finmodel_[j].origindex_]));
  }

  double covariance_ = 0;
  double invariance_ = 0;
  double depvariance_ = 0;
  bool checkrow_ = false;

  for (auto i = 0u; i < dep_orig_.size(); i++) {
    checkrow_ = false;
    for (unsigned int j = 0; j < t_finmodel_.size(); ++j)
      if (fabs(indeps_[t_finmodel_[j].origindex_][i]) > t_thresh_value_[j]) {
        checkrow_ = true;
        break;
      }

    if (checkrow_) {
      covariance_ += (dep_orig_[i]) * (dep_new_[i]);
      depvariance_ += GetSquareOf(dep_orig_[i]);
      invariance_ += GetSquareOf(dep_new_[i]);
    }
  }

  if (depvariance_ * invariance_ <= 0) return 0;

  return (covariance_ / (sqrt(depvariance_ * invariance_)));
}

double GetHDVCorrelationNoMean(const std::vector<double>& dep_, const std::vector<double>& indep_,
                               double t_thresh_value_ = -1) {
  if (dep_.empty() || indep_.empty() || (dep_.size() != indep_.size())) return 0;

  if (t_thresh_value_ < 0) {
    t_thresh_value_ = VectorUtils::GetStdevNoMean(dep_);
  }
  if (t_thresh_value_ <= 0) {
    return 0;
  }

  double covariance_ = 0;
  double invariance_ = 0;
  double depvariance_ = 0;

  for (auto i = 0u; i < dep_.size(); i++) {
    if (fabs(dep_[i]) > t_thresh_value_) {
      covariance_ += (dep_[i]) * (indep_[i]);
      depvariance_ += GetSquareOf(dep_[i]);
      invariance_ += GetSquareOf(indep_[i]);
    }
  }

  if (depvariance_ * invariance_ <= 0) return 0;

  return (covariance_ / (sqrt(depvariance_ * invariance_)));
}

double GetMedianNoMeanCorrelation(const std::vector<double>& dep_, const std::vector<double>& indep_,
                                  unsigned int num_folds_) {
  unsigned int total_size_ = dep_.size();
  if (total_size_ != indep_.size()) {
    return 0;
  }
  std::vector<unsigned int> start_indices_;
  std::vector<unsigned int> end_indices_;
  Math::GetBoundariesOfFolds(total_size_, num_folds_, start_indices_, end_indices_);
  std::vector<double> correlation_vector_;
  if (start_indices_.size() == end_indices_.size()) {
    for (auto i = 0u; i < start_indices_.size(); i++) {
      correlation_vector_.push_back(GetCorrelationNoMeanIndexed(dep_, indep_, start_indices_[i], end_indices_[i]));
    }
  }
  return VectorUtils::GetMedian(correlation_vector_);
}

double GetNoMeanCorrelationProdMeanSharpe(const std::vector<double>& dep_, const std::vector<double>& indep_,
                                          unsigned int num_folds_) {
  unsigned int total_size_ = dep_.size();
  if (total_size_ != indep_.size()) {
    return 0;
  }
  std::vector<unsigned int> start_indices_;
  std::vector<unsigned int> end_indices_;
  Math::GetBoundariesOfFolds(total_size_, num_folds_, start_indices_, end_indices_);
  std::vector<double> correlation_vector_;
  if (start_indices_.size() == end_indices_.size()) {
    for (auto i = 0u; i < start_indices_.size(); i++) {
      correlation_vector_.push_back(GetCorrelationNoMeanIndexed(dep_, indep_, start_indices_[i], end_indices_[i]));
    }
  }
  return VectorUtils::GetProdMeanSharpe(correlation_vector_);
}

double GetNoMeanCorrelationProdMeanFracSameSign(const std::vector<double>& dep_, const std::vector<double>& indep_,
                                                unsigned int num_folds_) {
  unsigned int total_size_ = dep_.size();
  if (total_size_ != indep_.size()) {
    return 0;
  }
  std::vector<unsigned int> start_indices_;
  std::vector<unsigned int> end_indices_;
  Math::GetBoundariesOfFolds(total_size_, num_folds_, start_indices_, end_indices_);
  std::vector<double> correlation_vector_;
  if (start_indices_.size() == end_indices_.size()) {
    for (auto i = 0u; i < start_indices_.size(); i++) {
      correlation_vector_.push_back(GetCorrelationNoMeanIndexed(dep_, indep_, start_indices_[i], end_indices_[i]));
    }
  }
  return VectorUtils::GetMeanFracSameSign(correlation_vector_);
}

double GetStableDepVarianceReduced(const std::vector<double>& dep_, const std::vector<double>& indep_,
                                   const double this_beta_, unsigned int num_folds_) {
  unsigned int total_size_ = dep_.size();
  if (total_size_ != indep_.size()) {
    return 0;
  }
  std::vector<unsigned int> start_indices_;
  std::vector<unsigned int> end_indices_;
  Math::GetBoundariesOfFolds(total_size_, num_folds_, start_indices_, end_indices_);
  std::vector<double> variance_reduced_vec_(num_folds_, 0);
  if (start_indices_.size() == end_indices_.size()) {
    for (auto i = 0u; i < start_indices_.size(); i++) {
      variance_reduced_vec_.push_back(
          GetLossReductionNoMeanIndexed(dep_, indep_, this_beta_, start_indices_[i], end_indices_[i]));
    }
  }
  if (variance_reduced_vec_.empty()) {
    return 0;
  }
  if (variance_reduced_vec_.size() < 2) {
    return variance_reduced_vec_[0];
  }
  return VectorUtils::GetProdMeanSharpe(variance_reduced_vec_);
}

void GetSLRCoeffCorrelationNoMean(const std::vector<double>& dep_, const std::vector<double>& indep_, double& coeff_,
                                  double& correlation_) {
  double covariance_ = 0;
  double invariance_ = 0;
  double depvariance_ = 0;

  for (auto i = 0u; i < dep_.size(); i++) {
    if (double(fabs(indep_[i])) > 0) {
      covariance_ += (indep_[i]) * (dep_[i]);
      depvariance_ += GetSquareOf(dep_[i]);
      invariance_ += GetSquareOf(indep_[i]);
    }
  }

  coeff_ = (covariance_ / invariance_);
  correlation_ = (covariance_ / (sqrt(depvariance_ * invariance_)));
}

void ModifyPreComputedCoeffs(std::vector<IndexCoeffPair_t>& finmodel_,
                             const std::vector<std::vector<double> >& coeff_dependancy_matrix_,
                             unsigned int chosen_index_, double dep_chosen_coeff_) {
  for (auto i = 0u; i < finmodel_.size(); i++) {
    unsigned int this_index_ = finmodel_[i].origindex_;
    finmodel_[i].coeff_ += coeff_dependancy_matrix_[this_index_][chosen_index_] * dep_chosen_coeff_;
  }
}

void RevertPreComputedCoeffs(std::vector<IndexCoeffPair_t>& finmodel_,
                             const std::vector<std::vector<double> >& coeff_dependancy_matrix_,
                             unsigned int chosen_index_, double dep_chosen_coeff_) {
  ModifyPreComputedCoeffs(finmodel_, coeff_dependancy_matrix_, chosen_index_, -dep_chosen_coeff_);
}

void IncrementDependancyMatrix(const std::vector<IndexCoeffPair_t>& finmodel_,
                               std::vector<std::vector<double> >& coeff_dependancy_matrix_, unsigned int indep_index_,
                               unsigned int chosen_index_, const double& indep_chosen_beta_) {
  for (auto i = 0u; i < finmodel_.size(); i++) {
    unsigned int this_index_ = finmodel_[i].origindex_;
    coeff_dependancy_matrix_[this_index_][indep_index_] +=
        (coeff_dependancy_matrix_[this_index_][chosen_index_] * indep_chosen_beta_);
  }
}

double GetCoeffLossReduced(const std::vector<int>& included_, const std::vector<IndexCoeffPair_t>& finmodel_,
                           const std::vector<std::vector<double> >& coeff_dependancy_matrix_,
                           const unsigned int indep_index_, double this_SRR_coeff_) {
  double this_coeff_loss_reduced_ = 0;  // ASSUMING that currently coeff is 0, hence squared is 0
  this_coeff_loss_reduced_ += 0 - GetSquareOf(this_SRR_coeff_);
  for (auto i = 0u; i < finmodel_.size(); i++) {
    if (finmodel_[i].origindex_ != indep_index_) {
      double this_coeff_addition_ = this_SRR_coeff_ * coeff_dependancy_matrix_[finmodel_[i].origindex_][indep_index_];
      double this_coeff_prev_value_ = finmodel_[i].coeff_;
      double this_coeff_new_value_ = this_coeff_prev_value_ + this_coeff_addition_;
      this_coeff_loss_reduced_ += GetSquareOf(this_coeff_prev_value_) - GetSquareOf(this_coeff_new_value_);
    }
  }

  return this_coeff_loss_reduced_;
}

void ComputeModelStatisticsOrigNormalized(const std::vector<double>& orig_values_,
                                          const std::vector<double>& final_values_, double& model_rsquared_,
                                          double& model_correlation_, double& stdev_final_dependant_,
                                          double& stdev_model_) {
  double orig_values_l2norm_ = 0;
  double final_values_l2norm_ = 0;
  orig_values_l2norm_ = VectorUtils::CalcL2Norm(orig_values_);
  final_values_l2norm_ = VectorUtils::CalcL2Norm(final_values_);
  model_rsquared_ = (orig_values_l2norm_ - final_values_l2norm_) / orig_values_l2norm_;
  model_correlation_ = sqrt(std::max(0.0, model_rsquared_));
  stdev_final_dependant_ = VectorUtils::GetStdev(final_values_);

  std::vector<double> pred_values_ = orig_values_;
  VectorUtils::ScaledVectorAddition(pred_values_, final_values_, -1.00);
  stdev_model_ = VectorUtils::GetStdev(pred_values_);
}
}
