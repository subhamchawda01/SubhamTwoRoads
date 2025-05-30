/*
 * SqlCppUtils.hpp
 *
 *  Created on: Apr 30, 2015
 *      Author: archit
 */

#ifndef SQLCPP_SQLCPPUTILS_HPP_
#define SQLCPP_SQLCPPUTILS_HPP_

#include <set>
#include <vector>
#include "basetrade/MToolsExe/result_line_set.hpp"
#include "basetrade/SqlCpp/results_db_access_manager.hpp"

namespace HFSAT {

class SqlCppUtils {
 public:
  static void GetResultLineSetVec(FileToResultLineSetMap& _return_map_, const std::string& _shortcode_,
                                  const std::vector<int>& _date_vec_,
                                  const std::set<std::string>& _stratbasename_set_ = std::set<std::string>(),
                                  int _maxloss_per_uts_ = 0);
};

} /* namespace HFSAT */

#endif /* SQLCPP_SQLCPPUTILS_HPP_ */
