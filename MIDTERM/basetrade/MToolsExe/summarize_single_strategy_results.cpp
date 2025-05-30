/**
   \file MToolsExe/summarize_single_strategy_results.cpp

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

    std::string email_string_ = "summarize_strategy_results received sigsegv on " + std::string(hostname_) + "\n";
    email_string_ += g_sigv_string_ + "\n";
    std::string email_address_ = "";

    HFSAT::Email email_;
    email_.setSubject(email_string_);

    if (!strncmp(getenv("USER"), "sghosh", strlen("sghosh"))) {
      email_address_ = "sghosh@circulumvite.com";
    } else if (!strncmp(getenv("USER"), "dvctrader", strlen("dvctrader"))) {
      email_address_ = "nseall@tworoads.co.in";
    } else if (!strncmp(getenv("USER"), "ankit", strlen("ankit"))) {
      email_address_ = "ankit@tworoads.co.in";
    } else if (!strncmp(getenv("USER"), "rkumar", strlen("rkumar"))) {
      email_address_ = "rakesh.kumar@tworoads.co.in";
    } else if (!strncmp(getenv("USER"), "ravi", strlen("ravi"))) {
      email_address_ = "ravi.parikh@tworoads.co.in";
    } else if (!strncmp(getenv("USER"), "kputta", strlen("kputta"))) {
      email_address_ = "kp@circulumvite.com";
    } else if (!strncmp(getenv("USER"), "anshul", strlen("anshul"))) {
      email_address_ = "anshul@tworoads.co.in";
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
int main(int argc, char** argv) {
  signal(SIGINT, termination_handler);
  signal(SIGSEGV, termination_handler);
  for (int i = 0; i < argc; i++) {
    g_sigv_string_ += std::string(argv[i]) + " ";
  }
  // local variables
  std::string shortcode_ = "";                     ///< shortcode of the product whose results these are
  std::string given_strategy_filename_base_ = "";  ///< basename of the file we want to profile
  std::string globalresultsdbdir_ = "";            ///< expected to be /spare/local/global_results_database_dir
  int trading_start_yyyymmdd_ = 0;
  int trading_end_yyyymmdd_ = 0;

  ResultLineSetVec result_set_vec_;

  // command line processing
  if (argc < 6) {
    std::cerr << argv[0]
              << " shortcode strategy_filename globalresultsdbdir trading_start_yyyymmdd trading_end_yyyymmdd "
              << std::endl;
    exit(0);
  }
  shortcode_ = argv[1];
  given_strategy_filename_base_ = argv[2];
  globalresultsdbdir_ = argv[3];
  trading_start_yyyymmdd_ = atoi(argv[4]);
  trading_end_yyyymmdd_ = atoi(argv[5]);

  if (trading_start_yyyymmdd_ < 20100101 || trading_start_yyyymmdd_ > 20201225) {
    std::cerr << "Tradingdate " << trading_start_yyyymmdd_ << " not in range\n";
    exit(0);
  }

  std::set<std::string> strategy_file_base_vec_;
  strategy_file_base_vec_.insert(given_strategy_filename_base_);

  int tradingdate_ = trading_start_yyyymmdd_;
  std::vector<int> specific_date_vec_;
  // build date vec only if specific dates are not specified
  while (tradingdate_ >= 20100101 && tradingdate_ <= std::min(trading_end_yyyymmdd_, 20201225)) {
    specific_date_vec_.push_back(tradingdate_);
    tradingdate_ = DateTime::CalcNextWeekDay(tradingdate_);
  }

  FileToResultLineSetMap file_to_res_map_;
  if (globalresultsdbdir_.compare("DB") == 0) {
    // TODO: optimize for single strat case
    SqlCppUtils::GetResultLineSetVec(file_to_res_map_, shortcode_, specific_date_vec_, strategy_file_base_vec_, 0);
  } else {
    HFSAT::ResultFileUtils::FetchResultsFromFile(file_to_res_map_, shortcode_, globalresultsdbdir_, specific_date_vec_,
                                                 strategy_file_base_vec_, 0);
  }

  // move data to vector .. now map not needed
  GetValueVecFromMap(file_to_res_map_, result_set_vec_);

  // sort all results by date
  // just for printing purposes
  for (auto i = 0u; i < result_set_vec_.size(); i++) {
    ResultLineSet& this_result_line_set_ = result_set_vec_[i];
    ResultLineVec& this_result_line_vec_ = this_result_line_set_.result_line_vec_;
    // Sort the vector using predicate and std::sort
    std::sort(this_result_line_vec_.begin(), this_result_line_vec_.end(), SortResultLineDate);
  }

  // compute statistics :
  for (auto i = 0u; i < result_set_vec_.size(); i++) {
    ResultLineSet& this_result_line_set_ = result_set_vec_[i];
    this_result_line_set_.ComputeStatistics();
  }

  // print results and statistics of the chosen files
  for (auto i = 0u; i < result_set_vec_.size(); i++) {
    const ResultLineSet& this_result_line_set_ = result_set_vec_[i];
    std::cout << "STRATEGYFILEBASE " << this_result_line_set_.strategy_filename_base_ << "\n";

    const ResultLineVec& this_result_line_vec_ = this_result_line_set_.result_line_vec_;
    for (auto i = 0u; i < this_result_line_vec_.size(); i++) {
      printf(
          "%d %d %d Turn: %d, %d norm-tcc %d min: %d max: %d draw: %d zs: %.2f S: %d B: %d A: %d I: %d apos: %.2f "
          "msgs: %d otl_hit: %d abs_op_pos: %.2f uts: %d ptrds: %d ttrds: %d\n",
          this_result_line_vec_[i].yyyymmdd_, (int)this_result_line_vec_[i].pnl_, this_result_line_vec_[i].volume_,
          this_result_line_vec_[i].median_time_to_close_trades_, this_result_line_vec_[i].average_time_to_close_trades_,
          this_result_line_vec_[i].volume_normalized_average_time_to_close_trades_,
          (int)this_result_line_vec_[i].min_pnl_, (int)this_result_line_vec_[i].max_pnl_,
          (int)this_result_line_vec_[i].max_drawdown_, this_result_line_vec_[i].pnl_zscore_,
          this_result_line_vec_[i].supporting_order_filled_percent_,
          this_result_line_vec_[i].best_level_order_filled_percent_,
          this_result_line_vec_[i].aggressive_order_filled_percent_,
          this_result_line_vec_[i].improve_order_filled_percent_, this_result_line_vec_[i].average_abs_position_,
          this_result_line_vec_[i].msg_count_, this_result_line_vec_[i].num_opentrade_hits_,
          this_result_line_vec_[i].abs_closing_position_, this_result_line_vec_[i].unit_trade_size_,
          this_result_line_vec_[i].ptrades_, this_result_line_vec_[i].ttrades_);
    }

    printf(
        "STATISTICS %d %d %d %.2f %d %d %d %d %.2f %d %d %d %d %d %.2f %d %d %d %d %.2f %.2f %d %.2f %.2f %.2f %.2f "
        "%.2f\n",
        (int)this_result_line_set_.pnl_average_, (int)this_result_line_set_.pnl_stdev_,
        (int)this_result_line_set_.volume_average_, this_result_line_set_.pnl_sharpe_,
        (int)(this_result_line_set_.pnl_average_ - 0.33 * this_result_line_set_.pnl_stdev_),
        (int)this_result_line_set_.average_min_adjusted_pnl_,
        (int)this_result_line_set_.median_average_time_to_close_trades_,
        (int)this_result_line_set_.median_volume_normalized_average_time_to_close_trades_,
        (double)this_result_line_set_.pnl_per_contract_, (int)this_result_line_set_.supporting_order_filled_percent_,
        (int)this_result_line_set_.best_level_order_filled_percent_,
        (int)this_result_line_set_.aggressive_order_filled_percent_,
        (int)this_result_line_set_.improve_order_filled_percent_, (int)this_result_line_set_.average_max_drawdown_,
        (double)this_result_line_set_.average_abs_position_, (int)this_result_line_set_.pnl_conservative_average_,
        (int)this_result_line_set_.pnl_median_average_, (int)this_result_line_set_.dd_adj_pnl_average_,
        (int)this_result_line_set_.average_msg_count_, this_result_line_set_.gain_to_pain_ratio_,
        this_result_line_set_.pnl_by_maxloss_, (int)this_result_line_set_.ninety_five_percentile_,
        this_result_line_set_.average_num_opentrade_hits_, this_result_line_set_.average_abs_closing_position_,
        this_result_line_set_.pnl_skewness_, this_result_line_set_.average_unit_trade_size_,
        this_result_line_set_.percent_positive_trades_);
    std::cout << std::endl;
  }

  return 0;
}
