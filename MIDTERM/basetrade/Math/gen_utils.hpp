/**
    \file Math/gen_utils.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_MATH_GEN_UTILS_H
#define BASE_MATH_GEN_UTILS_H

#include <math.h>
#include <iostream>
#include <vector>

namespace HFSAT {
namespace Math {

inline double GetStdev(const double& l1_sum_, const double& l2_sum_, const unsigned int& count_) {
  if (count_ >= 2) {
    return sqrt(std::max(0.00, (l2_sum_ - (l1_sum_ * l1_sum_ / count_)) / (count_ - 1)));
  }
  return 0;
}

inline void GetBoundariesOfFolds(const unsigned int total_size_, const unsigned int num_folds_,
                                 std::vector<unsigned int>& start_indices_, std::vector<unsigned int>& end_indices_) {
  if (total_size_ <= 0u) {
    return;
  }

  if (total_size_ <= 2 * num_folds_) {  // ignoring all base cases since we don't really expect to use this for small
                                        // values of total_size_
    start_indices_.push_back(0);
    end_indices_.push_back(total_size_ - 1);
  } else {
    start_indices_.push_back(0);
    for (unsigned int i = 1u; i < num_folds_; i++) {
      start_indices_.push_back((unsigned int)((i * total_size_) / num_folds_));
      end_indices_.push_back(std::max(start_indices_[i - 1] + 1, (start_indices_[i] - 1)));
    }
    end_indices_.push_back(total_size_ - 1);
  }
}
}
}

#endif  // BASE_MATH_GEN_UTILS_H
