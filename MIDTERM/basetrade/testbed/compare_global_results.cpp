// =====================================================================================
//
//       Filename:  compare_global_results.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  01/28/2013 10:23:54 AM
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

#include <iostream>
#include <string>
#include <string.h>
#include <fstream>
#include <vector>
#include <map>
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#include <cstdlib>

int main(int argc, char *argv[]) {
  if (argc < 5) {
    std::cerr << " Usage : < exec > < resultfile > < resultfile > < PNL-Threshold > < Vol-Threshold > \n";
    exit(1);
  }

  std::map<std::string, double> file1_key_pnls_;
  std::map<std::string, double> file2_key_pnls_;

  std::map<std::string, unsigned int> file1_key_vol_;
  std::map<std::string, unsigned int> file2_key_vol_;

  std::string this_results_file_1 = argv[1];
  std::string this_results_file_2 = argv[2];

  int pnl_threshold_ = atoi(argv[3]);
  int vol_threshold_ = atoi(argv[4]);

  std::ifstream results_one_file_;
  std::ifstream results_two_file_;

  results_one_file_.open(this_results_file_1.c_str());
  results_two_file_.open(this_results_file_2.c_str());

  if (!results_one_file_.is_open() || !results_two_file_.is_open()) {
    std::cerr << "Can't open files \n";
    exit(1);
  }

  char buffer[4096];

  memset(buffer, 0, 4096);

  while (results_one_file_.good()) {
    results_one_file_.getline(buffer, 4096);

    HFSAT::PerishableStringTokenizer st_(buffer, 4096);
    const std::vector<const char *> &tokens_ = st_.GetTokens();

    if (tokens_.size() < 4) continue;

    std::string map_key_ = std::string(tokens_[0]) + std::string(tokens_[1]);

    file1_key_pnls_[map_key_] = atof(tokens_[2]);
    file1_key_vol_[map_key_] = atoi(tokens_[3]);
  }

  results_one_file_.close();

  memset(buffer, 0, 4096);

  while (results_two_file_.good()) {
    results_two_file_.getline(buffer, 4096);

    HFSAT::PerishableStringTokenizer st_(buffer, 4096);
    const std::vector<const char *> &tokens_ = st_.GetTokens();

    if (tokens_.size() < 4) continue;

    std::string map_key_ = std::string(tokens_[0]) + std::string(tokens_[1]);

    file2_key_pnls_[map_key_] = atof(tokens_[2]);
    file2_key_vol_[map_key_] = atoi(tokens_[3]);
  }

  results_two_file_.close();

  std::map<std::string, double>::iterator itr_ = file1_key_pnls_.begin();

  while (itr_ != file1_key_pnls_.end()) {
    if (file2_key_pnls_.find(itr_->first) == file2_key_pnls_.end()) {
      std::cerr << " No value For : " << itr_->first << "\n";
      itr_++;
      continue;

    }

    else {
      if ((itr_->second) == 0) {
        itr_++;
        continue;
      }

      double abs_diff_ = abs((itr_->second) - file2_key_pnls_[(itr_->first)]);

      double this_pnls_diff_in_percent_ = (abs_diff_ / abs((itr_->second))) * 100;

      if (this_pnls_diff_in_percent_ < 0) {
        this_pnls_diff_in_percent_ *= -1;
      }

      if (this_pnls_diff_in_percent_ > pnl_threshold_) {
        printf(" Key : %s MPnl : %lf NewPnl : %lf Change : %lf \n", ((itr_->first).c_str()), (itr_->second),
               file2_key_pnls_[(itr_->first)], this_pnls_diff_in_percent_);
      }
    }

    itr_++;
  }

  std::map<std::string, unsigned int>::iterator iitr_ = file1_key_vol_.begin();

  while (iitr_ != file1_key_vol_.end()) {
    if (file2_key_vol_.find(iitr_->first) == file2_key_vol_.end()) {
      std::cerr << " No value For : " << iitr_->first << "\n";
      iitr_++;
      continue;

    }

    else {
      if ((iitr_->second) == 0) {
        iitr_++;
        continue;
      }

      int this_abs_vol_diff_ = abs(((iitr_->second) - file2_key_vol_[(iitr_->first)]));

      double this_vol_diff_in_percent_ = ((this_abs_vol_diff_) / (double)((iitr_->second))) * 100;

      if (this_vol_diff_in_percent_ < 0) {
        this_vol_diff_in_percent_ *= -1;
      }

      if (this_vol_diff_in_percent_ > vol_threshold_) {
        printf(" Key : %s Mvol : %d NewVol : %d Change : %lf \n", ((iitr_->first).c_str()), (iitr_->second),
               file2_key_vol_[(iitr_->first)], this_vol_diff_in_percent_);
      }
    }

    iitr_++;
  }

  return EXIT_SUCCESS;
}  // ----------  end of function main  ----------
