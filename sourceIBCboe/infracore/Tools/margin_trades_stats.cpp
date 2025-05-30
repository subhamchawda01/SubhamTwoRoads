/**
  \file Tools/volume_symbol_reconcilation.cpp

  \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
     Suite No 162, Evoma, #14, Bhattarhalli,
     Old Madras Road, Near Garden City College,
     KR Puram, Bangalore 560049, India
     +91 80 4190 3551
*/

#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iomanip>

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#define MAX_LINE_LENGTH 1024
#define MARGIN_TRADES_COLS 17

// TODO get our tradevol/exch trade volume over given time interval

// int GetTradeVolume( ){

// read MDs

//}

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cout << " Usage : <exec> <margin_trades_file> <shortcode> \n";
    exit(-1);
  }

  std::string margin_trades_file_name_ = argv[1];
  std::string shortcode_ = argv[2];

  std::ifstream margin_trades_file_;
  margin_trades_file_.open(margin_trades_file_name_.c_str(), std::ios::in);

  // read mds struct for time

  if (!margin_trades_file_.is_open()) {
    std::cerr << " File : " << margin_trades_file_name_ << " Doesn't Exists \n";
    exit(-1);
  }

  char buffer_[MAX_LINE_LENGTH];
  std::string line_read_ = "";

  double open_position_start_time_ = 0.0;

  bool continue_hold_ = false;

  while (margin_trades_file_.good()) {
    memset(buffer_, 0, MAX_LINE_LENGTH);
    line_read_ = "";

    margin_trades_file_.getline(buffer_, MAX_LINE_LENGTH);
    line_read_ = buffer_;

    HFSAT::PerishableStringTokenizer st_(buffer_, MAX_LINE_LENGTH);
    const std::vector<const char *> &tokens_ = st_.GetTokens();

    if (tokens_.size() == MARGIN_TRADES_COLS) {
      if (strcmp(tokens_[2], shortcode_.c_str())) continue;

      if (!strcmp(tokens_[1], "OPEN") && !continue_hold_) {
        open_position_start_time_ = atof(tokens_[0]);
        continue_hold_ = true;

      } else if (!strcmp(tokens_[1], "FLAT")) {
        double time_diff_ = (atof(tokens_[0]) - open_position_start_time_);
        int int_time_diff_ = time_diff_ * 1000;

        printf("%.6lf %d\n", open_position_start_time_, int_time_diff_);
        continue_hold_ = false;
      }
    }
  }

  margin_trades_file_.close();

  return 0;
}
