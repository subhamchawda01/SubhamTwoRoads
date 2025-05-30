/**
   \file InitCommonCode/strategy_desc.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#include "dvctrade/InitCommon/strategy_desc.hpp"
#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/sample_data_util.hpp"

namespace HFSAT {

StrategyDesc::StrategyDesc(DebugLogger &t_dbglogger_, const std::string &_strategy_desc_filename_,
                           const int tradingdate_)
    : strategy_desc_filename_(_strategy_desc_filename_),
      is_structured_trading_strategy_(false),
      is_combined_retail_trading_(false),
      is_portfolio_trading_strategy_(false),
      strategy_vec_(),
      structured_strategy_vec_(),
      portfolio_strategy_vec_(),
      spread_modelfile_vec_(),
      spread_paramfile_vec_(),
      simconfig_filename_("invalid") {
  std::ifstream strategy_desc_file_;
  strategy_desc_file_.open(strategy_desc_filename_.c_str(), std::ifstream::in);
  std::map<std::string, int> shortcode_to_strat_count_map_;

  if (strategy_desc_file_.is_open()) {
    const int kStrategyDescFileLineBufferLen = 1024;
    char readline_buffer_[kStrategyDescFileLineBufferLen];
    bzero(readline_buffer_, kStrategyDescFileLineBufferLen);

    while (strategy_desc_file_.good()) {
      bzero(readline_buffer_, kStrategyDescFileLineBufferLen);
      strategy_desc_file_.getline(readline_buffer_, kStrategyDescFileLineBufferLen);
      t_dbglogger_ << readline_buffer_ << '\n';
      t_dbglogger_.CheckToFlushBuffer();  // added logging to see later what was running

      std::string this_strategy_full_line_(readline_buffer_);
      PerishableStringTokenizer st_(readline_buffer_, kStrategyDescFileLineBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();

      // Different format for structured trading
      // For simple strategies
      // STRATEGYLINE  dep_shortcode_  strategy_name_  modelfilename_  paramfilename_  trading_start_hhmm_
      // trading_end_hhmm_ runtime_id_
      //
      // For structured trading
      // STRUCTURED_TRADING dep_structure_ strategy_name_ common_paramfilename_ trading_strat_hhmm_ trading_end_hhmm_
      // runtime_id_
      // STRATEGYLINE shortcode_ modelfilename_ paramfilename_
      // ...
      // ...
      //

      if (tokens_.size() >= 1 && strcmp(tokens_[0], "COMBINED_RETAIL_TRADING") == 0) {
        is_combined_retail_trading_ = true;
      }

      if (tokens_.size() >= 3 && strcmp(tokens_[0], "STRUCTURED_STRATEGYLINE") == 0) {
        is_structured_trading_strategy_ = true;

        std::string t_structured_strategy_file_ = std::string(tokens_[1]);

        StructuredStrategy t_structured_strategy_;

        LoadStructuredStrategy(t_dbglogger_, t_structured_strategy_file_, t_structured_strategy_, tradingdate_);

        t_structured_strategy_.runtime_id_ = atoi(tokens_[2]);

        // check whether runtime_id_ is unique
        for (auto i = 0u; i < strategy_vec_.size(); i++) {
          if (strategy_vec_[i].runtime_id_ == t_structured_strategy_.runtime_id_) {
            ExitVerbose(kStrategyDescRunTimeIdNotUnique, this_strategy_full_line_.c_str());
          }
        }

        for (auto i = 0u; i < structured_strategy_vec_.size(); i++) {
          if (structured_strategy_vec_[i].runtime_id_ == t_structured_strategy_.runtime_id_) {
            ExitVerbose(kStrategyDescRunTimeIdNotUnique, this_strategy_full_line_.c_str());
          }
        }

        structured_strategy_vec_.push_back(t_structured_strategy_);
      }
      if (tokens_.size() >= 8 && strcmp(tokens_[0], "PORT_STRATEGYLINE") == 0) {
        is_portfolio_trading_strategy_ = true;
        PortfolioStrategy portfolio_strat_;
        portfolio_strat_.strategy_name_ = tokens_[1];
        portfolio_strat_.strategy_type_ = tokens_[2];
        portfolio_strat_.prod_filename_ = tokens_[3];
        portfolio_strat_.global_paramfilename_ = tokens_[4];
        portfolio_strat_.runtime_id_ = atoi(tokens_[7]);

        GetTimeAndMFMFromString(tradingdate_, portfolio_strat_.trading_start_ttime_t_,
                                portfolio_strat_.trading_start_utc_mfm_, tokens_[5], false);
        GetTimeAndMFMFromString(tradingdate_, portfolio_strat_.trading_end_ttime_t_,
                                portfolio_strat_.trading_end_utc_mfm_, tokens_[6], false);

        portfolio_strategy_vec_.push_back(portfolio_strat_);
      }

      if (
          //! is_structured_trading_strategy_  && //allowing structured strat and normal strat in a catted file
          (tokens_.size() >= 8) && (strcmp(tokens_[0], "STRATEGYLINE") == 0)) {
        StrategyLine t_new_strategy_line_ = {};  // default initialisation
        std::string shortcode_ = tokens_[1];
        t_new_strategy_line_.dep_shortcode_ = tokens_[1];
        t_new_strategy_line_.strategy_name_ = tokens_[2];
        t_new_strategy_line_.modelfilename_ = tokens_[3];
        t_new_strategy_line_.paramfilename_ = GetRollParam(std::string(tokens_[4]), tradingdate_);
        // std::cerr << t_new_strategy_line_.paramfilename_ << std::endl;
        {
          const char *tz_hhmm_str_ = tokens_[5];
          // CGB and BAX end time changed >= 20130531
          if ((strcmp(tokens_[1], "CGB_0") == 0) && (tradingdate_ < 20130531) &&
              ((strncmp(tokens_[6], "EST_07", 6) == 0) || (strncmp(tokens_[6], "EST_7", 6) == 0) ||
               (strncmp(tokens_[6], "EST_06", 6) == 0) || (strncmp(tokens_[6], "EST_6", 6) == 0))) {
            tz_hhmm_str_ = "EST_0822";
          }

          GetTimeAndMFMFromString(tradingdate_, t_new_strategy_line_.trading_start_ttime_t_,
                                  t_new_strategy_line_.trading_start_utc_mfm_, tz_hhmm_str_, false);
          int tradingdate_in_utc_ =
              DateTime::GetUTCYYMMDDFromTZHHMMSS(tradingdate_, atoi(tz_hhmm_str_ + 4), tz_hhmm_str_);

          const char *tz_hhmm_str_end_ = tokens_[6];
          // CGB and BAX end time changed >= 20130531
          if ((strcmp(tokens_[1], "CGB_0") == 0) && (tradingdate_ < 20130531) &&
              (strncmp(tokens_[6], "EST_15", 6) == 0)) {
            tz_hhmm_str_end_ = "EST_1455";
          }

          SampleDataUtil::set_global_start_tz(std::string(tz_hhmm_str_));
          SampleDataUtil::set_global_end_tz(std::string(tz_hhmm_str_end_));

          int end_tradingdate_in_utc_ =
              DateTime::GetUTCYYMMDDFromTZHHMMSS(tradingdate_, atoi(tz_hhmm_str_end_ + 4), tz_hhmm_str_end_);

          if ((tradingdate_in_utc_ != tradingdate_ && tradingdate_in_utc_ != end_tradingdate_in_utc_) ||
              (strncmp(tz_hhmm_str_, "PREV_", 5) ==
               0))  // it's still night in england, hence fast forwared the end msces from midnight by one full day
            t_new_strategy_line_.trading_end_utc_mfm_ = 86400000;
          else
            t_new_strategy_line_.trading_end_utc_mfm_ = 0;

          GetTimeAndMFMFromString(tradingdate_, t_new_strategy_line_.trading_end_ttime_t_,
                                  t_new_strategy_line_.trading_end_utc_mfm_, tz_hhmm_str_end_, true);
        }

        // Hack to enable queries starting on prev day (and PREV_ not present in strat file)
        if (t_new_strategy_line_.trading_start_ttime_t_ > t_new_strategy_line_.trading_end_ttime_t_) {
          ttime_t dummy_day;
          dummy_day.tv_sec = 86400;
          dummy_day.tv_usec = 0;
          t_new_strategy_line_.trading_start_ttime_t_ = t_new_strategy_line_.trading_start_ttime_t_ - dummy_day;
          t_new_strategy_line_.trading_end_utc_mfm_ += 86400000;
        }

        t_new_strategy_line_.runtime_id_ = atoi(tokens_[7]);

        unsigned int is_event_based_strategy_ = 0;
        if (t_new_strategy_line_.strategy_name_.substr(0, 5) == "Event" &&
            t_new_strategy_line_.strategy_name_ != "EventBiasAggressiveTrading") {
          is_event_based_strategy_ = 1;
          if (tokens_.size() <= 8) {
            ExitVerbose(kStrategyDescTradedEzoneMissing);
          }
          t_new_strategy_line_.traded_ezone_ = tokens_[8];
        }

        if (tokens_.size() > 8 + is_event_based_strategy_) {  // This is multiplexed to be sim-config-filename in SIM
                                                              // and strategy-name in LIVE.
          simconfig_filename_ = tokens_[8 + is_event_based_strategy_];
        }

        t_new_strategy_line_.strategy_full_line_ = this_strategy_full_line_;

        // check whether runtime_id_ is unique
        for (auto i = 0u; i < strategy_vec_.size(); i++) {
          if (strategy_vec_[i].runtime_id_ == t_new_strategy_line_.runtime_id_) {
            ExitVerbose(kStrategyDescRunTimeIdNotUnique, t_new_strategy_line_.strategy_full_line_.c_str());
          }
        }

        for (auto i = 0u; i < structured_strategy_vec_.size(); i++) {
          if (structured_strategy_vec_[i].runtime_id_ == t_new_strategy_line_.runtime_id_) {
            ExitVerbose(kStrategyDescRunTimeIdNotUnique, t_new_strategy_line_.strategy_full_line_.c_str());
          }
        }

        if (!FileUtils::exists(t_new_strategy_line_.modelfilename_)) {
          ExitVerbose(kStrategyDescModelFileMissing, t_new_strategy_line_.modelfilename_.c_str());
        } else if (!FileUtils::exists(t_new_strategy_line_.paramfilename_)) {
          ExitVerbose(kStrategyDescParamFileMissing, t_new_strategy_line_.paramfilename_.c_str());
        } else {
          strategy_vec_.push_back(t_new_strategy_line_);
          if (shortcode_to_strat_count_map_.find(shortcode_) == shortcode_to_strat_count_map_.end()) {
            shortcode_to_strat_count_map_[shortcode_] = 1;
          } else {
            shortcode_to_strat_count_map_[shortcode_]++;
          }
        }
      }
    }
  }
  if ((strategy_vec_.empty() && structured_strategy_vec_.empty()) && !is_portfolio_trading_strategy_) {
    ExitVerbose(kStrategyDescNoEntry, "strategy vec empty");
  }

  if (is_combined_retail_trading_) {
    std::vector<std::string> existing_shcs_;
    for (auto i = 0u; i < structured_strategy_vec_.size(); i++) {
      if (structured_strategy_vec_[i].shortcodes_.size() < 2) {
        ExitVerbose(kCombinedRetailStrategyNotSpecifiedProperly, "RetailFlyStrategy should have atleast 2 shcs");
      }
      std::string t_shc_1_ = HFSAT::VectorUtils::Join(structured_strategy_vec_[i].shortcodes_, "-");

      if (VectorUtils::LinearSearchValue(existing_shcs_, t_shc_1_)) {
        std::ostringstream temp_oss_;
        temp_oss_ << "Same spread/fly specified twice " << t_shc_1_;
        ExitVerbose(kCombinedRetailStrategyNotSpecifiedProperly, temp_oss_.str().c_str());
      } else {
        existing_shcs_.push_back(t_shc_1_);
      }
    }
    for (auto i = 0u; i < strategy_vec_.size(); i++) {
      if (VectorUtils::LinearSearchValue(existing_shcs_, strategy_vec_[i].dep_shortcode_)) {
        ExitVerbose(kCombinedRetailStrategyNotSpecifiedProperly, "Same outright specified twice");
      } else {
        existing_shcs_.push_back(strategy_vec_[i].dep_shortcode_);
        existing_shcs_.push_back(strategy_vec_[i].dep_shortcode_);
      }
    }
  }

  if (is_structured_trading_strategy_) {
    for (auto i = 0u; i < structured_strategy_vec_.size(); i++) {
      std::vector<std::string> structure_shortcode_vec_;
      HFSAT::CurveUtils::GetStructureShortcodes(structured_strategy_vec_[i].trading_structure_,
                                                structure_shortcode_vec_, structured_strategy_vec_[i].strategy_name_);

      if (structured_strategy_vec_[i].strategy_name_ != "MinRiskPriceBasedAggressiveTrading" &&
          structured_strategy_vec_[i].strategy_name_ != "RetailFlyTrading" &&
          structured_strategy_vec_[i].strategy_name_ != "EquityTrading2" &&
          structured_strategy_vec_[i].strategy_name_ != "BaseOTrading" &&
          structured_strategy_vec_[i].strategy_name_ != "PairsTrading" &&
          structured_strategy_vec_[i].strategy_name_ != "StructuredGeneralTrading") {
        if (structure_shortcode_vec_.size() != structured_strategy_vec_[i].shortcodes_.size()) {
          std::cerr << "Invalid SpreadStrategy: " << structured_strategy_vec_[i].strategy_name_
                    << " structure_shortcode_vec_ & structured_strategy_.shortcodes_ differ in "
                       "length \n";
          exit(1);
        }

        for (unsigned int j = 0; j < structure_shortcode_vec_.size(); j++) {
          if (!HFSAT::VectorUtils::LinearSearchValue(structured_strategy_vec_[i].shortcodes_,
                                                     structure_shortcode_vec_[j])) {
            std::cerr << "Invalid SpreadStrategy: no STRATEGYLINE for shc_ " << structure_shortcode_vec_[i]
                      << std::endl;
            exit(1);
          }
        }
      }
    }
  }
}

void StrategyDesc::LoadStructuredStrategy(DebugLogger &t_dbglogger_, std::string structured_strategy_filename_,
                                          StructuredStrategy &structured_strategy_, const int tradingdate_) {
  std::ifstream structured_strategy_desc_file_;
  structured_strategy_desc_file_.open(structured_strategy_filename_.c_str(), std::ifstream::in);
  if (structured_strategy_desc_file_.is_open()) {
    const int kStrategyDescFileLineBufferLen = 1024;
    char readline_buffer_[kStrategyDescFileLineBufferLen];
    bzero(readline_buffer_, kStrategyDescFileLineBufferLen);

    while (structured_strategy_desc_file_.good()) {
      bzero(readline_buffer_, kStrategyDescFileLineBufferLen);
      structured_strategy_desc_file_.getline(readline_buffer_, kStrategyDescFileLineBufferLen);
      t_dbglogger_ << readline_buffer_ << '\n';
      t_dbglogger_.CheckToFlushBuffer();  // added logging to see later what was running

      std::string this_strategy_full_line_(readline_buffer_);
      PerishableStringTokenizer st_(readline_buffer_, kStrategyDescFileLineBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();

      if (tokens_.size() >= 6 && strcmp(tokens_[0], "STRUCTURED_TRADING") == 0) {
        structured_strategy_.trading_structure_ = tokens_[1];
        structured_strategy_.strategy_string_ = tokens_[2];
        structured_strategy_.strategy_name_ = tokens_[2];
        if (structured_strategy_.strategy_string_.find("-") != std::string::npos) {
          int32_t start_ = structured_strategy_.strategy_string_.find("-");
          int32_t mid_ = structured_strategy_.strategy_string_.find("-", start_ + 1);
          int32_t end_ = structured_strategy_.strategy_string_.length();
          structured_strategy_.sub_strategy_name_ =
              structured_strategy_.strategy_string_.substr(mid_ + 1, end_ - mid_ - 1);
          structured_strategy_.strategy_name_ =
              structured_strategy_.strategy_string_.substr(start_ + 1, mid_ - start_ - 1);
        }

        structured_strategy_.common_paramfilename_ = tokens_[3];
        {
          const char *tz_hhmm_str_ = tokens_[4];
          SampleDataUtil::set_global_start_tz(std::string(tz_hhmm_str_));

          GetTimeAndMFMFromString(tradingdate_, structured_strategy_.trading_start_ttime_t_,
                                  structured_strategy_.trading_start_utc_mfm_, tz_hhmm_str_, false);
          int tradingdate_in_utc_ =
              DateTime::GetUTCYYMMDDFromTZHHMMSS(tradingdate_, atoi(tz_hhmm_str_ + 4), tz_hhmm_str_);

          const char *tz_hhmm_str_end_ = tokens_[5];
          SampleDataUtil::set_global_end_tz(std::string(tz_hhmm_str_end_));
          int end_tradingdate_in_utc_ =
              DateTime::GetUTCYYMMDDFromTZHHMMSS(tradingdate_, atoi(tz_hhmm_str_end_ + 4), tz_hhmm_str_end_);
          if (tradingdate_in_utc_ != tradingdate_ &&
              tradingdate_in_utc_ !=
                  end_tradingdate_in_utc_)  // it's still night in england, hence fast forwared the end msces
                                            // from midnight by one full day
          {
            structured_strategy_.trading_end_utc_mfm_ = 86400000;
          } else {
            structured_strategy_.trading_end_utc_mfm_ = 0;
          }
          GetTimeAndMFMFromString(tradingdate_, structured_strategy_.trading_end_ttime_t_,
                                  structured_strategy_.trading_end_utc_mfm_, tz_hhmm_str_end_, true);
        }
      }

      if (is_structured_trading_strategy_ && tokens_.size() >= 4 && (strcmp(tokens_[0], "STRATEGYLINE") == 0)) {
        std::string t_shc_ = std::string(tokens_[1]);
        if (HFSAT::VectorUtils::LinearSearchValue(structured_strategy_.shortcodes_, std::string(tokens_[1]))) {
          structured_strategy_.shortcode_to_modelfile_vec_[t_shc_].push_back(std::string(tokens_[2]));
          structured_strategy_.shortcode_to_paramfile_vec_[t_shc_].push_back(std::string(tokens_[3]));
          structured_strategy_.shortcode_to_exec_vec_[t_shc_].push_back(NULL);
          structured_strategy_.shortcode_to_base_trader_vec_[t_shc_].push_back(NULL);
          if (tokens_.size() >= 6) {
            // Start End time specified
            {
              const char *tz_hhmm_str_ = tokens_[4];
              ttime_t trading_start_time_ = ttime_t(0, 0);
              int trading_start_mfm_ = 0;
              GetTimeAndMFMFromString(tradingdate_, trading_start_time_, trading_start_mfm_, tz_hhmm_str_, false);
              structured_strategy_.shortcode_to_trading_start_ttime_t_vec_[t_shc_].push_back(trading_start_time_);
              structured_strategy_.shortcode_to_trading_start_mfm_vec_[t_shc_].push_back(trading_start_mfm_);
            }
            {
              const char *tz_hhmm_str_ = tokens_[5];

              ttime_t trading_end_time_ = ttime_t(0, 0);
              int trading_end_mfm_ = 0;
              GetTimeAndMFMFromString(tradingdate_, trading_end_time_, trading_end_mfm_, tz_hhmm_str_, true);
              structured_strategy_.shortcode_to_trading_end_ttime_t_vec_[t_shc_].push_back(trading_end_time_);
              structured_strategy_.shortcode_to_trading_end_mfm_vec_[t_shc_].push_back(trading_end_mfm_);
            }
          }
        } else {
          HFSAT::VectorUtils::UniqueVectorAdd(structured_strategy_.shortcodes_, std::string(tokens_[1]));
          structured_strategy_.p_dep_market_view_vec_.push_back(NULL);
          std::vector<std::string> t_modelfile_vec_;
          t_modelfile_vec_.push_back(std::string(tokens_[2]));
          structured_strategy_.shortcode_to_modelfile_vec_[t_shc_] = t_modelfile_vec_;
          std::vector<std::string> t_paramfile_vec_;
          t_paramfile_vec_.push_back(std::string(tokens_[3]));
          structured_strategy_.shortcode_to_paramfile_vec_[t_shc_] = t_paramfile_vec_;
          std::vector<ExecInterface *> t_exec_vec_;
          t_exec_vec_.push_back(NULL);
          structured_strategy_.shortcode_to_exec_vec_[t_shc_] = t_exec_vec_;
          std::vector<BaseTrader *> t_base_trader_vec_;
          t_base_trader_vec_.push_back(NULL);
          structured_strategy_.shortcode_to_base_trader_vec_[t_shc_] = t_base_trader_vec_;
          if (tokens_.size() >= 6) {
            // Start End time specified
            {
              const char *tz_hhmm_str_ = tokens_[4];
              ttime_t trading_start_time_ = ttime_t(0, 0);
              int trading_start_mfm_ = 0;
              GetTimeAndMFMFromString(tradingdate_, trading_start_time_, trading_start_mfm_, tz_hhmm_str_, false);
              std::vector<ttime_t> t_time_vec_;
              t_time_vec_.push_back(trading_start_time_);
              std::vector<int> t_mfm_vec_;
              t_mfm_vec_.push_back(trading_start_mfm_);
              structured_strategy_.shortcode_to_trading_start_ttime_t_vec_[t_shc_] = t_time_vec_;
              structured_strategy_.shortcode_to_trading_start_mfm_vec_[t_shc_] = t_mfm_vec_;
            }
            {
              const char *tz_hhmm_str_ = tokens_[5];
              ttime_t trading_end_time_ = ttime_t(0, 0);
              int trading_end_mfm_ = 0;
              GetTimeAndMFMFromString(tradingdate_, trading_end_time_, trading_end_mfm_, tz_hhmm_str_, true);
              std::vector<ttime_t> t_time_vec_;
              t_time_vec_.push_back(trading_end_time_);
              std::vector<int> t_mfm_vec_;
              t_mfm_vec_.push_back(trading_end_mfm_);
              structured_strategy_.shortcode_to_trading_end_ttime_t_vec_[t_shc_] = t_time_vec_;
              structured_strategy_.shortcode_to_trading_end_mfm_vec_[t_shc_] = t_mfm_vec_;
            }
          }
        }
      }
    }
  }
}

void StrategyDesc::GetTimeAndMFMFromString(int _tradingdate_, ttime_t &_time_, int &_mfm_, const char *_tz_hhmm_str_,
                                           bool is_end_) {
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

std::string StrategyDesc::GetRollParam(std::string _paramfile_, const int _tradingdate_) {
  std::string paramfile_ = _paramfile_;
  if (paramfile_.substr(0, 5).compare("ROLL:") == 0) {
    std::string paramlist_name_ = paramfile_.substr(5);
    // paramlist_ format : STARTDATE ENDDATE paramfile
    std::ifstream paramlist_;
    paramlist_.open(paramlist_name_.c_str(), std::ifstream::in);
    if (paramlist_.is_open()) {
      const int buffer_len_ = 1024;
      char buffer_[buffer_len_];
      bzero(buffer_, buffer_len_);
      while (paramlist_.good()) {
        bzero(buffer_, buffer_len_);
        paramlist_.getline(buffer_, buffer_len_);
        PerishableStringTokenizer st_(buffer_, buffer_len_);
        const std::vector<const char *> &tokens_ = st_.GetTokens();
        if (tokens_.size() >= 3) {
          if (strncmp(tokens_[0], "#", 1) == 0) {
            continue;
          }
          if (strcmp(tokens_[0], "DEFAULT") == 0 || strcmp(tokens_[1], "DEFAULT") == 0) {
            paramfile_ = tokens_[2];
            continue;
          }
          int start_date_ = DateTime::GetIsoDateFromString(std::string(tokens_[0]));
          int end_date_ = DateTime::GetIsoDateFromString(std::string(tokens_[1]));
          // std::cerr << "sd_ : " << start_date_ << "ed_ : " << end_date_ << "\n";
          if (_tradingdate_ >= start_date_ && _tradingdate_ <= end_date_) {
            paramfile_ = tokens_[2];
            break;
          }
        }
      }
    }
    paramlist_.close();
  }
  return paramfile_;
}

ttime_t StrategyDesc::GetMinStartTime() {
  ttime_t retval;
  if (!structured_strategy_vec_.empty()) {
    retval = structured_strategy_vec_[0].trading_start_ttime_t_;

    for (unsigned int i = 1; i < structured_strategy_vec_.size(); i++) {
      if (structured_strategy_vec_[i].trading_start_ttime_t_ < retval) {
        retval = structured_strategy_vec_[i].trading_start_ttime_t_;
      }
    }
    return retval;
  } else if (!strategy_vec_.empty()) {
    retval = strategy_vec_[0].trading_start_ttime_t_;
  } else if (is_portfolio_trading_strategy_) {
    retval = portfolio_strategy_vec_[0].trading_start_ttime_t_;

    for (unsigned int i = 1; i < portfolio_strategy_vec_.size(); i++) {
      if (portfolio_strategy_vec_[i].trading_start_ttime_t_ < retval) {
        retval = portfolio_strategy_vec_[i].trading_start_ttime_t_;
      }
    }
    return retval;
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

ttime_t StrategyDesc::GetMaxEndTime() {
  if (is_structured_trading_strategy_) {
    ttime_t retval = structured_strategy_vec_[0].trading_end_ttime_t_;
    for (unsigned int i = 1; i < structured_strategy_vec_.size(); i++) {
      if (structured_strategy_vec_[i].trading_end_ttime_t_ > retval) {
        retval = structured_strategy_vec_[i].trading_end_ttime_t_;
      }
    }
    return retval;
  } else if (is_portfolio_trading_strategy_) {
    ttime_t retval = portfolio_strategy_vec_[0].trading_end_ttime_t_;
    for (unsigned int i = 1; i < portfolio_strategy_vec_.size(); i++) {
      if (portfolio_strategy_vec_[i].trading_end_ttime_t_ > retval) {
        retval = portfolio_strategy_vec_[i].trading_end_ttime_t_;
      }
    }
    return retval;
  }

  ttime_t retval_ = strategy_vec_[0].trading_end_ttime_t_;
  for (unsigned int i = 1; i < strategy_vec_.size(); i++) {
    if (strategy_vec_[i].trading_end_ttime_t_ > retval_) {
      retval_ = strategy_vec_[i].trading_end_ttime_t_;
    }
  }
  return retval_;
}
bool StrategyDesc::AllDependantsSame() {
  if (!structured_strategy_vec_.empty()) {
    return false;
  }
  if (strategy_vec_.size() < 2) {
    return true;
  } else {
    for (unsigned int i = 1u; i < strategy_vec_.size(); i++) {
      if (strategy_vec_[i].dep_shortcode_.compare(strategy_vec_[0].dep_shortcode_) != 0) {
        return false;
      }
    }
  }
  return true;
}
}
