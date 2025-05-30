/**
   \file Tools/t3processing.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
    Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_TOOLS_T3PROCESSING_HPP
#define BASE_TOOLS_T3PROCESSING_HPP

#include "basetrade/Tools/timed_data_to_reg_data_common.hpp"

namespace HFSAT {

void T3Processing(const std::string& input_data_filename_, SimpleLineProcessor& simple_line_processor_,
                  const bool is_returns_based_, const int msecs_to_predict_, DailyTradeVolumeReader* trade_vol_reader,
                  bool print_time_, unsigned fsudm_filter_level_) {
  std::ifstream input_data_file_base_;
  input_data_file_base_.open(input_data_filename_.c_str(), std::ifstream::in);

  std::ifstream input_data_file_pred1_;
  input_data_file_pred1_.open(input_data_filename_.c_str(), std::ifstream::in);

  std::ifstream input_data_file_pred2_;
  input_data_file_pred2_.open(input_data_filename_.c_str(), std::ifstream::in);

  std::ifstream input_data_file_pred3_;
  input_data_file_pred3_.open(input_data_filename_.c_str(), std::ifstream::in);

  std::vector<std::ifstream*> ifstream_vec_;
  ifstream_vec_.push_back(&input_data_file_base_);
  ifstream_vec_.push_back(&input_data_file_pred1_);
  ifstream_vec_.push_back(&input_data_file_pred2_);
  ifstream_vec_.push_back(&input_data_file_pred3_);

  int last_good_offset_ = 0;

  int msecs1_to_predict_ = std::max(1, (msecs_to_predict_ * 1) / 3);
  int msecs2_to_predict_ = std::max(1, (msecs_to_predict_ * 2) / 3);
  int msecs3_to_predict_ = msecs_to_predict_;

  // int msecs1_to_predict_ = msecs_to_predict_;
  // int msecs2_to_predict_ = 2 * msecs_to_predict_;
  // int msecs3_to_predict_ = 3 * msecs_to_predict_;

  // // Not just taking average but multiplying by sqrt(3.0) ensures that
  // // the stdev of pred_value ( Fut3, t seconds ) is roughly similar to pred_value ( Fut1, t seconds )
  // double nfac1_ = 1/sqrt(3.0); ///< Sqrt(3) * 1/3
  // double nfac2_ = 1/sqrt(6.0); ///< Sqrt(3) * 1/3 * 1/Sqrt(2)
  // double nfac3_ = 1/3.0; ///< Sqrt(3) * 1/3 * 1/Sqrt(3)
  // tried the above logic but stdev ( pred ( Fut3, t seconds ) ) was coming to be 1.4 * stdev ( pred ( Fut1, t seconds
  // ) )
  double nfac1_ = 4.0 / 9.0;
  double nfac2_ = 3.0 / 9.0;
  double nfac3_ = 2.0 / 9.0;

  if (input_data_file_base_.is_open() && input_data_file_pred1_.is_open() && input_data_file_pred2_.is_open() &&
      input_data_file_pred3_.is_open()) {
    const int kDataFileLineBufferLen = 10240;

    char base_line_buffer_[kDataFileLineBufferLen];
    bzero(base_line_buffer_, kDataFileLineBufferLen);

    char pred1_line_buffer_[kDataFileLineBufferLen];
    bzero(pred1_line_buffer_, kDataFileLineBufferLen);
    char pred2_line_buffer_[kDataFileLineBufferLen];
    bzero(pred2_line_buffer_, kDataFileLineBufferLen);
    char pred3_line_buffer_[kDataFileLineBufferLen];
    bzero(pred3_line_buffer_, kDataFileLineBufferLen);

    while (input_data_file_base_.good()) {
      bzero(base_line_buffer_, kDataFileLineBufferLen);
      last_good_offset_ = std::max(last_good_offset_, (int)input_data_file_base_.tellg());
      input_data_file_base_.getline(base_line_buffer_, kDataFileLineBufferLen);
      PerishableStringTokenizer base_st_(base_line_buffer_, kDataFileLineBufferLen);
      const std::vector<const char*>& base_tokens_ = base_st_.GetTokens();
      if (base_tokens_.size() < 4) continue;

      int base_msecs = atoi(base_tokens_[0]);
      double base_value_ = atof(base_tokens_[3]);
      double pred1_value_ = 0;
      double pred2_value_ = 0;
      double pred3_value_ = 0;

      int t_pred1_msecs_ = base_msecs;
      bool were_files_reset_ = false;

      bool found_ = false;
      // now find the first line at or after input_data_file_pred1_ such that msecs_from_midnight >= (base_msecs +
      // msecs1_to_predict_ )
      while (input_data_file_pred1_.good()) {
        bzero(pred1_line_buffer_, kDataFileLineBufferLen);
        last_good_offset_ = std::max(last_good_offset_, (int)input_data_file_pred1_.tellg());
        input_data_file_pred1_.getline(pred1_line_buffer_, kDataFileLineBufferLen);
        PerishableStringTokenizer pred1_st_(pred1_line_buffer_, kDataFileLineBufferLen);
        const std::vector<const char*>& pred1_tokens_ = pred1_st_.GetTokens();
        if (pred1_tokens_.size() < 3) continue;

        int pred1_msecs = atoi(pred1_tokens_[0]);
        if (pred1_msecs > (base_msecs + msecs1_to_predict_)) {
          found_ = true;
          pred1_value_ = atof(pred1_tokens_[2]) - base_value_;  // moved subtraction of base_value_ to here instead of
                                                                // at the end so that the averaging is done of changes
                                                                // and not prices
          break;                                                // no need to read any more right now
        }

        if (t_pred1_msecs_ > pred1_msecs) {
          ResetFilesToPos(ifstream_vec_, last_good_offset_);
          were_files_reset_ = true;
          break;
        }
        t_pred1_msecs_ = pred1_msecs;
      }

      if (were_files_reset_) {
        continue;
      }

      if (!found_) {
        // assuming msecs_from_midnight are in increasing order ... if pred1_value_ not found once .. we won't find it
        // for any higher base_msecs
        // hence break
        break;
      }

      int t_pred2_msecs_ = base_msecs;
      were_files_reset_ = false;

      found_ = false;  // reseting for next value
      // now find the first line at or after input_data_file_pred2_ such that msecs_from_midnight >= (base_msecs +
      // msecs2_to_predict_ )
      while (input_data_file_pred2_.good()) {
        bzero(pred2_line_buffer_, kDataFileLineBufferLen);
        last_good_offset_ = std::max(last_good_offset_, (int)input_data_file_pred2_.tellg());
        input_data_file_pred2_.getline(pred2_line_buffer_, kDataFileLineBufferLen);
        PerishableStringTokenizer pred2_st_(pred2_line_buffer_, kDataFileLineBufferLen);
        const std::vector<const char*>& pred2_tokens_ = pred2_st_.GetTokens();
        if (pred2_tokens_.size() < 3) continue;

        int pred2_msecs = atoi(pred2_tokens_[0]);
        if (pred2_msecs > (base_msecs + msecs2_to_predict_)) {
          found_ = true;
          pred2_value_ = atof(pred2_tokens_[2]) - base_value_;
          break;  // no need to read any more right now
        }

        if (t_pred2_msecs_ > pred2_msecs) {
          ResetFilesToPos(ifstream_vec_, last_good_offset_);
          were_files_reset_ = true;
          break;
        }
        t_pred2_msecs_ = pred2_msecs;
      }

      if (were_files_reset_) {
        continue;
      }

      if (!found_) {
        // assuming msecs_from_midnight are in increasing order ... if pred2_value_ not found once .. we won't find it
        // for any higher base_msecs
        // hence break
        break;
      }

      int t_pred3_msecs_ = base_msecs;
      were_files_reset_ = false;

      found_ = false;
      // now find the first line at or after input_data_file_pred3_ such that msecs_from_midnight >= (base_msecs +
      // msecs3_to_predict_ )
      while (input_data_file_pred3_.good()) {
        bzero(pred3_line_buffer_, kDataFileLineBufferLen);
        last_good_offset_ = std::max(last_good_offset_, (int)input_data_file_pred3_.tellg());
        input_data_file_pred3_.getline(pred3_line_buffer_, kDataFileLineBufferLen);
        PerishableStringTokenizer pred3_st_(pred3_line_buffer_, kDataFileLineBufferLen);
        const std::vector<const char*>& pred3_tokens_ = pred3_st_.GetTokens();
        if (pred3_tokens_.size() < 3) continue;

        int pred3_msecs = atoi(pred3_tokens_[0]);
        if (pred3_msecs > (base_msecs + msecs3_to_predict_)) {
          found_ = true;
          pred3_value_ = atof(pred3_tokens_[2]) - base_value_;
          break;  // no need to read any more right now
        }

        if (t_pred3_msecs_ > pred3_msecs) {
          ResetFilesToPos(ifstream_vec_, last_good_offset_);
          were_files_reset_ = true;
          break;
        }
        t_pred3_msecs_ = pred3_msecs;
      }

      if (were_files_reset_) {
        continue;
      }

      if (!found_) {
        // assuming msecs_from_midnight are in increasing order ... if pred_value_ not found once .. we won't find it
        // for any higher base_msecs
        // hence break
        break;
      }

      if (found_) {  // need not check "found" again since break would have been called otherwise
        double factor = trade_vol_reader == NULL ? 1.0 : trade_vol_reader->getTradedVolRatioAtTime(base_msecs / 1000);
        double pred_value_ = (nfac1_ * pred1_value_) + (nfac2_ * pred2_value_) + (nfac3_ * pred3_value_);
        double value_to_model = pred_value_;
        if (is_returns_based_) {
          value_to_model = (pred_value_ / base_value_);
        }

        if (fsudm_filter_level_ == 1) {
          factor = factor * std::fabs(value_to_model);
        } else if (fsudm_filter_level_ == 2) {
          factor = factor * value_to_model * value_to_model;
        } else if (fsudm_filter_level_ == 3) {
          factor = factor * std::fabs(value_to_model) * value_to_model * value_to_model;
        }

        // print value_to_model
        simple_line_processor_.AddWord(factor * value_to_model);

        // now print the variables
        for (unsigned int i = 4u; i < base_tokens_.size(); i++) {
          simple_line_processor_.AddWord(factor * atof(base_tokens_[i]));
        }

        if (print_time_) {
          simple_line_processor_.AddWord(base_msecs);
        }

        simple_line_processor_.FinishLine();
      }
    }

    simple_line_processor_.Close();
  }

  if (input_data_file_pred1_.is_open()) {
    input_data_file_pred1_.close();
  }
  if (input_data_file_pred2_.is_open()) {
    input_data_file_pred2_.close();
  }
  if (input_data_file_pred3_.is_open()) {
    input_data_file_pred3_.close();
  }

  if (input_data_file_base_.is_open()) {
    input_data_file_base_.close();
  }
}

void InMemT3Processing(const InMemData* timeseries_, InMemData* change_data_, const int msecs_to_predict_,
                       const int number_of_dependants_, bool is_returns_based_ = false) {
  // we need one base_rowindex and three future_rowindices
  // first column is time and second column is ( events/trades )
  // we need to know number of dependants on which change is computed

  unsigned int base_row_index_ = 0;
  unsigned int future_row_index1_ = 0;
  unsigned int future_row_index2_ = 0;
  unsigned int future_row_index3_ = 0;

  unsigned int num_columns_ = timeseries_->NumWords();
  unsigned int num_rows_ = timeseries_->NumLines();

  std::vector<double> base_line_(num_columns_, 0);
  std::vector<double> future_line1_(num_columns_, 0);
  std::vector<double> future_line2_(num_columns_, 0);
  std::vector<double> future_line3_(num_columns_, 0);

  int base_msecs_ = 0;
  int future_msecs1_ = 0;
  int future_msecs2_ = 0;
  int future_msecs3_ = 0;

  bool found_ = false;

  int msecs_to_predict1_ = std::max(1, (msecs_to_predict_ * 1) / 3);
  int msecs_to_predict2_ = std::max(1, (msecs_to_predict_ * 2) / 3);
  int msecs_to_predict3_ = msecs_to_predict_;

  // weights for each change
  double nfac1_ = 4.0 / 9.0;
  double nfac2_ = 3.0 / 9.0;
  double nfac3_ = 2.0 / 9.0;

  // base pointer is good
  while (base_row_index_ < num_rows_) {
    found_ = false;
    // future index is good
    timeseries_->GetRow(base_row_index_, base_line_);
    // we dont have check anywhere
    if (base_line_.size() < num_columns_) {
      std::cerr << "Malformed InMemData " << base_row_index_ << " row has less than expected number of words "
                << base_line_.size() << " instead " << num_columns_ << "\n";
      exit(-1);
    }
    base_msecs_ = base_line_[0];

    while (future_row_index1_ < num_rows_) {
      timeseries_->GetRow(future_row_index1_, future_line1_);
      if (future_line1_.size() < num_columns_) {
        std::cerr << "Malformed InMemData " << future_row_index1_ << " row has less than expected number of words "
                  << future_line1_.size() << " instead " << num_columns_ << "\n";
        exit(-1);
      }
      future_msecs1_ = future_line1_[0];
      if (future_msecs1_ > (base_msecs_ + msecs_to_predict1_) && future_msecs1_ > base_msecs_) {
        found_ = true;
        break;
      }
      future_row_index1_++;
    }
    if (!found_) {
      break;
    }

    found_ = false;

    while (future_row_index2_ < num_rows_) {
      timeseries_->GetRow(future_row_index2_, future_line2_);
      if (future_line2_.size() < num_columns_) {
        std::cerr << "Malformed InMemData " << future_row_index2_ << " row has less than expected number of words "
                  << future_line2_.size() << " instead " << num_columns_ << "\n";
        exit(-1);
      }
      future_msecs2_ = future_line2_[0];
      if (future_msecs2_ > (base_msecs_ + msecs_to_predict2_) && future_msecs2_ > base_msecs_) {
        found_ = true;
        break;
      }
      future_row_index2_++;
    }
    if (!found_) {
      break;
    }

    found_ = false;
    while (future_row_index3_ < num_rows_) {
      timeseries_->GetRow(future_row_index3_, future_line3_);
      if (future_line3_.size() < num_columns_) {
        std::cerr << "Malformed InMemData " << future_row_index3_ << " row has less than expected number of words "
                  << future_line3_.size() << " instead " << num_columns_ << "\n";
        exit(-1);
      }
      future_msecs3_ = future_line3_[0];
      if (future_msecs3_ > (base_msecs_ + msecs_to_predict3_) && future_msecs3_ > base_msecs_) {
        found_ = true;
        break;
      }
      future_row_index3_++;
    }
    if (!found_) {
      break;
    }

    if (found_) {
      // factor_ would be good at this point, a vector to be used for all rows or matrix weights data
      // dependants here
      for (int dependants_ = 0; dependants_ < number_of_dependants_; dependants_++) {
        register double change_value1_ = future_line1_[dependants_ + 2] - base_line_[dependants_ + 2];
        register double change_value2_ = future_line2_[dependants_ + 2] - base_line_[dependants_ + 2];
        register double change_value3_ = future_line3_[dependants_ + 2] - base_line_[dependants_ + 2];

        register double change_value_ = nfac1_ * change_value1_ + nfac2_ * change_value2_ + nfac3_ * change_value3_;

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
