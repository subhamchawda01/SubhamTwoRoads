/**
   \file Tools/timed_data_to_reg_data_common.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
    Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#pragma once

#ifndef BASE_TOOLS_TIMEDDATATOREGDATACOMMON_HPP
#define BASE_TOOLS_TIMEDDATATOREGDATACOMMON_HPP

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/error_codes.hpp"
#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/Utils/in_memory_data.hpp"

#include "basetrade/MTools/data_processing.hpp"
#include "basetrade/Tools/simple_line_processor.hpp"
#include "basetrade/Tools/daily_volume_reader.hpp"

namespace HFSAT {

typedef enum {
  na_t1,
  na_t3,
  na_t5,
  na_e1,
  na_e3,
  na_e5,
  ac_e3,
  ac_e5,
  na_s4,
  na_sall,
  na_mult,
  na_m4,
  na_t3_bd
} NormalizingAlgo;

NormalizingAlgo StringToNormalizingAlgo(const char *_text_) {
  if (strcmp(_text_, "na_t1") == 0)
    return na_t1;
  else if (strcmp(_text_, "na_t3") == 0)
    return na_t3;
  else if (strcmp(_text_, "na_t5") == 0)
    return na_t5;
  else if (strcmp(_text_, "na_e1") == 0)
    return na_e1;
  else if (strcmp(_text_, "na_e3") == 0)
    return na_e3;
  else if (strcmp(_text_, "na_e5") == 0)
    return na_e5;
  else if (strcmp(_text_, "ac_e3") == 0)
    return ac_e3;
  else if (strcmp(_text_, "ac_e5") == 0)
    return ac_e5;
  else if (strcmp(_text_, "na_s4") == 0)
    return na_s4;
  else if (strcmp(_text_, "na_sall") == 0)
    return na_sall;
  else if (strcmp(_text_, "na_mult") == 0)
    return na_mult;
  else if (strcmp(_text_, "na_m4") == 0)
    return na_m4;
  else if (strcmp(_text_, "na_t3_bd") == 0)
    return na_t3_bd;
  return na_t1;
}

/// Function to read the model filename and return by reference :
/// if the model is_returns_based_
bool GetDepCalcParams(const std::string &model_filename_, bool &is_returns_based_) {
  // format : ["MODELMATH"] [SignalAlgo] Change/Returns
  // MODELMATH LINEAR CHANGE
  // MODELMATH LINEAR RETURNS
  // MODELMATH NODEPLINEAR CHANGE
  // MODELMATH CLASSIFIER CHANGE
  // MODELMATH CART CHANGE
  // MODELMATH NEURALNETWORK CHANGE

  std::ifstream model_file_stream_;
  model_file_stream_.open(model_filename_.c_str(), std::ifstream::in);

  if (model_file_stream_.is_open()) {
    const int kModelFileLineBufferLen = 1024;

    char model_line_buffer_[kModelFileLineBufferLen];
    bzero(model_line_buffer_, kModelFileLineBufferLen);

    while (model_file_stream_.good()) {
      bzero(model_line_buffer_, kModelFileLineBufferLen);
      model_file_stream_.getline(model_line_buffer_, kModelFileLineBufferLen);
      PerishableStringTokenizer base_st_(model_line_buffer_, kModelFileLineBufferLen);
      const std::vector<const char *> &model_line_tokens_ = base_st_.GetTokens();
      if ((model_line_tokens_.size() >= 3) && (strcmp(model_line_tokens_[0], "MODELMATH") == 0)) {
        is_returns_based_ = (strcmp(model_line_tokens_[2], "RETURNS") == 0);
        break;
      }
    }
    model_file_stream_.close();
    return true;
  } else {
    std::cerr << " GetDepCalcParams could not open file " << model_filename_ << std::endl;
    exit(0);
  }
  return false;
}

bool GetIndicatorString(const std::string &model_filename_, int index_, std::string &indicator_string_) {
  std::ifstream model_file_stream_;
  model_file_stream_.open(model_filename_.c_str(), std::ifstream::in);

  indicator_string_ = "";
  int idx = 1;

  if (model_file_stream_.is_open()) {
    const int kModelFileLineBufferLen = 1024;

    char model_line_buffer_[kModelFileLineBufferLen];
    bzero(model_line_buffer_, kModelFileLineBufferLen);

    while (model_file_stream_.good()) {
      bzero(model_line_buffer_, kModelFileLineBufferLen);
      model_file_stream_.getline(model_line_buffer_, kModelFileLineBufferLen);
      PerishableStringTokenizer base_st_(model_line_buffer_, kModelFileLineBufferLen);
      const std::vector<const char *> &model_line_tokens_ = base_st_.GetTokens();

      if ((model_line_tokens_.size() >= 3) && (strcmp(model_line_tokens_[0], "INDICATOR") == 0)) {
        if (idx == index_) {
          for (unsigned int i = 2; i < model_line_tokens_.size(); i++) {
            if (strcmp(model_line_tokens_[i], "#") == 0) {
              break;
            }
            indicator_string_ += std::string(model_line_tokens_[i]) + " ";
          }
          model_file_stream_.close();
          return true;
        } else {
          idx++;
        }
      }
    }
  }

  model_file_stream_.close();
  return false;
}

void ResetFilesToPos(std::vector<std::ifstream *> &ifstream_vec_, const int target_pos_) {
  for (unsigned int ifs_ = 0; ifs_ < ifstream_vec_.size(); ++ifs_) {
    ifstream_vec_[ifs_]->seekg(target_pos_);
  }
}
}
#endif
