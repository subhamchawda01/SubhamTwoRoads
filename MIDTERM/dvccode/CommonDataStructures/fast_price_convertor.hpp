/**
    \file dvccode/CommonDataStructures/fast_price_convertor.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_COMMONDATASTRUCTURES_FAST_PRICE_CONVERTOR_H
#define BASE_COMMONDATASTRUCTURES_FAST_PRICE_CONVERTOR_H

#include <math.h>
#include <utility>
#include "dvccode/CommonDataStructures/circular_buffer.hpp"

namespace HFSAT {

// #define USING_MEM_OPT_ROUNDING
#define COMPUTATIONS_TO_REMEMBER 22

/// Testing whether remembering recently converted price values in std::pair < double , int > ( COMPUTATIONS_TO_REMEMBER
/// )
/// and hence instead of 1 division operation doing <= COMPUTATIONS_TO_REMEMBER double comparison operations is better.
struct FastPriceConvertor {
  const double min_price_increment_;       ///< min_price_increment_
  const double half_min_price_increment_;  ///< half of tick size used in equality comparison
  mutable CircularBuffer<std::pair<double, int> > prev_computations_;

  explicit FastPriceConvertor(const double& _min_price_increment_)
      : min_price_increment_(_min_price_increment_),
        half_min_price_increment_(_min_price_increment_ / 2.0),
        prev_computations_(COMPUTATIONS_TO_REMEMBER) {}

  inline int GetFastIntPx(const double& t_price_) const {
#ifndef USING_MEM_OPT_ROUNDING
    return ((int)round(t_price_ / min_price_increment_));
#else
    for (int i = 0; i < prev_computations_.size(); i++) {
      if (DblPxCompare(prev_computations_[i].first, t_price_)) {
        return prev_computations_[i].second;
      }
    }
    int retval = ((int)round(t_price_ / min_price_increment_));
    prev_computations_.push_back(std::make_pair(t_price_, retval));
    return retval;
#endif  // USING_MEM_OPT_ROUNDING
  }

  /// fast function to account for double value diffs and compare
  /// returns true if the values are not more than half_min_price_increment_ away
  inline bool DblPxCompare(const double& t_price1_, const double& t_price2_) const {
    register double tdiff = (t_price1_ - t_price2_);
    return ((tdiff > -half_min_price_increment_) && (tdiff < half_min_price_increment_));
  }

  inline int DblPxDiffSign(const double& t_price1_, const double& t_price2_) const {
    register double tdiff = (t_price1_ - t_price2_);
    if (tdiff >= half_min_price_increment_) return 1;
    if (tdiff <= -half_min_price_increment_) return -1;
    return 0;
  }

  inline double GetDoublePx(const int _intpx_) const { return min_price_increment_ * _intpx_; }
};
#undef COMPUTATIONS_TO_REMEMBER
}

#endif  // BASE_COMMONDATASTRUCTURES_FAST_PRICE_CONVERTOR_H
