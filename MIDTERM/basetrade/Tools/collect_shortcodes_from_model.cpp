// =====================================================================================
//
//       Filename:  collect_shortcodes_from_model.cpp
//
//    Description:  prints the list of shortcodes that are needed in the model or ilist
//
//        Version:  1.0
//        Created:  Thursday 31 March 2016 11:52:39  IST
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#include <iostream>
#include <sstream>
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvctrade/ModelMath/model_creator.hpp"
#include "dvctrade/Indicators/pca_weights_manager.hpp"
#include "dvctrade/Indicators/indicator_list.hpp"
#include "dvccode/CDef/error_codes.hpp"
#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CDef/file_utils.hpp"

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "EXEC MODELFILE/INDICATOR/INDICATORLISTFILE DATE(YYYYMMDD) "
                 "[0:model,1:indicator,2:indicators_list_file(file with just the list of indicators)]" << std::endl;
    exit(0);
  }

  std::string mfile_ = std::string(argv[1]);
  int yyyymmdd = atoi(argv[2]);
  int opt_ = 0;

  if (argc > 3) {
    opt_ = atoi(argv[3]);
  }

  HFSAT::DebugLogger dbglogger_(1024000, 1);
  HFSAT::Watch watch_(dbglogger_, yyyymmdd);

  std::vector<std::string> source_shortcode_vec_;
  std::vector<std::string> ors_needed_by_indicators_vec_;

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(yyyymmdd);
  HFSAT::PcaWeightsManager::SetUniqueInstance(yyyymmdd);

  const unsigned int kModelLineBufferLen = 1024;

  if (opt_ == 0) {
    std::string dep_shortcode_ = HFSAT::ModelCreator::CollectShortCodes(
        dbglogger_, watch_, mfile_, source_shortcode_vec_, ors_needed_by_indicators_vec_, true);

    for (auto& t_shc_ : ors_needed_by_indicators_vec_) {
      HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, t_shc_);
    }
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, dep_shortcode_);

    for (auto& t_shc_ : source_shortcode_vec_) {
      std::cout << t_shc_ << " ";
    }
    std::cout << "\n";
  } else if (opt_ == 1) {
    HFSAT::SetIndicatorListMap();
    HFSAT::PerishableStringTokenizer st_(argv[1], kModelLineBufferLen);
    std::vector<const char*> tokens_ = st_.GetTokens();
    if (strcmp(tokens_[0], "INDICATOR") != 0) {
      tokens_.insert(tokens_.begin(), "1.0");
      tokens_.insert(tokens_.begin(), "INDICATOR");
    }
    if (tokens_.size() > 2) {
      (HFSAT::CollectShortCodeFunc(tokens_[2]))(source_shortcode_vec_, ors_needed_by_indicators_vec_, tokens_);
    }

    for (auto& t_shc_ : ors_needed_by_indicators_vec_) {
      HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, t_shc_);
    }

    for (auto& t_shc_ : source_shortcode_vec_) {
      std::cout << t_shc_ << " ";
    }
    std::cout << "\n";
  } else if (opt_ == 2) {
    std::ifstream model_infile_;
    model_infile_.open(mfile_.c_str(), std::ifstream::in);
    if (!model_infile_.is_open()) {
      HFSAT::ExitVerbose(HFSAT::kModelCreationCouldNotOpenModelFile);
    } else {
      HFSAT::SetIndicatorListMap();

      const unsigned int kModelLineBufferLen = 1024;
      char readline_buffer_[kModelLineBufferLen];
      bzero(readline_buffer_, kModelLineBufferLen);

      int indc_idx_ = 0;
      while (model_infile_.good()) {
        bzero(readline_buffer_, kModelLineBufferLen);
        model_infile_.getline(readline_buffer_, kModelLineBufferLen);
        indc_idx_++;

        if (model_infile_.gcount() > 0) {
          HFSAT::PerishableStringTokenizer st_(readline_buffer_, kModelLineBufferLen);
          std::vector<const char*> tokens_ = st_.GetTokens();
          if (strcmp(tokens_[0], "INDICATOR") != 0) {
            tokens_.insert(tokens_.begin(), "1.0");
            tokens_.insert(tokens_.begin(), "INDICATOR");
          }
          if (tokens_.size() > 2) {
            source_shortcode_vec_.clear();
            ors_needed_by_indicators_vec_.clear();
            (HFSAT::CollectShortCodeFunc(tokens_[2]))(source_shortcode_vec_, ors_needed_by_indicators_vec_, tokens_);
          }

          for (auto& t_shc_ : ors_needed_by_indicators_vec_) {
            HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, t_shc_);
          }

          for (auto& t_shc_ : source_shortcode_vec_) {
            std::cout << t_shc_ << " ";
          }
        }
        std::cout << "\n";
      }
      model_infile_.close();
    }
  }
}
