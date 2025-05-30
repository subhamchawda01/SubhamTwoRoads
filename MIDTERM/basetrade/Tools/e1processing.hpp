/**
   \file Tools/e1processing.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
    Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_TOOLS_E1PROCESSING_HPP
#define BASE_TOOLS_E1PROCESSING_HPP

#include "basetrade/Tools/timed_data_to_reg_data_common.hpp"

namespace HFSAT {

void E1Processing(const std::string& input_data_filename_, SimpleLineProcessor& simple_line_processor_,
                  const bool is_returns_based_, const int counters_to_predict_,
                  DailyTradeVolumeReader* trade_vol_reader, bool print_time_, unsigned int fsudm_filter_level_) {
  std::ifstream input_data_file_base_;
  input_data_file_base_.open(input_data_filename_.c_str(), std::ifstream::in);

  std::ifstream input_data_file_pred_;
  input_data_file_pred_.open(input_data_filename_.c_str(), std::ifstream::in);

  std::vector<std::ifstream*> ifstream_vec_;
  ifstream_vec_.push_back(&input_data_file_base_);
  ifstream_vec_.push_back(&input_data_file_pred_);

  int last_good_offset_ = 0;

  if (input_data_file_base_.is_open() && input_data_file_pred_.is_open()) {
    const int kDataFileLineBufferLen = 10240;

    char base_line_buffer_[kDataFileLineBufferLen];
    bzero(base_line_buffer_, kDataFileLineBufferLen);
    char pred_line_buffer_[kDataFileLineBufferLen];
    bzero(pred_line_buffer_, kDataFileLineBufferLen);

    while (input_data_file_base_.good()) {
      bzero(base_line_buffer_, kDataFileLineBufferLen);
      last_good_offset_ = std::max(last_good_offset_, (int)input_data_file_base_.tellg());
      input_data_file_base_.getline(base_line_buffer_, kDataFileLineBufferLen);
      PerishableStringTokenizer base_st_(base_line_buffer_, kDataFileLineBufferLen);
      const std::vector<const char*>& base_tokens_ = base_st_.GetTokens();
      if (base_tokens_.size() < 4) continue;

      unsigned long long base_event_count = atol(base_tokens_[1]);
      bool found = false;
      double pred_value_ = 0;

      int base_msecs = atoi(base_tokens_[0]);
      int t_pred_msecs_ = base_msecs;
      bool were_files_reset_ = false;

      // now find the first line at or after input_data_file_pred_ such that events_from_midnight >= (base_events +
      // counters_to_predict_ )
      while (input_data_file_pred_.good()) {
        bzero(pred_line_buffer_, kDataFileLineBufferLen);
        last_good_offset_ = std::max(last_good_offset_, (int)input_data_file_pred_.tellg());
        input_data_file_pred_.getline(pred_line_buffer_, kDataFileLineBufferLen);
        PerishableStringTokenizer pred_st_(pred_line_buffer_, kDataFileLineBufferLen);
        const std::vector<const char*>& pred_tokens_ = pred_st_.GetTokens();
        if (pred_tokens_.size() < 3) continue;

        unsigned long long pred_event_count = atol(pred_tokens_[1]);
        if (pred_event_count > (base_event_count + counters_to_predict_)) {
          found = true;
          pred_value_ = atof(pred_tokens_[2]);
          break;  // no need to read any more right now
        }

        int pred_msecs = atoi(pred_tokens_[0]);
        if (t_pred_msecs_ > pred_msecs) {
          ResetFilesToPos(ifstream_vec_, last_good_offset_);
          were_files_reset_ = true;
          break;
        }
        t_pred_msecs_ = pred_msecs;
      }

      if (were_files_reset_) {
        continue;
      }

      if (found) {
        double factor = trade_vol_reader == NULL ? 1.0 : trade_vol_reader->getTradedVolRatioAtTime(base_msecs / 1000);
        register double base_value_ = atof(base_tokens_[3]);
        register double value_to_model = (pred_value_ - base_value_);
        if (is_returns_based_) {
          value_to_model /= base_value_;
        }

        if (fsudm_filter_level_ == 1) {
          factor = factor * std::fabs(value_to_model);
        } else if (fsudm_filter_level_ == 2) {
          factor = factor * value_to_model * value_to_model;
        } else if (fsudm_filter_level_ == 3) {
          factor = factor * std::fabs(value_to_model) * value_to_model * value_to_model;
        }

        simple_line_processor_.AddWord(factor * value_to_model);

        // now print the variables
        for (unsigned int i = 4u; i < base_tokens_.size(); i++) {
          simple_line_processor_.AddWord(factor * atof(base_tokens_[i]));
        }

        if (print_time_) {
          simple_line_processor_.AddWord(base_msecs);
        }
        simple_line_processor_.FinishLine();
      } else {
        // assuming event_count_from_midnight are in increasing order ... if pred_value_ not found once .. we won't find
        // it for any higher base_event_count
        // hence break
        break;
      }
    }

    simple_line_processor_.Close();
  }

  if (input_data_file_base_.is_open()) {
    input_data_file_base_.close();
  }
  if (input_data_file_pred_.is_open()) {
    input_data_file_pred_.close();
  }
}
}
#endif
