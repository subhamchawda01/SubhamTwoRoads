/**
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include <iostream>
#include <sstream>
#include <string.h>
#include <stdio.h>
#include <map>
#include <string>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

#define BASEFEATURESDIR "/spare/local/Features/"
#define CONFIGFILE "features_config.txt"

#define BUF_LEN 4096000

struct TrdSess {
  std::string shc_;
  int idx_;
  std::string start_time_;
  std::string end_time_;

  TrdSess(const std::string s_shc_, const int t_idx_, const std::string s_stime_, const std::string s_etime_)
      : shc_(s_shc_), idx_(t_idx_), start_time_(s_stime_), end_time_(s_etime_) {}
};

struct StatSet {
  std::vector<double> mean_;
  std::vector<double> median_;
  std::vector<double> stdev_;
  std::vector<double> mean_hquart_;
  std::vector<double> mean_lquart_;
  StatSet() {}
  void add(double t_mean_, double t_median_, double t_stdev_, double t_mhq_, double t_mlq_) {
    mean_.push_back(t_mean_);
    median_.push_back(t_median_);
    stdev_.push_back(t_stdev_);
    mean_hquart_.push_back(t_mhq_);
    mean_lquart_.push_back(t_mlq_);
  }

  double AvgMean() { return (HFSAT::VectorUtils::GetMean(mean_)); }
  double AvgMedian() { return (HFSAT::VectorUtils::GetMean(median_)); }
  double AvgStdev() { return (HFSAT::VectorUtils::GetMean(stdev_)); }
  double AvgMHQ() { return (HFSAT::VectorUtils::GetMean(mean_hquart_)); }
  double AvgMLQ() { return (HFSAT::VectorUtils::GetMean(mean_lquart_)); }
};

StatSet stats_vec_;

int getMsecsFromMidnight(char *);
void loadStats();
void printMeanStats();

char *shc_;
int sstring_;
int estring_;
char *feature_;
char *duration_;
int edate_;
int lback_;

int main(int argc, char **argv) {
  if (argc != 8) {
    std::cerr << "shc stime etime_ feature_ duration_ edate_ lback_, required\n";
    exit(-1);
  }
  shc_ = argv[1];
  feature_ = argv[4];
  duration_ = argv[5];
  edate_ = atoi(argv[6]);
  lback_ = atoi(argv[7]);
  sstring_ = getMsecsFromMidnight(argv[2]);
  estring_ = getMsecsFromMidnight(argv[3]);

  loadStats();
  printMeanStats();
}

void loadStats() {
  std::string ofname_;
  std::ifstream fstream_;
  int date_ = edate_;
  const int buf_len_ = 1024;
  char readline_buffer_[buf_len_];
  for (int i = 0; i < lback_; i++) {
    ofname_ = std::string(BASEFEATURESDIR) + std::to_string(date_) + "/" + feature_ + duration_ + ".txt";
    fstream_.open(ofname_.c_str(), std::ifstream::in);
    if (fstream_.is_open()) {
      while (fstream_.good()) {
        bzero(readline_buffer_, buf_len_);
        fstream_.getline(readline_buffer_, buf_len_);
        HFSAT::PerishableStringTokenizer st_(readline_buffer_, (unsigned int)BUF_LEN);
        const std::vector<const char *> &tokens_ = st_.GetTokens();
        if (tokens_.size() != 8) {
          continue;
        }
        if (strcmp(tokens_[0], shc_) == 0 && atoi(tokens_[1]) == sstring_ && atoi(tokens_[2]) == estring_) {
          stats_vec_.add(atof(tokens_[3]), atof(tokens_[4]), atof(tokens_[5]), atof(tokens_[6]), atof(tokens_[7]));
        }
      }
      fstream_.close();
    }
    date_ = HFSAT::DateTime::CalcPrevDay(date_);
  }
}

void printMeanStats() {
  std::cout << stats_vec_.AvgMean() << " " << stats_vec_.AvgMedian() << " " << stats_vec_.AvgStdev() << " "
            << stats_vec_.AvgMHQ() << " " << stats_vec_.AvgMLQ() << "\n";
}

int getMsecsFromMidnight(char *hhmmstr_) {
  if (strncmp(hhmmstr_, "PREV_", 5) == 0) {
    hhmmstr_ = hhmmstr_ + 5;
  }

  if ((strncmp(hhmmstr_, "EST_", 4) == 0) || (strncmp(hhmmstr_, "CST_", 4) == 0) ||
      (strncmp(hhmmstr_, "CET_", 4) == 0) || (strncmp(hhmmstr_, "BRT_", 4) == 0) ||
      (strncmp(hhmmstr_, "UTC_", 4) == 0) || (strncmp(hhmmstr_, "KST_", 4) == 0) ||
      (strncmp(hhmmstr_, "HKT_", 4) == 0) || (strncmp(hhmmstr_, "IST_", 4) == 0) ||
      (strncmp(hhmmstr_, "JST_", 4) == 0) || (strncmp(hhmmstr_, "MSK_", 4) == 0) ||
      (strncmp(hhmmstr_, "PAR_", 4) == 0) || (strncmp(hhmmstr_, "AMS_", 4) == 0) ||
      (strncmp(hhmmstr_, "LON_", 4) == 0) || (strncmp(hhmmstr_, "AST_", 4) == 0) ||
      (strncmp(hhmmstr_, "BST_", 4) == 0)) {
    return (HFSAT::GetMsecsFromMidnightFromHHMM(
        HFSAT::DateTime::GetUTCHHMMFromTZHHMM(edate_, atoi(hhmmstr_ + 4), hhmmstr_)));
  } else {
    return (atoi(hhmmstr_));
  }
}
