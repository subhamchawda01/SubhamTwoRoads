/**
   \file Tools/t1processing.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#include "basetrade/Tools/processing.hpp"
#ifndef BASE_TOOLS_T5PROCESSINGNEW_HPP
#define BASE_TOOLS_T5PROCESSINGNEW_HPP

namespace HFSAT {
class T5Processing_new : public Processing {
 protected:
  std::ifstream input_data_file_pred1_, input_data_file_pred2_, input_data_file_pred3_, input_data_file_pred4_,
      input_data_file_pred5_, input_data_file_pred6_;
  int msecs1_to_predict_, msecs2_to_predict_, msecs3_to_predict_, msecs4_to_predict_, msecs5_to_predict_;
  double nfac1_, nfac2_, nfac3_, nfac4_, nfac5_;

 public:
  T5Processing_new(const std::string& input_data_filename_, SimpleLineProcessor& _simple_line_processor_,
                   const bool _is_returns_based_, const int _msecs_to_predict_,
                   DailyTradeVolumeReader* _trade_vol_reader)
      : Processing(_simple_line_processor_, _is_returns_based_, _msecs_to_predict_, _trade_vol_reader) {
    input_data_file_pred1_.open(input_data_filename_.c_str(), std::ifstream::in);

    input_data_file_pred2_.open(input_data_filename_.c_str(), std::ifstream::in);

    input_data_file_pred3_.open(input_data_filename_.c_str(), std::ifstream::in);

    input_data_file_pred4_.open(input_data_filename_.c_str(), std::ifstream::in);

    input_data_file_pred5_.open(input_data_filename_.c_str(), std::ifstream::in);
    ifstream_vec_.push_back(&input_data_file_pred1_);
    ifstream_vec_.push_back(&input_data_file_pred2_);
    ifstream_vec_.push_back(&input_data_file_pred3_);
    ifstream_vec_.push_back(&input_data_file_pred4_);
    ifstream_vec_.push_back(&input_data_file_pred5_);
    msecs1_to_predict_ = std::max(1, (msecs_to_predict_ * 1) / 5);
    msecs2_to_predict_ = std::max(1, (msecs_to_predict_ * 2) / 5);
    msecs3_to_predict_ = std::max(1, (msecs_to_predict_ * 3) / 5);
    msecs4_to_predict_ = std::max(1, (msecs_to_predict_ * 4) / 5);
    msecs5_to_predict_ = msecs_to_predict_;
    nfac1_ = (1.00 / sqrt(1.00)) / ((1.00 / sqrt(1.00)) + (1.00 / sqrt(2.00)) + (1.00 / sqrt(3.00)) +
                                    (1.00 / sqrt(4.00)) + (1.00 / sqrt(5.00)));
    nfac2_ = (1.00 / sqrt(2.00)) / ((1.00 / sqrt(1.00)) + (1.00 / sqrt(2.00)) + (1.00 / sqrt(3.00)) +
                                    (1.00 / sqrt(4.00)) + (1.00 / sqrt(5.00)));
    nfac3_ = (1.00 / sqrt(3.00)) / ((1.00 / sqrt(1.00)) + (1.00 / sqrt(2.00)) + (1.00 / sqrt(3.00)) +
                                    (1.00 / sqrt(4.00)) + (1.00 / sqrt(5.00)));
    nfac4_ = (1.00 / sqrt(4.00)) / ((1.00 / sqrt(1.00)) + (1.00 / sqrt(2.00)) + (1.00 / sqrt(3.00)) +
                                    (1.00 / sqrt(4.00)) + (1.00 / sqrt(5.00)));
    nfac5_ = (1.00 / sqrt(5.00)) / ((1.00 / sqrt(1.00)) + (1.00 / sqrt(2.00)) + (1.00 / sqrt(3.00)) +
                                    (1.00 / sqrt(4.00)) + (1.00 / sqrt(5.00)));
  }
  void Process(int _base_msecs_, int _base_events_, bool& were_files_reset_, int& _last_good_offset_, bool& _found_,
               double& pred_value_, double base_value_) {
    if (input_data_file_pred1_.is_open() && input_data_file_pred2_.is_open() && input_data_file_pred3_.is_open() &&
        input_data_file_pred4_.is_open() && input_data_file_pred5_.is_open())

    {
      const int kDataFileLineBufferLen = 10240;

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
      int base_msecs = _base_msecs_;
      double pred1_value_ = 0;
      double pred2_value_ = 0;
      double pred3_value_ = 0;
      double pred4_value_ = 0;
      double pred5_value_ = 0;

      // now find the first line at or after input_data_file_pred_ such that msecs_from_midnight >= (base_msecs +
      // msecs_to_predict_ )
      while (input_data_file_pred1_.good()) {
        bzero(pred1_line_buffer_, kDataFileLineBufferLen);
        last_good_offset_ = std::max(last_good_offset_, (int)input_data_file_pred1_.tellg());
        input_data_file_pred1_.getline(pred1_line_buffer_, kDataFileLineBufferLen);
        PerishableStringTokenizer pred1_st_(pred1_line_buffer_, kDataFileLineBufferLen);
        const std::vector<const char*>& pred1_tokens_ = pred1_st_.GetTokens();
        if (pred1_tokens_.size() < 3) continue;

        int pred1_msecs = atoi(pred1_tokens_[0]);
        if (pred1_msecs > (base_msecs + msecs1_to_predict_)) {
          _found_ = true;
          pred1_value_ = atof(pred1_tokens_[2]) - base_value_;  // moved subtraction of base_value_ to here instead of
                                                                // at the end so that the averaging is done of changes
                                                                // and not prices
          break;                                                // no need to read any more right now
        }
      }
      if (!_found_) return;
      _found_ = false;
      while (input_data_file_pred2_.good()) {
        bzero(pred2_line_buffer_, kDataFileLineBufferLen);
        last_good_offset_ = std::max(last_good_offset_, (int)input_data_file_pred2_.tellg());
        input_data_file_pred2_.getline(pred2_line_buffer_, kDataFileLineBufferLen);
        PerishableStringTokenizer pred2_st_(pred2_line_buffer_, kDataFileLineBufferLen);
        const std::vector<const char*>& pred2_tokens_ = pred2_st_.GetTokens();
        if (pred2_tokens_.size() < 3) continue;

        int pred2_msecs = atoi(pred2_tokens_[0]);
        if (pred2_msecs > (base_msecs + msecs2_to_predict_)) {
          _found_ = true;
          pred2_value_ = atof(pred2_tokens_[2]) - base_value_;
          break;  // no need to read any more right now
        }
      }
      if (!_found_) return;
      _found_ = false;
      while (input_data_file_pred3_.good()) {
        bzero(pred3_line_buffer_, kDataFileLineBufferLen);
        last_good_offset_ = std::max(last_good_offset_, (int)input_data_file_pred3_.tellg());
        input_data_file_pred3_.getline(pred3_line_buffer_, kDataFileLineBufferLen);
        PerishableStringTokenizer pred3_st_(pred3_line_buffer_, kDataFileLineBufferLen);
        const std::vector<const char*>& pred3_tokens_ = pred3_st_.GetTokens();
        if (pred3_tokens_.size() < 3) continue;

        int pred3_msecs = atoi(pred3_tokens_[0]);
        if (pred3_msecs > (base_msecs + msecs3_to_predict_)) {
          _found_ = true;
          pred3_value_ = atof(pred3_tokens_[2]) - base_value_;
          break;  // no need to read any more right now
        }
      }
      if (!_found_) return;
      _found_ = false;
      while (input_data_file_pred4_.good()) {
        bzero(pred4_line_buffer_, kDataFileLineBufferLen);
        last_good_offset_ = std::max(last_good_offset_, (int)input_data_file_pred4_.tellg());
        input_data_file_pred4_.getline(pred4_line_buffer_, kDataFileLineBufferLen);
        PerishableStringTokenizer pred4_st_(pred4_line_buffer_, kDataFileLineBufferLen);
        const std::vector<const char*>& pred4_tokens_ = pred4_st_.GetTokens();
        if (pred4_tokens_.size() < 3) continue;

        int pred4_msecs = atoi(pred4_tokens_[0]);
        if (pred4_msecs > (base_msecs + msecs2_to_predict_)) {
          _found_ = true;
          pred4_value_ = atof(pred4_tokens_[2]) - base_value_;
          break;  // no need to read any more right now
        }
      }
      if (!_found_) return;
      _found_ = false;
      while (input_data_file_pred5_.good()) {
        bzero(pred5_line_buffer_, kDataFileLineBufferLen);
        last_good_offset_ = std::max(last_good_offset_, (int)input_data_file_pred5_.tellg());
        input_data_file_pred5_.getline(pred5_line_buffer_, kDataFileLineBufferLen);
        PerishableStringTokenizer pred5_st_(pred5_line_buffer_, kDataFileLineBufferLen);
        const std::vector<const char*>& pred5_tokens_ = pred5_st_.GetTokens();
        if (pred5_tokens_.size() < 3) continue;

        int pred5_msecs = atoi(pred5_tokens_[0]);
        if (pred5_msecs > (base_msecs + msecs2_to_predict_)) {
          _found_ = true;
          pred5_value_ = atof(pred5_tokens_[2]) - base_value_;
          break;  // no need to read any more right now
        }
      }
      if (!_found_) return;
      double factor = trade_vol_reader == NULL ? 1.0 : trade_vol_reader->getTradedVolRatioAtTime(base_msecs / 1000);
      pred_value_ = (nfac1_ * pred1_value_) + (nfac2_ * pred2_value_) + (nfac3_ * pred3_value_) +
                    (nfac4_ * pred4_value_) + (nfac5_ * pred5_value_);
      if (is_returns_based_) {
        pred_value_ = (pred_value_ / base_value_);
      }
      pred_value_ *= factor;
    }
  }
};
}
#endif
