/**
   \file IndicatorsCode/historical_stdev_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include <stdlib.h>
#include <sstream>

#include "dvccode/CDef/error_codes.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

#include "dvctrade/Indicators/historical_stdev_manager.hpp"

namespace HFSAT {

inline std::string ToString(int t_intdate_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << t_intdate_;
  return t_temp_oss_.str();
}

void HistoricalStdevManager::LoadStdevInfoFile() {
  std::string _stdev_info_filename_prefix_ = std::string(BASETRADEINFODIR) + "StdevInfo/stdev_info_";
  std::string _stdev_info_filename_ = _stdev_info_filename_prefix_ + "default.txt";

  std::map<std::string, unsigned int> date_first_seen_on_;

  int t_intdate_ = DateTime::GetCurrentIsoDateLocal();  // watch_.YYYYMMDD ( ) ;
  _stdev_info_filename_ = _stdev_info_filename_prefix_ + ToString(t_intdate_) + ".txt";
  for (auto i = 0u; i < 30; i++) {
    if (FileUtils::exists(_stdev_info_filename_)) {
      LoadFromFile(_stdev_info_filename_, date_first_seen_on_, t_intdate_);
    }

    t_intdate_ = DateTime::CalcPrevDay(t_intdate_);
    _stdev_info_filename_ = _stdev_info_filename_prefix_ + ToString(t_intdate_) + ".txt";
  }

  _stdev_info_filename_ = _stdev_info_filename_prefix_ + "default.txt";
  if (FileUtils::exists(_stdev_info_filename_)) {
    LoadFromFile(_stdev_info_filename_, date_first_seen_on_, 0);
  }
}

void HistoricalStdevManager::LoadFromFile(const std::string& _stdev_info_filename_,
                                          std::map<std::string, unsigned int>& date_first_seen_on_,
                                          const unsigned int t_intdate_) {
  std::ifstream stdev_info_file_;
  stdev_info_file_.open(_stdev_info_filename_.c_str(), std::ifstream::in);
  if (stdev_info_file_.is_open()) {
    const int kStdevDescriptionLen = 1024;
    char readline_buffer_[kStdevDescriptionLen];
    bzero(readline_buffer_, kStdevDescriptionLen);

    while (stdev_info_file_.good()) {
      bzero(readline_buffer_, kStdevDescriptionLen);
      stdev_info_file_.getline(readline_buffer_, kStdevDescriptionLen);
      PerishableStringTokenizer st_(readline_buffer_, kStdevDescriptionLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if ((tokens_.size() >= 2) && (tokens_[0][0] != '#')) {
        std::string _this_shortcode_ = tokens_[0];

        if ((date_first_seen_on_.find(_this_shortcode_) == date_first_seen_on_.end()) ||
            (date_first_seen_on_[_this_shortcode_] <= t_intdate_)) {
          shortcode_stdev_map_[_this_shortcode_] = atof(tokens_[1]);
          date_first_seen_on_[_this_shortcode_] = t_intdate_;
        }
      }
    }

    stdev_info_file_.close();
  }
}
}
