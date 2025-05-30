/*
 * model_utils.hpp
 *
 *  Created on: Aug 13, 2015
 *      Author: archit
 */

#ifndef DVCCODE_UTILS_MODEL_UTILS_HPP_
#define DVCCODE_UTILS_MODEL_UTILS_HPP_

namespace HFSAT {
namespace ModelUtils {
std::string GetModelMathStr(const std::string& _modelfile_) {
  std::string ans_ = "INVALID";
  std::ifstream ifs_(_modelfile_, std::ifstream::in);
  if (ifs_.is_open()) {
    const int kFileLineBufferLen = 1024;
    char readline_buffer_[kFileLineBufferLen];

    while (ifs_.good()) {
      bzero(readline_buffer_, kFileLineBufferLen);
      ifs_.getline(readline_buffer_, kFileLineBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, kFileLineBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() >= 2 && strcmp(tokens_[0], "MODELMATH") == 0) {
        ans_ = std::string(tokens_[1]);
        break;
      }
    }
    ifs_.close();
  } else {
    std::cerr << "GetModelMathStr Can't open modelfile: " << _modelfile_ << " for reading\n";
    exit(1);
  }

  return ans_;
}
}
}

#endif /* DVCCODE_UTILS_MODEL_UTILS_HPP_ */
