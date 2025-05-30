/**
    \file dvccode/CDef/math_utils.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_CDEF_MATH_UTILS_H
#define BASE_CDEF_MATH_UTILS_H

#include <math.h>
#include <stdlib.h>
#include <algorithm>
#include <vector>

namespace HFSAT {

template <typename T>
inline T Signum(const T _in_val_) {
  return ((_in_val_ < 0) ? -1 : 1);
}

template <typename T>
inline T GetSquareOf(const T _in_val_) {
  return _in_val_ * _in_val_;
}

namespace MathUtils {

inline int GetFlooredMultipleOf(const int _in_value_, const int _base_value_) {
  if (_base_value_ <= 1) {
    return _in_value_;
  }

  int sign_ = (_in_value_ < 0) ? -1 : 1;
  return (sign_ * (_base_value_ * (int)floor(abs(_in_value_) / _base_value_)));
}

inline int GetCeilMultipleOf(const int _in_value_, const int _base_value_) {
  if (_base_value_ <= 1) {
    return _in_value_;
  }

  int sign_ = (_in_value_ < 0) ? -1 : 1;
  return (sign_ * (_base_value_ * (int)ceil((double)abs(_in_value_) / _base_value_)));
}

inline int RoundOff(const double _in_value_, const int _round_fact_ = 1) {
  int sign_ = (_in_value_ < 0) ? -1 : 1;
  return sign_ * _round_fact_ * int((fabs(_in_value_)) / _round_fact_ + 0.5);
}

inline int SignedRoundOff(const double _in_value_, const int _round_fact_ = 1) {
  if (_in_value_ > 0) {
    return _round_fact_ * int(_in_value_ / _round_fact_ + 0.5);
  } else {
    return _round_fact_ * int(_in_value_ / _round_fact_ - 0.5);
  }
}

/// returns the factor alpha such that alpha ^ _number_fadeoffs_ = 0.5 , hence something decaying at alpha has half-life
/// _number_fadeoffs_
inline double CalcDecayFactor(int _number_fadeoffs_) {
  return (_number_fadeoffs_ >= 1) ? (pow(0.5, double(1.00 / std::max(1.00, (double)_number_fadeoffs_)))) : 1.00;
}

inline double GetSampleVarianceFromL1L2Sums(double sum_values_, double sum_square_values_, double num_values_) {
  return (num_values_ < 2)
             ? (0.0)
             : (std::max(0.0,
                         ((sum_square_values_ - (sum_values_ * sum_values_ / (num_values_))) / (num_values_ - 1))));
}

inline double GetSampleStdevFromL1L2Sums(double sum_values_, double sum_square_values_, double num_values_) {
  return sqrt(GetSampleVarianceFromL1L2Sums(sum_values_, sum_square_values_, num_values_));
}

// generate the next Gray code (in reverse)
// http://en.wikipedia.org/wiki/Gray_code
inline void load_subset(std::vector<bool> bit_mask, unsigned int req_size, std::vector<int>& oput) {
  if (std::count(bit_mask.begin(), bit_mask.end(), true) == req_size) {
    for (auto i = 0u; i < bit_mask.size(); ++i)
      if (bit_mask[i]) {
        oput.push_back(i + 1);
      }
  }
}
inline bool next_bitmask(std::vector<bool>& bit_mask) {
  std::size_t i = 0;
  for (; (i < bit_mask.size()) && bit_mask[i]; ++i) bit_mask[i] = false;

  if (i < bit_mask.size()) {
    bit_mask[i] = true;
    return true;
  } else
    return false;
}
inline int factorial(int n) { return (n == 1 || n == 0) ? 1 : factorial(n - 1) * n; }
inline std::vector<int> GetSubsets(unsigned int n, unsigned int k) {
  int size = factorial(n) / (factorial(k) * factorial(n - k));
  size = size * k;
  std::vector<int> output;
  std::vector<bool> bit_mask(n);
  do {
    load_subset(bit_mask, k, output);
  } while (next_bitmask(bit_mask));
  return output;
}
// pecentile
template <typename T>
inline T GetXPercentile(std::vector<T> const& vec, double X) {
  std::vector<T> copy_src_vec_(vec);
  if (X < 0 || X > 100) {
    return (T)-1;
  }
  double perc = (100.0 - X) / 100.00;
  sort(copy_src_vec_.begin(), copy_src_vec_.end());
  T ret = copy_src_vec_[std::max(0, (int)(((double)copy_src_vec_.size()) * perc))];
  return ret;
}

static inline bool DblPxCompare(const double& t_price1_, const double& t_price2_,
                                const double half_min_price_increment_) {
  double tdiff = (t_price1_ - t_price2_);
  return ((tdiff > -half_min_price_increment_) && (tdiff < half_min_price_increment_));
}
}
}

#endif  // BASE_CDEF_MATH_UTILS_H
