/*
 * model_scaling.cpp
 *
 *  Created on: Jan 13, 2015
 *      Author: archit
 */

#include "dvccode/Utils/model_scaling.hpp"

namespace HFSAT {

// TODO ... please add the logic in this function
bool ModelScaling::CheckIfStaticModelScalingPossible(std::string _modelfilename_, std::string _paramfilename_,
                                                     std::vector<double>& _model_scaling_factor_vec_) {
  _model_scaling_factor_vec_.clear();
  std::vector<double> param_stdev_vec_;
  std::vector<double> model_stdev_vec_;
  std::string param_regime_indicator_string_ = "";
  std::string model_regime_indicator_string_ = "";
  bool use_reg_stdev_ = false;

  // reading param_stdev_vec
  std::ifstream paramfile_;
  paramfile_.open(_paramfilename_.c_str(), std::ifstream::in);
  if (paramfile_.is_open()) {
    const int kBufferLen_ = 1024;
    char buffer_[kBufferLen_];
    bzero(buffer_, kBufferLen_);
    while (paramfile_.good()) {
      paramfile_.getline(buffer_, kBufferLen_);
      PerishableStringTokenizer st_(buffer_, kBufferLen_);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() < 1) {
        continue;  // empty lines
      }

      if (!use_reg_stdev_ && (strcmp(tokens_[0], "PARAMFILELIST") == 0) && (tokens_.size() >= 2)) {
        // If the first word is PARAMFILELIST and the line has at least two words
        // then the second word is expected to be a filename
        // PARAMFILELIST <filename>
        // TODO : Should I check if the file exists?
        // This seems like the best place to check file existence.
        param_stdev_vec_.push_back(GetOptimizedStdevForParam(tokens_[1]));
      } else if (strcmp(tokens_[0], "INDICATOR") == 0 && tokens_.size() >= 3) {
        // For lines that start with "INDICATOR"
        // We are starting to look at athe third word
        // Format: INDICATOR <regime_ind_string>
        for (auto i = 2u; i < tokens_.size(); i++) {
          if (tokens_[i][0] == '#') {
            break;
          }
          param_regime_indicator_string_ += std::string(tokens_[i]);
        }
      } else if (strcmp(tokens_[0], "OPTIMIZED_REGIME_MODEL_STDEV") == 0 && tokens_.size() >= 4) {
        // complete regime param has been optimized regime by regime
        // OPTIMIZED_REGIME_MODEL_STDEV reg1_stdev reg2_stdev ... regn_stdev comp_model_stdev
        param_stdev_vec_.clear();
        for (auto i = 1u; i < tokens_.size(); i++) {
          if (tokens_[i][0] == '#') {
            break;
          }
          param_stdev_vec_.push_back(std::max(0.0, atof(tokens_[i])));
        }
        use_reg_stdev_ = true;
      }
    }
    paramfile_.close();
  }

  if (param_stdev_vec_.size() <= 0) {
    // if non-regime param
    param_stdev_vec_.push_back(GetOptimizedStdevForParam(_paramfilename_));
    use_reg_stdev_ = false;
  }

  // reading model_stdev_vec
  std::ifstream modelfile_;
  modelfile_.open(_modelfilename_.c_str(), std::ifstream::in);
  unsigned int num_models_ = 0;
  if (modelfile_.is_open()) {
    const int kBufferLen_ = 1024;
    char buffer_[kBufferLen_];
    bzero(buffer_, kBufferLen_);
    while (modelfile_.good()) {
      modelfile_.getline(buffer_, kBufferLen_);
      PerishableStringTokenizer st_(buffer_, kBufferLen_);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() < 1) {
        continue;  // empty lines
      }

      if (strcmp(tokens_[0], "MODELINFO") == 0) {
        for (unsigned int i = 1; i < tokens_.size(); i++) {
          if (tokens_[i][0] == '#') {
            break;
          }
          model_stdev_vec_.push_back(std::max(0.0, atof(tokens_[i])));
        }
      } else if (strcmp(tokens_[0], "INDICATORSTART") == 0 || strcmp(tokens_[0], "INDICATORINTERMEDIATE") == 0 ||
                 strcmp(tokens_[0], "REGIMEMODEL") == 0) {
        num_models_++;
      } else if (strcmp(tokens_[0], "REGIMEINDICATOR") == 0) {
        for (unsigned int i = 2; i < tokens_.size(); i++) {
          if (tokens_[i][0] == '#') {
            break;
          }
          model_regime_indicator_string_ += std::string(tokens_[i]);
        }
      } else if (strcmp(tokens_[0], "MODELMATH") == 0) {
        if (tokens_.size() < 3 || strcmp(tokens_[2], "CHANGE") != 0 ||
            (strcmp(tokens_[1], "SELECTIVENEW") != 0 && strcmp(tokens_[1], "SELECTIVESIGLR") != 0 &&
             strcmp(tokens_[1], "LINEAR") != 0 && strcmp(tokens_[1], "SIGLR") != 0)) {
          // scaling only supported for linear,siglr models
          modelfile_.close();
          return false;
        }
      }
    }
    modelfile_.close();
  }

  // both vactors are filled, looking at compatibility and computing factors
  if (param_stdev_vec_.size() > 1 && model_regime_indicator_string_.compare(param_regime_indicator_string_) != 0) {
    // param has regimes and either model dont have a regime or it is different from param regime
    return false;
  }
  if ((num_models_ <= 0) || ((num_models_ == 1) && (model_stdev_vec_.size() != 1)) ||
      ((num_models_ > 1) && (model_stdev_vec_.size() != (num_models_ + 1)))) {
    // MODELINFO is not valid, for regime we should have (num_models_+1) stdevs, one for each regimes followed by stdev
    // of whole model
    return false;
  }

  double complete_model_stdev_ = model_stdev_vec_[model_stdev_vec_.size() - 1];  // last guy is complete model stdev
  if (model_stdev_vec_.size() > num_models_) {
    model_stdev_vec_.resize(num_models_);
  }

  if ((param_stdev_vec_.size() == 1) && (complete_model_stdev_ > 0.0)) {
    // non-regime param with non-zero optimized stdev

    // Here we are adding the same scaling factor param_stdev_vec[0]/complete_model_stdev_
    // many times to a vector. I am not sure why. I will have to ask others
    // to make the reasoning clear
    for (auto i = 0u; i < model_stdev_vec_.size(); i++) {
      _model_scaling_factor_vec_.push_back(param_stdev_vec_[0] / complete_model_stdev_);
    }
  } else if ((param_stdev_vec_.size() > 1) && (param_stdev_vec_.size() >= model_stdev_vec_.size())) {
    // regime param, model_stdev_vec_ has stdev of first model_stdev_vec_.size() models
    for (auto i = 0u; i < model_stdev_vec_.size(); i++) {
      double t_model_stdev_to_use_ = use_reg_stdev_ ? model_stdev_vec_[i] : complete_model_stdev_;
      if (t_model_stdev_to_use_ > 0.0) {
        _model_scaling_factor_vec_.push_back(param_stdev_vec_[i] / t_model_stdev_to_use_);
      } else {
        _model_scaling_factor_vec_.push_back(1.0);
      }
    }
  }

  bool ans_ = false;
  for (const auto t_model_scaling_factor : _model_scaling_factor_vec_) {
    if ((t_model_scaling_factor > 0.0) && (!HFSAT::MathUtils::DblPxCompare(t_model_scaling_factor, 1.0, 0.0005))) {
      ans_ = true;
    }
  }
  if (!ans_) {
    _model_scaling_factor_vec_.clear();
  }
  return ans_;
}

// We are searching for a line in this paramfile with the
// first word PARAMVALUE and the second word OPTIMIZED_MODEL_STDEV
// In that case the third word is a floating point value that
// we are looking for.
double ModelScaling::GetOptimizedStdevForParam(std::string _paramfilename_) {
  std::ifstream paramfile_;
  paramfile_.open(_paramfilename_.c_str(), std::ifstream::in);
  if (paramfile_.is_open()) {
    const int kBufferLen_ = 1024;
    char buffer_[kBufferLen_];
    bzero(buffer_, kBufferLen_);
    while (paramfile_.good()) {
      paramfile_.getline(buffer_, kBufferLen_);
      PerishableStringTokenizer st_(buffer_, kBufferLen_);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if ((tokens_.size() >= 3) && (strcmp(tokens_[0], "PARAMVALUE") == 0) &&
          (strcmp(tokens_[1], "OPTIMIZED_MODEL_STDEV") == 0)) {
        paramfile_.close();
        return std::max(0.0, atof(tokens_[2]));
      }
    }
  }
  paramfile_.close();
  return 0.0;
}

} /* namespace HFSAT */
