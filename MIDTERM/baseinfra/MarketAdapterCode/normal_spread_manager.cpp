/**
   \file MarketAdapterCode/normal_spread_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#include <stdlib.h>
#include <string>
#include <sstream>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

#include "baseinfra/MarketAdapter/normal_spread_manager.hpp"

namespace HFSAT {

NormalSpreadManager* NormalSpreadManager::GetUniqueInstance(const int t_yyyymmdd_) {
  static std::map<int, NormalSpreadManager*> t_isodate_to_normal_spread_manager_map_;
  if (t_isodate_to_normal_spread_manager_map_.find(t_yyyymmdd_) == t_isodate_to_normal_spread_manager_map_.end()) {
    t_isodate_to_normal_spread_manager_map_[t_yyyymmdd_] = new NormalSpreadManager(t_yyyymmdd_);
    /// TODO ... figure out a good way to free this memory
  }

  return t_isodate_to_normal_spread_manager_map_[t_yyyymmdd_];
}

NormalSpreadManager::NormalSpreadManager(const int t_yyyymmdd_)
    : yyyymmdd_(t_yyyymmdd_), t_normal_spread_info_filename_("NULL"), shortcode_normal_spread_increment_map_() {
  int this_yyyymmdd_ = t_yyyymmdd_;
  for (auto i = 0u; i < 180;
       i++) {  // try 180 days back from this day to get the last time normal_spread_info was updated
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << BASETRADEINFODIR << "BidAskSpreadInfo/normal_spread_info_" << this_yyyymmdd_ << ".txt";
    t_normal_spread_info_filename_ = t_temp_oss_.str();
    if (FileUtils::exists(t_normal_spread_info_filename_)) {
      break;
    } else {
      this_yyyymmdd_ = DateTime::CalcPrevDay(this_yyyymmdd_);
    }
  }

  {
    if (!FileUtils::exists(t_normal_spread_info_filename_)) {
      std::ostringstream t_temp_oss_;
      t_temp_oss_ << BASETRADEINFODIR << "BidAskSpreadInfo/normal_spread_info_default.txt";
      t_normal_spread_info_filename_ = t_temp_oss_.str();
    }
  }

  LoadNormalSpreadInfoFile();
}

double NormalSpreadManager::GetNormalSpreadIncrements(const int t_yyyymmdd_, const std::string& t_shortcode_) {
  return (GetUniqueInstance(t_yyyymmdd_))->t_GetNormalSpreadIncrements(t_shortcode_);
}

double NormalSpreadManager::t_GetNormalSpreadIncrements(const std::string& t_shortcode_) {
  // if this string found then return the value from the map,
  // else return 1.0, but throw exception ... basically print error and exit for now

  if (shortcode_normal_spread_increment_map_.find(t_shortcode_) == shortcode_normal_spread_increment_map_.end()) {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << t_normal_spread_info_filename_ << " missing entry for " << t_shortcode_ << std::endl;
    // @ramkris Assume a default of 1.0 & continue rather than exit
    //	ExitVerbose ( kNormalSpreadManagerMissingInfoShortCode, t_temp_oss_.str().c_str() );
    return 1.0;
  }
  return shortcode_normal_spread_increment_map_[t_shortcode_];
}

/// @brief to load the normalspreadinfo from file
/// Reads a file of the form
/// shortcode_ normal_spread_  // since this is stored in values this file needs to be updated if the product settings
/// change on the exchange //
void NormalSpreadManager::LoadNormalSpreadInfoFile() {
  std::ifstream normal_spread_info_file_;
  normal_spread_info_file_.open(t_normal_spread_info_filename_.c_str(), std::ifstream::in);
  if (normal_spread_info_file_.is_open()) {
    const int kSpreadInfoLineLen = 1024;
    char readline_buffer_[kSpreadInfoLineLen];
    bzero(readline_buffer_, kSpreadInfoLineLen);

    while (normal_spread_info_file_.good()) {
      bzero(readline_buffer_, kSpreadInfoLineLen);
      normal_spread_info_file_.getline(readline_buffer_, kSpreadInfoLineLen);
      PerishableStringTokenizer st_(readline_buffer_, kSpreadInfoLineLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() >= 2) {
        std::string t_this_shortcode_ = tokens_[0];
        double t_this_normal_spread_increments_ = atof(tokens_[1]);

        shortcode_normal_spread_increment_map_[t_this_shortcode_] = t_this_normal_spread_increments_;
      }
    }
    normal_spread_info_file_.close();
  }
}
}
