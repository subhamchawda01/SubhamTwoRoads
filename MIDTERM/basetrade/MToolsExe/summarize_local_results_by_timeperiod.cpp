/**
   \file MToolsExe/summarize_local_results_by_timeperiod.cpp

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
#include <map>
#include <algorithm>
#include <signal.h>
#include <unistd.h>

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonDataStructures/map_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#include "basetrade/MToolsExe/result_line_set.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CDef/email_utils.hpp"
#include "basetrade/SqlCpp/SqlCppUtils.hpp"

using namespace HFSAT;

std::string g_sigv_string_;
void termination_handler(int signum) {
  if (signum == SIGSEGV) {  // On a segfault inform , so this is picked up and fixed.
    char hostname_[128];
    hostname_[127] = '\0';
    gethostname(hostname_, 127);

    std::string email_string_ =
        "summarize_local_results_by_timeperiod received sigsegv on " + std::string(hostname_) + "\n";
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

/// This script is meant to be called by the find_optimal_intervals_for_pickstrats script
int main(int argc, char **argv) {
  signal(SIGINT, termination_handler);
  signal(SIGSEGV, termination_handler);
  for (int i = 0; i < argc; i++) {
    g_sigv_string_ += std::string(argv[i]) + " ";
  }
  std::vector<std::string> resultsfilename_vec_;

  // local variables
  ResultLineSetVec result_set_vec_;  ///< input .. and results spliced from this as they are chosen and put into chosen

  // command line processing
  if (argc < 5) {
    std::cerr << argv[0] << " timeperiod startdate enddate results_top_directory" << std::endl;
    exit(0);
  }

  // process command line arguments
  const bool is_us_period_ = (strcmp(argv[1], "US_MORN_DAY") == 0) ? 1 : 0;

  const int trading_start_yyyymmdd_ = atoi(argv[2]);
  const int trading_end_yyyymmdd_ = atoi(argv[3]);

  std::string globalresultsdbdir_ = argv[4];

  bool initialized = false;

  if (trading_end_yyyymmdd_ < 20110101 || trading_end_yyyymmdd_ > 20140101) {
    std::cerr << "Tradingdate " << trading_end_yyyymmdd_ << " not in range\n";
    exit(0);
  }
  FileToResultLineSetMap file_to_res_map_;

  {
    int tradingdate_ = trading_end_yyyymmdd_;
    for (auto i = 0u; i < 2000; i++)  // at most 2000 days at a time
    {
      HFSAT::YYYYMMDDStr_t _yyyymmdd_str_ = HFSAT::DateTime::BreakDateYYYYMMDD(tradingdate_);

      std::ostringstream t_temp_oss_;
      t_temp_oss_ << globalresultsdbdir_ << "/" << _yyyymmdd_str_.yyyy_str_ << "/" << _yyyymmdd_str_.mm_str_ << "/"
                  << _yyyymmdd_str_.dd_str_ << "/results_database.txt";
      // fprintf( stderr, "file name is %s \n", t_temp_oss_.str( ).c_str( ) );
      std::string this_resultfilename_(t_temp_oss_.str());

      std::ifstream infile_;
      infile_.open(this_resultfilename_.c_str());

      if (infile_.is_open()) {
        const unsigned int kLineBufferLen = 1024;
        char readline_buffer_[kLineBufferLen];
        bzero(readline_buffer_, kLineBufferLen);

        while (infile_.good()) {
          bzero(readline_buffer_, kLineBufferLen);
          infile_.getline(readline_buffer_, kLineBufferLen);
          HFSAT::PerishableStringTokenizer st_(readline_buffer_, kLineBufferLen);
          const std::vector<const char *> &tokens_ = st_.GetTokens();

          if (tokens_.size() >= 4) {  // filename date pnl volume supporting% bestlevel% agg% [ Average_Abs_Pos
                                      // Median_Trade_Close_Seconds  Avg_Trade_Close_Seconds  Median_Trade_PNL
                                      // Avg_Trade_PNL  Stdev_Trade_PNL  Sharpe_Trade_PNL  Fracpos_Trade_PNL
                                      // MIN_PNL_OF_DAY  MAX_PNL_OF_DAY ]

            // If PNL and Vol is 0, then skip this line
            if (atoi(tokens_[2]) == 0 && atoi(tokens_[3]) == 0) {
              continue;
            }

            std::string strategy_filename_base_ =
                tokens_[0];  // not matching to any list .. all the files here are important

            ResultLine result_line_(tokens_);
            if (file_to_res_map_.find(strategy_filename_base_) ==
                file_to_res_map_.end()) {  // first time so add the strategy_filename_base_ if it matches criteria

              if (initialized)
                continue;  // these strats as not found after the iteration of trading end day were removed in the
                           // interval

              // int end_date_on_file_ = GetInsampleDate ( strategy_filename_base_ );
              // HFSAT::StrategyDesc str = HFSAT::StrategyLine(strategy_filename_base_);
              ///*
              char *t_strat_basename = new char[strategy_filename_base_.size() + 1];
              strcpy(t_strat_basename, strategy_filename_base_.c_str());
              char *token = strtok(t_strat_basename, "_");
              while (token != NULL && (strlen(token) < 3 || strncmp(token, "201", 3) != 0)) token = strtok(NULL, "_");
              if (token == NULL || strlen(token) < 3) continue;
              token = strtok(NULL, "_");
              int end_date_on_file_ = atoi(token);
              if (end_date_on_file_ > trading_end_yyyymmdd_)
                continue;  // these strats were added after the interval considered

              // checking for the validity of timeperiod of strategy
              token = strtok(NULL, "_");
              std::string start_hhmm, end_hhmm;
              if ((strncmp(token, "EST", 3) == 0) || (strncmp(token, "CST", 3) == 0) ||
                  (strncmp(token, "CET", 3) == 0) || (strncmp(token, "BRT", 3) == 0) ||
                  (strncmp(token, "UTC", 3) == 0)) {
                start_hhmm.append(token);
                token = strtok(NULL, "_");
                start_hhmm.append("_");
                start_hhmm.append(token);
                token = strtok(NULL, "_");
                end_hhmm.append(token);
                token = strtok(NULL, "_");
                end_hhmm.append("_");
                end_hhmm.append(token);
                if (is_us_period_) {
                  int end_utc_time =
                      HFSAT::DateTime::GetUTCHHMMFromTZHHMM(tradingdate_, atoi(end_hhmm.c_str() + 4), end_hhmm.c_str());
                  if (end_utc_time < 1200) continue;
                } else {
                  int start_utc_time = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(tradingdate_, atoi(start_hhmm.c_str() + 4),
                                                                             start_hhmm.c_str());
                  if (start_utc_time > 1300) continue;
                }
              } else {
                if (is_us_period_ == 0) {
                  if (atoi(token) > 1300) continue;
                } else {
                  token = strtok(NULL, "_");
                  if (atoi(token) < 1200) continue;
                }
              }
              //*/
              // int end_hhmm_on_file_ = GetTradingEndHHMM ( strategy_filename_base_ );
              // if ( is_us_period_ && end_hhmm_on_file_)
              // continue;//invalid for the timeperiod
              file_to_res_map_[strategy_filename_base_].strategy_filename_base_ = strategy_filename_base_;
              file_to_res_map_[strategy_filename_base_].result_line_vec_.clear();
            }
            if (initialized) file_to_res_map_[strategy_filename_base_].result_line_vec_.push_back(result_line_);
          }
        }
        initialized = true;
        infile_.close();
      }

      tradingdate_ = HFSAT::DateTime::CalcPrevDay(tradingdate_);
      if ((tradingdate_ < 20100101 || tradingdate_ > 20140101) || (tradingdate_ > trading_end_yyyymmdd_) ||
          (tradingdate_ < trading_start_yyyymmdd_)) {
        break;
      }
    }
  }
  // move data to vector .. now map not needed
  GetValueVecFromMap(file_to_res_map_, result_set_vec_);

  // sort all results by date
  // just for printing purposes
  for (auto i = 0u; i < result_set_vec_.size(); i++) {
    ResultLineSet &this_result_line_set_ = result_set_vec_[i];
    ResultLineVec &this_result_line_vec_ = this_result_line_set_.result_line_vec_;
    // Sort the vector using predicate and std::sort
    std::sort(this_result_line_vec_.begin(), this_result_line_vec_.end(), SortResultLineDate);
  }

  // compute statistics :
  for (auto i = 0u; i < result_set_vec_.size(); i++) {
    ResultLineSet &this_result_line_set_ = result_set_vec_[i];
    this_result_line_set_.ComputeStatistics();

    // std::cout << this_result_line_set_.strategy_filename_base_ << ' ';

    printf("%d %d %.4f %s %.4f %.2f %d", (int)this_result_line_set_.pnl_average_,
           (int)this_result_line_set_.volume_average_,
           this_result_line_set_.pnl_average_ / this_result_line_set_.average_max_drawdown_,
           this_result_line_set_.strategy_filename_base_.c_str(), this_result_line_set_.pnl_stdev_,
           this_result_line_set_.pnl_sharpe_, (int)this_result_line_set_.median_average_time_to_close_trades_);

    /*
          printf ( "%d %d %d %.2f %d %d %d %.2f %d %d %d %d\n",
          (int)this_result_line_set_.pnl_average_ ,
          (int)this_result_line_set_.pnl_stdev_ ,
          (int)this_result_line_set_.volume_average_ ,
          this_result_line_set_.pnl_sharpe_ ,
          (int)( this_result_line_set_.pnl_average_ - 0.33 * this_result_line_set_.pnl_stdev_ ),
          (int)this_result_line_set_.average_min_adjusted_pnl_,
          (int)this_result_line_set_.median_average_time_to_close_trades_ ,
          (double)this_result_line_set_.pnl_per_contract_,
          (int)this_result_line_set_.supporting_order_filled_percent_,
          (int)this_result_line_set_.best_level_order_filled_percent_,
          (int)this_result_line_set_.aggressive_order_filled_percent_,
          (int)this_result_line_set_.average_max_drawdown_ ) ;
     */
    std::cout << std::endl;
  }
  return 0;
}
