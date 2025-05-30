/*
 * model_scaling.hpp
 *
 *  Created on: Jan 13, 2015
 *      Author: archit
 */

#ifndef UTILSCODE_MODEL_SCALING_HPP_
#define UTILSCODE_MODEL_SCALING_HPP_

#include <vector>
#include <string>
#include <string.h>
#include <iostream>
#include <fstream>
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CDef/math_utils.hpp"

namespace HFSAT {

class ModelScaling {
 public:
  static bool CheckIfStaticModelScalingPossible(std::string _modelfilename_, std::string _paramfilename_,
                                                std::vector<double>& _model_scaling_factor_vec_);
  static double GetOptimizedStdevForParam(std::string _paramfilename_);
};

} /* namespace HFSAT */

#endif /* UTILSCODE_MODEL_SCALING_HPP_ */
