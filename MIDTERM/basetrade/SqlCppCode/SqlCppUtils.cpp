/*
 * SqlCppUtils.cpp
 *
 *  Created on: Apr 30, 2015
 *      Author: archit
 */

#include "basetrade/SqlCpp/SqlCppUtils.hpp"

namespace HFSAT {

void SqlCppUtils::GetResultLineSetVec(FileToResultLineSetMap& _return_map_, const std::string& _shortcode_,
                                      const std::vector<int>& _date_vec_,
                                      const std::set<std::string>& _stratbasename_set_, int _maxloss_per_uts_) {
  ResultsAccessManager& ram_ = ResultsAccessManager::GetUniqueInstance();
  for (unsigned int d_idx_ = 0; d_idx_ < _date_vec_.size(); d_idx_++) {
    ram_.FetchResults(_return_map_, _shortcode_, _date_vec_[d_idx_], _stratbasename_set_, _maxloss_per_uts_);
  }
}

} /* namespace HFSAT */
