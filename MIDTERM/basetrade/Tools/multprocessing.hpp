/**
   \file Tools/t1processing.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_TOOLS_MULTPROCESSING_HPP
#define BASE_TOOLS_MULTPROCESSING_HPP

#include "basetrade/Tools/timed_data_to_reg_data_common.hpp"
#include "basetrade/Tools/t1processing_new.hpp"
#include "basetrade/Tools/t5processing_new.hpp"
#include "basetrade/Tools/t3processing_new.hpp"
#include "basetrade/Tools/e1processing_new.hpp"
#include "basetrade/Tools/e3processing_new.hpp"
#include "basetrade/Tools/e5processing_new.hpp"
//#include "basetrade/Tools/processing.hpp"

namespace HFSAT {
bool checkifOpenList(std::vector<std::ifstream *> &stream_vec_) {
  bool retval = true;
  for (auto i = 0u; i < stream_vec_.size(); i++) {
    retval = retval && stream_vec_[i]->is_open();
  }
  return retval;
}

bool checkifGoodList(std::vector<std::ifstream *> &stream_vec_) {
  bool retval = true;
  for (auto i = 0u; i < stream_vec_.size(); i++) {
    retval = retval && stream_vec_[i]->good();
  }
  return retval;
}

void MultProcessing(const std::string &input_data_filename_, SimpleLineProcessor &simple_line_processor_,
                    const bool is_returns_based_, std::vector<int> msecs_to_predict_,
                    std::vector<NormalizingAlgo> normalizing_algo_, DailyTradeVolumeReader *trade_vol_reader) {
  std::ifstream input_data_file_base_;
  input_data_file_base_.open(input_data_filename_.c_str(), std::ifstream::in);

  std::vector<std::ifstream *> ifstream_vec_;
  std::vector<Processing *> processings_;
  ifstream_vec_.push_back(&input_data_file_base_);
  for (auto i = 0u; i < msecs_to_predict_.size(); i++) {
    if (normalizing_algo_[i] == na_t1) {
      Processing *t_processing_ = new T1Processing_new(input_data_filename_, simple_line_processor_, is_returns_based_,
                                                       msecs_to_predict_[i], trade_vol_reader);
      processings_.push_back(t_processing_);
    }
    if (normalizing_algo_[i] == na_t3) {
      Processing *t_processing_ = new T3Processing_new(input_data_filename_, simple_line_processor_, is_returns_based_,
                                                       msecs_to_predict_[i], trade_vol_reader);
      processings_.push_back(t_processing_);
    }
    if (normalizing_algo_[i] == na_t5) {
      Processing *t_processing_ = new T5Processing_new(input_data_filename_, simple_line_processor_, is_returns_based_,
                                                       msecs_to_predict_[i], trade_vol_reader);
      processings_.push_back(t_processing_);
    }
    if (normalizing_algo_[i] == na_e1) {
      Processing *t_processing_ = new E1Processing_new(input_data_filename_, simple_line_processor_, is_returns_based_,
                                                       msecs_to_predict_[i], trade_vol_reader);
      processings_.push_back(t_processing_);
    }
    if (normalizing_algo_[i] == na_e3) {
      Processing *t_processing_ = new E3Processing_new(input_data_filename_, simple_line_processor_, is_returns_based_,
                                                       msecs_to_predict_[i], trade_vol_reader);
      processings_.push_back(t_processing_);
    }
    if (normalizing_algo_[i] == na_e5) {
      Processing *t_processing_ = new E5Processing_new(input_data_filename_, simple_line_processor_, is_returns_based_,
                                                       msecs_to_predict_[i], trade_vol_reader);
      processings_.push_back(t_processing_);
    }
  }

  int last_good_offset_ = 0;

  if (input_data_file_base_.is_open()) {
    const int kDataFileLineBufferLen = 10240;

    char base_line_buffer_[kDataFileLineBufferLen];
    bzero(base_line_buffer_, kDataFileLineBufferLen);

    while (input_data_file_base_.good()) {
      bzero(base_line_buffer_, kDataFileLineBufferLen);
      last_good_offset_ = std::max(last_good_offset_, (int)input_data_file_base_.tellg());
      input_data_file_base_.getline(base_line_buffer_, kDataFileLineBufferLen);
      PerishableStringTokenizer base_st_(base_line_buffer_, kDataFileLineBufferLen);
      const std::vector<const char *> &base_tokens_ = base_st_.GetTokens();
      if (base_tokens_.size() < 4) continue;

      int base_msecs = atoi(base_tokens_[0]);
      int base_events = atoi(base_tokens_[1]);
      bool found = false;
      std::vector<double> pred_value_(msecs_to_predict_.size(), 0);
      double base_value_ = atof(base_tokens_[3]);

      bool were_files_reset_ = false;

      // now find the first line at or after input_data_file_pred_ such that msecs_from_midnight >= (base_msecs +
      // msecs_to_predict_ )
      for (auto i = 0u; i < msecs_to_predict_.size(); i++) {
        if (normalizing_algo_[i] == na_t3)
          ((T3Processing_new *)processings_[i])
              ->Process(base_msecs, base_events, were_files_reset_, last_good_offset_, found, pred_value_[i],
                        base_value_);
        if (normalizing_algo_[i] == na_t1)
          ((T1Processing_new *)processings_[i])
              ->Process(base_msecs, base_events, were_files_reset_, last_good_offset_, found, pred_value_[i],
                        base_value_);
        if (normalizing_algo_[i] == na_t5)
          ((T5Processing_new *)processings_[i])
              ->Process(base_msecs, base_events, were_files_reset_, last_good_offset_, found, pred_value_[i],
                        base_value_);
        if (normalizing_algo_[i] == na_e3)
          ((E3Processing_new *)processings_[i])
              ->Process(base_msecs, base_events, were_files_reset_, last_good_offset_, found, pred_value_[i],
                        base_value_);
        if (normalizing_algo_[i] == na_e1)
          ((E1Processing_new *)processings_[i])
              ->Process(base_msecs, base_events, were_files_reset_, last_good_offset_, found, pred_value_[i],
                        base_value_);
        if (normalizing_algo_[i] == na_e5)
          ((E5Processing_new *)processings_[i])
              ->Process(base_msecs, base_events, were_files_reset_, last_good_offset_, found, pred_value_[i],
                        base_value_);
      }
      if (!found) break;
      double factor = trade_vol_reader == NULL ? 1.0 : trade_vol_reader->getTradedVolRatioAtTime(base_msecs / 1000);
      for (auto i = 0u; i < pred_value_.size(); i++) simple_line_processor_.AddWord(pred_value_[i]);
      for (unsigned int i = 4u; i < base_tokens_.size(); i++)
        simple_line_processor_.AddWord(factor * atof(base_tokens_[i]));
      simple_line_processor_.FinishLine();
    }

    simple_line_processor_.Close();
  }

  if (input_data_file_base_.is_open()) {
    input_data_file_base_.close();
  }
  for (auto i = 0u; i < processings_.size(); i++) processings_[i]->CloseFiles();
}
}
#endif
