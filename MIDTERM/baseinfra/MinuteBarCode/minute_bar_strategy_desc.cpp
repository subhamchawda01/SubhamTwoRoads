/**
   \file MinuteBar/minute_bar_strategy_desc.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "baseinfra/MinuteBar/minute_bar_strategy_desc.hpp"

namespace HFSAT {

MinuteBarStrategyDesc::MinuteBarStrategyDesc(DebugLogger &dbglogger, const std::string &strategy_desc_filename,
                                             const int tradingdate)
    : strategy_vec_() {
  std::ifstream strategy_desc_file;
  strategy_desc_file.open(strategy_desc_filename.c_str(), std::ifstream::in);

  if (strategy_desc_file.is_open()) {
    const int kStrategyDescFileLineBufferLen = 1024;
    char readline_buffer_[kStrategyDescFileLineBufferLen];
    bzero(readline_buffer_, kStrategyDescFileLineBufferLen);

    while (strategy_desc_file.good()) {
      bzero(readline_buffer_, kStrategyDescFileLineBufferLen);
      strategy_desc_file.getline(readline_buffer_, kStrategyDescFileLineBufferLen);
      dbglogger << readline_buffer_ << '\n';
      dbglogger.CheckToFlushBuffer();  // added logging to see later what was running

      std::string this_strategy_full_line_(readline_buffer_);
      PerishableStringTokenizer st_(readline_buffer_, kStrategyDescFileLineBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();

      // For simple strategies
      // STRATEGYLINE  ExecLogicName  StartTime EndTime RunTimeID  ConfigFileName
      // SHC_TO_TRADE Shc_1 Shc_2 Shc_3 ..
      // SHC_FOR_DATA Shc_1 Shc_2 Shc_3..

      if ((tokens_.size() >= 6) && (strcmp(tokens_[0], "STRATEGYLINE") == 0)) {
        MinuteBarStrategyLine strategy_line;  // default initialisation
        strategy_line.exec_name_ = tokens_[1];
        {
          const char *tz_hhmm_str_ = tokens_[2];
          // CGB and BAX end time changed >= 20130531
          if ((strcmp(tokens_[1], "CGB_0") == 0) && (tradingdate < 20130531) &&
              ((strncmp(tokens_[6], "EST_07", 6) == 0) || (strncmp(tokens_[6], "EST_7", 6) == 0) ||
               (strncmp(tokens_[6], "EST_06", 6) == 0) || (strncmp(tokens_[6], "EST_6", 6) == 0))) {
            tz_hhmm_str_ = "EST_0822";
          }

          GetTimeAndMFMFromString(tradingdate, strategy_line.trading_start_ttime_t_,
                                  strategy_line.trading_start_utc_mfm_, tz_hhmm_str_, false);
          int tradingdate_in_utc_ =
              DateTime::GetUTCYYMMDDFromTZHHMMSS(tradingdate, atoi(tz_hhmm_str_ + 4), tz_hhmm_str_);

          if (tradingdate_in_utc_ != tradingdate ||
              (strncmp(tz_hhmm_str_, "PREV_", 5) ==
               0))  // it's still night in england, hence fast forwared the end msces from midnight by one full day
            strategy_line.trading_end_utc_mfm_ = 86400000;
          else
            strategy_line.trading_end_utc_mfm_ = 0;
        }

        {
          const char *tz_hhmm_str = tokens_[3];
          // CGB and BAX end time changed >= 20130531
          if ((strcmp(tokens_[1], "CGB_0") == 0) && (tradingdate < 20130531) &&
              (strncmp(tokens_[6], "EST_15", 6) == 0)) {
            tz_hhmm_str = "EST_1455";
          }
          GetTimeAndMFMFromString(tradingdate, strategy_line.trading_end_ttime_t_, strategy_line.trading_end_utc_mfm_,
                                  tz_hhmm_str, true);
        }

        // Hack to enable queries starting on prev day (and PREV_ not present in strat file)
        if (strategy_line.trading_start_ttime_t_ > strategy_line.trading_end_ttime_t_) {
          ttime_t dummy_day;
          dummy_day.tv_sec = 86400;
          dummy_day.tv_usec = 0;
          strategy_line.trading_start_ttime_t_ = strategy_line.trading_start_ttime_t_ - dummy_day;
          strategy_line.trading_end_utc_mfm_ += 86400000;
        }

        strategy_line.runtime_id_ = atoi(tokens_[4]);
        strategy_line.config_file_ = tokens_[5];

        // check whether runtime_id_ is unique
        for (auto i = 0u; i < strategy_vec_.size(); i++) {
          if (strategy_vec_[i].runtime_id_ == strategy_line.runtime_id_) {
            ExitVerbose(kStrategyDescRunTimeIdNotUnique, tokens_[4]);
          }
        }

        if (strategy_line.config_file_ != "NULL" && !FileUtils::exists(strategy_line.config_file_)) {
          ExitVerbose(kStrategyDescParamFileMissing, strategy_line.config_file_.c_str());

        } else {
          strategy_vec_.push_back(strategy_line);
        }
      } else if ((tokens_.size() >= 2) && (strcmp(tokens_[0], "SHC_TO_TRADE") == 0)) {
        for (unsigned int i = 1; i < tokens_.size(); i++) {
          std::string shc = tokens_[i];
          VectorUtils::UniqueVectorAdd(strategy_vec_.back().dep_shortcode_list_, shc);
          VectorUtils::UniqueVectorAdd(strategy_vec_.back().indep_shortcode_list_, shc);
        }
      } else if ((tokens_.size() >= 2) && (strcmp(tokens_[0], "SHC_FOR_DATA") == 0)) {
        for (unsigned int i = 1; i < tokens_.size(); i++) {
          std::string shc = tokens_[i];
          VectorUtils::UniqueVectorAdd(strategy_vec_.back().indep_shortcode_list_, shc);
        }
      } else if ((tokens_.size() >= 2) && (strcmp(tokens_[0], "SIGNAL") == 0)) {
        std::vector<std::string> signal_tokens;
        for (unsigned int i = 1; i < tokens_.size(); i++) {
          std::string token = tokens_[i];
          signal_tokens.push_back(token);
        }
        strategy_vec_.back().signal_tokens_list_.push_back(signal_tokens);
      }
    }
  }
  if (strategy_vec_.empty()) {
    ExitVerbose(kStrategyDescNoEntry, "strategy vec empty");
  }
}

std::vector<std::string> MinuteBarStrategyDesc::GetAllDepShortcodes() {
  std::vector<std::string> dep_shc_list;
  for (auto strategy : strategy_vec_) {
    VectorUtils::UniqueVectorAdd(dep_shc_list, strategy.dep_shortcode_list_);
  }
  return dep_shc_list;
}

std::vector<std::string> MinuteBarStrategyDesc::GetAllSourceShortcodes() {
  std::vector<std::string> source_shc_list;
  for (auto strategy : strategy_vec_) {
    VectorUtils::UniqueVectorAdd(source_shc_list, strategy.dep_shortcode_list_);
    VectorUtils::UniqueVectorAdd(source_shc_list, strategy.indep_shortcode_list_);
  }
  return source_shc_list;
}

void MinuteBarStrategyDesc::GetTimeAndMFMFromString(int _tradingdate_, ttime_t &_time_, int &_mfm_,
                                                    const char *_tz_hhmm_str_, bool is_end_) {
  if ((strncmp(_tz_hhmm_str_, "EST_", 4) == 0) || (strncmp(_tz_hhmm_str_, "CST_", 4) == 0) ||
      (strncmp(_tz_hhmm_str_, "CET_", 4) == 0) || (strncmp(_tz_hhmm_str_, "BRT_", 4) == 0) ||
      (strncmp(_tz_hhmm_str_, "UTC_", 4) == 0) || (strncmp(_tz_hhmm_str_, "KST_", 4) == 0) ||
      (strncmp(_tz_hhmm_str_, "HKT_", 4) == 0) || (strncmp(_tz_hhmm_str_, "MSK_", 4) == 0) ||
      (strncmp(_tz_hhmm_str_, "IST_", 4) == 0) || (strncmp(_tz_hhmm_str_, "JST_", 4) == 0) ||
      (strncmp(_tz_hhmm_str_, "BST_", 4) == 0) || (strncmp(_tz_hhmm_str_, "AST_", 4) == 0)) {
    _time_ = ttime_t(
        DateTime::GetTimeFromTZHHMMSS(_tradingdate_, DateTime::GetHHMMSSTime(_tz_hhmm_str_ + 4), _tz_hhmm_str_), 0);
    if (is_end_)
      _mfm_ += GetMsecsFromMidnightFromHHMMSS(
          DateTime::GetUTCHHMMSSFromTZHHMMSS(_tradingdate_, DateTime::GetHHMMSSTime(_tz_hhmm_str_ + 4), _tz_hhmm_str_));
    else
      _mfm_ = GetMsecsFromMidnightFromHHMMSS(
          DateTime::GetUTCHHMMSSFromTZHHMMSS(_tradingdate_, DateTime::GetHHMMSSTime(_tz_hhmm_str_ + 4), _tz_hhmm_str_));
  } else if ((strncmp(_tz_hhmm_str_, "PREV_", 5) == 0) &&
             ((strncmp(_tz_hhmm_str_ + 5, "EST_", 4) == 0) || (strncmp(_tz_hhmm_str_ + 5, "CST_", 4) == 0) ||
              (strncmp(_tz_hhmm_str_ + 5, "CET_", 4) == 0) || (strncmp(_tz_hhmm_str_ + 5, "BRT_", 4) == 0) ||
              (strncmp(_tz_hhmm_str_ + 5, "UTC_", 4) == 0) || (strncmp(_tz_hhmm_str_ + 5, "KST_", 4) == 0) ||
              (strncmp(_tz_hhmm_str_ + 5, "HKT_", 4) == 0) || (strncmp(_tz_hhmm_str_ + 5, "MSK_", 4) == 0) ||
              (strncmp(_tz_hhmm_str_ + 5, "IST_", 4) == 0) || (strncmp(_tz_hhmm_str_ + 5, "JST_", 4) == 0) ||
              (strncmp(_tz_hhmm_str_ + 5, "BST_", 4) == 0) || (strncmp(_tz_hhmm_str_ + 5, "AST_", 4) == 0))) {
    _time_ = ttime_t(DateTime::GetTimeFromTZHHMMSS(DateTime::CalcPrevDay(_tradingdate_),
                                                   DateTime::GetHHMMSSTime(_tz_hhmm_str_ + 9), _tz_hhmm_str_ + 5),
                     0);
    if (is_end_)
      _mfm_ += GetMsecsFromMidnightFromHHMMSS(DateTime::GetUTCHHMMSSFromTZHHMMSS(
          DateTime::CalcPrevDay(_tradingdate_), DateTime::GetHHMMSSTime(_tz_hhmm_str_ + 9), _tz_hhmm_str_ + 5));
    else
      _mfm_ = GetMsecsFromMidnightFromHHMMSS(DateTime::GetUTCHHMMSSFromTZHHMMSS(
          DateTime::CalcPrevDay(_tradingdate_), DateTime::GetHHMMSSTime(_tz_hhmm_str_ + 9), _tz_hhmm_str_ + 5));
  } else {
    _time_ = ttime_t(DateTime::GetTimeFromTZHHMMSS(_tradingdate_, DateTime::GetHHMMSSTime(_tz_hhmm_str_), "UTC_"), 0);
    if (is_end_)
      _mfm_ += GetMsecsFromMidnightFromHHMMSS(DateTime::GetHHMMSSTime(_tz_hhmm_str_));
    else
      _mfm_ = GetMsecsFromMidnightFromHHMMSS(DateTime::GetHHMMSSTime(_tz_hhmm_str_));
  }
}

ttime_t MinuteBarStrategyDesc::GetMinStartTime() {
  ttime_t retval;
  if (!strategy_vec_.empty()) {
    retval = strategy_vec_[0].trading_start_ttime_t_;
  } else {
    ExitVerbose(kStrategyDescNoEntry);
  }

  for (auto i = 0u; i < strategy_vec_.size(); i++) {
    if (strategy_vec_[i].trading_start_ttime_t_ < retval) {
      retval = strategy_vec_[i].trading_start_ttime_t_;
    }
  }

  return retval;
}

ttime_t MinuteBarStrategyDesc::GetMaxEndTime() {
  ttime_t retval_ = strategy_vec_[0].trading_end_ttime_t_;

  for (unsigned int i = 1; i < strategy_vec_.size(); i++) {
    if (strategy_vec_[i].trading_end_ttime_t_ > retval_) {
      retval_ = strategy_vec_[i].trading_end_ttime_t_;
    }
  }
  return retval_;
}
}
