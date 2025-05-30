// =====================================================================================
//
//       Filename:  read_resultlines_from_file.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  Tuesday 31 May 2016 03:30:06  IST
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

#ifndef BASE_READ_RESULTLINES_FROM_FILE_HPP
#define BASE_READ_RESULTLINES_FROM_FILE_HPP

#include <strings.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <map>
#include <algorithm>

#include "dvctrade/InitCommon/paramset.hpp"
#include "dvctrade/InitCommon/strategy_desc.hpp"
#include "basetrade/MToolsExe/result_line_set.hpp"

namespace HFSAT {

class ResultFileUtils {
 public:
  static void FetchResultsFromFile(FileToResultLineSetMap& _return_map_, const std::string& shortcode_,
                                   const std::string& globalresultsdbdir_, const std::vector<int>& _date_vec_,
                                   const std::set<std::string>& _stratbasename_set_ = std::set<std::string>(),
                                   int _maxloss_per_uts_ = 0, bool summarize_all_strats_ = false);

 private:
  static int getMaxLoss(const std::string& strategy_filename_base_, const std::string& shortcode_, int tradingdate_);
};

void ResultFileUtils::FetchResultsFromFile(FileToResultLineSetMap& _return_map_, const std::string& shortcode_,
                                           const std::string& globalresultsdbdir_, const std::vector<int>& _date_vec_,
                                           const std::set<std::string>& _stratbasename_set_, int _maxloss_per_uts_,
                                           bool summarize_all_strats_) {
  for (unsigned int d_idx_ = 0; d_idx_ < _date_vec_.size(); d_idx_++) {
    int tradingdate_ = _date_vec_[d_idx_];
    HFSAT::YYYYMMDDStr_t _yyyymmdd_str_ = HFSAT::DateTime::BreakDateYYYYMMDD(tradingdate_);

    std::ostringstream t_temp_oss_;
    t_temp_oss_ << globalresultsdbdir_ << "/" << shortcode_ << "/" << _yyyymmdd_str_.yyyy_str_ << "/"
                << _yyyymmdd_str_.mm_str_ << "/" << _yyyymmdd_str_.dd_str_ << "/results_database.txt";
	std::string curr_dir = globalresultsdbdir_ + "/" + shortcode_ + "/" + _yyyymmdd_str_.yyyy_str_ + "/" + _yyyymmdd_str_.mm_str_ + "/"+ _yyyymmdd_str_.dd_str_;
	std::string merge_files_cmd = "if [ -d " + curr_dir + " ] && [ `find " + curr_dir + "/results_database.txt.* -type f 2>/dev/null | wc -l` -gt 0 ]; then cat " + curr_dir + "/results_database.txt.* > " + curr_dir + "/results_database.txt ; fi";
	if ((system(merge_files_cmd.c_str())) != 0) {
		std::cout<<"Unable to merge files "<<(system(merge_files_cmd.c_str()))<<std::endl;
	}
    std::string this_resultfilename_(t_temp_oss_.str());

    std::ifstream infile_;
    infile_.open(this_resultfilename_.c_str());
    if (infile_.is_open()) {
      const unsigned int kLineBufferLen = 1024;
      char readline_buffer_[kLineBufferLen];
      bzero(readline_buffer_, kLineBufferLen);

      std::map<std::string, bool> is_strategy_filename_processed_;

      while (infile_.good()) {
        bzero(readline_buffer_, kLineBufferLen);
        infile_.getline(readline_buffer_, kLineBufferLen);
        HFSAT::PerishableStringTokenizer st_(readline_buffer_, kLineBufferLen);
        const std::vector<const char*>& tokens_ = st_.GetTokens();

        if (tokens_.size() >= 4) {  // filename date pnl volume
          // If PNL and Vol is 0, then skip this line
          if (atoi(tokens_[2]) == 0 && atoi(tokens_[3]) == 0) {
            continue;
          }

          std::string strategy_filename_base_ = tokens_[0];
          if ((_stratbasename_set_.find(strategy_filename_base_) != _stratbasename_set_.end() ||
               summarize_all_strats_) &&
              is_strategy_filename_processed_.find(strategy_filename_base_) ==
                  is_strategy_filename_processed_.end()  // to avoid including a strat more than once for a date in
                                                         // case file has duplicate results
              ) {                                        // found as a key ... hence this file was in the list specified
            is_strategy_filename_processed_[strategy_filename_base_] = true;
            ResultLine result_line_(tokens_);

            if (_return_map_.find(strategy_filename_base_) ==
                _return_map_.end()) {  // first time so add the strategy_filename_base_
              _return_map_[strategy_filename_base_].strategy_filename_base_ = strategy_filename_base_;
              _return_map_[strategy_filename_base_].result_line_vec_.clear();
            }

            // checking for max_loss of the strategy
            if (_maxloss_per_uts_ != 0) {
              int max_loss_ = _maxloss_per_uts_ > 0 ? _maxloss_per_uts_ * result_line_.unit_trade_size_
                                                    : getMaxLoss(strategy_filename_base_, shortcode_, tradingdate_);

              if ((-1) * max_loss_ > result_line_.min_pnl_) {
                result_line_.pnl_ = -1 * max_loss_;
                result_line_.min_pnl_ = -1 * max_loss_;
              }
            }
            if (SanityCheckResultLine(result_line_, shortcode_, result_line_.unit_trade_size_))
              _return_map_[strategy_filename_base_].result_line_vec_.push_back(result_line_);
          }
        }
      }
    }
    infile_.close();
  }
}

int ResultFileUtils::getMaxLoss(const std::string& strategy_filename_base_, const std::string& shortcode_,
                                int tradingdate_) {
  int max_loss_ = 10000;
  std::string StratsDir = HFSAT::FileUtils::AppendHome("modelling/strats/" + shortcode_);
  DIR* strats_directory_stream_ = opendir(StratsDir.c_str());
  if (strats_directory_stream_ == NULL) {
    std::cout << "Error in opening strats directory " << StratsDir << std::endl;
    return max_loss_;
  }
  struct dirent* strats_directory_;
  while ((strats_directory_ = readdir(strats_directory_stream_)) != NULL) {
    if (strcmp(strats_directory_->d_name, ".") == 0 || strcmp(strats_directory_->d_name, "..") == 0) {
      continue;
    }
    std::string TimeperiodDir = StratsDir + "/" + strats_directory_->d_name;
    DIR* timeperiod_directory_stream_ = opendir(TimeperiodDir.c_str());
    if (timeperiod_directory_stream_ == NULL) {
      std::cout << "Error in opening directory " << TimeperiodDir << std::endl;
      return max_loss_;
    }
    struct dirent* timeperiod_directory_;
    while ((timeperiod_directory_ = readdir(timeperiod_directory_stream_)) != NULL) {
      if (strcmp(timeperiod_directory_->d_name, strategy_filename_base_.c_str()) == 0) {
        std::string strat_file_name_ = TimeperiodDir + "/" + strategy_filename_base_;
        std::ifstream strat_file_;
        strat_file_.open(strat_file_name_.c_str(), std::ifstream::in);
        if (strat_file_.is_open()) {
          const int kStrategyFileLineBufferLen = 1024;
          char readline_buffer_[kStrategyFileLineBufferLen];
          bzero(readline_buffer_, kStrategyFileLineBufferLen);
          if (strat_file_.good()) {
            strat_file_.getline(readline_buffer_, kStrategyFileLineBufferLen);
            std::string this_strategy_full_line_(readline_buffer_);
            PerishableStringTokenizer st_(readline_buffer_, kStrategyFileLineBufferLen);
            const std::vector<const char*>& tokens_ = st_.GetTokens();
            if (tokens_.size() > 0) {
              HFSAT::ParamSet param = HFSAT::ParamSet(tokens_[4], tradingdate_, shortcode_);
              max_loss_ = param.max_loss_;
            }
          }
        }
        strat_file_.close();
        break;
      }
    }
    closedir(timeperiod_directory_stream_);
    // if(timeperiod_directory_ !=NULL)
    // break;
  }
  closedir(strats_directory_stream_);
  return max_loss_;
}
}

#endif  // BASE_READ_RESULTLINES_FROM_FILE_HPP
