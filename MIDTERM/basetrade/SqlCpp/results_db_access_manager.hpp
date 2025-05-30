/**
   \file SqlCpp/results_db_access_manager.hpp
   Manages access to results mysql database

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include <cstring>
#include <iostream>
#include <set>
#include <stdlib.h>
#include <string>
#include <vector>

// Including mysql connector headers (for operating on results DB)
#include "basetrade/MToolsExe/result_line_set.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <mysql_connection.h>

// TODO:change these to get from the schema in constructor
// Number of fields (columns) in the results table (must be changed when new columns are added to table)
#define RESULT_VECTOR_SIZE 29

// Index of the column which contains stratid
#define STRAT_ID_INDEX 2

#define REPLACE_CFG_WITH_SNAME 1

#define DEFAULT_READDB_CONFIG "/spare/local/files/DBCONFIG_results"
#define DEFAULT_WRITEDB_CONFIG "/spare/local/files/DBCONFIG_results"

#define BACKTEST_READDB_CONFIG "/spare/local/files/DBCONFIG_backtest_results"
#define BACKTEST_WRITEDB_CONFIG "/spare/local/files/DBCONFIG_backtest_results"

namespace HFSAT {
class ResultsAccessManager {
 private:
  sql::Driver *mysql_driver_;
  sql::Connection *connection_;
  sql::PreparedStatement *read_prep_stmnt_;
  sql::PreparedStatement *write_prep_stmnt_;
  sql::PreparedStatement *get_stratid_prep_stmnt_;
  sql::PreparedStatement *del_res_prep_stmnt_;

  sql::PreparedStatement *wf_res_read_prep_stmnt_;
  sql::PreparedStatement *wf_res_write_prep_stmnt_;
  sql::PreparedStatement *get_configid_prep_stmnt_;
  sql::PreparedStatement *wf_res_del_prep_stmnt_;
  sql::PreparedStatement *results_fetch_date_stmnt_;

#ifdef REPLACE_CFG_WITH_SNAME
  sql::PreparedStatement *cfg_to_strat_name_;
  std::map<std::string, std::string> cfg_to_strat_name_map_;
#endif

  std::string dbconfig_filename_;
  bool is_writing_;
  bool backtest_;

  /// could be static but we  are anyways using singleton class, so doesn't matter
  std::vector<std::string> results_table_stats_cols_;

  ResultsAccessManager();
  ResultsAccessManager(const ResultsAccessManager &);

  void InitializeFieldNameVecs();
  void SetWriteDB();
  void SetBacktest(bool flag);
  void Connect();
  void Connect(std::string connect_config);
  void Disconnect();

  void SetStratReadStatement();
  void SetStratReadStatementForPool();

  void SetResultReadStatement();
  void SetResultWriteStatement();

  void SetWfResultReadStatement();
  void SetWfResultWriteStatemnt();

  void SetCfgToStratNameStatement();

 public:
  static ResultsAccessManager &GetUniqueInstance() {
    static ResultsAccessManager unique_instance_;
    return unique_instance_;
  }
  ~ResultsAccessManager() { Disconnect(); }

  // Inserts a row in results table
  bool InsertResults(const std::string &_strat_name_, int _date_, const std::vector<std::string> &_results_vec_,
                     const std::string &_pnl_sample_string_ = "");

  // Returns corresponding results for a given shortcode and date (YYYYMMDD)
  int FetchResults(const std::string &_shortcode_, int _date_, std::vector<std::vector<std::string> > &_results_vec_);
  int FetchResults(FileToResultLineSetMap &_return_map_, const std::string &_shortcode_, int _date_,
                   const std::set<std::string> &_stratbasename_set_ = std::set<std::string>(),
                   int _maxloss_per_uts_ = 0);
  int FetchStrats(std::string &_shortcode_, std::string &_start_time_, std::string &_end_time,
                  std::string &_pool_or_staged_, std::vector<std::string> &_strats_vec_);
  int FetchStratsWithPool(std::string &_shortcode_, std::string &_start_time, std::string &_end_time,
                          std::string &_pool_or_staged_, std::string _pool_tag_,
                          std::vector<std::string> &_strats_vec_);
  int FetchStratNameFromConfigName(const std::string &configname, std::string &stratname);
  int IsValidConfig(const std::string &configname);
  int IsValidStrat(const std::string &stratname);
  bool DeleteResultsForStratAndDate(const int stratid, const int tradingdate);
  bool DeleteResultsForConfigAndDate(const int configid, const int tradingdate);
  int FetchDates(int mode_, int min_date_, int max_date_, std::vector<int> &date_vec_);
};
}
