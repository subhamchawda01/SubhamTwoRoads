/**
   \file MToolsExe/summarize_strategy_results.cpp

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
#include <boost/algorithm/string.hpp>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonDataStructures/vector_utils_weighted.hpp"
#include "dvccode/CommonDataStructures/map_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/email_utils.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"

#include "dvctrade/InitCommon/paramset.hpp"
#include "dvctrade/InitCommon/strategy_desc.hpp"

#include "basetrade/MToolsExe/result_line_set.hpp"
#include "basetrade/MToolsExe/read_resultlines_from_file.hpp"

#define INVALID_MAX_LOSS 9999999

using namespace HFSAT;
std::string g_sigv_string_;
void termination_handler(int signum) {
  if (signum == SIGSEGV) {  // On a segfault inform , so this is picked up and fixed.
    char hostname_[128];
    hostname_[127] = '\0';
    gethostname(hostname_, 127);

    std::string email_string_ = "summarize_strategy_results received sigsegv on " + std::string(hostname_) + "\n";
    email_string_ += g_sigv_string_ + "\n";
    std::string email_address_ = "";

    HFSAT::Email email_;
    email_.setSubject(email_string_);

    if (!strncmp(getenv("USER"), "dvctrader", strlen("dvctrader"))) {
      email_address_ = "nseall@tworoads.co.in";
    } else if (!strncmp(getenv("USER"), "ankit", strlen("ankit"))) {
      email_address_ = "ankit@circulumvite.com";
    } else if (!strncmp(getenv("USER"), "rkumar", strlen("rkumar"))) {
      email_address_ = "rakesh.kumar@tworoads.co.in";
    } else if (!strncmp(getenv("USER"), "ravi", strlen("ravi"))) {
      email_address_ = "ravi.parikh@tworoads.co.in";
    } else if (!strncmp(getenv("USER"), "kputta", strlen("kputta"))) {
      email_address_ = "kp@circulumvite.com";
    } else if (!strncmp(getenv("USER"), "anshul", strlen("anshul"))) {
      email_address_ = "anshul@tworoads.co.in";
    } else if (!strncmp(getenv("USER"), "abhijit", strlen("abhijit"))) {
      email_address_ = "abhijit.sharang@tworoads.co.in";
    } else if (!strncmp(getenv("USER"), "diwakar", strlen("diwakar"))) {
      email_address_ = "diwakar@circulumvite.com";
    } else {  // Not sure if others want to receive these emails.
      email_address_ = "nseall@tworoads.co.in";
    }
    email_.addRecepient(email_address_);
    email_.addSender(email_address_);
    email_.content_stream << email_string_ << "<br/>";
    email_.sendMail();
    abort();
  }
  exit(0);
}

/// This script is meant to be called by run_simulations.pl
/// After having generated data for all the trading days on all the files
/// At the end it calls this exec with
/// the shortcode,
/// the list of strategy files,
/// the starting trading date
/// the ending trading date
///
/// On the tradingdate yyyymmdd it finds the results in
/// /spare/local/global_results_database_dir/shortcode/yyyy/mm/dd/results_database.txt

bool SkipThisDate(int tradingdate_, std::vector<int> &skip_date_vec_) {
  for (auto i = 0u; i < skip_date_vec_.size(); ++i) {
    if (tradingdate_ == skip_date_vec_[i]) return true;
  }
  return false;
}

bool IncludeThisDate(int tradingdate_, std::vector<int> &specific_date_vec_) {
  // std::cerr << "IncludeThisDate " << tradingdate_ << std::endl;

  if (specific_date_vec_.size() < 1) {
    return true;
  }

  for (auto i = 0u; i < specific_date_vec_.size(); ++i) {
    if (tradingdate_ == specific_date_vec_[i]) {
      return true;
    }
  }

  return false;
}

int main(int argc, char **argv) {
  // local variables
  signal(SIGINT, termination_handler);
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
  std::string specific_dates_file_ = "";  // specific dates mentioned in this file
  std::vector<int> specific_date_vec_;
  std::string pool_or_staged_ = "N";
  int address_global_train_days = 0;
  std::vector<int> global_train_date_vec_;

  bool shorter_output_ = true;

  std::set<std::string> strategy_file_base_included_;  ///< only used since here the results file may have info
  /// regarding many more files than are in the list
  /// strategy_list_filename_

  ResultLineSetVec result_set_vec_left_to_choose_;  ///< input .. and results spliced from this as they are chosen and
  /// put into chosen
  ResultLineSetVec result_set_vec_chosen_;  ///< chosen files

  // command line processing
  if (argc < 6) {
    std::cerr << argv[0]
              << " shortcode strategy_list_filename_/dir_with_strat_files/pool_timings globalresultsdbdir "
                 "trading_start_yyyymmdd "
                 "trading_end_yyyymmdd [skip_dates_file or INVALIDFILE] [sortalgo=kCNAPnlAdjAverage] [MaxLossPerUts(0 "
                 "no_maxloss|-ve param.maxloss)=0] [specific_dates_file or specific_dates_with_weights_file_ or "
                 "INVALIDFILE] [shorter_output=1] [pool_or_staged=N] [address_global_train_days=0 (0: Don't use global "
                 "training dates(gtd) 1: exclude gtd 2: take intersection with gtd)]" << std::endl;
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

  ChooseNextAlgo_t cna_ = kCNAPnlAdjAverage;
  if (argc >= 8) {
    if (atoi(argv[7]) == 0)
      cna_ = GetChooseNextAlgoFromString(argv[7]);
    else
      cna_ = ChooseNextAlgo_t(std::max(0, std::min(atoi(argv[7]), ((int)kCNAMAX) - 1)));
  }

  int given_max_loss_per_uts_ = 0;  // 0 for not using max_loss, INVALID_MAX_LOSS for using max_loss in param, and any
                                    // other positive value to use that as per_uts_max_loss
  if (argc >= 9) {
    given_max_loss_per_uts_ = atoi(argv[8]);
  }

  std::map<int, double> date_to_weight_map_;
  if (argc >= 10) {
    specific_dates_file_ = argv[9];
    std::ifstream infile_;
    const unsigned int kLineBufferLen = 1024;
    char readline_buffer_[kLineBufferLen];
    bzero(readline_buffer_, kLineBufferLen);
    if (specific_dates_file_.compare("INVALIDFILE")) {
      infile_.open(specific_dates_file_.c_str());
      while (infile_.good()) {
        bzero(readline_buffer_, kLineBufferLen);
        infile_.getline(readline_buffer_, kLineBufferLen);
        HFSAT::PerishableStringTokenizer st_(readline_buffer_, kLineBufferLen);
        const std::vector<const char *> &tokens_ = st_.GetTokens();

        if (tokens_.size() >= 1) {  // expects the word to be the dates
          specific_date_vec_.push_back(
              atoi(tokens_[0]));  // mapped value does not matter ... as long as exists as a key ... fine
        }
        if (tokens_.size() == 2) {  // <date> <weight>
          date_to_weight_map_[atoi(tokens_[0])] = atof(tokens_[1]);
        }
      }
      if (infile_.is_open()) {
        infile_.close();
      }
    }
  }

  if (argc >= 11) {
    shorter_output_ = (atoi(argv[10]) > 0) ? true : false;
  }

  if (argc >= 12) {
    pool_or_staged_ = argv[11];
  }

  // 1 for removing training days. 20 days from the date of running (today) are not removed. Assumed no strategy is
  // learned on recent 20 days
  // 2 for using dates which are in intersection of specific dates and global training dates.
  if (argc >= 13) {
    address_global_train_days = atoi(argv[12]);
  }

  if (address_global_train_days != 0) {
    // int global_max_date_ = DateTime::GetIsoDateFromString("TODAY-20");
    // int global_min_date_ = trading_start_yyyymmdd_;
    // if (not specific_date_vec_.empty()) {
    //  global_min_date_ = *std::min_element(specific_date_vec_.begin(), specific_date_vec_.end());
    // }
    // ResultsAccessManager &ram_ = ResultsAccessManager::GetUniqueInstance();
    // fetch global training dates
    // ram_.FetchDates(1, global_min_date_, global_max_date_, global_train_date_vec_);
  }

  if (globalresultsdbdir_.compare("DEF") == 0) {
    globalresultsdbdir_ = HFSAT::FileUtils::AppendHome("modelling/results");
  }

  bool summarize_all_strats_ = (strategy_list_filename_.compare("ALL") == 0);

  if (!summarize_all_strats_) {
    // read strategy_list_filename_ and put entries in map
    struct stat t_stat_;
    int retval = stat(strategy_list_filename_.c_str(), &t_stat_);
    if (retval == 0) {
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
      }
    } else {
      // handle for pool timings
      if (strategy_list_filename_.find("-")) {
        std::vector<std::string> strat_names;

        // get the start_end time by splitting on '-'
        std::string delimiter = "-";

        // Initialize object for accessing strats from DB
        // ResultsAccessManager &ram_ = ResultsAccessManager::GetUniqueInstance();

        std::vector<std::string> tokens_;
        boost::split(tokens_, strategy_list_filename_, boost::is_any_of(delimiter));

        std::string start_time_ = tokens_[0];
        std::string end_time_ = tokens_[1];

        if (tokens_.size() > 2) {
          std::string pool_tag_ = tokens_[2];
          // fetch strategies with pool timings and pool tag
          // ram_.FetchStratsWithPool(shortcode_, start_time_, end_time_, pool_or_staged_, pool_tag_, strat_names);
        } else {
          // fetch the strategies for pool timings
          // ram_.FetchStrats(shortcode_, start_time_, end_time_, pool_or_staged_, strat_names);
        }

        // insert the strategies to the set strategy_file_base_included_
        for (std::vector<std::string>::iterator siter = strat_names.begin(); siter != strat_names.end(); siter++) {
          strategy_file_base_included_.insert(*siter);
        }
      } else {
        fprintf(stderr, " invalid file path/pool %s specified..\nExiting..\n", strategy_list_filename_.c_str());
        exit(0);
      }
    }
    if (strategy_file_base_included_.empty()) {
      fprintf(stderr, " no strats found in %s file/dir/pool..\nExiting..\n", strategy_list_filename_.c_str());
      exit(0);
    }
  }

  if (trading_start_yyyymmdd_ < 20110101 || trading_start_yyyymmdd_ > 22201225) {
    std::cerr << "Tradingdate " << trading_start_yyyymmdd_ << " not in range\n";
    exit(0);
  }

  int tradingdate_ = trading_start_yyyymmdd_;
  if (specific_date_vec_.empty()) {
    // build date vec only if specific dates are not specified
    while (tradingdate_ >= 20100101 && tradingdate_ <= std::min(trading_end_yyyymmdd_, 22201225)) {
      if (VectorUtils::LinearSearchValue(skip_date_vec_, tradingdate_)) {
        tradingdate_ = DateTime::CalcNextWeekDay(tradingdate_);
        continue;
      }
      specific_date_vec_.push_back(tradingdate_);
      tradingdate_ = DateTime::CalcNextWeekDay(tradingdate_);
    }
  }

  if (not global_train_date_vec_.empty()) {
    std::vector<int> specific_date_vec_set_operated_(specific_date_vec_.size());
    std::sort(specific_date_vec_.begin(), specific_date_vec_.end());
    std::vector<int>::iterator it;
    // remove training days from specific dates vector
    if (address_global_train_days == 1) {
      it = std::set_difference(specific_date_vec_.begin(), specific_date_vec_.end(), global_train_date_vec_.begin(),
                               global_train_date_vec_.end(), specific_date_vec_set_operated_.begin());
    }
    // keep only training days from specific dates vector
    else {
      it = std::set_intersection(specific_date_vec_.begin(), specific_date_vec_.end(), global_train_date_vec_.begin(),
                                 global_train_date_vec_.end(), specific_date_vec_set_operated_.begin());
    }
    specific_date_vec_set_operated_.resize(it - specific_date_vec_set_operated_.begin());
    specific_date_vec_ = specific_date_vec_set_operated_;
  }

  FileToResultLineSetMap file_to_res_map_;
//  if (globalresultsdbdir_.compare("DB") == 0) {
//    SqlCppUtils::GetResultLineSetVec(file_to_res_map_, shortcode_, specific_date_vec_, strategy_file_base_included_,
 //                                    given_max_loss_per_uts_);
//  } else if (getenv("USE_BACKTEST") != nullptr || globalresultsdbdir_.compare("BACKTEST_DB") == 0) {
//    setenv("USE_BACKTEST", "1", 1);
//    SqlCppUtils::GetResultLineSetVec(file_to_res_map_, shortcode_, specific_date_vec_, strategy_file_base_included_,
//                                     given_max_loss_per_uts_);
//  } else {
    ResultFileUtils::FetchResultsFromFile(file_to_res_map_, shortcode_, globalresultsdbdir_, specific_date_vec_,
                                          strategy_file_base_included_, given_max_loss_per_uts_, false);
//  }

  // move data to vector .. now map not needed
  GetValueVecFromMap(file_to_res_map_, result_set_vec_left_to_choose_);

  // sort all results by date
  // just for printing purposes
  for (auto i = 0u; i < result_set_vec_left_to_choose_.size(); i++) {
    ResultLineSet &this_result_line_set_ = result_set_vec_left_to_choose_[i];
    ResultLineVec &this_result_line_vec_ = this_result_line_set_.result_line_vec_;
    // Sort the vector using predicate and std::sort
    std::sort(this_result_line_vec_.begin(), this_result_line_vec_.end(), SortResultLineDate);
  }

  // compute statistics :
  for (auto i = 0u; i < result_set_vec_left_to_choose_.size(); i++) {
    ResultLineSet &this_result_line_set_ = result_set_vec_left_to_choose_[i];
    if (!date_to_weight_map_.empty()) {
      this_result_line_set_.addWeightsfromMap(date_to_weight_map_);
    }
    this_result_line_set_.ComputeStatistics();
  }

  // choose files
  int num_files_left_to_choose_ = result_set_vec_left_to_choose_.size();
  while ((num_files_left_to_choose_ > 0) && (result_set_vec_left_to_choose_.size() > 0)) {
    ChooseNext(result_set_vec_left_to_choose_, num_files_left_to_choose_, cna_, result_set_vec_chosen_);
  }

  // print results and statistics of the chosen files
  for (auto i = 0u; i < result_set_vec_chosen_.size(); i++) {
    const ResultLineSet &this_result_line_set_ = result_set_vec_chosen_[i];
    if (shorter_output_) {
      printf(
          "STRATEGYFILEBASE %s %d %d %d %.2f %d %d %d %d %d %.2f %d %d %d %d %d %.2f %.2f %d %.2f %.2f %d %.2f %.2f "
          "%.2f %.2f %d %d %.2f %.2f\n",
          this_result_line_set_.strategy_filename_base_.c_str(), (int)this_result_line_set_.pnl_average_,
          (int)this_result_line_set_.pnl_stdev_, (int)this_result_line_set_.volume_average_,
          this_result_line_set_.pnl_sharpe_, (int)this_result_line_set_.pnl_conservative_average_,
          (int)this_result_line_set_.pnl_median_average_,
          (int)this_result_line_set_.median_average_time_to_close_trades_,
          (int)this_result_line_set_.median_volume_normalized_average_time_to_close_trades_,
          (int)this_result_line_set_.average_min_adjusted_pnl_, (double)this_result_line_set_.pnl_per_contract_,
          (int)this_result_line_set_.supporting_order_filled_percent_,
          (int)this_result_line_set_.best_level_order_filled_percent_,
          (int)this_result_line_set_.aggressive_order_filled_percent_,
          (int)this_result_line_set_.improve_order_filled_percent_, (int)this_result_line_set_.average_max_drawdown_,
          this_result_line_set_.dd_adj_pnl_average_, this_result_line_set_.average_abs_position_,
          (int)this_result_line_set_.average_msg_count_, this_result_line_set_.gain_to_pain_ratio_,
          this_result_line_set_.pnl_by_maxloss_, (int)this_result_line_set_.ninety_five_percentile_,
          this_result_line_set_.average_num_opentrade_hits_, this_result_line_set_.average_abs_closing_position_,
          this_result_line_set_.average_unit_trade_size_, this_result_line_set_.percent_positive_trades_,
          (int)this_result_line_set_.max_cumulative_dd_, (int)this_result_line_set_.longest_negative_stretch_,
          this_result_line_set_.cumulative_pnl_by_max_dd_,
          GetScoreFromSortAlgo(this_result_line_set_,
                               cna_)  // please always keep score in end, might break other scripts
          );
    } else {
      std::cout << "STRATEGYFILEBASE " << this_result_line_set_.strategy_filename_base_ << "\n";

      const ResultLineVec &this_result_line_vec_ = this_result_line_set_.result_line_vec_;
      double total_pnl_ = 0;
      double count_ = 0;
      double max_pnl_ = 0;

      for (auto i = 0u; i < this_result_line_vec_.size(); i++) {
	double tratio = (this_result_line_vec_[i].total_trades == 0) ? 0 : 
		        this_result_line_vec_[i].msg_count_ /(double)this_result_line_vec_[i].total_trades;
        printf(
            "%d %d %d Turn: %d, %d norm-ttc %d min: %d max: %d draw: %d zs: %.2f S: %d B: %d A: %d I: %d apos: %.2f "
            "msgs: %d otl_hit: %d abs_op_pos: %.2f uts: %d ptrds: %d ttrds: %d TotalTrades:%d tratio: %.3f\n",
            this_result_line_vec_[i].yyyymmdd_, (int)this_result_line_vec_[i].pnl_, this_result_line_vec_[i].volume_,
            this_result_line_vec_[i].median_time_to_close_trades_,
            this_result_line_vec_[i].average_time_to_close_trades_,
            this_result_line_vec_[i].volume_normalized_average_time_to_close_trades_,
            (int)this_result_line_vec_[i].min_pnl_, (int)this_result_line_vec_[i].max_pnl_,
            (int)this_result_line_vec_[i].max_drawdown_, this_result_line_vec_[i].pnl_zscore_,
            this_result_line_vec_[i].supporting_order_filled_percent_,
            this_result_line_vec_[i].best_level_order_filled_percent_,
            this_result_line_vec_[i].aggressive_order_filled_percent_,
            this_result_line_vec_[i].improve_order_filled_percent_, this_result_line_vec_[i].average_abs_position_,
            this_result_line_vec_[i].msg_count_, this_result_line_vec_[i].num_opentrade_hits_,
            this_result_line_vec_[i].abs_closing_position_, this_result_line_vec_[i].unit_trade_size_,
            this_result_line_vec_[i].ptrades_, this_result_line_vec_[i].ttrades_,
	    this_result_line_vec_[i].total_trades, tratio);
  	total_pnl_ += this_result_line_vec_[i].pnl_;
        count_++;
        if (this_result_line_vec_[i].pnl_ > max_pnl_)
          max_pnl_ = this_result_line_vec_[i].pnl_;

      }

      std::cout << this_result_line_set_.strategy_filename_base_ << " ";
      printf(
          "STATISTICS Pnl: %d avg: %d stdev: %d ab_op_pos: %.2f vol: %d sharpe: %.2f avg_min_adj_pnl: %d med-ttc: %d, %d mkt_pt: %.2f draw: %d apos: %.2f msgs: %d pnl_by_loss: %.2f 95%%loss: %d ttv: %.2f %%+vetrds: %.2f cnt: %d max: %d tot-max*sharpe: %0.2f\n",
	  (int)total_pnl_, (int)this_result_line_set_.pnl_average_, (int)this_result_line_set_.pnl_stdev_,
      	  this_result_line_set_.average_abs_closing_position_,
          (int)this_result_line_set_.volume_average_, this_result_line_set_.pnl_sharpe_,
          (int)this_result_line_set_.average_min_adjusted_pnl_,
          (int)this_result_line_set_.median_average_time_to_close_trades_,
          (int)this_result_line_set_.median_volume_normalized_average_time_to_close_trades_,
          (double)this_result_line_set_.aggressive_order_filled_percent_,
	  (int)this_result_line_set_.average_max_drawdown_, (double)this_result_line_set_.average_abs_position_, 
          (int)this_result_line_set_.average_msg_count_, 
          this_result_line_set_.pnl_by_maxloss_, (int)this_result_line_set_.ninety_five_percentile_,
          this_result_line_set_.average_num_opentrade_hits_, 
          this_result_line_set_.percent_positive_trades_, (int)count_, (int)max_pnl_,
          (total_pnl_ > 0.0)?(total_pnl_ - max_pnl_)*this_result_line_set_.pnl_sharpe_
			:-1*(total_pnl_ - max_pnl_)*this_result_line_set_.pnl_sharpe_);
//     printf("STATISTICS Pnl: %d avg: %d stdev: %d ab_op_pos: %.2f vol: %d sharpe: %.2f avg_min_adj_pnl: %d med-ttc: %d, %d mkt_pt: %.2f",(int)total_pnl_, (int)this_result_line_set_.pnl_average_, (int)this_result_line_set_.pnl_stdev_,this_result_line_set_.average_abs_closing_position_,(int)this_result_line_set_.volume_average_, this_result_line_set_.pnl_sharpe_,(int)this_result_line_set_.average_min_adjusted_pnl_,(int)this_result_line_set_.median_average_time_to_close_trades_,(int)this_result_line_set_.median_volume_normalized_average_time_to_close_trades_,this_result_line_set_.aggressive_order_filled_percent_);//,(int)this_result_line_set_.average_max_drawdown_);
      std::cout << std::endl;
    }
  }

  return 0;
}

