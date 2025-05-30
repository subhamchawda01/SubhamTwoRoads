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
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#define BUF_LEN 4096000

const std::string USER = getenv("HOME");
const std::string BASEFEATURESDIR = "/NAS1/SampleData";
const std::string SPARE_BASEFEATURESDIR = "/spare/local/SampleFeatures/";

int DEFAULT_DURATION = 300;
std::string shortcode_;
std::vector<int> durations_;
std::map<std::string, std::string> replace_string_;
std::map<std::string, std::string> inverse_replace_string_;

std::vector<std::string> features_;
std::vector<std::string> source_features_;
std::vector<std::string> expr_div_features_;
std::map<std::string, std::vector<int>> features2durations_;

std::map<std::string, std::string> raw_indicators2keys_;

void loadConfig();
bool checkFileExists(std::string ofname_);

std::string config_fname_;
std::vector<std::string> features_keys_;

bool is_file_exist(std::string fileName)
{
    std::ifstream infile(fileName.c_str());
    return infile.good();
}

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cerr << "USAGE: <script> <config_file/shortcode> <date>" << std::endl;
    exit(-1);
  }
  config_fname_ = std::string(argv[1]);
  if (!is_file_exist(config_fname_)) {
  	config_fname_ = "/home/dvctrader/modelling/samples_features_configs/" + config_fname_ + "_config.txt";
	if (!is_file_exist(config_fname_)) {
	  std::cout<<"File/Shortcode doesnot exists: "<<argv[1]<<std::endl;
	  return 0;
	}
  }
  int date_ = atoi(argv[2]);

  replace_string_["RollingAvgL1Size"] = "L1SizeTrend";
  replace_string_["RollingAvgOrdSize"] = "L1OrderTrend";
  replace_string_["RollingSumTrades"] = "RecentSimpleTradesMeasure";
  replace_string_["RollingSumVolume"] = "RecentSimpleVolumeMeasure";
  replace_string_["RollingAvgTradeSize"] = "MovingAvgTradeSize";
  replace_string_["RollingStdev"] = "SlowStdevCalculator";
  replace_string_["RollingCorrelation"] = "SlowCorrCalculator";
  replace_string_["AvgPrice"] = "SimplePriceType";

  for (auto &it_ : replace_string_) {
    inverse_replace_string_[it_.second] = it_.first;
  }

  loadConfig();
  bool computed = true;

  for (auto indc_key_ : features_keys_) {
    std::string ofdir_ = std::string(BASEFEATURESDIR) + "/" + shortcode_ + "/" + std::to_string(date_);
    std::string ofname_ = ofdir_ + "/" + indc_key_ + ".txt";
    bool file_exists_ = checkFileExists(ofname_);
    computed = computed & file_exists_;
    std::cout << ofname_ << " " << file_exists_ << std::endl;
  }
  if ( !computed ) {
  	std::cout<<"Not Computed"<<std::endl;
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
          shortcode_ = std::string(tokens_[1]);
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

      if (shortcode_.empty()) {
        std::cerr << " Error: No \"SHC <shortcode> <start_hhmm> <end_hhmm> in "
                     "the file" << std::endl;
        exit(-1);
      }
      if (durations_.empty()) {
        durations_.push_back(DEFAULT_DURATION);
      }

      std::vector<int> *t_durations_;
      for (const std::string &t_feature_ : features_) {
        t_durations_ =
            (features2durations_[t_feature_].size() == 0) ? &(durations_) : &(features2durations_[t_feature_]);
        for (const int &t_duration_ : *t_durations_) {
          std::string key_ = ((inverse_replace_string_.find(t_feature_) == inverse_replace_string_.end())
                                  ? t_feature_
                                  : inverse_replace_string_[t_feature_]) +
                             std::to_string(t_duration_);
          features_keys_.push_back(key_);
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

          std::string key_ = ((inverse_replace_string_.find(t_feature_word_) == inverse_replace_string_.end())
                                  ? t_feature_word_
                                  : inverse_replace_string_[t_feature_word_]) +
                             std::to_string(t_duration_) + "_" + t_indep_;
          features_keys_.push_back(key_);
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

          std::string key_ = ((inverse_replace_string_.find(t_feature_word_) == inverse_replace_string_.end())
                                  ? t_feature_word_
                                  : inverse_replace_string_[t_feature_word_]) +
                             "_Ratio_" + std::to_string(t_duration_) + "_" + t_indep_;
          features_keys_.push_back(key_);
        }
      }
      for (auto &t_indc_ : raw_indicators2keys_) {
        features_keys_.push_back(t_indc_.first);
      }
    }
  }
}

bool checkFileExists(std::string ofname_) {
  struct stat buffer_;
  return (stat(ofname_.c_str(), &buffer_) == 0);
}
