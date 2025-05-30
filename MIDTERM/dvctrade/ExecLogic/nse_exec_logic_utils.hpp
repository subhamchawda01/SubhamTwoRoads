/**
   \file ExecLogic/nse_exec_logic_utils.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
 */

#pragma once

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"

#include "dvccode/Utils/exchange_names.hpp"
#include "dvccode/Utils/holiday_manager.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"

#include "sqlite3.h"
#define DEF_MIDTERM_DB "/spare/local/tradeinfo/NSE_Files/midterm_db"

namespace HFSAT {

namespace NSEExecLogicUtils {

inline std::vector<std::string> GetOptionsShortcodesFromLogic(const std::vector<const char*>& tokens_) {
  std::vector<std::string> options_;

  if (tokens_.size() >= 4) {
    char futures_[40];
    strcpy(futures_, tokens_[0]);
    char* basename = strtok(futures_, "_");  // NSE
    if (basename != NULL) {
      basename = strtok(NULL, "_");  // BASENAME
    }

    int length_of_steps = atoi(tokens_[2]);
    unsigned int number_of_contracts = atoi(tokens_[3]);

    // basename, in_the_moneyness, call/put, no_of_consecutive contracts basing on current schema
    std::vector<std::string> t_call_otm_options_ =
      NSESecurityDefinitions::GetOptionShortcodesForUnderlyingInCurrScheme(basename, -1, 1, (number_of_contracts * length_of_steps));
    std::vector<std::string> t_put_otm_options_ =
      NSESecurityDefinitions::GetOptionShortcodesForUnderlyingInCurrScheme(basename, -1, -1, (number_of_contracts * length_of_steps));
    // for (unsigned int i = 0; i < t_put_otm_options_.size(); i++) {
    //  std::cerr << t_put_otm_options_[i] << "\n";
    //}
    std::vector<std::string> t_call_itm_options_ =
      NSESecurityDefinitions::GetOptionShortcodesForUnderlyingInCurrScheme(basename, 1, 1, (number_of_contracts * length_of_steps));
    std::vector<std::string> t_put_itm_options_ =
      NSESecurityDefinitions::GetOptionShortcodesForUnderlyingInCurrScheme(basename, 1, -1, (number_of_contracts * length_of_steps));
    // std::cerr << t_call_otm_options_.size() << " " << t_put_otm_options_.size() << " " << "\n";

    if (strcmp(tokens_[1], "OTMInAOI") == 0) {
      // Adding closest ITM excluding ATM
      /*
        if (t_put_itm_options_[0].compare(t_put_itm_options_[0].size() - 2, 2, "_A") != 0) {
        options_.push_back(t_put_itm_options_[length_of_steps - 1]);
      } else {
        options_.push_back(t_put_itm_options_[length_of_steps]);
      }

      if (t_call_itm_options_[0].compare(t_call_itm_options_[0].size() - 2, 2, "_A") != 0) {
        options_.push_back(t_call_itm_options_[length_of_steps - 1]);
      } else {
        options_.push_back(t_call_itm_options_[length_of_steps]);
      }
      */

      // if ATM doesnt exists then add to the otm vectors
      /*if (t_call_otm_options_[0].compare(t_call_otm_options_[0].size() - 2, 2, "_A") != 0) {
        t_call_otm_options_.insert(t_call_otm_options_.begin(),
                                   std::string("NSE_") + std::string(basename) + std::string("_C0_A"));
      }

      if (t_put_otm_options_[0].compare(t_put_otm_options_[0].size() - 2, 2, "_A") != 0) {
        t_put_otm_options_.insert(t_put_otm_options_.begin(),
                                  std::string("NSE_") + std::string(basename) + std::string("_P0_A"));
				  }*/

      for (unsigned int i = 0; i < (number_of_contracts * length_of_steps); i += length_of_steps) {
        if (i < t_call_otm_options_.size()) {
          options_.push_back(t_call_otm_options_[i]);
        }
        if (i < t_put_otm_options_.size()) {
          options_.push_back(t_put_otm_options_[i]);
        }
      }
      unsigned int t_idx_ = 0;

      if (t_put_itm_options_[t_idx_].compare(t_put_itm_options_[t_idx_].size() - 2, 2, "_A") != 0) {
	options_.push_back(t_put_itm_options_[t_idx_]);

	options_.push_back(t_call_itm_options_[t_idx_]);
	options_.push_back(t_call_itm_options_[(t_idx_ + 1)]);
      } else {
	options_.push_back(t_call_itm_options_[t_idx_]);

	options_.push_back(t_put_itm_options_[t_idx_]);
	options_.push_back(t_put_itm_options_[(t_idx_ + 1)]);
      }

      /*VectorUtils::UniqueVectorAdd(options_, t_call_itm_options_);
      VectorUtils::UniqueVectorAdd(options_, t_call_otm_options_);
      VectorUtils::UniqueVectorAdd(options_, t_put_itm_options_);
      VectorUtils::UniqueVectorAdd(options_, t_put_otm_options_);*/

      // push back calls copyconstructor and hence should be a deep copy at vector and string level
    }
  }
  return options_;
}

inline void SetDontTradeForBan(const Watch& watch_, DebugLogger& _dbglogger_, std::vector<int>& dont_trade_secid_vec_) {
  std::ostringstream t_temp_oss;
  t_temp_oss << "/spare/local/tradeinfo/NSE_Files/SecuritiesUnderBan/fo_secban_" << watch_.YYYYMMDD() << ".csv";
  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();

  std::ifstream fo_banned_securities_stream;
  fo_banned_securities_stream.open(t_temp_oss.str().c_str(), std::ifstream::in);

  char line_buffer[1024];

  while (fo_banned_securities_stream.good()) {
    fo_banned_securities_stream.getline(line_buffer, 1024);
    if (std::string(line_buffer).length() < 1) continue;
    char t_secname_[24];
    sprintf(t_secname_, "NSE_%s_FUT0", line_buffer);
    int t_secid_ = sec_name_indexer_.GetIdFromString(t_secname_);
    if (t_secid_ >= 0) {
      VectorUtils::UniqueVectorAdd(dont_trade_secid_vec_, t_secid_);
      _dbglogger_ << " DontTrade for " << t_secname_ << " set to True \n";
    }
  }
  fo_banned_securities_stream.close();
  _dbglogger_.DumpCurrentBuffer();
}

inline void SetDontTradeForCorAct(const Watch& watch_, DebugLogger& _dbglogger_,
                                  std::vector<int>& dont_trade_secid_vec_) {
  std::ostringstream t_temp_oss;
  t_temp_oss << "/spare/local/tradeinfo/NSE_Files/Corporate_Adjustments.csv";
  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  std::ifstream fo_coract_securities_stream;
  fo_coract_securities_stream.open(t_temp_oss.str().c_str(), std::ifstream::in);

  char readline_buffer_[1024];
  while (fo_coract_securities_stream.good()) {
    memset(readline_buffer_, 0, sizeof(readline_buffer_));
    fo_coract_securities_stream.getline(readline_buffer_, 1024);

    std::vector<char*> tokens_;
    char readline_buffer_copy_[1024];
    memset(readline_buffer_copy_, 0, sizeof(readline_buffer_copy_));
    strcpy(readline_buffer_copy_, readline_buffer_);

    HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(readline_buffer_copy_, ",", tokens_);

    std::string t_date_;
    std::string underlying_;
    // we dont trade on t_date_ - 1 t_date_ t_date_ + 1 business days
    if (tokens_.size() > 4 && tokens_[0][0] != '#') {
      int t_date_ = atoi(strtok(tokens_[2], "."));
      if (t_date_ < 20150101) {
        continue;
      }
      int t_ldate_ = t_date_;
      int t_udate_ = t_date_;
      // std::stringstream ss;
      // ss << token_[2].substr(0,8);
      while (HolidayManagerNoThrow::IsExchangeHoliday(EXCHANGE_KEYS::kExchSourceNSEStr, t_ldate_, true)) {
        t_ldate_ = HFSAT::DateTime::CalcPrevWeekDay(t_ldate_);
      }
      while (HolidayManagerNoThrow::IsExchangeHoliday(EXCHANGE_KEYS::kExchSourceNSEStr, t_udate_, true)) {
        t_udate_ = HFSAT::DateTime::CalcNextWeekDay(t_udate_);
      }
      if (watch_.YYYYMMDD() >= t_ldate_ && watch_.YYYYMMDD() <= t_udate_) {
        char t_secname_[24];
        sprintf(t_secname_, "NSE_%s_FUT0", tokens_[3]);
        int t_secid_ = sec_name_indexer_.GetIdFromString(t_secname_);
        if (t_secid_ >= 0) {
          VectorUtils::UniqueVectorAdd(dont_trade_secid_vec_, t_secid_);
          _dbglogger_ << " DontTrade for " << t_secname_ << " set to True \n";
        }
      }
    }
  }
  fo_coract_securities_stream.close();
  _dbglogger_.DumpCurrentBuffer();
}

inline int NumDaysToExpire(int _yyyymmdd_, std::string _segment_) {
  int t_yyyymmdd_ = _yyyymmdd_ / 10000;
  std::ostringstream t_temp_oss;
  t_temp_oss << "/spare/local/tradeinfo/NSE_Files/expiry_dates_" << t_yyyymmdd_;
  std::ifstream t_expiry_file_ifstream_;
  t_expiry_file_ifstream_.open(t_temp_oss.str().c_str(), std::ifstream::in);
  char readline_buffer_[1024];
  int count_ = 0;
  while (t_expiry_file_ifstream_.good()) {
    memset(readline_buffer_, 0, sizeof(readline_buffer_));
    t_expiry_file_ifstream_.getline(readline_buffer_, 1024);

    std::vector<char*> tokens_;
    char readline_buffer_copy_[1024];
    memset(readline_buffer_copy_, 0, sizeof(readline_buffer_copy_));
    strcpy(readline_buffer_copy_, readline_buffer_);
    HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(readline_buffer_copy_, " ", tokens_);
    if (tokens_.size() < 2) {
      continue;
    }
    if (_segment_.compare(std::string(tokens_[0])) == 0) {
      if (atoi(tokens_[1]) < _yyyymmdd_) {
        continue;
      } else {  // >=
        int t_expiry_date_ = atoi(tokens_[1]);
        // no of business days
        while (t_expiry_date_ >= _yyyymmdd_) {
          while (HolidayManagerNoThrow::IsExchangeHoliday(EXCHANGE_KEYS::kExchSourceNSEStr, t_expiry_date_, true)) {
            t_expiry_date_ = HFSAT::DateTime::CalcPrevWeekDay(t_expiry_date_);
          }
          t_expiry_date_ = HFSAT::DateTime::CalcPrevWeekDay(t_expiry_date_);
          count_++;
        }
        break;
      }
    }
  }
  t_expiry_file_ifstream_.close();
  return count_;
}
// we load earnings dates from database. GetFlat is set if an earnings is close by.
inline void SetDontTradeForEarnings(const Watch& watch_, DebugLogger& _dbglogger_,
                                    std::vector<int>& dont_trade_secid_vec_,
                                    const std::vector<SecurityMarketView*> dep_market_view_vec_) {
  std::vector<int> t_alldates_;
  std::map<int, int> all_dates_indices_;
  // Step 1. Create a map with indexes of all dates;
  sqlite3* dbconn;
  if (sqlite3_open_v2(DEF_MIDTERM_DB, &dbconn, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
    std::cerr << "Could not open DB .. error " << sqlite3_errmsg(dbconn) << '\n';
    _dbglogger_ << "Could not open DB .. error " << sqlite3_errmsg(dbconn) << '\n';
    exit(-1);
  }
  // Prepare statement extracting all dates
  char sql_stat[1024];
  sprintf(sql_stat, "select day from ALLDATES order by day asc");
  sqlite3_stmt* sql_prep_;
  if (sqlite3_prepare_v2(dbconn, sql_stat, -1, &sql_prep_, NULL) != SQLITE_OK) {
    std::cerr << "SQLite Error in preparing statement " << sqlite3_errmsg(dbconn) << '\n';
    _dbglogger_ << "SQLite Error in preparing statement " << sqlite3_errmsg(dbconn) << '\n';
    exit(-1);
  }

  int t_index_ = 0;
  while (sqlite3_step(sql_prep_) == SQLITE_ROW) {
    all_dates_indices_[sqlite3_column_int(sql_prep_, 0)] = t_index_;
    t_alldates_.push_back(sqlite3_column_int(sql_prep_, 0));
    t_index_++;
  }

  int num_total_products_ = dep_market_view_vec_.size();
  for (int t_ctr_ = 0; t_ctr_ < num_total_products_; t_ctr_++) {
    // Step 2. Populate a sorted vector of indices of earnings dates. Current day is compared against this map/index
    // later.
    std::vector<int> earnings_dates_indices_;
    std::string t_str_(dep_market_view_vec_[t_ctr_]->shortcode());
    HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
    int t_secid_ = sec_name_indexer_.GetIdFromString(t_str_);
    sprintf(sql_stat, "select DISTINCT day from EARNINGS_DATES where stock == \"%s\" order by day asc",
            t_str_.substr(4, t_str_.length() - 9).c_str());
    if (sqlite3_prepare_v2(dbconn, sql_stat, -1, &sql_prep_, NULL) != SQLITE_OK) {
      std::cerr << "SQLite Error in preparing statement " << sqlite3_errmsg(dbconn) << '\n';
      _dbglogger_ << "SQLite Error in preparing statement " << sqlite3_errmsg(dbconn) << '\n';
      exit(-1);
    }

    while (sqlite3_step(sql_prep_) == SQLITE_ROW) {
      int t_curr_date_ = sqlite3_column_int(sql_prep_, 0);
      if (all_dates_indices_.find(t_curr_date_) != all_dates_indices_.end()) {
        earnings_dates_indices_.push_back(all_dates_indices_[t_curr_date_]);
      } else  // earnings date is on a weekend/holiday or a day absent in alldates ( eg missing logged data etc )
      {
        std::vector<int>::iterator t_up_ = std::upper_bound(t_alldates_.begin(), t_alldates_.end(), t_curr_date_);
        // Index of earnings day is t_up_ - t_alldates_.begin() ; we check for duplicates since different holidays can
        // have same business day succedding it.
        // edge cases ( earnings prior to first day of record or after last day of record ) are ignored
        if (t_up_ != t_alldates_.begin() && t_up_ != t_alldates_.end() &&
            (earnings_dates_indices_.size() == 0 ||
             earnings_dates_indices_[earnings_dates_indices_.size() - 1] != (t_up_ - t_alldates_.begin()))) {
          earnings_dates_indices_.push_back(t_up_ - t_alldates_.begin());
        }
      }
    }

    int t_num_days_to_ = 100;
    int t_num_days_from_ = 100;
    if (all_dates_indices_.find(watch_.YYYYMMDD()) != all_dates_indices_.end()) {
      int current_day_comparison_index_ = all_dates_indices_[watch_.YYYYMMDD()];
      // find closest indices prior to and post this date
      std::vector<int>::iterator t_ubound_ = std::upper_bound(
          earnings_dates_indices_.begin(), earnings_dates_indices_.end(), current_day_comparison_index_);
      if (t_ubound_ != earnings_dates_indices_.end()) {
        t_num_days_to_ = (*t_ubound_ - current_day_comparison_index_);
      }
      if (t_ubound_ != earnings_dates_indices_.begin() && t_ubound_ != earnings_dates_indices_.end()) {
        t_ubound_--;
        t_num_days_from_ = (current_day_comparison_index_ - *t_ubound_);
      }
      if (t_num_days_to_ <= 1 || t_num_days_from_ <= 1) {
        VectorUtils::UniqueVectorAdd(dont_trade_secid_vec_, t_secid_);
        _dbglogger_ << "Earnings GetFlat set for " << t_str_ << " secid " << t_secid_ << '\n';
      }
    }
  }
  _dbglogger_.DumpCurrentBuffer();
  sqlite3_close(dbconn);
}
}
}
