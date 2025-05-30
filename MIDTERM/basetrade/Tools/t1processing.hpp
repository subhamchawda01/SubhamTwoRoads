/**
   \file Tools/t1processing.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
    Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_TOOLS_T1PROCESSING_HPP
#define BASE_TOOLS_T1PROCESSING_HPP

#include "basetrade/Tools/timed_data_to_reg_data_common.hpp"

namespace HFSAT {

void T1Processing(const std::string& input_data_filename_, SimpleLineProcessor& simple_line_processor_,
                  const bool is_returns_based_, const int msecs_to_predict_, DailyTradeVolumeReader* trade_vol_reader,
                  bool print_time_, unsigned int fsudm_filter_level_) {
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

      int base_msecs = atoi(base_tokens_[0]);
      bool found = false;
      double pred_value_ = 0;

      int t_pred_msecs_ = base_msecs;
      bool were_files_reset_ = false;

      // now find the first line at or after input_data_file_pred_ such that msecs_from_midnight >= (base_msecs +
      // msecs_to_predict_ )
      while (input_data_file_pred_.good()) {
        bzero(pred_line_buffer_, kDataFileLineBufferLen);
        last_good_offset_ = std::max(last_good_offset_, (int)input_data_file_pred_.tellg());
        input_data_file_pred_.getline(pred_line_buffer_, kDataFileLineBufferLen);
        PerishableStringTokenizer pred_st_(pred_line_buffer_, kDataFileLineBufferLen);
        const std::vector<const char*>& pred_tokens_ = pred_st_.GetTokens();
        if (pred_tokens_.size() < 3) continue;

        int pred_msecs = atoi(pred_tokens_[0]);
        if (pred_msecs > (base_msecs + msecs_to_predict_)) {
          found = true;
          pred_value_ = atof(pred_tokens_[2]);
          break;  // no need to read any more right now
        }

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
        // assuming msecs_from_midnight are in increasing order ... if pred_value_ not found once .. we won't find it
        // for any higher base_msecs
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

void InMemT1Processing(const InMemData* timeseries_, InMemData* change_data_, const int msecs_to_predict_,
                       const int number_of_dependants_, bool is_returns_based_ = false) {
  // we need one base_rowindex and one future_rowindex
  // first column is time and second column is ( events/trades )
  // we need to know number of dependants on which change is computed

  unsigned int base_row_index_ = 0;
  unsigned int future_row_index_ = 0;

  unsigned int num_columns_ = timeseries_->NumWords();
  unsigned int num_rows_ = timeseries_->NumLines();

  std::vector<double> base_line_(num_columns_, 0);
  std::vector<double> future_line_(num_columns_, 0);

  int base_msecs_ = 0;
  int future_msecs_ = 0;
  bool found_ = false;
  // base pointer is good
  while (base_row_index_ < num_rows_) {
    // future index is good
    found_ = false;
    timeseries_->GetRow(base_row_index_, base_line_);
    // we dont have check anywhere
    if (base_line_.size() < num_columns_) {
      std::cerr << "Malformed InMemData " << base_row_index_ << " row has less than expected number of words "
                << base_line_.size() << " instead " << num_columns_ << "\n";
      exit(-1);
    }
    base_msecs_ = base_line_[0];

    while (future_row_index_ < num_rows_) {
      timeseries_->GetRow(future_row_index_, future_line_);
      if (future_line_.size() < num_columns_) {
        std::cerr << "Malformed InMemData " << future_row_index_ << " row has less than expected number of words "
                  << future_line_.size() << " instead " << num_columns_ << "\n";
        exit(-1);
      }
      future_msecs_ = future_line_[0];
      if (future_msecs_ > (base_msecs_ + msecs_to_predict_)) {
        found_ = true;
        break;
      }
      future_row_index_++;
    }

    if (found_) {
      // factor_ would be good at this point, a vector to be used for all rows or matrix weights data
      // dependants here
      for (int dependants_ = 0; dependants_ < number_of_dependants_; dependants_++) {
        register double change_value_ = future_line_[dependants_ + 2] - base_line_[dependants_ + 2];
        if (is_returns_based_) {
          change_value_ /= base_line_[dependants_ + 2];
        }
        change_data_->AddWord(change_value_);
      }

      // signal values here
      for (unsigned int signals_ = number_of_dependants_; signals_ < num_columns_ - 2; signals_++) {
        change_data_->AddWord(base_line_[2 + signals_]);
      }
      change_data_->FinishLine();
    } else {
      break;
    }
    base_row_index_++;
  }
}
}
#endif
