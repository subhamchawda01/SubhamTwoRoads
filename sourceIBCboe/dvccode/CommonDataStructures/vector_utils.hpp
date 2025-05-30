/**
   \file dvccode/CommonDataStructures/vector_utils.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#ifndef BASE_COMMONDATASTRUCTURES_VECTOR_UTILS_H
#define BASE_COMMONDATASTRUCTURES_VECTOR_UTILS_H

#include <iostream>
#include <cstdlib>

#include <vector>
#include <map>
#include <algorithm>
#include <complex>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/math_utils.hpp"

namespace HFSAT {

namespace VectorUtils {
template <typename T>
inline T GetAbsMax(const std::vector<T>& train_data_) {
  T max_value_ = 0;
  for (auto i = 0u; i < train_data_.size(); i++) {
    max_value_ = std::max(max_value_, fabs(train_data_[i]));
  }
  return max_value_;
}

template <typename T>
inline T GetMax(const std::vector<T>& train_data_) {
  T max_value_ = 0;
  for (auto i = 0u; i < train_data_.size(); i++) {
    max_value_ = std::max(max_value_, train_data_[i]);
  }
  return max_value_;
}

template <typename T>
inline T GetMin(const std::vector<T>& train_data_) {
  T min_value_ = 0;
  for (auto i = 0u; i < train_data_.size(); i++) {
    min_value_ = std::min(min_value_, train_data_[i]);
  }
  return min_value_;
}

template <typename T>
inline T GetSum(const std::vector<T>& train_data_) {
  T sum_values_ = 0;
  for (auto i = 0u; i < train_data_.size(); i++) {
    sum_values_ += train_data_[i];
  }
  return sum_values_;
}

template <typename T>
inline std::string Join(const std::vector<T>& t_value_vec_, const std::string& _delim_) {
  if (t_value_vec_.empty()) {
    return "";
  }

  std::ostringstream temp_oss_;
  temp_oss_ << t_value_vec_[0];
  for (unsigned int i = 1; i < t_value_vec_.size(); i++) {
    temp_oss_ << _delim_ << t_value_vec_[i];
  }
  return temp_oss_.str();
}

template <typename T>
inline T GetSumLosses(const std::vector<T>& t_value_vec_) {
  T sum_losses_ = 0;
  for (auto i = 0u; i < t_value_vec_.size(); i++) {
    if (t_value_vec_[i] < 0) {
      sum_losses_ += t_value_vec_[i];
    }
  }
  return sum_losses_;
}

/*! Set all values in _class_vec_ to _new_item_
 * \param _class_vec_ The vector to be filled
 * \param _new_item_ The value to be set
 * \return void */
template <typename T>
inline void FillInValue(std::vector<T>& _class_vec_, const T& _new_item_) {
  for (auto i = 0u; i < _class_vec_.size(); i++) {
    _class_vec_[i] = _new_item_;
  }
}

/*! Check if all values in _class_vec_ are set to _new_item_
 * \param _class_vec_ The vector to be checked
 * \param _new_item_ The value to be asserted
 * \return bool:true if all values are = _new_item_  */
template <typename T>
inline bool CheckAllForValue(const std::vector<T>& _class_vec_, const T& _new_item_) {
  for (auto i = 0u; i < _class_vec_.size(); i++) {
    if (_class_vec_[i] != _new_item_) {
      return false;
    }
  }
  return true;
}

/// Searches for the existence of the given item in the vector
template <typename T>
inline bool LinearSearchValue(const std::vector<T>& _class_vec_, const T& _new_item_) {
  for (auto i = 0u; i < _class_vec_.size(); i++) {
    if (_class_vec_[i] == _new_item_) {
      return true;
    }
  }
  return false;
}

/// Searches for the existence of the given item in the vector and returns index
template <typename T>
inline unsigned int LinearSearchValueIdx(const std::vector<T>& _class_vec_, const T& _new_item_) {
  for (auto i = 0u; i < _class_vec_.size(); i++) {
    if (_class_vec_[i] == _new_item_) {
      return i;
    }
  }
  return _class_vec_.size();
}

template <typename T>
inline unsigned int LinearSearchValueIdx(const std::vector<std::vector<T> >& _class_vec_, const T& _new_item_) {
  for (auto i = 0u; i < _class_vec_.size(); i++) {
    for (unsigned j = 0; j < _class_vec_[i].size(); j++) {
      if (_class_vec_[i][j] == _new_item_) {
        return i;
      }
    }
  }
  return _class_vec_.size();
}

template <typename T>
inline std::vector<unsigned int> LinearSearchValueIdxVec(const std::vector<std::vector<T> >& _class_vec_,
                                                         const T& _new_item_) {
  std::vector<unsigned> index_vector_;
  index_vector_.resize(0);

  for (auto i = 0u; i < _class_vec_.size(); i++) {
    for (unsigned j = 0; j < _class_vec_[i].size(); j++) {
      if (_class_vec_[i][j] == _new_item_) {
        index_vector_.push_back(i);
        ;
      }
    }
  }

  return index_vector_;
}

template <typename T>
inline bool UniqueVectorAddFirst(std::vector<T>& _class_vec_, const T& _new_item_) {
  for (auto it = _class_vec_.begin(); it != _class_vec_.end(); it++) {
    if (*it == _new_item_) {
      _class_vec_.erase(it);
      break;
    }
  }
  _class_vec_.insert(_class_vec_.begin(), _new_item_);
  return true;
}

/// If the given value is not found, then and only then adds it to vector
/// Needs T to have a comparison operator defined
template <typename T>
inline bool UniqueVectorAdd(std::vector<T>& _class_vec_, const T& _new_item_) {
  for (auto i = 0u; i < _class_vec_.size(); i++) {
    if (_class_vec_[i] == _new_item_)  // hence need T to have a comparison operator defined
    {
      return false;
    }
  }
  _class_vec_.push_back(_new_item_);
  return true;
}

/// For each value in the vector _new_item_vec_, check if it's present in _class_vec_ and if not then add it
template <typename T>
inline void UniqueVectorAdd(std::vector<T>& _class_vec_, const std::vector<T>& _new_item_vec_) {
  for (auto i = 0u; i < _new_item_vec_.size(); i++) {
    UniqueVectorAdd(_class_vec_, _new_item_vec_[i]);
  }
}

/// If the given item _new_item_ is present in _class_vec_, then remove it
template <typename T>
inline bool UniqueVectorRemove(std::vector<T>& _class_vec_, const T& _new_item_) {
  typename std::vector<T>::iterator _class_vec_iter_ = _class_vec_.begin();
  for (; _class_vec_iter_ != _class_vec_.end(); _class_vec_iter_++) {
    if ((*_class_vec_iter_) == _new_item_)  // hence need T to have a comparison operator defined
    {
      _class_vec_iter_ = _class_vec_.erase(_class_vec_iter_);
      return true;
    }
  }
  return false;
}

/// Given a vector and an iterator, get element at that point, send it to back
template <typename T>
inline void SendItemToBack(std::vector<T>& t_input_vec_, typename std::vector<T>::iterator t_iter_) {
  if (t_iter_ != t_input_vec_.end()) {
    T t_item_ = *t_iter_;  // copy this item

    typename std::vector<T>::iterator t_next_iter_ = t_iter_;
    t_next_iter_++;  // advance by 1
    if (t_next_iter_ != t_input_vec_.end()) {
      t_iter_ = t_input_vec_.erase(t_iter_);
      t_input_vec_.push_back(t_item_);
    }
  }
}

/// Assuming vector _dest_vec_ and _src_vec_ have the same length, adds ( _src_vec_[j] * _mult_factor_ ) to
/// _dest_vec_[j]
template <typename T>
inline void ScaledVectorAddition(std::vector<T>& _dest_vec_, const std::vector<T>& _src_vec_, const T& _mult_factor_) {
  for (auto i = 0u; i < _dest_vec_.size(); i++) {
    _dest_vec_[i] += _src_vec_[i] * _mult_factor_;
  }
}

/// Given a vector, return the sorted list of indices
template <class T>
struct index_cmp {
  index_cmp(const T& arr) : arr(arr) {}
  bool operator()(const size_t a, const size_t b) const { return fabs(arr[a]) > fabs(arr[b]); }
  const T& arr;
};

template <typename T>
void index_sort(const std::vector<T>& unsorted, std::vector<size_t>& index_map) {
  // Original unsorted index map
  for (size_t i = 0; i < unsorted.size(); i++) {
    index_map.push_back(i);
  }
  // Sort the index map, using unsorted for comparison
  sort(index_map.begin(), index_map.end(), index_cmp<std::vector<T> >(unsorted));
}

template <class T>
struct signed_index_cmp {
  signed_index_cmp(const T& arr) : arr(arr) {}
  bool operator()(const size_t a, const size_t b) const { return (arr[a] > arr[b]); }
  const T& arr;
};

template <typename T>
void signed_index_sort(const std::vector<T>& unsorted, std::vector<size_t>& index_map) {
  for (size_t i = 0; i < unsorted.size(); i++) {
    index_map.push_back(i);
  }
  sort(index_map.begin(), index_map.end(), signed_index_cmp<std::vector<T> >(unsorted));
}

/// Multiplies given _mult_factor_ to each element of _dest_vec_
template <typename T>
inline void VectorMult(std::vector<T>& _dest_vec_, const T& _mult_factor_) {
  for (auto i = 0u; i < _dest_vec_.size(); i++) {
    _dest_vec_[i] = _dest_vec_[i] * _mult_factor_;
  }
}

/// Used in call to transform in BaseOrderManager
struct RetrieveKey {
  template <typename T>
  typename T::first_type operator()(T keyValuePair) const {
    return keyValuePair.first;
  }
};

/// Function takes two sorted and unique vectors and merges them
/// maintaining sorting and keeping the output unique
template <typename T>
inline void UniqueMergeUniqueSortedVectorsGreater(const std::vector<T>& _T_sorted_vec_1_,
                                                  const std::vector<T>& _T_sorted_vec_2_,
                                                  std::vector<T>& _T_sorted_vec_out_) {
  typename std::vector<T>::const_iterator _tsv1_citer_ = _T_sorted_vec_1_.begin();
  typename std::vector<T>::const_iterator _tsv2_citer_ = _T_sorted_vec_2_.begin();
  while ((_tsv1_citer_ != _T_sorted_vec_1_.end()) || (_tsv2_citer_ != _T_sorted_vec_2_.end())) {
    if (_tsv1_citer_ ==
        _T_sorted_vec_1_.end()) {  // if we are at the end of vector 1, then just cat vector 2 into final and done
      for (; _tsv2_citer_ != _T_sorted_vec_2_.end(); _tsv2_citer_++) {
        _T_sorted_vec_out_.push_back(*_tsv2_citer_);
      }
      break;
    }

    if (_tsv2_citer_ ==
        _T_sorted_vec_2_.end()) {  // if at the end of vector 2, then cat vector 1 to end of _T_sorted_vec_out_ and done
      for (; _tsv1_citer_ != _T_sorted_vec_1_.end(); _tsv1_citer_++) {
        _T_sorted_vec_out_.push_back(*_tsv1_citer_);
      }
      break;
    }

    if (*_tsv2_citer_ >
        *_tsv1_citer_) {  // if top item of vec1 is strictly smaller, then take item from 2 and advance marker
      _T_sorted_vec_out_.push_back(*_tsv2_citer_);
      _tsv2_citer_++;
    } else if (*_tsv2_citer_ <
               *_tsv1_citer_) {  // if top item of vec2 is strictly smaller, then take item from 1 and advance marker
      _T_sorted_vec_out_.push_back(*_tsv1_citer_);
      _tsv1_citer_++;
    } else {  // both tops are same, then take 1 copy of these items and advance both markers. Sine we know that given
              // vectors are strictly unique this item shall not encountered again
      _T_sorted_vec_out_.push_back(*_tsv2_citer_);
      _tsv2_citer_++;
      _tsv1_citer_++;
    }
  }
}

/// Function takes two sorted and unique vectors and merges them
/// maintaining sorting and keeping the output unique
template <typename T>
inline void UniqueMergeUniqueSortedVectorsLess(const std::vector<T>& _T_sorted_vec_1_,
                                               const std::vector<T>& _T_sorted_vec_2_,
                                               std::vector<T>& _T_sorted_vec_out_) {
  typename std::vector<T>::const_iterator _tsv1_citer_ = _T_sorted_vec_1_.begin();
  typename std::vector<T>::const_iterator _tsv2_citer_ = _T_sorted_vec_2_.begin();
  while ((_tsv1_citer_ != _T_sorted_vec_1_.end()) || (_tsv2_citer_ != _T_sorted_vec_2_.end())) {
    if (_tsv1_citer_ == _T_sorted_vec_1_.end()) {
      for (; _tsv2_citer_ != _T_sorted_vec_2_.end(); _tsv2_citer_++) {
        _T_sorted_vec_out_.push_back(*_tsv2_citer_);
      }
      break;
    }

    if (_tsv2_citer_ == _T_sorted_vec_2_.end()) {
      for (; _tsv1_citer_ != _T_sorted_vec_1_.end(); _tsv1_citer_++) {
        _T_sorted_vec_out_.push_back(*_tsv1_citer_);
      }
      break;
    }

    if (*_tsv2_citer_ < *_tsv1_citer_) {
      _T_sorted_vec_out_.push_back(*_tsv2_citer_);
      _tsv2_citer_++;
    } else if (*_tsv2_citer_ > *_tsv1_citer_) {
      _T_sorted_vec_out_.push_back(*_tsv1_citer_);
      _tsv1_citer_++;
    } else {  // *_tsv2_citer_ == *_tsv1_citer_
      _T_sorted_vec_out_.push_back(*_tsv2_citer_);
      _tsv2_citer_++;
      _tsv1_citer_++;
    }
  }
}

/// Returns the mean value of the given vector
template <typename T>
inline T GetMean(const std::vector<T>& src_vec_) {
  T retval_ = 0;
  if (src_vec_.empty()) return 0;  // need to check this else the return value will be ill defined
  for (auto i = 0u; i < src_vec_.size(); i++) {
    retval_ += src_vec_[i];
  }
  return (retval_ / (T)(src_vec_.size()));
}

/// Returns the standard deviation of the given vector src_vec_
template <typename T>
inline T GetMeanSquared(const std::vector<T>& src_vec_) {
  if (src_vec_.size() >= 2) {
    T sample_sumsquarevalues = 0;
    for (auto i = 0u; i < src_vec_.size(); i++) {
      sample_sumsquarevalues += GetSquareOf(src_vec_[i]);
    }
    double mean_squared_ = sample_sumsquarevalues / ((double)((int)src_vec_.size()));
    return mean_squared_;
  }
  return 0;
}

template <typename T>
inline T GetNonZeroMean(const std::vector<T>& src_vec_) {
  T retval_ = 0;
  if (src_vec_.empty()) return 0;  // need to check this else the return value will be ill defined
  unsigned int count_non_zero_ = 0;
  for (auto i = 0u; i < src_vec_.size(); i++) {
    retval_ += src_vec_[i];
    if (src_vec_[i] != 0) {
      count_non_zero_++;
    }
  }
  if (count_non_zero_ != 0)  // pathological case but non uncommon if one is careless
  {
    return (retval_ / (T)(count_non_zero_));
  } else {
    return 0;
  }
}

/// Returns the mean value of the given vector
template <typename T>
inline T GetMeanHighestQuartile(const std::vector<T>& src_vec_) {
  T retval_ = 0;
  if (src_vec_.empty()) {
    return 0;  // need to check this else the return value will be ill defined
  }
  std::vector<T> copy_src_vec_(src_vec_);
  std::sort(copy_src_vec_.begin(), copy_src_vec_.end());
  unsigned int start_idx = (unsigned int)std::max(
      0u, (((unsigned int)copy_src_vec_.size()) - std::max(1u, (unsigned int)(copy_src_vec_.size() / 4))));
  // std::cerr << " origsz " << copy_src_vec_.size() << " sidx " << start_idx << " sz " << ( copy_src_vec_.size ( ) -
  // start_idx ) << std::endl;
  for (unsigned int i = start_idx; i < copy_src_vec_.size(); i++) {
    retval_ += copy_src_vec_[i];
  }
  return (retval_ / (T)(copy_src_vec_.size() - start_idx));
}

/// Returns the mean value of the given vector
template <typename T>
inline T GetMeanLowestQuartile(const std::vector<T>& src_vec_) {
  T retval_ = 0;
  if (src_vec_.empty()) {
    return 0;  // need to check this else the return value will be ill defined
  }
  std::vector<T> copy_src_vec_(src_vec_);
  std::sort(copy_src_vec_.begin(), copy_src_vec_.end());
  unsigned int end_idx = (unsigned int)std::max(1u, (unsigned int)(copy_src_vec_.size() / 4));
  // std::cerr << " origsz " << copy_src_vec_.size() << " sidx " << start_idx << " sz " << ( copy_src_vec_.size ( ) -
  // start_idx ) << std::endl;
  for (auto i = 0u; i < end_idx; i++) {
    retval_ += copy_src_vec_[i];
  }
  return (retval_ / (T)(end_idx));
}

/// Returns the standard deviation of the given vector src_vec_
template <typename T>
inline T GetStdev(const std::vector<T>& src_vec_) {
  if (src_vec_.size() >= 2) {
    const T sample_mean = GetMean(src_vec_);
    T sample_sumsquarevalues = 0;
    for (auto i = 0u; i < src_vec_.size(); i++) {
      sample_sumsquarevalues += GetSquareOf(src_vec_[i] - sample_mean);
    }
    double sample_variance_ = std::max((T)0.0, sample_sumsquarevalues) / (double)((int)src_vec_.size() - 1);
    return sqrt(sample_variance_);
  }
  return 0;
}

/// Returns the standard deviation of the given vector src_vec_
template <typename T>
inline T GetStdevAndMean(const std::vector<T>& src_vec_, T& sample_mean) {
  if (src_vec_.size() >= 2) {
    sample_mean = GetMean(src_vec_);
    T sample_sumsquarevalues = 0;
    for (auto i = 0u; i < src_vec_.size(); i++) {
      sample_sumsquarevalues += GetSquareOf(src_vec_[i] - sample_mean);
    }

    double sample_variance_ = std::max(0.0, sample_sumsquarevalues) / (double)((int)src_vec_.size() - 1);
    return sqrt(sample_variance_);
  }
  return 0;
}

// consider only the non-zero points in mean calculation
template <typename T>
inline T GetNonZeroStdevAndMean(const std::vector<T>& src_vec_, T& sample_mean) {
  if (src_vec_.size() >= 2) {
    sample_mean = GetNonZeroMean(src_vec_);
    T sample_sumsquarevalues = 0;
    unsigned int count_non_zero_ = 0;
    for (auto i = 0u; i < src_vec_.size(); i++) {
      if (src_vec_[i] != 0) {
        sample_sumsquarevalues += GetSquareOf(src_vec_[i] - sample_mean);
        count_non_zero_++;
      }
    }

    if (count_non_zero_ <= 1)  // absurdly pathological case
    {
      return 0;
    }

    double sample_variance_ = std::max(0.0, sample_sumsquarevalues) / (double)((int)count_non_zero_ - 1);
    return sqrt(sample_variance_);
  }
  return 0;
}

/// Returns the standard deviation of the given vector src_vec_, and sample_mean
template <typename T>
inline T GetStdev(const std::vector<T>& src_vec_, const T sample_mean) {
  if (src_vec_.size() >= 2) {
    T sample_sumsquarevalues = 0;
    for (auto i = 0u; i < src_vec_.size(); i++) {
      sample_sumsquarevalues += GetSquareOf(src_vec_[i] - sample_mean);
    }
    double sample_variance_ = std::max(0.0, sample_sumsquarevalues) / (double)((int)src_vec_.size() - 1);
    return sqrt(sample_variance_);
  }
  return 0;
}

/// Returns the sample skewness of the given vector src_vec_, and sample_mean, and sample_stdev
template <typename T>
inline T GetSkewness(const std::vector<T>& src_vec_, const T sample_mean, const T sample_stdev) {
  if (src_vec_.size() >= 2) {
    T sample_sumcubevalues = 0;
    for (auto i = 0u; i < src_vec_.size(); i++) {
      sample_sumcubevalues += pow((src_vec_[i] - sample_mean), 3);
    }
    return (sample_sumcubevalues / src_vec_.size()) / pow(sample_stdev, 3);
  }
  return 0;
}

/// Assume vector sorted in ascending order
/// remove max and count min twice, return average
template <typename T>
inline T GetConservativeAverage(const std::vector<T>& src_vec_) {
  if (src_vec_.empty()) return 0;  // need to check this else the return value will be ill defined

  T retval_ = src_vec_[0];  // counting min once
  unsigned int num_entries_ = 1;
  for (auto i = 0u; i < (src_vec_.size() - 1 /* skipping max */); i++) {
    retval_ += src_vec_[i];
    num_entries_++;
  }

  return (num_entries_ > 0) ? (retval_ / (T)(num_entries_)) : (0);
}

/// Assume vector sorted in ascending order
/// Removing the 20% max values and counting bottom 20% values twice, returns average
template <typename T>
inline T GetConservativeAverageFifth(const std::vector<T>& src_vec_) {
  if (src_vec_.empty()) return 0;  // need to check this else the return value will be ill defined

  T retval_ = 0;
  unsigned int num_entries_ = 0;
  unsigned int onefifth = (unsigned int)std::max(1.00, round(double(src_vec_.size()) / 5.0));

  for (auto i = 0u; i < onefifth; i++) {
    retval_ += src_vec_[i];
    num_entries_++;
  }

  for (auto i = 0u; i < (src_vec_.size() - onefifth); i++) {
    retval_ += src_vec_[i];
    num_entries_++;
  }

  return (num_entries_ > 0) ? (retval_ / (T)(num_entries_)) : (0);
}

/// Assume src_vec is not sorted but for efficient implementatation sort src_vec before calling and set _already_sorted_
/// flag to true
/// returns average of the middle 50% of the values
template <typename T>
inline T GetMedianAverage(const std::vector<T>& src_vec_, bool _already_sorted_ = false) {
  T retval_ = 0;
  unsigned int num_entries_ = 0;
  if (src_vec_.empty()) return 0;  // need to check this else the return value will be ill defined

  const std::vector<T>* p_sort_vec_ = &src_vec_;
  std::vector<T> copy_src_vec_;
  if (!_already_sorted_) {
    copy_src_vec_ = std::vector<T>(src_vec_);
    std::sort(copy_src_vec_.begin(), copy_src_vec_.end());
    p_sort_vec_ = &copy_src_vec_;
  }

  unsigned int onefourth = p_sort_vec_->size() / 4;

  for (unsigned int i = onefourth; i < (p_sort_vec_->size() - onefourth); i++) {
    retval_ += (*p_sort_vec_)[i];
    num_entries_++;
  }

  return (num_entries_ > 0) ? (retval_ / (T)(num_entries_)) : (0);
}

/// returns median of the values
template <typename T>
inline T GetMedian(const std::vector<T>& src_vec_) {
  T retval_ = 0;
  if (src_vec_.empty()) {
    return retval_;  // need to check this else the return value will be ill defined
  } else {
    std::vector<T> copy_src_vec_(src_vec_);
    std::sort(copy_src_vec_.begin(), copy_src_vec_.end());
    unsigned int median_index_ = std::max(1u, (unsigned int)((copy_src_vec_.size() + 1u) / 2)) - 1;
    return copy_src_vec_[median_index_];
  }
}

/// returns product of mean and sharpe
template <typename T>
inline T GetProdMeanSharpe(const std::vector<T>& src_vec_) {
  T mean_value_ = 0;
  T stdev_value_ = GetStdevAndMean(src_vec_, mean_value_);
  if (stdev_value_ > 0) {
    return (mean_value_ > 0) ? (mean_value_ * mean_value_ / stdev_value_) : (-mean_value_ * mean_value_ / stdev_value_);
  } else {
    return mean_value_;
  }
}

/// returns product of mean and sharpe
template <typename T>
inline T GetMeanFracSameSign(const std::vector<T>& src_vec_) {
  T mean_value_ = 0;
  if (src_vec_.size() > 1) {
    mean_value_ = GetMean(src_vec_);
  } else {
    mean_value_ = (src_vec_.size() > 0) ? src_vec_[0] : 0;
  }
  int num_same_sign_ = 0;
  for (auto i = 0u; i < src_vec_.size(); i++) {
    if ((src_vec_[i] * mean_value_) > 0) {
      num_same_sign_++;
    }
  }
  return (mean_value_ * num_same_sign_) / src_vec_.size();
}

/// Returns the standard deviation of the given vector src_vec_ assuming that the mean of the given vector is 0
template <typename T>
inline T GetStdevNoMean(const std::vector<T>& src_vec_) {
  if (src_vec_.size() >= 2) {
    T sample_sumsquarevalues = 0;
    for (auto i = 0u; i < src_vec_.size(); i++) {
      sample_sumsquarevalues += GetSquareOf(src_vec_[i]);
    }
    return (sample_sumsquarevalues) / (double)(src_vec_.size() - 1);
  }
  return 0;
}

/// On every element of series, removes mean and divides by stdev.
template <typename T>
void NormalizeData(std::vector<T>& train_data_dependant_, const T& mean_dependant_, const T& stdev_dependant_) {
  if (stdev_dependant_ > 0.00000000001) {
    for (unsigned int dataline_num_ = 0; dataline_num_ < train_data_dependant_.size(); dataline_num_++) {
      train_data_dependant_[dataline_num_] =
          (train_data_dependant_[dataline_num_] - mean_dependant_) / stdev_dependant_;
    }
  }
}

template <typename T>
void NormalizeNonZeroData(std::vector<T>& train_data_dependant_, const T& mean_dependant_, const T& stdev_dependant_) {
  if (stdev_dependant_ > 0.00000000001) {
    for (unsigned int dataline_num_ = 0; dataline_num_ < train_data_dependant_.size(); dataline_num_++) {
      train_data_dependant_[dataline_num_] =
          fabs(train_data_dependant_[dataline_num_]) > 0
              ? (train_data_dependant_[dataline_num_] - mean_dependant_) / stdev_dependant_
              : 0;  // we ignore zero points since they are mostly artificially generatated
    }
  }
}

/// Calculates mean and stdev.
/// On every element of series, removes mean and divides by stdev.
/// Returns mean & stdev by the passed in references
template <typename T>
void CalcMeanStdevNormalizeData(std::vector<T>& train_data_dependant_, T& mean_dependant_, T& stdev_dependant_) {
  stdev_dependant_ = GetStdevAndMean(train_data_dependant_, mean_dependant_);
  NormalizeData(train_data_dependant_, mean_dependant_, stdev_dependant_);
}

// include only non zero points in mean and stdev calculation
template <typename T>
void CalcNonZeroMeanStdevNormalizeData(std::vector<T>& train_data_dependant_, T& mean_dependant_, T& stdev_dependant_) {
  stdev_dependant_ = GetNonZeroStdevAndMean(train_data_dependant_, mean_dependant_);
  NormalizeNonZeroData(train_data_dependant_, mean_dependant_, stdev_dependant_);
}

/// Deducts the given value from each element of the series _in_series_
template <typename T>
inline void RemoveMeanSeries(std::vector<T>& _in_series_, const T& value_to_deduct_) {
  for (auto i = 0u; i < _in_series_.size(); i++) {
    _in_series_[i] -= value_to_deduct_;
  }
}

/// Calculates and Deducts the computed mean from each element of the series _in_series_
template <typename T>
inline T CalcAndRemoveMeanFromSeries(std::vector<T>& _in_series_) {
  const T _in_mean_ = GetMean(_in_series_);
  for (auto i = 0u; i < _in_series_.size(); i++) {
    _in_series_[i] -= _in_mean_;
  }
  return _in_mean_;
}

/// Calls CalcAndRemoveMeanFromSeries on each element of _in_series_
template <typename T>
inline std::vector<T> CalcAndRemoveMeanFromSeriesVec(std::vector<std::vector<T> >& _in_series_) {
  std::vector<T> retval(_in_series_.size());
  for (auto i = 0u; i < _in_series_.size(); i++) {
    retval[i] = CalcAndRemoveMeanFromSeries(_in_series_[i]);
  }
  return retval;
}

template <typename T>
inline T CalcDotProduct(const std::vector<T>& in_Y_, const std::vector<T>& in_X_) {
  T outvalue_ = 0;
  for (unsigned int j = 0; j < in_Y_.size(); j++) {
    outvalue_ += in_Y_[j] * in_X_[j];
  }
  return outvalue_;
}

template <typename T>
inline void CalcDotProductVec(const std::vector<T>& in_Y_, const std::vector<std::vector<T> >& in_X_,
                              std::vector<T>& initcorr_) {
  for (auto i = 0u; i < in_X_.size(); i++) {
    initcorr_[i] = CalcDotProduct(in_Y_, in_X_[i]);
  }
}

template <typename T>
inline void CalcSum(const std::vector<T>& train_data_, const T& sum_values_) {
  sum_values_ = 0;
  for (auto i = 0u; i < train_data_.size(); i++) {
    sum_values_ += train_data_[i];
  }
}

template <typename T>
inline T CalcL2Norm(const std::vector<T>& train_data_) {
  T sum_square_values_ = 0;
  for (auto i = 0u; i < train_data_.size(); i++) {
    sum_square_values_ += (train_data_[i] * train_data_[i]);
  }
  return sum_square_values_;
}

template <typename T>
inline void CalcMeanSumSquare(const std::vector<T>& train_data_, T& sum_values_, T& sum_square_values_) {
  sum_values_ = 0;
  sum_square_values_ = 0;
  for (unsigned int dataline_num_ = 0; dataline_num_ < train_data_.size(); dataline_num_++) {
    sum_values_ += train_data_[dataline_num_];
    sum_square_values_ += (train_data_[dataline_num_] * train_data_[dataline_num_]);
  }
}

template <typename T>
inline unsigned int CalcNonZeroMeanSumSquare(const std::vector<T>& train_data_, T& sum_values_, T& sum_square_values_) {
  sum_values_ = 0;
  sum_square_values_ = 0;
  unsigned int count_non_zero_ = 0;
  for (unsigned int dataline_num_ = 0; dataline_num_ < train_data_.size(); dataline_num_++) {
    sum_values_ += train_data_[dataline_num_];
    sum_square_values_ += (train_data_[dataline_num_] * train_data_[dataline_num_]);
    if (fabs(train_data_[dataline_num_]) > 0) {
      count_non_zero_++;
    }
  }
  return count_non_zero_;
}

template <typename T>
inline void CalcMeanSumSquareVec(const std::vector<std::vector<T> >& train_data_, std::vector<T>& sum_values_,
                                 std::vector<T>& sum_square_values_) {
  for (unsigned int indep_index_ = 0; indep_index_ < train_data_.size(); indep_index_++) {
    CalcMeanSumSquare(train_data_[indep_index_], sum_values_[indep_index_], sum_square_values_[indep_index_]);
  }
}

template <typename T>
inline void CalcNonZeroMeanStdev(const std::vector<T>& train_data_, T& mean_values_, T& stdev_values_) {
  T sum_values_ = 0;
  T sum_square_values_ = 0;
  unsigned int num_lines_data_ = CalcNonZeroMeanSumSquare(train_data_, sum_values_, sum_square_values_);
  if (num_lines_data_ <= 1) {
    num_lines_data_ = train_data_.size();
  }
  mean_values_ = sum_values_ / num_lines_data_;
  stdev_values_ = sqrt((sum_square_values_ - (sum_values_ * mean_values_)) / (num_lines_data_ - 1));
}

template <typename T>
inline void CalcMeanStdev(const std::vector<T>& train_data_, T& mean_values_, T& stdev_values_) {
  T sum_values_ = 0;
  T sum_square_values_ = 0;
  CalcMeanSumSquare(train_data_, sum_values_, sum_square_values_);
  unsigned int num_lines_data_ = train_data_.size();
  mean_values_ = sum_values_ / num_lines_data_;
  stdev_values_ = sqrt((sum_square_values_ - (sum_values_ * mean_values_)) / (num_lines_data_ - 1));
}

template <typename T>
inline void CalcMeanStdevVec(const std::vector<std::vector<T> >& train_data_, std::vector<T>& mean_value_vec_,
                             std::vector<T>& stdev_value_vec_) {
  for (unsigned int indep_index_ = 0; indep_index_ < train_data_.size(); indep_index_++) {
    CalcMeanStdev(train_data_[indep_index_], mean_value_vec_[indep_index_], stdev_value_vec_[indep_index_]);
  }
}

template <typename T>
inline void CalcNonZeroMeanStdevVec(const std::vector<std::vector<T> >& train_data_, std::vector<T>& mean_value_vec_,
                                    std::vector<T>& stdev_value_vec_) {
  for (unsigned int indep_index_ = 0; indep_index_ < train_data_.size(); indep_index_++) {
    CalcNonZeroMeanStdev(train_data_[indep_index_], mean_value_vec_[indep_index_], stdev_value_vec_[indep_index_]);
  }
}

/// Returns the number of entries that are equal to the given value
template <typename T>
inline unsigned int GetCountEquals(const std::vector<T>& _class_vec_, const T& _new_item_) {
  unsigned int retval = 0;
  for (auto i = 0u; i < _class_vec_.size(); i++) {
    if (_class_vec_[i] == _new_item_) {
      retval++;
    }
  }
  return retval;
}
}
}

#endif  // BASE_COMMONDATASTRUCTURES_VECTOR_UTILS_H
