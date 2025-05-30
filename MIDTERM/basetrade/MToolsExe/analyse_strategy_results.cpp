/**
   \file MToolsExe/analyse_strategy_results.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
 */

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

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonDataStructures/map_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "basetrade/MToolsExe/result_line_set.hpp"
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/InitCommon/paramset.hpp"
#include "dvccode/CDef/email_utils.hpp"
#include "basetrade/SqlCpp/SqlCppUtils.hpp"
#include "basetrade/MToolsExe/read_resultlines_from_file.hpp"

using namespace HFSAT;

std::string g_sigv_string_;

void termination_handler(int signum) {
  if (signum == SIGSEGV) {  // On a segfault inform , so this is picked up and fixed.
    char hostname_[128];
    hostname_[127] = '\0';
    gethostname(hostname_, 127);

    std::string email_string_ = "analyse_strategy_results received sigsegv on " + std::string(hostname_) + "\n";
    email_string_ += g_sigv_string_ + "\n";
    std::string email_address_ = "";

    HFSAT::Email email_;
    email_.setSubject(email_string_);

    if (strncmp(getenv("USER"), "dvctrader", strlen("dvctrader")) ==
        0) {  // Not sure if others want to receive these emails.
      email_address_ = "nseall@tworoads.co.in";

      email_.addRecepient(email_address_);
      email_.addSender(email_address_);
      email_.content_stream << email_string_ << "<br/>";
      email_.sendMail();
      abort();
    }
  }
  exit(0);
}

int getParam(std::string strategy_filename_base_, std::string shortcode_, int tradingdate_, const char *what_);

bool SkipThisDate(int tradingdate_, std::vector<int> &skip_date_vec_) {
  for (auto i = 0u; i < skip_date_vec_.size(); ++i) {
    if (tradingdate_ == skip_date_vec_[i]) return true;
  }
  return false;
}

int main(int argc, char **argv) {
  // local variables
  signal(SIGSEGV, termination_handler);
  for (int i = 0; i < argc; i++) {
    g_sigv_string_ += std::string(argv[i]) + " ";
  }
  std::string shortcode_ = "";               ///< shortcode of the product whose results these are
  std::string strategy_list_filename_ = "";  ///< only used since here the results file may have info regarding many
  /// more files than are in the list strategy_list_filename_
  std::string globalresultsdbdir_ = "";  ///< expected to be /spare/local/global_results_database_dir
  int trading_start_yyyymmdd_ = 0;
  int trading_end_yyyymmdd_ = 0;
  std::string skip_dates_file_ = "";  // skip dates mentioned in this file
  std::vector<int> skip_date_vec_;

  std::set<std::string> strategy_file_base_included_;  ///< only used since here the results file may have info
  /// regarding many more files than are in the list
  /// strategy_list_filename_

  ResultLineSetVec result_set_vec_left_to_choose_;  ///< input .. and results spliced from this as they are chosen and
  /// put into chosen
  ResultLineSetVec result_set_vec_chosen_;  ///< chosen files

  std::vector<int> all_dates_;
  std::map<int, int> daily_strat_pool_median_;
  std::map<int, int> daily_strat_pool_average_;

  // command line processing
  if (argc < 10) {
    std::cerr << argv[0] << " shortcode strategy_list_filename_/dir_with_strat_files globalresultsdbdir "
                            "trading_start_yyyymmdd trading_end_yyyymmdd [skip_dates_file or INVALIDFILE] "
                            "no_of_strats_to_pick max_loss_per_uts global_max_loss_per_uts " << std::endl;
    exit(0);
  }
  shortcode_ = argv[1];
  strategy_list_filename_ = argv[2];
  globalresultsdbdir_ = argv[3];
  trading_start_yyyymmdd_ = atoi(argv[4]);
  trading_end_yyyymmdd_ = atoi(argv[5]);
  if (argc >= 7) {
    skip_dates_file_ = argv[6];
    std::ifstream infile_;
    const unsigned int kLineBufferLen = 1024;
    char readline_buffer_[kLineBufferLen];
    bzero(readline_buffer_, kLineBufferLen);
    if (skip_dates_file_.compare("INVALIDFILE")) {
      infile_.open(skip_dates_file_.c_str());
      while (infile_.good()) {
        bzero(readline_buffer_, kLineBufferLen);
        infile_.getline(readline_buffer_, kLineBufferLen);
        HFSAT::PerishableStringTokenizer st_(readline_buffer_, kLineBufferLen);
        const std::vector<const char *> &tokens_ = st_.GetTokens();

        if (tokens_.size() == 1) {  // expects the word to be the basename of a strategy file
          skip_date_vec_.push_back(
              atoi(tokens_[0]));  // mapped value does not matter ... as long as exists as a key ... fine
        }
      }
      if (infile_.is_open()) {
        infile_.close();
      }
    }
  }

  int strats_to_pick_ = -1;
  if (argc >= 8) {
    strats_to_pick_ = (atoi(argv[7]) > 0) ? atoi(argv[7]) : strats_to_pick_;
  }

  int max_loss_per_uts_ = -1;
  if (argc >= 9) {
    max_loss_per_uts_ = (atoi(argv[8]) > 0) ? atoi(argv[8]) : max_loss_per_uts_;
  }

  int global_max_loss_per_uts_ = -1;
  int global_max_loss_ = -1;
  if (argc >= 10) {
    global_max_loss_per_uts_ = (atoi(argv[9]) > 0) ? atoi(argv[9]) : global_max_loss_per_uts_;
  }

  if (globalresultsdbdir_.compare("DEF") == 0) {
    globalresultsdbdir_ = HFSAT::FileUtils::AppendHome("modelling/results");
  }
  // read strategy_list_filename_ and put entries in map
  {
    struct stat t_stat_;
    stat(strategy_list_filename_.c_str(), &t_stat_);
    /// handle case of file
    if (S_ISREG(t_stat_.st_mode)) {
      std::ifstream infile_;
      infile_.open(strategy_list_filename_.c_str());

      const unsigned int kLineBufferLen = 1024;
      char readline_buffer_[kLineBufferLen];
      bzero(readline_buffer_, kLineBufferLen);

      while (infile_.good()) {
        bzero(readline_buffer_, kLineBufferLen);
        infile_.getline(readline_buffer_, kLineBufferLen);
        HFSAT::PerishableStringTokenizer st_(readline_buffer_, kLineBufferLen);
        const std::vector<const char *> &tokens_ = st_.GetTokens();

        if (tokens_.size() >= 1) {  // expects the word to be the basename of a strategy file
          strategy_file_base_included_.insert(tokens_[0]);
        }
      }

      if (infile_.is_open()) {
        infile_.close();
      }
    }
    /// handle case of directory
    else if (S_ISDIR(t_stat_.st_mode)) {
      std::vector<std::string> strat_names;
      FileUtils::GetFileNames(strategy_list_filename_, strat_names);
      for (std::vector<std::string>::iterator siter = strat_names.begin(); siter != strat_names.end(); siter++) {
        strategy_file_base_included_.insert(*siter);
      }
    } else {
      fprintf(stderr, " invalid file path %s specified..\nExiting..\n", strategy_list_filename_.c_str());
      exit(0);
    }
  }

  if (trading_start_yyyymmdd_ < 20110101 || trading_start_yyyymmdd_ > 20201225) {
    std::cerr << "Tradingdate " << trading_start_yyyymmdd_ << " not in range\n";
    exit(0);
  }

  int tradingdate_ = trading_start_yyyymmdd_;
  std::vector<int> specific_date_vec_;
  // build date vec only if specific dates are not specified
  while (tradingdate_ >= 20100101 && tradingdate_ <= std::min(trading_end_yyyymmdd_, 20201225)) {
    if (VectorUtils::LinearSearchValue(skip_date_vec_, tradingdate_)) {
      tradingdate_ = DateTime::CalcNextWeekDay(tradingdate_);
      continue;
    }
    specific_date_vec_.push_back(tradingdate_);
    tradingdate_ = DateTime::CalcNextWeekDay(tradingdate_);
  }

  FileToResultLineSetMap file_to_res_map_;
  if (globalresultsdbdir_.compare("DB") == 0) {
    SqlCppUtils::GetResultLineSetVec(file_to_res_map_, shortcode_, specific_date_vec_, strategy_file_base_included_,
                                     max_loss_per_uts_);
  } else {
    HFSAT::ResultFileUtils::FetchResultsFromFile(file_to_res_map_, shortcode_, globalresultsdbdir_, specific_date_vec_,
                                                 strategy_file_base_included_);
  }

  // move data to vector .. now map not needed
  GetValueVecFromMap(file_to_res_map_, result_set_vec_left_to_choose_);

  unsigned int total_strats_ = result_set_vec_left_to_choose_.size();
  std::vector<int> sub_idxs_ = MathUtils::GetSubsets(total_strats_, strats_to_pick_);

  for (unsigned int gid = 0; gid < sub_idxs_.size();) {
    std::map<int, int> date_min_pnl_;
    std::map<int, int> date_pnl_;
    int sum_pnl_ = 0;
    int sum_prime_pnl_ = 0;
    int sum_min_pnl_ = 0;
    std::vector<int> min_pnl_vec_;

    for (int id = 0; id < strats_to_pick_; id++, gid++) {
      unsigned int i = sub_idxs_[gid] - 1;
      std::cout << i << " ";
      std::map<int, double> pnl_map;
      std::map<int, double> min_pnl_map;
      result_set_vec_left_to_choose_[i].GetPNLDateMap(pnl_map);
      result_set_vec_left_to_choose_[i].GetMinPNLDateMap(min_pnl_map);
      for (std::map<int, double>::iterator it = pnl_map.begin(); it != pnl_map.end(); ++it) {
        if (global_max_loss_ <= 0) {
          // initialzing here to avoid sigsegv
          global_max_loss_ = result_set_vec_left_to_choose_[0].result_line_vec_[0].unit_trade_size_ * strats_to_pick_ *
                             global_max_loss_per_uts_;
        }
        date_pnl_[it->first] += it->second;
        date_min_pnl_[it->first] += min_pnl_map[it->first];
      }
    }
    for (std::map<int, int>::iterator it = date_pnl_.begin(); it != date_pnl_.end(); it++) {
      if ((-1 * global_max_loss_) > date_min_pnl_[it->first]) {
        sum_prime_pnl_ += (-1 * global_max_loss_);
        sum_min_pnl_ += (-1 * global_max_loss_);
        min_pnl_vec_.push_back(-1 * global_max_loss_);
      } else {
        sum_prime_pnl_ += (it->second);
        sum_min_pnl_ += date_min_pnl_[it->first];
        min_pnl_vec_.push_back(date_min_pnl_[it->first]);
      }
      sum_pnl_ += (it->second);
      /*printf ( "%d %d %d \n",
                   it -> first,
                   it -> second,
                   date_min_pnl_ [ it -> first ] ) ; */
    }
    int fP_ = MathUtils::GetXPercentile<int>(min_pnl_vec_, 75.0);
    int nfP_ = MathUtils::GetXPercentile<int>(min_pnl_vec_, 80.0);

    if (date_pnl_.size() > 0) {
      int n = date_pnl_.size();
      printf("Diff: %d ", sum_pnl_ - sum_prime_pnl_);
      printf("N: %d PrimeAvg: %d ", n, sum_prime_pnl_ / n);
      printf("25Perc: %d 20Perc: %d \n", fP_, nfP_);
    }
  }
  return 0;
}

int getParam(std::string strategy_filename_base_, std::string shortcode_, int tradingdate_, const char *what_) {
  int ret_ = 0;
  std::string StratsDir = HFSAT::FileUtils::AppendHome("modelling/strats/" + shortcode_);
  DIR *strats_directory_stream_ = opendir(StratsDir.c_str());
  if (strats_directory_stream_ == NULL) {
    std::cout << "Error in opening strats directory " << StratsDir << std::endl;
    return ret_;
  }
  struct dirent *strats_directory_;
  while ((strats_directory_ = readdir(strats_directory_stream_)) != NULL) {
    if (strcmp(strats_directory_->d_name, ".") == 0 || strcmp(strats_directory_->d_name, "..") == 0) {
      continue;
    }
    std::string TimeperiodDir = StratsDir + "/" + strats_directory_->d_name;
    DIR *timeperiod_directory_stream_ = opendir(TimeperiodDir.c_str());
    if (timeperiod_directory_stream_ == NULL) {
      std::cout << "Error in opening directory " << TimeperiodDir << std::endl;
      return ret_;
    }
    struct dirent *timeperiod_directory_;
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
            const std::vector<const char *> &tokens_ = st_.GetTokens();
            if (tokens_.size() > 0) {
              HFSAT::ParamSet param = HFSAT::ParamSet(tokens_[4], tradingdate_, shortcode_);
              if (strcmp(what_, "MAXLOSS") == 0) {
                return param.max_loss_;
              } else if (strcmp(what_, "UTS") == 0) {
                return param.unit_trade_size_;
              }
            }
          }
        }
      }
    }
    // if(timeperiod_directory_ !=NULL)
    // break;
  }
  return ret_;
}
