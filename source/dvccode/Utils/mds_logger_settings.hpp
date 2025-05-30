/**
  \file mds_logger_settings.hpp

  \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
  Address:
    Suite No 162, Evoma, #14, Bhattarhalli,
    Old Madras Road, Near Garden City College,
    KR Puram, Bangalore 560049, India
    +91 80 4190 3551
*/

#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

namespace HFSAT {

#define TIMEOUT 180  // default TIMEOUT

struct ConfigParams {
  int start_time_;
  int end_time_;
  std::string timezone_;  // use some enum
  int tolerance_;

  ConfigParams() {
    start_time_ = end_time_ = 0;
    timezone_ = "UTC";  // default timezone
    tolerance_ = 180;   // default timeout
  }
};

class MDSLoggerSettings {
  //  std::vector < std::string > shortcode_vec_ ;  //per product basis, not handling now
  std::vector<struct ConfigParams> configuration_params_vec_;
  int tradingdate_;
  int last_midnight_sec_;

  // A user can set this to a required value using combined control messages,
  // defalut : -1 -> uses tolerance from config file
  // if not equal to -1 , return this explicit_tolerance_
  int explicit_tolerance_;

  MDSLoggerSettings(const MDSLoggerSettings&);  // disable copy constructor

 public:
  MDSLoggerSettings()
      : configuration_params_vec_(),
        tradingdate_(HFSAT::DateTime::GetCurrentIsoDateLocal()),
        last_midnight_sec_(HFSAT::DateTime::GetTimeMidnightUTC(tradingdate_)),
        explicit_tolerance_(-1) {}

  void init(std::string filename_, std::string exchange_) {
    std::ifstream logger_config_file_;

    logger_config_file_.open(filename_.c_str(), std::ios::in);

    if (!logger_config_file_.is_open()) {
      std::cerr << "mds_logger_settings : Couldn't open the configuration file : " << filename_
                << ". Continuing with default values." << std::endl;
      // not exiting now, if the file does not exist continue with default values
    }

    char line_buffer_[1024];
    std::string line_read_ = "";

    while (logger_config_file_.good()) {
      memset(line_buffer_, 0, 1024);
      line_read_ = "";

      logger_config_file_.getline(line_buffer_, sizeof(line_buffer_));

      line_read_ = line_buffer_;
      if (line_read_.find("#") != std::string::npos) continue;  // comments

      HFSAT::PerishableStringTokenizer st_(line_buffer_, 1024);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() >= 5) {
        if (!strcmp(exchange_.c_str(), tokens_[0])) {
          struct ConfigParams cfg_params_;

          cfg_params_.start_time_ =
              HFSAT::DateTime::GetUTCMsecsFromMidnightFromTZHHMM(tradingdate_, atoi(tokens_[1]), tokens_[3]);
          cfg_params_.end_time_ =
              HFSAT::DateTime::GetUTCMsecsFromMidnightFromTZHHMM(tradingdate_, atoi(tokens_[2]), tokens_[3]);
          cfg_params_.timezone_ = tokens_[3];
          cfg_params_.tolerance_ = atoi(tokens_[4]);

          if (cfg_params_.start_time_ < 0) {
            if (cfg_params_.end_time_ <= 0) {
              cfg_params_.start_time_ += 86400 * 1000;
              cfg_params_.end_time_ += 86400 * 1000;
            } else {
              struct ConfigParams cfg_params_1;
              cfg_params_1.start_time_ = cfg_params_.start_time_ + 86400 * 1000;
              cfg_params_1.end_time_ = 86400 * 1000;
              cfg_params_1.tolerance_ = cfg_params_.tolerance_;
              cfg_params_1.timezone_ = cfg_params_.timezone_;
              cfg_params_.start_time_ = 0;
              configuration_params_vec_.push_back(cfg_params_1);
            }
          }

          configuration_params_vec_.push_back(cfg_params_);
        }
      }
    }

    logger_config_file_.close();
  }

  int getCurrentTolerance() {
    // this explicit_tolerance_ can be set through user message, if positive use this as tolerance
    if (explicit_tolerance_ > 0) return explicit_tolerance_;

    // skip time calculations if there is no entry for the exchange or the file doesn't exist
    // TODO OPT remove the entries from the vec for which end time has been elapsed
    if (configuration_params_vec_.size() > 0) {
      HFSAT::ttime_t time_ = HFSAT::GetTimeOfDay();

      int msecs_from_midnight_ =
          (((int)time_.tv_sec - (int)last_midnight_sec_) * 1000) + (int)(time_.tv_usec / 1000);  // need to be optimized
      if (msecs_from_midnight_ >= 86400 * 1000) msecs_from_midnight_ -= 86400000;

      // currently for chi where combined writer starts on previous day with NEXTDAY argument,
      // last_midnight_sec_ ( of next day ) is greater than current time ( of prev day ), so add
      // 86400000 to msecs_from_midnight_ to shift to it to next day time value
      if (msecs_from_midnight_ < 0) msecs_from_midnight_ += 86400000;

      for (unsigned int config_counter_ = 0; config_counter_ < configuration_params_vec_.size(); config_counter_++) {
        if (configuration_params_vec_[config_counter_].start_time_ <= msecs_from_midnight_ &&
            configuration_params_vec_[config_counter_].end_time_ >= msecs_from_midnight_)
          return configuration_params_vec_[config_counter_].tolerance_;
      }
    }
    return TIMEOUT;
  }

  void setExplicitTolerance(int tolerance_) { explicit_tolerance_ = tolerance_; }
};
}
