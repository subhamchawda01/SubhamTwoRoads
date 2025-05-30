/**
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

// simple price_based
// SimpleTrend // simple_trend // SimpleTrend BR_DOL_0 300 MidPrice
// StableScaledTrend // stable_scaled_trend // StableScaledTrend BR_DOL_0 300
// MidPrice

// simple economics
// RollingAvgL1Size // l1_size_trend // L1SizeTrend BR_DOL_0 300 MidPrice
// RollingAvgOrdSize // l1_order_trend // L1OrderTrend BR_DOL_0 300 MidPrice
// RollingSumTrades // recent_simple_trades_measure // RecentSimpleTradesMeasure
// BR_DOL_0 300 MidPrice
// RollingSumVolume // recent_simple_volume_measure // RecentSimpleVolumeMeasure
// BR_DOL_0 300 MidPrice
// RollingAvgTradeSize // rolling_avg_trade_size // RollingAvgTradeSize BR_DOL_0
// 300 MidPrice

// complex
// TurnOverRate // turn_over_rate // TurnOverRate BR_DOL_0 300 MidPrice

// Reading durations/shcs from /spare/local/features/input.txt
// durations_ = { 1, 10, 30, 60, 90, 120, 300, 600, 900, 1200, 2400, 3600 }
// shc sess stime etime { FGBM_0 3 EST_800 CET_1900 }
// shc sess stime etime { FGBM_0 2 CET_800 EST_800 }
// SimpleTrend_300.txt
// FGBM_0 3 mean medium mean-2*sd mean+2*sd max min

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
  std::string shc_;
  std::string start_time_;
  std::string end_time_;
  std::string mean_;
  std::string median_;
  std::string stdev_;
  std::string mean_hquart_;
  std::string mean_lquart_;
  StatSet(char *foo) {
    HFSAT::PerishableStringTokenizer st_(foo, (unsigned int)BUF_LEN);
    const std::vector<const char *> &tokens_ = st_.GetTokens();
    if (tokens_.size() == 8) {
      shc_ = std::string(tokens_[0]);
      start_time_ = std::string(tokens_[1]);
      end_time_ = std::string(tokens_[2]);
      mean_ = std::string(tokens_[3]);
      median_ = std::string(tokens_[4]);
      stdev_ = std::string(tokens_[5]);
      mean_hquart_ = std::string(tokens_[6]);
      mean_lquart_ = std::string(tokens_[7]);
    }
  }
};

char dexec_[] = "/home/dvctrader/LiveExec/bin/datagen";
char dout_[] = "STATS";
char times_[] = "1000";
char evts_[] = "0";
char trds_[] = "0";
char ecos_[] = "0";

std::vector<int> durations_;
std::map<std::string, TrdSess *> shc_ss_;
std::map<std::string, TrdSess *> ex_ss_;
std::vector<std::string> features_;
std::map<std::string, std::vector<StatSet *>> stats_map_;
std::map<std::string, std::string> replace_string_;
std::map<std::string, std::string> inverse_replace_string_;

void loadConfig();
void collectStats();
void writeStats();

char *date_;
int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "date required\n";
    exit(-1);
  }
  date_ = argv[1];

  replace_string_["RollingAvgL1Size"] = "L1SizeTrend";
  replace_string_["RollingAvgOrdSize"] = "L1OrderTrend";
  replace_string_["RollingSumTrades"] = "RecentSimpleTradesMeasure";
  replace_string_["RollingSumVolume"] = "RecentSimpleVolumeMeasure";
  replace_string_["RollingAvgTradeSize"] = "MovingAvgTradeSize";
  replace_string_["RollingStdev"] = "SlowStdevCalculator";

  for (std::map<std::string, std::string>::iterator it_ = replace_string_.begin(); it_ != replace_string_.end();
       it_++) {
    inverse_replace_string_[it_->second] = it_->first;
  }

  loadConfig();
  // for lack of skill
  collectStats();
  writeStats();
}

void loadConfig() {
  std::string config_fname_ = std::string(BASEFEATURESDIR) + std::string(CONFIGFILE);
  if (HFSAT::FileUtils::exists(config_fname_)) {
    std::ifstream config_fstream_;
    config_fstream_.open(config_fname_.c_str(), std::ifstream::in);
    if (config_fstream_.is_open()) {
      const int buf_len_ = 1024;
      char readline_buffer_[buf_len_];

      while (config_fstream_.good()) {
        bzero(readline_buffer_, buf_len_);
        config_fstream_.getline(readline_buffer_, buf_len_);
        HFSAT::PerishableStringTokenizer st_(readline_buffer_, buf_len_);
        const std::vector<const char *> &tokens_ = st_.GetTokens();
        if (tokens_.size() < 1) continue;
        if (tokens_.size() > 1 && strcmp(tokens_[0], "DURATIONS") == 0) {
          for (unsigned int i = 1; i < tokens_.size(); i++) {
            durations_.push_back(atoi(tokens_[i]));
          }
        } else if (tokens_.size() == 5 && strcmp(tokens_[0], "SHC") == 0) {
          std::string key = std::string(tokens_[2]);
          key.append(tokens_[1]);
          shc_ss_[key] =
              new TrdSess(std::string(tokens_[2]), atoi(tokens_[1]), std::string(tokens_[3]), std::string(tokens_[4]));
        } else if (tokens_.size() == 2 && strcmp(tokens_[0], "FEATURE") == 0) {
          if (replace_string_.find(tokens_[1]) != replace_string_.end()) {
            features_.push_back(replace_string_[tokens_[1]]);
          } else {
            features_.push_back(std::string(tokens_[1]));
          }
        }
      }
    }
  }
}

void collectStats() {
  for (std::map<std::string, TrdSess *>::iterator it_ = shc_ss_.begin(); it_ != shc_ss_.end(); ++it_) {
    std::cout << it_->second->shc_ << "\n";
    std::ofstream fhandle_;
    std::string fname_ = BASEFEATURESDIR + it_->first + ".ilist";
    fhandle_.open(fname_);
    fhandle_ << "MODELINIT DEPBASE " << it_->second->shc_ << " MidPrice MidPrice\n";
    fhandle_ << "MODELMATH LINEAR CHANGE\n";
    fhandle_ << "INDICATORSTART\n";

    for (unsigned int j = 0; j < features_.size(); j++) {
      for (unsigned int k = 0; k < durations_.size(); k++) {
        fhandle_ << "INDICATOR 1 " << features_[j] << " " << it_->second->shc_ << " " << durations_[k] << " MidPrice\n";
      }
    }
    fhandle_ << "INDICATOREND\n";
    fhandle_.close();

    static int ipid_ = 112897;
    ipid_++;
    std::string cpid_ = std::to_string(ipid_);
    char *id_ = new char[cpid_.length()];
    strcpy(id_, cpid_.c_str());

    std::vector<char> ilist_(fname_.begin(), fname_.end());
    ilist_.push_back('\0');
    std::vector<char> stime_(it_->second->start_time_.begin(), it_->second->start_time_.end());
    stime_.push_back('\0');
    std::vector<char> etime_(it_->second->end_time_.begin(), it_->second->end_time_.end());
    etime_.push_back('\0');

    pid_t pid;
    int mypipe[2];
    char foo[BUF_LEN];
    if (pipe(mypipe)) exit(-1);
    // char * output_fname_ = "STATS_" + BASEFEATURESDIR + "/" + date_ + ""
    pid = fork();
    if (pid == 0) {
      dup2(mypipe[1], STDOUT_FILENO);
      close(mypipe[0]);
      close(mypipe[1]);

      char *args[] = {dexec_, &ilist_[0], date_, &stime_[0], &etime_[0], id_,
                      dout_,  times_,     evts_, trds_,      ecos_,      (char *)0};
      if (execv(dexec_, args) < 0) {
        perror("execv");
      }
    } else if (pid < 0) {
      std::cerr << "error in fork\n";
      exit(-1);
    } else {
      int returnStatus;
      waitpid(pid, &returnStatus, 0);
      if (returnStatus == 1) {
        std::cerr << "child process failed\n";
      }
      close(mypipe[1]);
      read(mypipe[0], foo, sizeof(foo));
      close(mypipe[0]);
      std::stringstream ss(foo);
      std::string to;
      unsigned int counter_ = 0;
      int q = 0;
      int r = 0;
      std::vector<char *> tokens_;
      HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(foo, "\n", tokens_);
      unsigned int limit = durations_.size() * features_.size();
      while (counter_ < tokens_.size() && counter_ < limit) {
        q = counter_ / durations_.size();
        r = counter_ % durations_.size();
        std::string key_ = ((inverse_replace_string_.find(features_[q]) == inverse_replace_string_.end())
                                ? features_[q]
                                : inverse_replace_string_[features_[q]]) +
                           std::to_string(durations_[r]);
        stats_map_[key_].push_back(new StatSet(tokens_[counter_]));
        counter_++;
      }
    }
  }
}

void writeStats() {
  std::string ofdir_ = std::string(BASEFEATURESDIR) + date_ + "/";
  std::string ofname_;
  for (std::map<std::string, std::vector<StatSet *>>::iterator it_ = stats_map_.begin(); it_ != stats_map_.end();
       it_++) {
    ofname_ = ofdir_ + it_->first + ".txt";
    HFSAT::FileUtils::MkdirEnclosing(ofname_);
    std::ofstream fhandle_;
    std::cout << ofname_ << "\n";
    fhandle_.open(ofname_);
    for (auto i = 0u; i < it_->second.size(); i++) {
      fhandle_ << it_->second[i]->shc_ << " " << it_->second[i]->start_time_ << " " << it_->second[i]->end_time_ << " "
               << it_->second[i]->mean_ << " " << it_->second[i]->median_ << " " << it_->second[i]->stdev_ << " "
               << it_->second[i]->mean_hquart_ << " " << it_->second[i]->mean_lquart_ << "\n";
    }
    fhandle_.close();
  }
}
