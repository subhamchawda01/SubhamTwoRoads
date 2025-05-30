/**
   \file dvccode/CommonDataStructures/vector_utils_weighted.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#ifndef BASE_COMMONDATASTRUCTURES_VECTOR_UTILS_WEIGHTED_H
#define BASE_COMMONDATASTRUCTURES_VECTOR_UTILS_WEIGHTED_H

#include <iostream>
#include <cstdlib>

#include <vector>
#include <map>
#include <algorithm>
#include <complex>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/math_utils.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"

namespace HFSAT {

namespace VectorUtils {

template <typename T>
inline T GetWeightedSum(const std::vector<T>& train_data_, std::vector<double> weights_vec_) {
  if (train_data_.size() != weights_vec_.size()) {
    return GetSum(train_data_);
  }
  T sum_values_ = 0;
  for (auto i = 0u; i < train_data_.size(); i++) {
    sum_values_ += train_data_[i] * weights_vec_[i];
  }
  return sum_values_;
}

template <typename T>
inline T GetWeightedSumLosses(const std::vector<T>& train_data_, std::vector<double> weights_vec_) {
  if (train_data_.size() != weights_vec_.size()) {
    return GetSumLosses(train_data_);
  }
  T sum_values_ = 0;
  for (auto i = 0u; i < train_data_.size(); i++) {
    if (train_data_[i] < 0) {
      sum_values_ += train_data_[i] * weights_vec_[i];
    }
  }
  return sum_values_;
}

/// Returns the mean value of the given vector
template <typename T>
inline T GetWeightedMean(const std::vector<T>& src_vec_, const std::vector<double>& weights_vec_) {
  if (src_vec_.size() != weights_vec_.size()) {
    return GetMean(src_vec_);
  }
  if (src_vec_.empty()) return 0;  // need to check this else the return value will be ill defined

  T retval_ = 0;
  double total_weight_ = 0;
  for (auto i = 0u; i < src_vec_.size(); i++) {
    retval_ += src_vec_[i] * weights_vec_[i];
    total_weight_ += weights_vec_[i];
  }
  if (total_weight_ != 0) {
    return (retval_ / total_weight_);
  } else {
    return 0;
  }
}

template <typename T>
inline T GetWeightedNonZeroMean(const std::vector<T>& src_vec_, const std::vector<double>& weights_vec_) {
  if (src_vec_.size() != src_vec_.size()) {
    return GetNonZeroMean(src_vec_);
  }
  if (src_vec_.empty()) return 0;  // need to check this else the return value will be ill defined

  T retval_ = 0;
  unsigned int total_weight_non_zero_ = 0;
  for (auto i = 0u; i < src_vec_.size(); i++) {
    retval_ += src_vec_[i] * total_weight_non_zero_;
    if (src_vec_[i] != 0) {
      total_weight_non_zero_ += weights_vec_[i];
    }
  }
  if (total_weight_non_zero_ != 0)  // pathological case but non uncommon if one is careless
  {
    return (retval_ / (total_weight_non_zero_));
  } else {
    return 0;
  }
}

/// Returns the mean value of the given vector
template <typename T>
inline T GetWeightedMeanHighestQuartile(const std::vector<T>& src_vec_, const std::vector<double>& weights_vec_) {
  if (src_vec_.size() != weights_vec_.size()) {
    return GetMeanHighestQuartile(src_vec_);
  }
  if (src_vec_.empty()) return 0;  // need to check this else the return value will be ill defined

  std::vector<size_t> idx(src_vec_.size());
  for (size_t i = 0; i != idx.size(); ++i) idx[i] = i;

  std::sort(idx.begin(), idx.end(), [&src_vec_](size_t i1, size_t i2) { return src_vec_[i1] < src_vec_[i2]; });

  double sum_weights_ = 0;
  for (size_t i = 0; i != idx.size(); ++i) sum_weights_ += weights_vec_[idx[i]];

  unsigned int threefourth_index_ = 0;
  double cumul_weights_ = 0;
  for (size_t i = 0; i != idx.size(); ++i) {
    if (cumul_weights_ >= (sum_weights_ * 0.75)) {
      threefourth_index_ = i;
      break;
    }
    cumul_weights_ += weights_vec_[idx[i]];
  }

  T retval_ = 0;
  double total_weight_ = 0;
  for (size_t i = threefourth_index_; i < idx.size(); ++i) {
    retval_ += src_vec_[idx[i]] * weights_vec_[idx[i]];
    total_weight_ += weights_vec_[i];
  }
  if (total_weight_ != 0) {
    return (retval_ / total_weight_);
  } else {
    return 0;
  }
}

/// Returns the mean value of the given vector
template <typename T>
inline T GetWeightedMeanLowestQuartile(const std::vector<T>& src_vec_, const std::vector<double>& weights_vec_) {
  if (src_vec_.size() != weights_vec_.size()) {
    return GetMeanLowestQuartile(src_vec_);
  }
  if (src_vec_.empty()) return 0;  // need to check this else the return value will be ill defined

  std::vector<size_t> idx(src_vec_.size());
  for (size_t i = 0; i != idx.size(); ++i) idx[i] = i;

  std::sort(idx.begin(), idx.end(), [&src_vec_](size_t i1, size_t i2) { return src_vec_[i1] < src_vec_[i2]; });

  double sum_weights_ = 0;
  for (size_t i = 0; i != idx.size(); ++i) sum_weights_ += weights_vec_[idx[i]];

  unsigned int onefourth_index_ = 0;
  double cumul_weights_ = 0;
  for (size_t i = 0; i != idx.size(); ++i) {
    if ((cumul_weights_ + weights_vec_[idx[i]]) > (sum_weights_ * 0.25)) {
      onefourth_index_ = i;
      break;
    }
    cumul_weights_ += weights_vec_[idx[i]];
  }

  T retval_ = 0;
  double total_weight_ = 0;
  for (size_t i = 0; i < onefourth_index_; ++i) {
    retval_ += src_vec_[idx[i]] * weights_vec_[idx[i]];
    total_weight_ += weights_vec_[i];
  }
  if (total_weight_ != 0) {
    return (retval_ / total_weight_);
  } else {
    return 0;
  }
}

/// Returns the standard deviation of the given vector src_vec_
template <typename T>
inline T GetWeightedStdev(const std::vector<T>& src_vec_, const std::vector<double>& weights_vec_) {
  if (src_vec_.size() != weights_vec_.size()) {
    return GetStdev(src_vec_);
  }

  if (src_vec_.size() >= 2) {
    const T sample_mean = GetWeightedMean(src_vec_, weights_vec_);
    T sample_sumsquarevalues = 0;
    double total_weight_ = 0;
    int nonzero_numweights_ = 0;
    for (auto i = 0u; i < src_vec_.size(); i++) {
      sample_sumsquarevalues += GetSquareOf(src_vec_[i] - sample_mean) * weights_vec_[i];
      total_weight_ += weights_vec_[i];
      if (weights_vec_[i] != 0) {
        nonzero_numweights_++;
      }
    }

    if (nonzero_numweights_ <= 1) {
      return 0;
    }
    double denom_estimator_ = total_weight_ * (nonzero_numweights_ - 1) / nonzero_numweights_;
    double sample_variance_ = std::max(0.0, sample_sumsquarevalues) / (denom_estimator_);
    return sqrt(sample_variance_);
  }
  return 0;
}

/// Returns the standard deviation of the given vector src_vec_
template <typename T>
inline T GetWeightedStdevAndMean(const std::vector<T>& src_vec_, T& sample_mean,
                                 const std::vector<double>& weights_vec_) {
  if (src_vec_.size() != weights_vec_.size()) {
    return GetStdevAndMean(src_vec_, sample_mean);
  }

  if (src_vec_.size() >= 2) {
    sample_mean = GetWeightedMean(src_vec_, weights_vec_);
    T sample_sumsquarevalues = 0;
    double total_weight_ = 0;
    int nonzero_numweights_ = 0;
    for (auto i = 0u; i < src_vec_.size(); i++) {
      sample_sumsquarevalues += GetSquareOf(src_vec_[i] - sample_mean) * weights_vec_[i];
      total_weight_ += weights_vec_[i];
      if (weights_vec_[i] != 0) {
        nonzero_numweights_++;
      }
    }

    if (nonzero_numweights_ <= 1) {
      return 0;
    }
    double denom_estimator_ = total_weight_ * (nonzero_numweights_ - 1) / nonzero_numweights_;
    double sample_variance_ = std::max(0.0, sample_sumsquarevalues) / (denom_estimator_);
    return sqrt(sample_variance_);
  }
  return 0;
}

// consider only the non-zero points in mean calculation
template <typename T>
inline T GetWeightedNonZeroStdevAndMean(const std::vector<T>& src_vec_, T& sample_mean,
                                        const std::vector<double>& weights_vec_) {
  if (src_vec_.size() != weights_vec_.size()) {
    return GetNonZeroStdevAndMean(src_vec_, sample_mean);
  }

  if (src_vec_.size() >= 2) {
    sample_mean = GetWeightedNonZeroMean(src_vec_);
    T sample_sumsquarevalues = 0;
    double total_weight_ = 0;
    int nonzero_numweights_ = 0;
    for (auto i = 0u; i < src_vec_.size(); i++) {
      if (src_vec_[i] != 0) {
        sample_sumsquarevalues += GetSquareOf(src_vec_[i] - sample_mean) * weights_vec_[i];
        total_weight_ += weights_vec_[i];
        if (weights_vec_[i] != 0) {
          nonzero_numweights_++;
        }
      }
    }

    if (nonzero_numweights_ <= 1) {  // absurdly pathological case
      return 0;
    }
    double denom_estimator_ = total_weight_ * (nonzero_numweights_ - 1) / nonzero_numweights_;
    double sample_variance_ = std::max(0.0, sample_sumsquarevalues) / (denom_estimator_);
    return sqrt(sample_variance_);
  }
  return 0;
}

/// Returns the sample skewness of the given vector src_vec_, and sample_mean, and sample_stdev
template <typename T>
inline T GetWeightedSkewness(const std::vector<T>& src_vec_, const T sample_mean, const T sample_stdev,
                             const std::vector<double>& weights_vec_) {
  if (src_vec_.size() != weights_vec_.size()) {
    return GetSkewness(src_vec_, sample_mean, sample_stdev);
  }

  if (src_vec_.size() >= 2) {
    T sample_sumcubevalues = 0;
    double total_weight_ = 0;
    int nonzero_numweights_ = 0;
    for (auto i = 0u; i < src_vec_.size(); i++) {
      sample_sumcubevalues += pow((src_vec_[i] - sample_mean), 3) * weights_vec_[i];
      total_weight_ += weights_vec_[i];
      if (weights_vec_[i] != 0) {
        nonzero_numweights_++;
      }
    }

    if (nonzero_numweights_ <= 1) {  // absurdly pathological case
      return 0;
    }
    double denom_estimator_ = total_weight_ * (nonzero_numweights_ - 1) / nonzero_numweights_;
    return (sample_sumcubevalues / denom_estimator_) / pow(sample_stdev, 3);
  }
  return 0;
}

/// Assume vector sorted in ascending order
/// remove max and count min twice, return average
template <typename T>
inline T GetWeightedConservativeAverage(const std::vector<T>& src_vec_, const std::vector<double>& weights_vec_) {
  if (src_vec_.size() != weights_vec_.size()) {
    return GetConservativeAverage(src_vec_);
  }

  T retval_ = src_vec_[0] * weights_vec_[0];
  double total_weight_ = weights_vec_[0];
  if (src_vec_.empty()) return 0;  // need to check this else the return value will be ill defined
  for (auto i = 0u; i < (src_vec_.size() - 1 /* skipping max */); i++) {
    retval_ += src_vec_[i] * weights_vec_[i];
    total_weight_ += weights_vec_[i];
  }
  if (total_weight_ != 0) {
    return (retval_ / total_weight_);
  } else {
    return 0;
  }
}

/// Assume vector sorted in ascending order
/// Removing the 20% max values and counting bottom 20% values twice, returns average
template <typename T>
inline T GetWeightedConservativeAverageFifth(const std::vector<T>& src_vec_, const std::vector<double>& weights_vec_) {
  if (src_vec_.size() != weights_vec_.size()) {
    return GetConservativeAverageFifth(src_vec_);
  }
  if (src_vec_.empty()) return 0;  // need to check this else the return value will be ill defined

  T retval_ = 0;
  double total_weight_ = 0;
  unsigned int onefifth = (unsigned int)std::max(1.00, round(double(src_vec_.size()) / 5.0));

  for (auto i = 0u; i < onefifth; i++) {
    retval_ += src_vec_[i] * weights_vec_[i];
    total_weight_ += weights_vec_[i];
  }

  for (auto i = 0u; i < (src_vec_.size() - onefifth); i++) {
    retval_ += src_vec_[i] * weights_vec_[i];
    total_weight_ += weights_vec_[i];
  }

  if (total_weight_ != 0) {
    return (retval_ / total_weight_);
  } else {
    return 0;
  }
}

/// Assume src_vec is not sorted but for efficient implementatation sort src_vec before calling and set _already_sorted_
/// flag to true
/// returns average of the middle 50% of the values
template <typename T>
inline T GetWeightedMedianAverage(const std::vector<T>& src_vec_, bool _already_sorted_,
                                  const std::vector<double>& weights_vec_) {
  if (src_vec_.size() != weights_vec_.size()) {
    return GetMedianAverage(src_vec_);
  }
  if (src_vec_.empty()) return 0;  // need to check this else the return value will be ill defined

  std::vector<size_t> idx(src_vec_.size());
  for (size_t i = 0; i != idx.size(); ++i) idx[i] = i;

  if (!_already_sorted_) {
    std::sort(idx.begin(), idx.end(), [&src_vec_](size_t i1, size_t i2) { return src_vec_[i1] < src_vec_[i2]; });
  }

  double sum_weights_ = 0;
  for (size_t i = 0; i != idx.size(); ++i) sum_weights_ += weights_vec_[idx[i]];

  unsigned int onefourth_index_ = 0;
  unsigned int threefourth_index_ = 0;
  double cumul_weights_ = 0;
  size_t i = 0;
  for (; i != idx.size(); ++i) {
    if ((cumul_weights_ + weights_vec_[idx[i]]) > (sum_weights_ * 0.25)) {
      onefourth_index_ = i;
      break;
    }
    cumul_weights_ += weights_vec_[idx[i]];
  }
  for (; i != idx.size(); ++i) {
    if (cumul_weights_ >= (sum_weights_ * 0.75)) {
      threefourth_index_ = i;
      break;
    }
    cumul_weights_ += weights_vec_[idx[i]];
  }

  T retval_ = 0;
  double total_weight_ = 0;
  for (size_t i = onefourth_index_; i < threefourth_index_; ++i) {
    retval_ += src_vec_[idx[i]] * weights_vec_[idx[i]];
    total_weight_ += weights_vec_[i];
  }
  if (total_weight_ != 0) {
    return (retval_ / total_weight_);
  } else {
    return 0;
  }
}

/// returns median of the values
template <typename T>
inline T GetWeightedMedian(const std::vector<T>& src_vec_, const std::vector<double>& weights_vec_) {
  if (src_vec_.size() != weights_vec_.size()) {
    return GetMedian(src_vec_);
  }
  if (src_vec_.empty()) return 0;  // need to check this else the return value will be ill defined

  std::vector<size_t> idx(src_vec_.size());
  for (size_t i = 0; i != idx.size(); ++i) idx[i] = i;

  std::sort(idx.begin(), idx.end(), [&src_vec_](size_t i1, size_t i2) { return src_vec_[i1] < src_vec_[i2]; });

  double sum_weights_ = 0;
  for (size_t i = 0; i != idx.size(); ++i) sum_weights_ += weights_vec_[idx[i]];

  unsigned int median_index_ = 0;
  double cumul_weights_ = 0;
  for (size_t i = 0; i != idx.size(); ++i) {
    if ((cumul_weights_ + weights_vec_[idx[i]]) > (sum_weights_ / 2)) {
      median_index_ = idx[i];
      break;
    }
    cumul_weights_ += weights_vec_[idx[i]];
  }

  return src_vec_[median_index_];
}

/// returns product of mean and sharpe
template <typename T>
inline T GetWeightedProdMeanSharpe(const std::vector<T>& src_vec_, const std::vector<double>& weights_vec_) {
  T mean_value_ = 0;
  T stdev_value_ = GetWeightedStdevAndMean(src_vec_, mean_value_, weights_vec_);
  if (stdev_value_ > 0) {
    return (mean_value_ > 0) ? (mean_value_ * mean_value_ / stdev_value_) : (-mean_value_ * mean_value_ / stdev_value_);
  } else {
    return mean_value_;
  }
}
}
}

#endif  // BASE_COMMONDATASTRUCTURES_VECTOR_UTILS_WEIGHTED_H
