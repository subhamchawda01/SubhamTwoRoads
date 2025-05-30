/**
   \file MToolsExe/choose_strategy_set_by_algo.cpp

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

#include "dvccode/CDef/file_utils.hpp"

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonDataStructures/map_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#include "dvccode/CDef/email_utils.hpp"
#include "basetrade/MToolsExe/result_line_set.hpp"

using namespace HFSAT;

std::string g_sigv_string_;
void termination_handler(int signum) {
  if (signum == SIGSEGV) {  // On a segfault inform , so this is picked up and fixed.
    char hostname_[128];
    hostname_[127] = '\0';
    gethostname(hostname_, 127);

    std::string email_string_ = "choose_strategy_set_by_algo received sigsegv on " + std::string(hostname_) +
                                std::string(" run by ") + std::string(getenv("USER"));
    std::string email_address_ = "nseall@tworoads.co.in";

    HFSAT::Email email_;
    email_.setSubject(email_string_);
    email_.addRecepient(email_address_);
    email_.addSender(email_address_);
    email_.content_stream << email_string_ << "<br/>";
    email_.sendMail();
    abort();
  }
  exit(0);
}

/// This script is meant to be called by the script that makes strategies from models
/// It will be given a local database with results of only the files that are being tested here
/// This exec will look at all the result_files provided
/// read them "strategyfilenamebase date pnl volume"
/// build the map of strategyfilenamebase to a streuct with results and statistics on the results
/// then it chooses based on the algo specified
/// and prints the chosen strategy files ( with results and statistics )
int main(int argc, char** argv) {
  signal(SIGINT, termination_handler);
  signal(SIGSEGV, termination_handler);
  for (int i = 0; i < argc; i++) {
    g_sigv_string_ += std::string(argv[i]) + " ";
  }
  // input variables
  unsigned int min_num_files_to_choose_ = 0;
  unsigned int num_files_to_choose_ = 1;
  // input filters
  double min_pnl_per_contract_to_allow_ = -1;
  unsigned int min_volume_to_allow_ = 10;
  unsigned int max_ttc_to_allow_ = 60;

  std::vector<std::string> resultsfilename_vec_;

  // local variables
  ResultLineSetVec result_set_vec_left_to_choose_;  ///< input .. and result_sets are spliced from this as they are
  /// chosen and put into chosen
  ResultLineSetVec result_set_vec_chosen_;  ///< chosen files

  // command line processing
  if (argc < 7) {
    std::cerr << argv[0]
              << " sortalgo min_num_files_to_choose_ num_files_to_choose_ min_ppt min_volume max_ttc resultsfiles"
              << std::endl;
    exit(0);
  }

  // process command line arguments
  ChooseNextAlgo_t cna_ = GetChooseNextAlgoFromString(argv[1]);

  min_num_files_to_choose_ = std::max(0, atoi(argv[2]));
  num_files_to_choose_ = atoi(argv[3]);
  min_pnl_per_contract_to_allow_ = atof(argv[4]);
  min_volume_to_allow_ = atoi(argv[5]);
  max_ttc_to_allow_ = atoi(argv[6]);

  // loading up the result file name arugments
  for (int i = 7; i < argc; i++) {
    if (FileUtils::ExistsAndReadable(argv[i])) {
      resultsfilename_vec_.push_back(argv[i]);
    } else {  // if a file is not readable chances are this is an error
      break;
    }
  }

  {
    FileToResultLineSetMap file_to_res_map_;
    // process one file at a time
    // just read the data and build the map file_to_res_map_
    for (auto i = 0u; i < resultsfilename_vec_.size(); i++) {
      std::ifstream infile_;
      infile_.open(resultsfilename_vec_[i].c_str());
      if (!infile_.is_open()) {
        std::cerr << " Input result file " << resultsfilename_vec_[i] << " did not open " << std::endl;
        exit(0);
      }

      const unsigned int kLineBufferLen = 1024;
      char readline_buffer_[kLineBufferLen];
      bzero(readline_buffer_, kLineBufferLen);

      while (infile_.good()) {
        bzero(readline_buffer_, kLineBufferLen);
        infile_.getline(readline_buffer_, kLineBufferLen);
        HFSAT::PerishableStringTokenizer st_(readline_buffer_, kLineBufferLen);
        const std::vector<const char*>& tokens_ = st_.GetTokens();

        if (tokens_.size() >= 4) {  // filename date pnl volume supporting% bestlevel% agg% [ Average_Abs_Pos
                                    // Median_Trade_Close_Seconds  Avg_Trade_Close_Seconds  Median_Trade_PNL
                                    // Avg_Trade_PNL  Stdev_Trade_PNL  Sharpe_Trade_PNL  Fracpos_Trade_PNL
                                    // MIN_PNL_OF_DAY  MAX_PNL_OF_DAY ] Max_Trade_Close_Seconds

          std::string strategy_filename_base_ =
              tokens_[0];  // not matching to any list .. all the files here are important

          ResultLine result_line_(tokens_);

          if (file_to_res_map_.find(strategy_filename_base_) ==
              file_to_res_map_.end()) {  // first time so add the strategy_filename_base_
            file_to_res_map_[strategy_filename_base_].strategy_filename_base_ = strategy_filename_base_;
            file_to_res_map_[strategy_filename_base_].result_line_vec_.clear();
          }
          file_to_res_map_[strategy_filename_base_].result_line_vec_.push_back(result_line_);
        }
      }

      if (infile_.is_open()) {
        infile_.close();
      }
    }

    // move data to vector .. now map not needed
    GetValueVecFromMap(file_to_res_map_, result_set_vec_left_to_choose_);
  }

  // sort all results by date
  // just for printing purposes
  for (auto i = 0u; i < result_set_vec_left_to_choose_.size(); i++) {
    ResultLineSet& this_result_line_set_ = result_set_vec_left_to_choose_[i];
    ResultLineVec& this_result_line_vec_ = this_result_line_set_.result_line_vec_;
    // Sort the vector using predicate and std::sort
    std::sort(this_result_line_vec_.begin(), this_result_line_vec_.end(), SortResultLineDate);
  }

  // compute statistics :
  for (auto i = 0u; i < result_set_vec_left_to_choose_.size(); i++) {
    ResultLineSet& this_result_line_set_ = result_set_vec_left_to_choose_[i];
    this_result_line_set_.ComputeStatistics();
  }

  // initial sort
  auto sort_func_ = GetSortFunc(cna_);
  std::sort(result_set_vec_left_to_choose_.rbegin(), result_set_vec_left_to_choose_.rend(), sort_func_);

  // for ( unsigned int i = 0 ; i < result_set_vec_left_to_choose_.size ( ); i ++ )
  //   {
  //     const ResultLineSet & this_result_line_set_ = result_set_vec_left_to_choose_[i];
  //     // const ResultLineVec & this_result_line_vec_ = this_result_line_set_.result_line_vec_;

  //     std::cout << "STRATEGYFILEBASE " << this_result_line_set_.strategy_filename_base_ << "\n";

  //     printf ( "STATISTICS %d %d %d %.2f %d %d %d %d %.2f %d %d %d %d %.2f %d %d %d %.2f %.2f %d\n",
  // 	       (int) this_result_line_set_.pnl_average_ ,
  // 	       (int) this_result_line_set_.pnl_stdev_ ,
  // 	       (int) this_result_line_set_.volume_average_ ,
  // 	       this_result_line_set_.pnl_sharpe_ ,
  // 	       (int)( this_result_line_set_.pnl_average_ - 0.33 * this_result_line_set_.pnl_stdev_ ),
  // 	       (int) this_result_line_set_.average_min_adjusted_pnl_,
  // 	       (int) this_result_line_set_.median_average_time_to_close_trades_ ,
  // 	       (int) this_result_line_set_.median_volume_normalized_average_time_to_close_trades_ ,
  // 	       (double)this_result_line_set_.pnl_per_contract_,
  // 	       (int) this_result_line_set_.supporting_order_filled_percent_,
  // 	       (int) this_result_line_set_.best_level_order_filled_percent_,
  // 	       (int) this_result_line_set_.aggressive_order_filled_percent_,
  // 	       (int) this_result_line_set_.average_max_drawdown_ ,
  // 	       (double) this_result_line_set_.average_abs_position_,
  // 	       (int) this_result_line_set_.pnl_conservative_average_ ,
  // 	       (int) this_result_line_set_.pnl_median_average_ ,
  // 	       this_result_line_set_.average_msg_count_,
  // 	       this_result_line_set_.gain_to_pain_ratio_,
  // 	       this_result_line_set_.pnl_by_maxloss_,
  // 	       (int)this_result_line_set_.ninety_five_percentile_ );
  //     std::cout << std::endl;
  //   }

  // choose files
  int num_files_chosen_already_ = 0;
  int num_files_to_choose_now_ = min_num_files_to_choose_;
  ResultLineSet sum_of_chosen_result_sets_;
  while ((num_files_to_choose_now_ > 0) && (result_set_vec_left_to_choose_.size() > 0)) {
    // ChooseNext ( result_set_vec_left_to_choose_, num_files_to_choose_now_, cna_, result_set_vec_chosen_ ) ;
    // sum_of_chosen_result_sets_.AddResultSet ( result_set_vec_chosen_.back() ) ;
    AddNextChoiceToSet(result_set_vec_left_to_choose_, num_files_to_choose_now_, cna_, result_set_vec_chosen_,
                       sum_of_chosen_result_sets_);
    num_files_chosen_already_++;
    // std::cout << " Choice (" << num_files_chosen_already_ << ") : " <<
    // result_set_vec_chosen_.back().strategy_filename_base_ << std::endl;
  }

  // remove results lines with very low volume || very high ttc || low ppt
  for (ResultLineSetVecIter_t rlsviter_ = result_set_vec_left_to_choose_.begin();
       rlsviter_ != result_set_vec_left_to_choose_.end();) {
    if ((rlsviter_->median_volume_normalized_average_time_to_close_trades_ > max_ttc_to_allow_) ||
        (rlsviter_->volume_average_ < min_volume_to_allow_) ||
        (rlsviter_->pnl_per_contract_ < min_pnl_per_contract_to_allow_)) {
      rlsviter_ = result_set_vec_left_to_choose_.erase(rlsviter_);
    } else {
      rlsviter_++;
    }
  }

  int num_files_left_to_choose_ = num_files_to_choose_ - num_files_chosen_already_;
  while ((num_files_left_to_choose_ > 0) && (result_set_vec_left_to_choose_.size() > 0)) {
    // ChooseNext ( result_set_vec_left_to_choose_, num_files_left_to_choose_, cna_, result_set_vec_chosen_ ) ;
    // sum_of_chosen_result_sets_.AddResultSet ( result_set_vec_chosen_.back() ) ;
    AddNextChoiceToSet(result_set_vec_left_to_choose_, num_files_left_to_choose_, cna_, result_set_vec_chosen_,
                       sum_of_chosen_result_sets_);
    num_files_chosen_already_++;
    // std::cout << " Choice (" << num_files_chosen_already_ << ") : " <<
    // result_set_vec_chosen_.back().strategy_filename_base_ << std::endl;
  }

  // print results and statistics of the chosen files
  // Sum
  if (0) {
    std::cout << "STRATEGYFILENAMEBASE SUM\n";

    const ResultLineSet& this_result_line_set_ = sum_of_chosen_result_sets_;
    const ResultLineVec& this_result_line_vec_ = this_result_line_set_.result_line_vec_;

    for (auto i = 0u; i < this_result_line_vec_.size(); i++) {
      printf(
          "%d %d %d Turn: %d, %d norm-tcc %d min: %d max: %d draw: %d zs: %.2f S: %d B: %d A: %d apos: %.2f msgs: %d\n",
          this_result_line_vec_[i].yyyymmdd_, (int)this_result_line_vec_[i].pnl_, this_result_line_vec_[i].volume_,
          this_result_line_vec_[i].median_time_to_close_trades_, this_result_line_vec_[i].average_time_to_close_trades_,
          this_result_line_vec_[i].volume_normalized_average_time_to_close_trades_,
          (int)this_result_line_vec_[i].min_pnl_, (int)this_result_line_vec_[i].max_pnl_,
          (int)this_result_line_vec_[i].max_drawdown_, this_result_line_vec_[i].pnl_zscore_,
          this_result_line_vec_[i].supporting_order_filled_percent_,
          this_result_line_vec_[i].best_level_order_filled_percent_,
          this_result_line_vec_[i].aggressive_order_filled_percent_, this_result_line_vec_[i].average_abs_position_,
          this_result_line_vec_[i].msg_count_);
    }

    printf("STATISTICS %d %d %d %.2f %d %d %d %d %.2f %d %d %d %d %.2f %d %d %d %.2f %.2f %d\n",
           (int)this_result_line_set_.pnl_average_, (int)this_result_line_set_.pnl_stdev_,
           (int)this_result_line_set_.volume_average_, this_result_line_set_.pnl_sharpe_,
           (int)(this_result_line_set_.pnl_average_ - 0.33 * this_result_line_set_.pnl_stdev_),
           (int)this_result_line_set_.average_min_adjusted_pnl_,
           (int)this_result_line_set_.median_average_time_to_close_trades_,
           (int)this_result_line_set_.median_volume_normalized_average_time_to_close_trades_,
           (double)this_result_line_set_.pnl_per_contract_, (int)this_result_line_set_.supporting_order_filled_percent_,
           (int)this_result_line_set_.best_level_order_filled_percent_,
           (int)this_result_line_set_.aggressive_order_filled_percent_,
           (int)this_result_line_set_.average_max_drawdown_, (double)this_result_line_set_.average_abs_position_,
           (int)this_result_line_set_.pnl_conservative_average_, (int)this_result_line_set_.pnl_median_average_,
           this_result_line_set_.average_msg_count_, this_result_line_set_.gain_to_pain_ratio_,
           this_result_line_set_.pnl_by_maxloss_, (int)this_result_line_set_.ninety_five_percentile_);
    std::cout << std::endl;
  }
  // Individual
  for (auto i = 0u; i < result_set_vec_chosen_.size(); i++) {
    const ResultLineSet& this_result_line_set_ = result_set_vec_chosen_[i];
    const ResultLineVec& this_result_line_vec_ = this_result_line_set_.result_line_vec_;

    std::cout << "STRATEGYFILEBASE " << this_result_line_set_.strategy_filename_base_ << "\n";

    for (auto i = 0u; i < this_result_line_vec_.size(); i++) {
      printf(
          "%d %d %d Turn: %d, %d norm-tcc %d min: %d max: %d draw: %d zs: %.2f S: %d B: %d A: %d apos: %.2f msgs: %d\n",
          this_result_line_vec_[i].yyyymmdd_, (int)this_result_line_vec_[i].pnl_, this_result_line_vec_[i].volume_,
          this_result_line_vec_[i].median_time_to_close_trades_, this_result_line_vec_[i].average_time_to_close_trades_,
          this_result_line_vec_[i].volume_normalized_average_time_to_close_trades_,
          (int)this_result_line_vec_[i].min_pnl_, (int)this_result_line_vec_[i].max_pnl_,
          (int)this_result_line_vec_[i].max_drawdown_, this_result_line_vec_[i].pnl_zscore_,
          this_result_line_vec_[i].supporting_order_filled_percent_,
          this_result_line_vec_[i].best_level_order_filled_percent_,
          this_result_line_vec_[i].aggressive_order_filled_percent_, this_result_line_vec_[i].average_abs_position_,
          this_result_line_vec_[i].msg_count_);
    }

    printf("STATISTICS %d %d %d %.2f %d %d %d %d %.2f %d %d %d %d %.2f %d %d %d %.2f %.2f %d\n",
           (int)this_result_line_set_.pnl_average_, (int)this_result_line_set_.pnl_stdev_,
           (int)this_result_line_set_.volume_average_, this_result_line_set_.pnl_sharpe_,
           (int)(this_result_line_set_.pnl_average_ - 0.33 * this_result_line_set_.pnl_stdev_),
           (int)this_result_line_set_.average_min_adjusted_pnl_,
           (int)this_result_line_set_.median_average_time_to_close_trades_,
           (int)this_result_line_set_.median_volume_normalized_average_time_to_close_trades_,
           (double)this_result_line_set_.pnl_per_contract_, (int)this_result_line_set_.supporting_order_filled_percent_,
           (int)this_result_line_set_.best_level_order_filled_percent_,
           (int)this_result_line_set_.aggressive_order_filled_percent_,
           (int)this_result_line_set_.average_max_drawdown_, (double)this_result_line_set_.average_abs_position_,
           (int)this_result_line_set_.pnl_conservative_average_, (int)this_result_line_set_.pnl_median_average_,
           this_result_line_set_.average_msg_count_, this_result_line_set_.gain_to_pain_ratio_,
           this_result_line_set_.pnl_by_maxloss_, (int)this_result_line_set_.ninety_five_percentile_);
    std::cout << std::endl;
  }

  return 0;
}
