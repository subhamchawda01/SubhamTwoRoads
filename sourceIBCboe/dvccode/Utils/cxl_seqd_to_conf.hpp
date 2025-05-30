/**
   file dvccode/Utils/cxl_seqd_to_conf.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551

*/

#ifndef BASE_UTILS_CXLSEQD_CONF_H_
#define BASE_UTILS_CXLSEQD_CONF_H_

#include <iostream>
#include <sstream>
#include <fstream>
#include <string.h>
#include <string>
#include <map>
#include <vector>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#define DATA_DIR "/NAS1/data/ORSData/CXLCONF/"
#define MAX_LINE_LENGTH 1024

namespace HFSAT {
namespace UTILS {

class CxlConfStats {
 private:
  std::map<int, ttime_t> saos_to_cxl_seqd_;
  std::map<int, ttime_t> saos_to_cxl_conf_;

  std::map<ttime_t, ttime_t> time_to_cxl_seqd_to_conf_time_;

 public:
  CxlConfStats(std::string shortcode_, int yyyymmdd_) {
    HFSAT::ExchangeSymbolManager::SetUniqueInstance(yyyymmdd_);
    const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);

    std::ostringstream t_temp_oss_;
    t_temp_oss_ << DATA_DIR << exchange_symbol_ << "_" << yyyymmdd_;

    std::string filename_ = t_temp_oss_.str();

    std::ifstream data_stream_;
    data_stream_.open(filename_.c_str());

    if (!data_stream_.is_open()) {
      std::cerr << " File : " << filename_ << " Doesn't Exists" << std::endl;
      return;
    }

    char line_buffer_[MAX_LINE_LENGTH];
    memset(line_buffer_, 0, MAX_LINE_LENGTH);

    while (data_stream_.good()) {
      memset(line_buffer_, 0, MAX_LINE_LENGTH);
      data_stream_.getline(line_buffer_, MAX_LINE_LENGTH);

      HFSAT::PerishableStringTokenizer st_(line_buffer_, MAX_LINE_LENGTH);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() == 3) {
        if (!strcmp(tokens_[0], "Send")) {
          int server_assigned_sequence_ = atoi(tokens_[2]);

          if (saos_to_cxl_seqd_.find(server_assigned_sequence_) == saos_to_cxl_seqd_.end()) {
            if (strlen(tokens_[1]) > 17 || strlen(tokens_[1]) < 14) continue;  // malformed line

            std::string timestamp_str_ = tokens_[1];

            if (timestamp_str_.find(".") == std::string::npos) continue;
            if (timestamp_str_.find(".") != 10) continue;

            HFSAT::ttime_t timestamp_(atoi(timestamp_str_.substr(0, timestamp_str_.find(".")).c_str()),
                                      atoi(timestamp_str_.substr(timestamp_str_.find(".") + 1).c_str()));

            saos_to_cxl_seqd_[server_assigned_sequence_] = timestamp_;
          }

        } else if (!strcmp(tokens_[0], "Conf")) {
          int server_assigned_sequence_ = atoi(tokens_[2]);

          if (saos_to_cxl_conf_.find(server_assigned_sequence_) == saos_to_cxl_conf_.end()) {
            if (strlen(tokens_[1]) > 17 || strlen(tokens_[1]) < 14) continue;  // malformed line

            std::string timestamp_str_ = tokens_[1];

            if (timestamp_str_.find(".") == std::string::npos) continue;
            if (timestamp_str_.find(".") != 10) continue;

            HFSAT::ttime_t timestamp_(atoi(timestamp_str_.substr(0, timestamp_str_.find(".")).c_str()),
                                      atoi(timestamp_str_.substr(timestamp_str_.find(".") + 1).c_str()));

            saos_to_cxl_conf_[server_assigned_sequence_] = timestamp_;
          }
        }
      }
    }

    data_stream_.close();

    std::map<int, ttime_t>::iterator saos_to_cxl_seqd_itr_;

    for (saos_to_cxl_seqd_itr_ = saos_to_cxl_seqd_.begin(); saos_to_cxl_seqd_itr_ != saos_to_cxl_seqd_.end();
         saos_to_cxl_seqd_itr_++) {
      if (saos_to_cxl_conf_.find(saos_to_cxl_seqd_itr_->first) != saos_to_cxl_conf_.end() &&
          ((saos_to_cxl_seqd_itr_->second) < (saos_to_cxl_conf_[saos_to_cxl_seqd_itr_->first]))) {
        // Need cxl_seqd_to_conf_time at cxl_seqd time.

        //-ve check
        if (saos_to_cxl_seqd_itr_->second < (saos_to_cxl_conf_[saos_to_cxl_seqd_itr_->first]))
          time_to_cxl_seqd_to_conf_time_[(saos_to_cxl_seqd_itr_->second)] =
              ((saos_to_cxl_conf_[saos_to_cxl_seqd_itr_->first]) - (saos_to_cxl_seqd_itr_->second));
      }
    }
  }

  CxlConfStats(std::string shortcode_, int yyyymmdd_, HFSAT::DebugLogger& dbglogger_) {
    HFSAT::ExchangeSymbolManager::SetUniqueInstance(yyyymmdd_);
    const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);

    std::ostringstream t_temp_oss_;
    t_temp_oss_ << DATA_DIR << exchange_symbol_ << "_" << yyyymmdd_;

    std::string filename_ = t_temp_oss_.str();

    std::ifstream data_stream_;
    data_stream_.open(filename_.c_str());

    if (!data_stream_.is_open()) {
      DBGLOG_CLASS_FUNC_LINE << " File : " << filename_ << " Doesn't Exists" << DBGLOG_ENDL_FLUSH;
      return;
    }

    char line_buffer_[MAX_LINE_LENGTH];
    memset(line_buffer_, 0, MAX_LINE_LENGTH);

    while (data_stream_.good()) {
      memset(line_buffer_, 0, MAX_LINE_LENGTH);
      data_stream_.getline(line_buffer_, MAX_LINE_LENGTH);

      HFSAT::PerishableStringTokenizer st_(line_buffer_, MAX_LINE_LENGTH);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() == 3) {
        if (!strcmp(tokens_[0], "Send")) {
          int server_assigned_sequence_ = atoi(tokens_[2]);

          if (saos_to_cxl_seqd_.find(server_assigned_sequence_) == saos_to_cxl_seqd_.end()) {
            if (strlen(tokens_[1]) > 17 || strlen(tokens_[1]) < 14) continue;  // malformed line

            std::string timestamp_str_ = tokens_[1];

            if (timestamp_str_.find(".") == std::string::npos) continue;
            if (timestamp_str_.find(".") != 10) continue;

            HFSAT::ttime_t timestamp_(atoi(timestamp_str_.substr(0, timestamp_str_.find(".")).c_str()),
                                      atoi(timestamp_str_.substr(timestamp_str_.find(".") + 1).c_str()));

            saos_to_cxl_seqd_[server_assigned_sequence_] = timestamp_;
          }

        } else if (!strcmp(tokens_[0], "Conf")) {
          int server_assigned_sequence_ = atoi(tokens_[2]);

          if (saos_to_cxl_conf_.find(server_assigned_sequence_) == saos_to_cxl_conf_.end()) {
            if (strlen(tokens_[1]) > 17 || strlen(tokens_[1]) < 14) continue;  // malformed line

            std::string timestamp_str_ = tokens_[1];

            if (timestamp_str_.find(".") == std::string::npos) continue;
            if (timestamp_str_.find(".") != 10) continue;

            HFSAT::ttime_t timestamp_(atoi(timestamp_str_.substr(0, timestamp_str_.find(".")).c_str()),
                                      atoi(timestamp_str_.substr(timestamp_str_.find(".") + 1).c_str()));

            saos_to_cxl_conf_[server_assigned_sequence_] = timestamp_;
          }
        }
      }
    }

    data_stream_.close();

    std::map<int, ttime_t>::iterator saos_to_cxl_seqd_itr_;

    for (saos_to_cxl_seqd_itr_ = saos_to_cxl_seqd_.begin(); saos_to_cxl_seqd_itr_ != saos_to_cxl_seqd_.end();
         saos_to_cxl_seqd_itr_++) {
      if (saos_to_cxl_conf_.find(saos_to_cxl_seqd_itr_->first) != saos_to_cxl_conf_.end() &&
          ((saos_to_cxl_seqd_itr_->second) < (saos_to_cxl_conf_[saos_to_cxl_seqd_itr_->first]))) {
        // Need cxl_seqd_to_conf_time at cxl_seqd time.

        //-ve check
        if (saos_to_cxl_seqd_itr_->second < (saos_to_cxl_conf_[saos_to_cxl_seqd_itr_->first]))
          time_to_cxl_seqd_to_conf_time_[(saos_to_cxl_seqd_itr_->second)] =
              ((saos_to_cxl_conf_[saos_to_cxl_seqd_itr_->first]) - (saos_to_cxl_seqd_itr_->second));
      }
    }
  }

  int getTotalCxlSeqd() { return saos_to_cxl_seqd_.size(); }

  int getTotalCxlConf() { return saos_to_cxl_conf_.size(); }

  std::map<ttime_t, ttime_t>& getCxlConfMap() { return time_to_cxl_seqd_to_conf_time_; }
};
}
}

#endif
