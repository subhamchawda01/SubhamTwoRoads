/**
   \file Tools/ace3processing.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
    Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_TOOLS_ACE3PROCESSING_HPP
#define BASE_TOOLS_ACE3PROCESSING_HPP

#include "basetrade/Tools/timed_data_to_reg_data_common.hpp"

namespace HFSAT {

void ACE3Processing(const std::string& input_data_filename_, SimpleLineProcessor& simple_line_processor_,
                    const bool is_returns_based_, const int counters_to_predict_,
                    DailyTradeVolumeReader* trade_vol_reader, bool print_time_, unsigned int fsudm_filter_level_) {
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

  int event_count1_to_predict_ = std::max(1, (counters_to_predict_ * 1) / 3);
  int event_count2_to_predict_ = std::max(1, (counters_to_predict_ * 2) / 3);
  int event_count3_to_predict_ = counters_to_predict_;

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

  std::vector<std::vector<double> > dep_with_price_;
  std::vector<std::vector<double> > indicators_;
  std::vector<int> base_msec_vec_;
  dep_with_price_.resize(2);

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

      unsigned long long base_event_count = atol(base_tokens_[1]);
      double base_value_ = atof(base_tokens_[3]);
      double pred1_value_ = 0;
      double pred2_value_ = 0;
      double pred3_value_ = 0;

      int base_msecs = atoi(base_tokens_[0]);
      int t_pred1_msecs_ = base_msecs;
      bool were_files_reset_ = false;

      bool found = false;
      // now find the first line at or after input_data_file_pred1_ such that event_count_from_midnight >=
      // (base_event_count + event_count1_to_predict_ )
      while (input_data_file_pred1_.good()) {
        bzero(pred1_line_buffer_, kDataFileLineBufferLen);
        last_good_offset_ = std::max(last_good_offset_, (int)input_data_file_pred1_.tellg());
        input_data_file_pred1_.getline(pred1_line_buffer_, kDataFileLineBufferLen);
        PerishableStringTokenizer pred1_st_(pred1_line_buffer_, kDataFileLineBufferLen);
        const std::vector<const char*>& pred1_tokens_ = pred1_st_.GetTokens();
        if (pred1_tokens_.size() < 3) continue;

        unsigned long long pred1_event_count = atol(pred1_tokens_[1]);
        if (pred1_event_count > (base_event_count + event_count1_to_predict_)) {
          found = true;
          pred1_value_ = atof(pred1_tokens_[2]) - base_value_;  // moved subtraction of base_value_ to here instead of
                                                                // at the end so that the averaging is done of changes
                                                                // and not prices
          break;                                                // no need to read any more right now
        }

        int pred1_msecs = atoi(pred1_tokens_[0]);
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
        // assuming event_count_from_midnight are in increasing order ... if pred1_value_ not found once .. we won't
        // find it for any higher base_event_count
        // hence break
        break;
      }

      int t_pred2_msecs_ = base_msecs;
      were_files_reset_ = false;

      found = false;  // reseting for next value
      // now find the first line at or after input_data_file_pred2_ such that event_count_from_midnight >=
      // (base_event_count + event_count2_to_predict_ )
      while (input_data_file_pred2_.good()) {
        bzero(pred2_line_buffer_, kDataFileLineBufferLen);
        last_good_offset_ = std::max(last_good_offset_, (int)input_data_file_pred2_.tellg());
        input_data_file_pred2_.getline(pred2_line_buffer_, kDataFileLineBufferLen);
        PerishableStringTokenizer pred2_st_(pred2_line_buffer_, kDataFileLineBufferLen);
        const std::vector<const char*>& pred2_tokens_ = pred2_st_.GetTokens();
        if (pred2_tokens_.size() < 3) continue;

        unsigned long long pred2_event_count = atol(pred2_tokens_[1]);
        if (pred2_event_count > (base_event_count + event_count2_to_predict_)) {
          found = true;
          pred2_value_ = atof(pred2_tokens_[2]) - base_value_;
          break;  // no need to read any more right now
        }

        int pred2_msecs = atoi(pred2_tokens_[0]);
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
        // assuming event_count_from_midnight are in increasing order ... if pred2_value_ not found once .. we won't
        // find it for any higher base_event_count
        // hence break
        break;
      }

      int t_pred3_msecs_ = base_msecs;
      were_files_reset_ = false;

      found = false;
      // now find the first line at or after input_data_file_pred3_ such that event_count_from_midnight >=
      // (base_event_count + event_count3_to_predict_ )
      while (input_data_file_pred3_.good()) {
        bzero(pred3_line_buffer_, kDataFileLineBufferLen);
        last_good_offset_ = std::max(last_good_offset_, (int)input_data_file_pred3_.tellg());
        input_data_file_pred3_.getline(pred3_line_buffer_, kDataFileLineBufferLen);
        PerishableStringTokenizer pred3_st_(pred3_line_buffer_, kDataFileLineBufferLen);
        const std::vector<const char*>& pred3_tokens_ = pred3_st_.GetTokens();
        if (pred3_tokens_.size() < 3) continue;

        unsigned long long pred3_event_count = atol(pred3_tokens_[1]);
        if (pred3_event_count > (base_event_count + event_count3_to_predict_)) {
          found = true;
          pred3_value_ = atof(pred3_tokens_[2]) - base_value_;
          break;  // no need to read any more right now
        }

        int pred3_msecs = atoi(pred3_tokens_[0]);
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
        // assuming event_count_from_midnight are in increasing order ... if pred_value_ not found once .. we won't find
        // it for any higher base_event_count
        // hence break
        break;
      }

      if (found) {  // need not check "found" again since break would have been called otherwise
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

        if (indicators_.empty() && (base_tokens_.size() > 4u)) {
          indicators_.resize(base_tokens_.size() - 4u);
        }

        if (base_tokens_.size() == indicators_.size() + 4u) {
          dep_with_price_[0].push_back(factor * value_to_model);
          dep_with_price_[1].push_back(factor * base_value_);

          // now print the variables
          for (auto i = 0u; i < indicators_.size(); i++) {
            indicators_[i].push_back(factor * atof(base_tokens_[i + 4u]));
          }
          base_msec_vec_.push_back(base_msecs);
        }
      }
    }
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

  if (!dep_with_price_.empty()) {
    VectorUtils::CalcAndRemoveMeanFromSeries(dep_with_price_[0]);
    VectorUtils::CalcAndRemoveMeanFromSeries(dep_with_price_[1]);

    double mean_rev_beta_ = GetSLRCoeffNoMean(dep_with_price_[0], dep_with_price_[1]);
    VectorUtils::ScaledVectorAddition(dep_with_price_[0], dep_with_price_[1], -mean_rev_beta_);
    for (auto i = 0u; i < dep_with_price_[0].size(); i++) {
      // print value_to_model
      simple_line_processor_.AddWord(dep_with_price_[0][i]);

      // now print the variables
      for (unsigned int j = 0u; j < indicators_.size(); j++) {
        simple_line_processor_.AddWord(indicators_[j][i]);
      }

      if (print_time_) {
        simple_line_processor_.AddWord(base_msec_vec_[i]);
        //  simple_line_processor_.AddWord ( base_msecs );
      }
      simple_line_processor_.FinishLine();
    }
    simple_line_processor_.Close();
  }
}
}
#endif
