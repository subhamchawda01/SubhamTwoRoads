// =====================================================================================
//
//       Filename:  get_trades_within_time_frame.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  12/06/2012 09:16:05 AM
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

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

int main(int argc, char *argv[]) {
  if (argc < 6) {
    std::cerr << "Usage : <exec> <in_tradesfile> <date> <start_time_utc> <end_time_utc> <out_tradesfile>\n ";
    exit(1);
  }

  std::string this_trades_filename_ = argv[1];
  std::string this_out_trade_filename_ = argv[5];

  int start_date_ = atoi(argv[2]);

  std::string hours_start_str_ = argv[3];
  std::string hours_end_str_ = argv[4];

  int utc_start_time_;
  int utc_end_time_;

  utc_start_time_ = atoi(hours_start_str_.substr(0, hours_start_str_.length() - 2).c_str()) * 3600 +
                    atoi(hours_start_str_.substr(hours_start_str_.length() - 2, 2).c_str()) * 60;
  utc_end_time_ = atoi(hours_end_str_.substr(0, hours_end_str_.length() - 2).c_str()) * 3600 +
                  atoi(hours_end_str_.substr(hours_end_str_.length() - 2, 2).c_str()) * 60;

  struct tm timeinfo = {0};

  timeinfo.tm_year = (start_date_ / 10000) - 1900;
  timeinfo.tm_mon = (start_date_ / 100) % 100 - 1;
  timeinfo.tm_mday = (start_date_ % 100);

  time_t unixtime_start_ = mktime(&timeinfo);
  time_t unixtime_end_ = mktime(&timeinfo);

  unixtime_start_ += utc_start_time_;
  unixtime_end_ += utc_end_time_;

  char this_trade_line_[1024];

  std::ifstream this_trade_file_;
  this_trade_file_.open(this_trades_filename_.c_str());

  if (!this_trade_file_.is_open()) {
    std::cerr << " Could Not Open Trade File : " << this_trades_filename_ << " to Read \n";
    exit(1);
  }

  std::ofstream this_out_trade_file_;
  this_out_trade_file_.open(this_out_trade_filename_.c_str());

  if (!this_out_trade_file_.is_open()) {
    std::cerr << " Could Not Open Trade File : " << this_out_trade_filename_ << " to Write \n";
    exit(1);
  }

  while (this_trade_file_.good()) {
    this_trade_file_.getline(this_trade_line_, 1024);

    std::string this_line_buffer_ = this_trade_line_;

    HFSAT::PerishableStringTokenizer st_(this_trade_line_, 1024);
    const std::vector<const char *> &tokens_ = st_.GetTokens();

    if (tokens_.size() != 16) continue;

    std::string this_time_token_ = tokens_[0];

    int this_trade_time_ = atoi((this_time_token_.substr(0, this_time_token_.find("."))).c_str());

    if (this_trade_time_ < (int)unixtime_start_ || this_trade_time_ > (int)unixtime_end_) continue;

    this_out_trade_file_ << this_line_buffer_ << "\n";
  }

  this_out_trade_file_.close();
  this_trade_file_.close();

  return EXIT_SUCCESS;
}  // ----------  end of function main  ----------
