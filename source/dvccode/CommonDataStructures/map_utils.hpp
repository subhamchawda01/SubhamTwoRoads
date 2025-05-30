/**
   \file dvccode/CommonDataStructures/map_utils.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_COMMONDATASTRUCTURES_MAP_UTILS_H
#define BASE_COMMONDATASTRUCTURES_MAP_UTILS_H

#include <iostream>
#include <cstdlib>

#include <vector>
#include <map>
#include <algorithm>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/math_utils.hpp"

namespace HFSAT {

/// Returns the vector of values of the map
template <typename K, typename V>
inline void GetValueVecFromMap(const std::map<K, V>& given_map_, std::vector<V>& return_value_vec_) {
  typename std::map<K, V>::const_iterator _citer_;
  for (_citer_ = given_map_.begin(); _citer_ != given_map_.end(); _citer_++) {
    return_value_vec_.push_back(_citer_->second);  // by value
  }
}
}

#endif  // BASE_COMMONDATASTRUCTURES_MAP_UTILS_H
