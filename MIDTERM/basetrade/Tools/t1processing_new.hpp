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
#ifndef BASE_TOOLS_T1PROCESSINGNEW_HPP
#define BASE_TOOLS_T1PROCESSINGNEW_HPP

namespace HFSAT {
class T1Processing_new : public Processing {
 protected:
  std::ifstream input_data_file_pred_;

 public:
  T1Processing_new(const std::string& input_data_filename_, SimpleLineProcessor& _simple_line_processor_,
                   const bool _is_returns_based_, const int _msecs_to_predict_,
                   DailyTradeVolumeReader* _trade_vol_reader)
      : Processing(_simple_line_processor_, _is_returns_based_, _msecs_to_predict_, _trade_vol_reader) {
    input_data_file_pred_.open(input_data_filename_.c_str(), std::ifstream::in);
    ifstream_vec_.push_back(&input_data_file_pred_);
  }
  void Process(int _base_msecs_, int _base_events_, bool& were_files_reset_, int& _last_good_offset_, bool& _found_,
               double& pred_value_, double base_value_) {
    if (input_data_file_pred_.is_open()) {
      const int kDataFileLineBufferLen = 10240;

      char pred_line_buffer_[kDataFileLineBufferLen];
      bzero(pred_line_buffer_, kDataFileLineBufferLen);
      int base_msecs = _base_msecs_;

      // now find the first line at or after input_data_file_pred_ such that msecs_from_midnight >= (base_msecs +
      // msecs_to_predict_ )
      while (input_data_file_pred_.good()) {
        bzero(pred_line_buffer_, kDataFileLineBufferLen);
        input_data_file_pred_.getline(pred_line_buffer_, kDataFileLineBufferLen);
        PerishableStringTokenizer pred_st_(pred_line_buffer_, kDataFileLineBufferLen);
        const std::vector<const char*>& pred_tokens_ = pred_st_.GetTokens();
        if (pred_tokens_.size() < 3) continue;

        int pred_msecs = atoi(pred_tokens_[0]);
        if (pred_msecs > (base_msecs + msecs_to_predict_)) {
          _found_ = true;
          pred_value_ = atof(pred_tokens_[2]);
          register double value_to_model = (pred_value_ - base_value_);
          if (is_returns_based_) value_to_model /= base_value_;
          double factor = trade_vol_reader == NULL ? 1.0 : trade_vol_reader->getTradedVolRatioAtTime(base_msecs / 1000);
          pred_value_ = factor * value_to_model;
          break;
        }
      }
    }
  }
};
}
#endif
