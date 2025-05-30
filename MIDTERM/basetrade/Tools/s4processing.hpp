/**
   \file Tools/s4processing.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
    Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_TOOLS_S4PROCESSING_HPP
#define BASE_TOOLS_S4PROCESSING_HPP

#include "basetrade/Tools/timed_data_to_reg_data_common.hpp"

namespace HFSAT {

void S4Processing(const std::string& input_data_filename_, SimpleLineProcessor& simple_line_processor_,
                  const bool is_returns_based_, const int msecs_to_predict_, DailyTradeVolumeReader* trade_vol_reader,
                  bool print_time_) {
  std::ifstream input_data_file_base_;
  input_data_file_base_.open(input_data_filename_.c_str(), std::ifstream::in);

  std::ifstream input_data_file_pred1_;
  input_data_file_pred1_.open(input_data_filename_.c_str(), std::ifstream::in);

  std::ifstream input_data_file_pred2_;
  input_data_file_pred2_.open(input_data_filename_.c_str(), std::ifstream::in);

  std::ifstream input_data_file_pred3_;
  input_data_file_pred3_.open(input_data_filename_.c_str(), std::ifstream::in);

  std::ifstream input_data_file_pred4_;
  input_data_file_pred4_.open(input_data_filename_.c_str(), std::ifstream::in);

  std::ifstream input_data_file_pred5_;
  input_data_file_pred5_.open(input_data_filename_.c_str(), std::ifstream::in);

  std::ifstream input_data_file_pred6_;
  input_data_file_pred6_.open(input_data_filename_.c_str(), std::ifstream::in);

  std::ifstream input_data_file_pred7_;
  input_data_file_pred7_.open(input_data_filename_.c_str(), std::ifstream::in);

  std::ifstream input_data_file_pred8_;
  input_data_file_pred8_.open(input_data_filename_.c_str(), std::ifstream::in);

  std::vector<std::ifstream*> ifstream_vec_;
  ifstream_vec_.push_back(&input_data_file_base_);
  ifstream_vec_.push_back(&input_data_file_pred1_);
  ifstream_vec_.push_back(&input_data_file_pred2_);
  ifstream_vec_.push_back(&input_data_file_pred3_);
  ifstream_vec_.push_back(&input_data_file_pred4_);
  ifstream_vec_.push_back(&input_data_file_pred5_);
  ifstream_vec_.push_back(&input_data_file_pred6_);
  ifstream_vec_.push_back(&input_data_file_pred7_);
  ifstream_vec_.push_back(&input_data_file_pred8_);

  int last_good_offset_ = 0;

  int msecs8_to_predict_ = msecs_to_predict_;
  int msecs7_to_predict_ = std::max(1, (msecs8_to_predict_ / 2));
  int msecs6_to_predict_ = std::max(1, (msecs7_to_predict_ / 2));
  int msecs5_to_predict_ = std::max(1, (msecs6_to_predict_ / 2));
  int msecs4_to_predict_ = std::max(1, (msecs5_to_predict_ / 2));
  int msecs3_to_predict_ = std::max(1, (msecs4_to_predict_ / 2));
  int msecs2_to_predict_ = std::max(1, (msecs3_to_predict_ / 2));
  int msecs1_to_predict_ = std::max(1, (msecs2_to_predict_ / 2));

  if (input_data_file_base_.is_open() && input_data_file_pred1_.is_open() && input_data_file_pred2_.is_open() &&
      input_data_file_pred3_.is_open() && input_data_file_pred4_.is_open() && input_data_file_pred5_.is_open() &&
      input_data_file_pred6_.is_open() && input_data_file_pred7_.is_open() && input_data_file_pred8_.is_open()) {
    const int kDataFileLineBufferLen = 10240;

    char base_line_buffer_[kDataFileLineBufferLen];
    bzero(base_line_buffer_, kDataFileLineBufferLen);

    char pred1_line_buffer_[kDataFileLineBufferLen];
    bzero(pred1_line_buffer_, kDataFileLineBufferLen);
    char pred2_line_buffer_[kDataFileLineBufferLen];
    bzero(pred2_line_buffer_, kDataFileLineBufferLen);
    char pred3_line_buffer_[kDataFileLineBufferLen];
    bzero(pred3_line_buffer_, kDataFileLineBufferLen);
    char pred4_line_buffer_[kDataFileLineBufferLen];
    bzero(pred4_line_buffer_, kDataFileLineBufferLen);
    char pred5_line_buffer_[kDataFileLineBufferLen];
    bzero(pred5_line_buffer_, kDataFileLineBufferLen);
    char pred6_line_buffer_[kDataFileLineBufferLen];
    bzero(pred6_line_buffer_, kDataFileLineBufferLen);
    char pred7_line_buffer_[kDataFileLineBufferLen];
    bzero(pred7_line_buffer_, kDataFileLineBufferLen);
    char pred8_line_buffer_[kDataFileLineBufferLen];
    bzero(pred8_line_buffer_, kDataFileLineBufferLen);

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
      double pred4_value_ = 0;
      double pred5_value_ = 0;
      double pred6_value_ = 0;
      double pred7_value_ = 0;
      double pred8_value_ = 0;

      int t_pred1_msecs_ = base_msecs;
      bool were_files_reset_ = false;

      bool found = false;
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
          found = true;
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

      if (!found) {
        // assuming msecs_from_midnight are in increasing order ... if pred1_value_ not found once .. we won't find it
        // for any higher base_msecs
        // hence break
        break;
      }

      int t_pred2_msecs_ = base_msecs;
      were_files_reset_ = false;

      found = false;  // reseting for next value
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
          found = true;
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

      if (!found) {
        // assuming msecs_from_midnight are in increasing order ... if pred2_value_ not found once .. we won't find it
        // for any higher base_msecs
        // hence break
        break;
      }

      int t_pred3_msecs_ = base_msecs;
      were_files_reset_ = false;

      found = false;
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
          found = true;
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

      if (!found) {
        // assuming msecs_from_midnight are in increasing order ... if pred_value_ not found once .. we won't find it
        // for any higher base_msecs
        // hence break
        break;
      }

      int t_pred4_msecs_ = base_msecs;
      were_files_reset_ = false;

      found = false;
      // now find the first line at or after input_data_file_pred4_ such that msecs_from_midnight >= (base_msecs +
      // msecs4_to_predict_ )
      while (input_data_file_pred4_.good()) {
        bzero(pred4_line_buffer_, kDataFileLineBufferLen);
        last_good_offset_ = std::max(last_good_offset_, (int)input_data_file_pred4_.tellg());
        input_data_file_pred4_.getline(pred4_line_buffer_, kDataFileLineBufferLen);
        PerishableStringTokenizer pred4_st_(pred4_line_buffer_, kDataFileLineBufferLen);
        const std::vector<const char*>& pred4_tokens_ = pred4_st_.GetTokens();
        if (pred4_tokens_.size() < 3) continue;

        int pred4_msecs = atoi(pred4_tokens_[0]);
        if (pred4_msecs > (base_msecs + msecs4_to_predict_)) {
          found = true;
          pred4_value_ = atof(pred4_tokens_[2]) - base_value_;
          break;  // no need to read any more right now
        }

        if (t_pred4_msecs_ > pred4_msecs) {
          ResetFilesToPos(ifstream_vec_, last_good_offset_);
          were_files_reset_ = true;
          break;
        }
        t_pred4_msecs_ = pred4_msecs;
      }

      if (were_files_reset_) {
        continue;
      }

      if (!found) {
        // assuming msecs_from_midnight are in increasing order ... if pred_value_ not found once .. we won't find it
        // for any higher base_msecs
        // hence break
        break;
      }

      int t_pred5_msecs_ = base_msecs;
      were_files_reset_ = false;

      found = false;
      // now find the first line at or after input_data_file_pred5_ such that msecs_from_midnight >= (base_msecs +
      // msecs5_to_predict_ )
      while (input_data_file_pred5_.good()) {
        bzero(pred5_line_buffer_, kDataFileLineBufferLen);
        last_good_offset_ = std::max(last_good_offset_, (int)input_data_file_pred5_.tellg());
        input_data_file_pred5_.getline(pred5_line_buffer_, kDataFileLineBufferLen);
        PerishableStringTokenizer pred5_st_(pred5_line_buffer_, kDataFileLineBufferLen);
        const std::vector<const char*>& pred5_tokens_ = pred5_st_.GetTokens();
        if (pred5_tokens_.size() < 3) continue;

        int pred5_msecs = atoi(pred5_tokens_[0]);
        if (pred5_msecs > (base_msecs + msecs5_to_predict_)) {
          found = true;
          pred5_value_ = atof(pred5_tokens_[2]) - base_value_;
          break;  // no need to read any more right now
        }

        if (t_pred5_msecs_ > pred5_msecs) {
          ResetFilesToPos(ifstream_vec_, last_good_offset_);
          were_files_reset_ = true;
          break;
        }
        t_pred5_msecs_ = pred5_msecs;
      }

      if (were_files_reset_) {
        continue;
      }

      if (!found) {
        // assuming msecs_from_midnight are in increasing order ... if pred_value_ not found once .. we won't find it
        // for any higher base_msecs
        // hence break
        break;
      }

      int t_pred6_msecs_ = base_msecs;
      were_files_reset_ = false;

      found = false;
      // now find the first line at or after input_data_file_pred6_ such that msecs_from_midnight >= (base_msecs +
      // msecs6_to_predict_ )
      while (input_data_file_pred6_.good()) {
        bzero(pred6_line_buffer_, kDataFileLineBufferLen);
        last_good_offset_ = std::max(last_good_offset_, (int)input_data_file_pred6_.tellg());
        input_data_file_pred6_.getline(pred6_line_buffer_, kDataFileLineBufferLen);
        PerishableStringTokenizer pred6_st_(pred6_line_buffer_, kDataFileLineBufferLen);
        const std::vector<const char*>& pred6_tokens_ = pred6_st_.GetTokens();
        if (pred6_tokens_.size() < 3) continue;

        int pred6_msecs = atoi(pred6_tokens_[0]);
        if (pred6_msecs > (base_msecs + msecs6_to_predict_)) {
          found = true;
          pred6_value_ = atof(pred6_tokens_[2]) - base_value_;
          break;  // no need to read any more right now
        }

        if (t_pred6_msecs_ > pred6_msecs) {
          ResetFilesToPos(ifstream_vec_, last_good_offset_);
          were_files_reset_ = true;
          break;
        }
        t_pred6_msecs_ = pred6_msecs;
      }

      if (were_files_reset_) {
        continue;
      }

      if (!found) {
        // assuming msecs_from_midnight are in increasing order ... if pred_value_ not found once .. we won't find it
        // for any higher base_msecs
        // hence break
        break;
      }

      int t_pred7_msecs_ = base_msecs;
      were_files_reset_ = false;

      found = false;
      // now find the first line at or after input_data_file_pred7_ such that msecs_from_midnight >= (base_msecs +
      // msecs7_to_predict_ )
      while (input_data_file_pred7_.good()) {
        bzero(pred7_line_buffer_, kDataFileLineBufferLen);
        last_good_offset_ = std::max(last_good_offset_, (int)input_data_file_pred7_.tellg());
        input_data_file_pred7_.getline(pred7_line_buffer_, kDataFileLineBufferLen);
        PerishableStringTokenizer pred7_st_(pred7_line_buffer_, kDataFileLineBufferLen);
        const std::vector<const char*>& pred7_tokens_ = pred7_st_.GetTokens();
        if (pred7_tokens_.size() < 3) continue;

        int pred7_msecs = atoi(pred7_tokens_[0]);
        if (pred7_msecs > (base_msecs + msecs7_to_predict_)) {
          found = true;
          pred7_value_ = atof(pred7_tokens_[2]) - base_value_;
          break;  // no need to read any more right now
        }

        if (t_pred7_msecs_ > pred7_msecs) {
          ResetFilesToPos(ifstream_vec_, last_good_offset_);
          were_files_reset_ = true;
          break;
        }
        t_pred7_msecs_ = pred7_msecs;
      }

      if (were_files_reset_) {
        continue;
      }

      if (!found) {
        // assuming msecs_from_midnight are in increasing order ... if pred_value_ not found once .. we won't find it
        // for any higher base_msecs
        // hence break
        break;
      }

      int t_pred8_msecs_ = base_msecs;
      were_files_reset_ = false;

      found = false;
      // now find the first line at or after input_data_file_pred8_ such that msecs_from_midnight >= (base_msecs +
      // msecs8_to_predict_ )
      while (input_data_file_pred8_.good()) {
        bzero(pred8_line_buffer_, kDataFileLineBufferLen);
        last_good_offset_ = std::max(last_good_offset_, (int)input_data_file_pred8_.tellg());
        input_data_file_pred8_.getline(pred8_line_buffer_, kDataFileLineBufferLen);
        PerishableStringTokenizer pred8_st_(pred8_line_buffer_, kDataFileLineBufferLen);
        const std::vector<const char*>& pred8_tokens_ = pred8_st_.GetTokens();
        if (pred8_tokens_.size() < 3) continue;

        int pred8_msecs = atoi(pred8_tokens_[0]);
        if (pred8_msecs > (base_msecs + msecs8_to_predict_)) {
          found = true;
          pred8_value_ = atof(pred8_tokens_[2]) - base_value_;
          break;  // no need to read any more right now
        }

        if (t_pred8_msecs_ > pred8_msecs) {
          ResetFilesToPos(ifstream_vec_, last_good_offset_);
          were_files_reset_ = true;
          break;
        }
        t_pred8_msecs_ = pred8_msecs;
      }

      if (were_files_reset_) {
        continue;
      }

      if (!found) {
        // assuming msecs_from_midnight are in increasing order ... if pred_value_ not found once .. we won't find it
        // for any higher base_msecs
        // hence break
        break;
      }

      if (found) {  // need not check "found" again since break would have been called otherwise
        double factor = trade_vol_reader == NULL ? 1.0 : trade_vol_reader->getTradedVolRatioAtTime(base_msecs / 1000);

        // print 4 dependant columns
        simple_line_processor_.AddWord((is_returns_based_)
                                           ? (factor * (pred1_value_ + pred2_value_) / (2.0 * base_value_))
                                           : (factor * (pred1_value_ + pred2_value_) / 2.0));
        simple_line_processor_.AddWord((is_returns_based_)
                                           ? (factor * (pred3_value_ + pred4_value_) / (2.0 * base_value_))
                                           : (factor * (pred3_value_ + pred4_value_) / 2.0));
        simple_line_processor_.AddWord((is_returns_based_)
                                           ? (factor * (pred5_value_ + pred6_value_) / (2.0 * base_value_))
                                           : (factor * (pred5_value_ + pred6_value_) / 2.0));
        simple_line_processor_.AddWord((is_returns_based_)
                                           ? (factor * (pred7_value_ + pred8_value_) / (2.0 * base_value_))
                                           : (factor * (pred7_value_ + pred8_value_) / 2.0));

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
  if (input_data_file_pred4_.is_open()) {
    input_data_file_pred4_.close();
  }
  if (input_data_file_pred5_.is_open()) {
    input_data_file_pred5_.close();
  }
  if (input_data_file_pred6_.is_open()) {
    input_data_file_pred6_.close();
  }
  if (input_data_file_pred7_.is_open()) {
    input_data_file_pred7_.close();
  }
  if (input_data_file_pred8_.is_open()) {
    input_data_file_pred8_.close();
  }

  if (input_data_file_base_.is_open()) {
    input_data_file_base_.close();
  }
}
}
#endif
