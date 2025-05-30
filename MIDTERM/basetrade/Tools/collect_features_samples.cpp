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
#include <algorithm>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvctrade/Indicators/indicator_list.hpp"

#define BUF_LEN 4096000

struct StartEndTime {
  std::string start_time_;
  std::string end_time_;
  bool next_day_;
  StartEndTime(const std::string s_stime_, const std::string s_etime_, bool t_next_day_ = false)
      : start_time_(s_stime_), end_time_(s_etime_), next_day_(t_next_day_) {}
};

struct TrdSess {
  std::string shc_;
  std::vector<StartEndTime> periods_;

  TrdSess(const std::string s_shc_) : shc_(s_shc_) {}

  void AddPeriod(const std::string s_stime_, const std::string s_etime_, bool t_next_day_ = false) {
    periods_.push_back(StartEndTime(s_stime_, s_etime_, t_next_day_));
  }
};

struct SampleSet {
  std::string shc_;
  std::string feature_key_;
  std::string indicator_string_;
  std::string shortcodes_string_;
  std::vector<std::string> hhmm_val_vec_;
  bool feature_file_exists_;
  int indc_weight_;
  std::string dgen_args_;

  SampleSet(std::string _shc_, std::string _feature_key_, std::string _indicator_string_)
      : shc_(_shc_),
        feature_key_(_feature_key_),
        indicator_string_(_indicator_string_),
        feature_file_exists_(0),
        indc_weight_(1),
        dgen_args_("1000 0 0 0") {}

  void AddSample(std::vector<char *> hhmm_val_) {
    std::string hhmm_val_str_ = "";
    for (auto i = 0u; i < hhmm_val_.size(); i++) {
      hhmm_val_str_ += std::string(hhmm_val_[i]) + " ";
    }
    hhmm_val_vec_.push_back(hhmm_val_str_);
  }

  std::string GetCombinationString() {
    std::string indc_line_ = "INDICATOR 1.0 " + indicator_string_;
    HFSAT::PerishableStringTokenizer st_((char *)indc_line_.c_str(), indc_line_.size());
    const std::vector<const char *> &tokens_ = st_.GetTokens();
    std::vector<std::string> rw_shortcodes_affecting_this_model_;
    std::vector<std::string> ors_source_needed_vec_;

    (HFSAT::CollectShortCodeFunc(tokens_[2]))(rw_shortcodes_affecting_this_model_, ors_source_needed_vec_, tokens_);
    std::sort(rw_shortcodes_affecting_this_model_.begin(), rw_shortcodes_affecting_this_model_.end());
    std::string shortcodes_string_ = boost::algorithm::join(rw_shortcodes_affecting_this_model_, " ");
    shortcodes_string_ += " " + dgen_args_;
    return shortcodes_string_;
  }

  void clearSamples() { hhmm_val_vec_.clear(); }
};

bool update_existing_ = false;
bool print_log_ = false;
const std::string USER = getenv("HOME");
const std::string BASEFEATURESDIR = USER + "/SampleData";
const std::string SPARE_BASEFEATURESDIR = "/spare/local/SampleFeatures/";

std::string dexec_s_ = USER + "/basetrade_install/bin/datagen";
char *dexec_ = strdup(dexec_s_.c_str());
char dout_[] = "STATS_SAMPLES";

int DEFAULT_DURATION = 300;
std::vector<int> durations_;
TrdSess *shc_ss_ = NULL;
std::map<int, SampleSet *> stats_map_;
std::map<std::string, std::string> replace_string_;
std::map<std::string, std::string> inverse_replace_string_;

std::vector<std::string> features_;
std::vector<std::string> source_features_;
std::vector<std::string> expr_div_features_;
std::map<std::string, std::vector<int>> features2durations_;

std::map<std::string, std::string> raw_indicators2keys_;

std::map<std::string, std::vector<int>> shortcodesstring2indicator_idx_;

void loadConfig();
bool checkFileExists(int date_, SampleSet *t_set_);
void collectStats(int date_);
void RunDatagen(std::map<std::string, int> &, char *, int, char *, char *, char *);
void writeStats(int date_);

std::string config_fname_;

bool is_file_exist(std::string fileName) {
  struct stat buffer_;
  return (stat(fileName.c_str(), &buffer_) == 0);
}

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cerr << "USAGE: <script> <config_file> <date> [<lookback-days>] [update_existing=0/1 (default:0)] "
                 "[PrintLog=0/1 (default:1)]" << std::endl;
    exit(-1);
  }
  config_fname_ = std::string(argv[1]);
  if (!is_file_exist(config_fname_)) {
    config_fname_ = "/home/dvctrader/modelling/samples_features_configs/" + config_fname_ + "_config.txt";
    if (!is_file_exist(config_fname_)) {
      std::cout << "File/Shortcode does not exist: " << argv[1] << std::endl;
      return 0;
    }
  }

  int start_date_ = atoi(argv[2]);
  int lookback_days_ = 1;
  if (argc > 3) {
    lookback_days_ = atoi(argv[3]);
  }
  if (argc > 4) {
    update_existing_ = atoi(argv[4]);
  }
  if (argc > 5) {
    print_log_ = atoi(argv[5]);
  }

  replace_string_["RollingAvgL1Size"] = "L1SizeTrend";
  replace_string_["RollingAvgOrdSize"] = "L1OrderTrend";
  replace_string_["RollingSumTrades"] = "RecentSimpleTradesMeasure";
  replace_string_["RollingSumVolume"] = "RecentSimpleVolumeMeasure";
  replace_string_["RollingAvgTradeSize"] = "MovingAvgTradeSize";
  replace_string_["RollingStdev"] = "SlowStdevCalculator";
  replace_string_["RollingCorrelation"] = "SlowCorrCalculator";
  replace_string_["AvgPrice"] = "SimplePriceType";
  replace_string_["AvgPriceImpliedVol"] = "SimplePriceTypeImpliedVol";
  replace_string_["RollingTrendStdev"] = "SlowStdevTrendCalculator";
  replace_string_["StdevL1Bias"] = "StdevL1Bias";

  for (auto &it_ : replace_string_) {
    inverse_replace_string_[it_.second] = it_.first;
  }

  loadConfig();

  bool all_file_exists_;
  if (strncmp(shc_ss_->shc_.c_str(), "NSE_", 4) == 0) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(start_date_).LoadNSESecurityDefinitions();
  }
  if (strncmp(shc_ss_->shc_.c_str(), "BSE_", 4) == 0) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(start_date_).LoadBSESecurityDefinitions();
  }
  HFSAT::ExchSource_t exch_src_ = HFSAT::SecurityDefinitions::GetContractExchSource(shc_ss_->shc_, start_date_);
  if (exch_src_ == HFSAT::kExchSourceASX) {
    shc_ss_->AddPeriod("0000", "AST_700");
    shc_ss_->AddPeriod("AST_835", "0000", true);
  } else if (exch_src_ == HFSAT::kExchSourceCME) {
    shc_ss_->AddPeriod("0000", "EST_1700");
    shc_ss_->AddPeriod("PREV_EST_1800", "0000", true);
  } else {
    shc_ss_->AddPeriod("0000", "2400");
  }

  int date_ = HFSAT::DateTime::CalcPrevWeekDay(lookback_days_ - 1, start_date_);

  while (date_ <= start_date_) {
    all_file_exists_ = false;
    for (auto &it_ : stats_map_) {
      SampleSet *t_set_ = it_.second;
      t_set_->clearSamples();
      if (!update_existing_) {
        all_file_exists_ &= checkFileExists(date_, t_set_);
      }
    }
    if (!all_file_exists_) {
      collectStats(date_);
      writeStats(date_);
    }
    if (exch_src_ == HFSAT::kExchSourceASX || exch_src_ == HFSAT::kExchSourceCME) {
      date_ = HFSAT::DateTime::CalcNextDay(date_);
    } else {
      date_ = HFSAT::DateTime::CalcNextWeekDay(date_);
    }
  }
}

void loadConfig() {
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
        if (tokens_.size() >= 4 && strcmp(tokens_[0], "SHC") == 0) {
          shc_ss_ = new TrdSess(std::string(tokens_[1]));
        } else if (tokens_.size() > 1 && strcmp(tokens_[0], "DURATIONS") == 0) {
          for (unsigned int i = 1; i < tokens_.size(); i++) {
            durations_.push_back(atoi(tokens_[i]));
          }
        } else if (tokens_.size() >= 2 && strcmp(tokens_[0], "FEATURE") == 0) {
          std::string t_feature_ = (replace_string_.find(tokens_[1]) != replace_string_.end())
                                       ? replace_string_[tokens_[1]]
                                       : std::string(tokens_[1]);
          if (std::find(features_.begin(), features_.end(), t_feature_) == features_.end()) {
            features_.push_back(t_feature_);
            features2durations_[t_feature_] = std::vector<int>();
          }
          for (unsigned int i = 2; i < tokens_.size(); i++) {
            features2durations_[t_feature_].push_back(atoi(tokens_[i]));
          }
        } else if (tokens_.size() >= 3 && strcmp(tokens_[0], "SOURCEFEATURE") == 0) {
          std::string t_feature_ = (replace_string_.find(tokens_[1]) != replace_string_.end())
                                       ? replace_string_[tokens_[1]]
                                       : std::string(tokens_[1]);
          t_feature_ = t_feature_ + " " + std::string(tokens_[2]);
          if (std::find(source_features_.begin(), source_features_.end(), t_feature_) == source_features_.end()) {
            source_features_.push_back(t_feature_);
            features2durations_[t_feature_] = std::vector<int>();
          }
          for (unsigned int i = 3; i < tokens_.size(); i++) {
            features2durations_[t_feature_].push_back(atoi(tokens_[i]));
          }
        } else if (tokens_.size() >= 2 && strcmp(tokens_[0], "RAW_INDICATOR") == 0) {
          std::stringstream raw_indicator_;
          std::stringstream key_;
          raw_indicator_ << tokens_[1];
          key_ << tokens_[1];
          for (unsigned int i = 2; i < tokens_.size(); i++) {
            raw_indicator_ << " " << tokens_[i];
            key_ << "_" << tokens_[i];
          }
          raw_indicators2keys_[key_.str()] = raw_indicator_.str();
        } else if (tokens_.size() >= 2 && strcmp(tokens_[0], "RATIOEXPR_FEATURE") == 0) {
          std::string t_feature_ = (replace_string_.find(tokens_[1]) != replace_string_.end())
                                       ? replace_string_[tokens_[1]]
                                       : std::string(tokens_[1]);
          t_feature_ = t_feature_ + " " + std::string(tokens_[2]);
          if (std::find(expr_div_features_.begin(), expr_div_features_.end(), t_feature_) == expr_div_features_.end()) {
            expr_div_features_.push_back(t_feature_);
            features2durations_[t_feature_] = std::vector<int>();
          }
          for (unsigned int i = 3; i < tokens_.size(); i++) {
            features2durations_[t_feature_].push_back(atoi(tokens_[i]));
          }
        }
      }

      if (shc_ss_ == NULL) {
        std::cerr << " Error: No \"SHC <shortcode> <start_hhmm> <end_hhmm> in "
                     "the file" << std::endl;
        exit(-1);
      }
      if (durations_.empty()) {
        durations_.push_back(DEFAULT_DURATION);
      }
      unsigned int indicator_idx_ = 0;

      std::vector<int> *t_durations_;
      for (const std::string &t_feature_ : features_) {
        t_durations_ =
            (features2durations_[t_feature_].size() == 0) ? &(durations_) : &(features2durations_[t_feature_]);
        for (const int &t_duration_ : *t_durations_) {
          std::stringstream t_indicator_;
          if (t_feature_.compare("SimplePriceType") == 0) {
            t_indicator_ << "SimplePriceType " << shc_ss_->shc_ << " MidPrice";
          } else if (t_feature_.compare("SimplePriceTypeImpliedVol") == 0) {
            t_indicator_ << "SimplePriceType " << shc_ss_->shc_ << " ImpliedVol";
          } else if (t_feature_.compare("SlowStdevTrendCalculator") == 0) {
            t_indicator_ << "SlowStdevTrendCalculator " << shc_ss_->shc_ << " 900 " << t_duration_ << " MidPrice";
          } else if (t_feature_.compare("StdevL1Bias") == 0) {
            t_indicator_ << "StdevL1Bias " << shc_ss_->shc_ << " " << t_duration_ << " MktSizeWPrice";
          } else if (t_feature_.compare("SimpleTrendMax") == 0) {
            t_indicator_ << "SimpleTrend " << shc_ss_->shc_ << " " << t_duration_ << " MktSizeWPrice";
          } else {
            t_indicator_ << t_feature_ << " " << shc_ss_->shc_ << " " << t_duration_ << " MidPrice";
          }

          if (t_feature_.compare("SimpleTrendMax") == 0) {
            std::string key_ = std::string("SimpleTrendMax") + std::to_string(t_duration_);
            stats_map_[indicator_idx_] = new SampleSet(shc_ss_->shc_, key_, t_indicator_.str());
            stats_map_[indicator_idx_]->indc_weight_ = -1;
            stats_map_[indicator_idx_]->dgen_args_ = "1000 C3 0 0";
          } else {
            std::string key_ = ((inverse_replace_string_.find(t_feature_) == inverse_replace_string_.end())
                                    ? t_feature_
                                    : inverse_replace_string_[t_feature_]) +
                               std::to_string(t_duration_);
            stats_map_[indicator_idx_] = new SampleSet(shc_ss_->shc_, key_, t_indicator_.str());
            stats_map_[indicator_idx_]->dgen_args_ = "1000 0 0 0";
          }
          indicator_idx_ += 1;
        }
      }
      for (const std::string &t_feature_ : source_features_) {
        t_durations_ =
            (features2durations_[t_feature_].size() == 0) ? &(durations_) : &(features2durations_[t_feature_]);
        for (const int &t_duration_ : *t_durations_) {
          std::vector<std::string> t_tokens_;
          boost::split(t_tokens_, t_feature_, boost::is_any_of(" "));
          std::string &t_feature_word_ = t_tokens_[0];
          std::string &t_indep_ = t_tokens_[1];

          std::stringstream t_indicator_;
          t_indicator_ << t_feature_word_ << " " << shc_ss_->shc_ << " " << t_indep_ << " " << t_duration_
                       << " MidPrice";
          std::string key_ = ((inverse_replace_string_.find(t_feature_word_) == inverse_replace_string_.end())
                                  ? t_feature_word_
                                  : inverse_replace_string_[t_feature_word_]) +
                             std::to_string(t_duration_) + "_" + t_indep_;
          stats_map_[indicator_idx_++] = new SampleSet(shc_ss_->shc_, key_, t_indicator_.str());
        }
      }
      for (const std::string &t_feature_ : expr_div_features_) {
        t_durations_ =
            (features2durations_[t_feature_].size() == 0) ? &(durations_) : &(features2durations_[t_feature_]);
        for (const int &t_duration_ : *t_durations_) {
          std::vector<std::string> t_tokens_;
          boost::split(t_tokens_, t_feature_, boost::is_any_of(" "));
          std::string &t_feature_word_ = t_tokens_[0];
          std::string &t_indep_ = t_tokens_[1];

          std::stringstream t_indicator_;
          t_indicator_ << "Expression DIV 3 1.0 " << t_feature_word_ << " " << t_indep_ << " " << t_duration_
                       << " MidPrice 3 1.0 " << t_feature_word_ << " " << shc_ss_->shc_ << " " << t_duration_
                       << " MidPrice";
          std::string key_ = ((inverse_replace_string_.find(t_feature_word_) == inverse_replace_string_.end())
                                  ? t_feature_word_
                                  : inverse_replace_string_[t_feature_word_]) +
                             "_Ratio_" + std::to_string(t_duration_) + "_" + t_indep_;
          stats_map_[indicator_idx_++] = new SampleSet(shc_ss_->shc_, key_, t_indicator_.str());
        }
      }
      for (auto &t_indc_ : raw_indicators2keys_) {
        stats_map_[indicator_idx_++] = new SampleSet(shc_ss_->shc_, t_indc_.first, t_indc_.second);
      }
      HFSAT::SetIndicatorListMap();
      for (unsigned int indicator_idx_ = 0; indicator_idx_ < stats_map_.size(); indicator_idx_++) {
        std::string combination_key_ = stats_map_[indicator_idx_]->GetCombinationString();

        if (shortcodesstring2indicator_idx_.find(combination_key_) == shortcodesstring2indicator_idx_.end()) {
          shortcodesstring2indicator_idx_[combination_key_] = std::vector<int>();
        }
        shortcodesstring2indicator_idx_[combination_key_].push_back(indicator_idx_);
      }
      config_fstream_.close();
    }
  }
}

bool checkFileExists(int date_, SampleSet *t_set_) {
  std::string ofdir_ = std::string(BASEFEATURESDIR) + "/" + t_set_->shc_ + "/" + std::to_string(date_);
  std::string ofname_ = ofdir_ + "/" + t_set_->feature_key_ + ".txt";

  struct stat buffer_;
  t_set_->feature_file_exists_ = (stat(ofname_.c_str(), &buffer_) == 0);

  /*
  char dTa[10];
  strftime(dTa, sizeof(dTa), "%Y%m%d", gmtime(&buffer_.st_mtime));

  if (t_set_->feature_file_exists_ && atoi(dTa) <= date_) {
    t_set_->feature_file_exists_ = 0;
  }
  */

  return t_set_->feature_file_exists_;
}

void collectStats(int date_) {
  // std::cout << shc_ss_->shc_ << "\n";
  std::ofstream fhandle_;

  std::string fname_ = SPARE_BASEFEATURESDIR + "/ilist_" + shc_ss_->shc_ + "_" + std::to_string(date_) + ".ilist";
  HFSAT::FileUtils::MkdirEnclosing(fname_);

  for (auto const &ind_group_ : shortcodesstring2indicator_idx_) {
    int this_indicator_idx_ = 0;
    for (auto &indicator_idx_ : ind_group_.second) {
      SampleSet *t_set_ = stats_map_[indicator_idx_];
      if (!t_set_->feature_file_exists_) {
        this_indicator_idx_++;
      }
    }

    if (this_indicator_idx_ == 0) {
      continue;
    }

    fhandle_.open(fname_);
    fhandle_ << "MODELINIT DEPBASE " << shc_ss_->shc_ << " MidPrice MidPrice\n";
    fhandle_ << "MODELMATH LINEAR CHANGE\n";
    fhandle_ << "INDICATORSTART\n";

    std::map<std::string, int> indicator_indx_map_;
    char *dgen_args_ = (char *)"";
    this_indicator_idx_ = 0;
    for (auto &indicator_idx_ : ind_group_.second) {
      SampleSet *t_set_ = stats_map_[indicator_idx_];
      if (!t_set_->feature_file_exists_) {
        fhandle_ << "INDICATOR " << t_set_->indc_weight_ << " " << t_set_->indicator_string_ << "\n";
        if (print_log_) {
          std::cout << "INDICATOR " << t_set_->indc_weight_ << " " << t_set_->indicator_string_ << "\n";
        }
        indicator_indx_map_["INDICATOR_" + std::to_string(this_indicator_idx_)] = indicator_idx_;
        this_indicator_idx_++;
        dgen_args_ = (char *)t_set_->dgen_args_.c_str();
      }
    }
    fhandle_ << "INDICATOREND\n";
    fhandle_.close();

    for (auto &tperiod_ : shc_ss_->periods_) {
      int tdate_ = (tperiod_.next_day_) ? HFSAT::DateTime::CalcNextDay(date_) : date_;
      RunDatagen(indicator_indx_map_, (char *)fname_.c_str(), tdate_, (char *)tperiod_.start_time_.c_str(),
                 (char *)tperiod_.end_time_.c_str(), dgen_args_);
    }
    if (remove(fname_.c_str()) != 0) {
      // std::cerr << "Error removing file: " << fname_ << std::endl;
    }
  }
}

void RunDatagen(std::map<std::string, int> &indicator_indx_map_, char *fname_, int tdate_, char *stime_, char *etime_,
                char *dgen_args_) {
  char id_[] = "112897";
  pid_t pid;
  int mypipe[2];
  char foo[BUF_LEN];
  if (pipe(mypipe)) exit(-1);

  pid = fork();
  if (pid == 0) {
    char date_[10];
    sprintf(date_, "%d", tdate_);

    char *times_ = strtok(dgen_args_, " ");
    char *evts_ = strtok(NULL, " ");
    char *trds_ = strtok(NULL, " ");
    char *ecos_ = strtok(NULL, " ");

    char *args[] = {dexec_, fname_, date_, stime_, etime_, id_, dout_, times_, evts_, trds_, ecos_, (char *)0};

    std::ostringstream ss;
    for (int i = 0; i < 12; i++) {
      ss << args[i] << " ";
    }
    if (print_log_) {
      std::cout << ss.str() << std::endl;
    }

    dup2(mypipe[1], STDOUT_FILENO);
    close(mypipe[0]);
    close(mypipe[1]);

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
    // memset(foo, 0, sizeof(foo));
    int bytes_read_ = read(mypipe[0], foo, sizeof(foo));
    close(mypipe[0]);

    if (bytes_read_ <= 0) {
      return;
    } else {
      foo[bytes_read_] = '\0';
      std::stringstream ss(foo);
      std::vector<char *> tokens_;
      HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(foo, "\n", tokens_);
      for (unsigned int counter_ = 0; counter_ < tokens_.size(); counter_++) {
        std::vector<char *> ln_tokens_;
        HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(tokens_[counter_], " \t", ln_tokens_);

        std::string indicator_idx_ = std::string(ln_tokens_[0]);
        if (indicator_indx_map_.find(indicator_idx_) == indicator_indx_map_.end()) {
          if (print_log_) {
            std::cout << std::to_string(tdate_) << ": Unrecognized indicator_idx_ observed: " << tokens_[counter_]
                      << std::endl;
          }
          continue;
        }
        ln_tokens_.erase(ln_tokens_.begin());
        stats_map_[indicator_indx_map_[indicator_idx_]]->AddSample(ln_tokens_);
      }
    }
  }
}

void writeStats(int date_) {
  std::string ofdir_;
  std::string ofname_;

  for (auto &it_ : stats_map_) {
    SampleSet *t_set_ = it_.second;
    if (!t_set_->feature_file_exists_) {
      if (t_set_->hhmm_val_vec_.size() > 0) {
        ofdir_ = std::string(BASEFEATURESDIR) + "/" + t_set_->shc_ + "/" + std::to_string(date_);
        ofname_ = ofdir_ + "/" + t_set_->feature_key_ + ".txt";
        HFSAT::FileUtils::MkdirEnclosing(ofname_);
        std::ofstream fhandle_;
        if (print_log_) {
          std::cout << ofname_ << "\n";
        }
        fhandle_.open(ofname_);
        for (auto i = 0u; i < t_set_->hhmm_val_vec_.size(); i++) {
          fhandle_ << t_set_->hhmm_val_vec_[i] << "\n";
        }
        fhandle_.close();
      }
    }
  }
}
