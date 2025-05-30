/**
  \file SqlCppCode/results_db_access_manager.cpp
  Manages access to results mysql database

  \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
Address:
Suite No 162, Evoma, #14, Bhattarhalli,
Old Madras Road, Near Garden City College,
KR Puram, Bangalore 560049, India
+91 80 490 3551
*/
#include "basetrade/SqlCpp/results_db_access_manager.hpp"

namespace HFSAT {

ResultsAccessManager::ResultsAccessManager()
    : mysql_driver_(nullptr),
      connection_(nullptr),
      read_prep_stmnt_(nullptr),
      write_prep_stmnt_(nullptr),
      get_stratid_prep_stmnt_(nullptr),
      del_res_prep_stmnt_(nullptr),
      wf_res_read_prep_stmnt_(nullptr),
      wf_res_write_prep_stmnt_(nullptr),
      get_configid_prep_stmnt_(nullptr),
      wf_res_del_prep_stmnt_(nullptr),
      results_fetch_date_stmnt_(nullptr),
      dbconfig_filename_(DEFAULT_READDB_CONFIG),
      is_writing_(false),
      backtest_(false),
      results_table_stats_cols_() {
  // Check for env varialbe and connect to appropriate DB

  bool set_backtest_flag = false;
  const char *set_backtest = getenv("USE_BACKTEST");
  if (set_backtest != nullptr) {
    // First look at env variable
    set_backtest_flag = atoi(set_backtest) != 0;
  }
  /*else if (strncmp(getenv("USER"), "dvctrader", strlen("dvctrader")) != 0) {
    // by default use backtest database for local user

    set_backtest_flag = true;
  }*/

  Connect();
  SetBacktest(set_backtest_flag);

  InitializeFieldNameVecs();
}

void ResultsAccessManager::Disconnect() {
  if (read_prep_stmnt_ != nullptr) delete read_prep_stmnt_;
  if (write_prep_stmnt_ != nullptr) delete write_prep_stmnt_;
  if (get_stratid_prep_stmnt_ != nullptr) delete get_stratid_prep_stmnt_;
  if (del_res_prep_stmnt_ != nullptr) delete del_res_prep_stmnt_;
  if (wf_res_read_prep_stmnt_ != nullptr) delete wf_res_read_prep_stmnt_;
  if (wf_res_write_prep_stmnt_ != nullptr) delete wf_res_write_prep_stmnt_;
  if (get_configid_prep_stmnt_ != nullptr) delete get_configid_prep_stmnt_;
  if (wf_res_del_prep_stmnt_ != nullptr) delete wf_res_del_prep_stmnt_;
  if (results_fetch_date_stmnt_ != nullptr) delete results_fetch_date_stmnt_;

  if (connection_ != nullptr) delete connection_;

  // setting all statements to nullptr, to avoid garbage location acesses
  read_prep_stmnt_ = nullptr;
  write_prep_stmnt_ = nullptr;
  get_stratid_prep_stmnt_ = nullptr;
  get_stratid_prep_stmnt_ = nullptr;
  connection_ = nullptr;

  wf_res_read_prep_stmnt_ = nullptr;
  wf_res_write_prep_stmnt_ = nullptr;
  get_configid_prep_stmnt_ = nullptr;
  wf_res_del_prep_stmnt_ = nullptr;
  results_fetch_date_stmnt_ = nullptr;
}

/**
 *
 * @param flag
 */
void ResultsAccessManager::SetBacktest(bool flag) {
  if (flag == backtest_) {
    // If new mode is same then nothing needs to be done
    return;
  } else {
    // set appropriate config-filename
    if (flag) {
      dbconfig_filename_ = std::string(BACKTEST_READDB_CONFIG);
    } else {
      dbconfig_filename_ = std::string(DEFAULT_READDB_CONFIG);
    }
    backtest_ = flag;
    // Reconnect with DB with new parameters
    Connect();
  }
}

void ResultsAccessManager::Connect() { Connect(dbconfig_filename_); }

void ResultsAccessManager::Connect(std::string connect_config) {
  /*  Drop the connection if already connected */
  Disconnect();

  std::map<std::string, std::vector<std::string>> key_valvel_map_;
  HFSAT::PerishableStringTokenizer::ParseConfig(connect_config, key_valvel_map_);
  if (key_valvel_map_.find("HOSTNAME") == key_valvel_map_.end()) {
    std::cerr << "HOSTNAME not specified in " << connect_config << std::endl;
    exit(0);
  }

  if (key_valvel_map_.find("DBNAME") == key_valvel_map_.end()) {
    std::cerr << "DBNAME not specified in " << connect_config << std::endl;
    exit(0);
  }

  std::string dbname_ = key_valvel_map_["DBNAME"][0];
  std::string host_ = key_valvel_map_["HOSTNAME"][0];
  std::string user_ = key_valvel_map_.find("USERNAME") != key_valvel_map_.end() ? key_valvel_map_["USERNAME"][0] : "";
  std::string passwd_ = key_valvel_map_.find("PASSWORD") != key_valvel_map_.end() ? key_valvel_map_["PASSWORD"][0] : "";

  char hostname[64];
  hostname[63] = '\0';
  gethostname(hostname, 63);
  if (strncmp(hostname, "ip-10-0-1", 9) == 0) {
    // using 52.87.81.158 from workers because public ip don't work for mysql from workers
    if (key_valvel_map_.find("EC2HOSTNAME") != key_valvel_map_.end()) {
      host_ = key_valvel_map_["EC2HOSTNAME"][0];
    }
  }

  try {
    /* Create a connection */
    mysql_driver_ = get_driver_instance();
    connection_ = mysql_driver_->connect(host_, user_, passwd_);

    /* Connect to the MySQL results database */
    connection_->setSchema(dbname_);
  } catch (sql::SQLException &e) {
    std::cerr << "Exception Raised while connecting to " << user_ << "@" << host_ << std::endl;
    std::cerr << "# ERR: SQLException in " << __FILE__;
    std::cerr << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cerr << "# ERR: " << e.what();
    std::cerr << " (MySQL error code: " << e.getErrorCode();
    std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    exit(0);
  }
}

void ResultsAccessManager::SetWriteDB() {
  if (!is_writing_) {
    dbconfig_filename_ = DEFAULT_WRITEDB_CONFIG;
    if (backtest_) {
      dbconfig_filename_ = BACKTEST_WRITEDB_CONFIG;
    }
    is_writing_ = true;
    Connect();
  }
}

/**
 * Insert results for a strategy
 * @param _strat_name_
 * @param _date_
 * @param _results_vec_
 * @param _pnl_sample_string_
 * @return
 */
bool ResultsAccessManager::InsertResults(const std::string &_strat_name_, int _date_,
                                         const std::vector<std::string> &_results_vec_,
                                         const std::string &_pnl_sample_string_) {
  SetWriteDB();

  if (_results_vec_.size() != results_table_stats_cols_.size()) {
    // Inappropriate number of arguments (number of columns have been modified recently)
    std::cerr << "Inappropriate number of fields provided for insertion.\n"
              << "STRAT: " << _strat_name_ << " DATE: " << _date_ << " PROVIDED(" << _results_vec_.size()
              << ") != REQUIRED(" << results_table_stats_cols_.size() << ")\n";
    return false;
  }

  // 1. Get strat id from strat_name provided
  // 2. Delete existing results for this strats on this day (as these 2 are primary keys)
  // 3. Insert the new updated result

  try {
    int t_stratid_ = IsValidStrat(_strat_name_);
    if (t_stratid_ > 0) {
      DeleteResultsForStratAndDate(t_stratid_, _date_);
      // Insert new result
      if (write_prep_stmnt_ == nullptr) {
        SetResultWriteStatement();
      }

      // No need to worry about vector size (as we have already checked it before)
      unsigned int field = 1;
      while (field <= results_table_stats_cols_.size()) {
        write_prep_stmnt_->setString(field, _results_vec_[field - 1]);
        field++;
      }
      write_prep_stmnt_->setInt(field++, _date_);
      write_prep_stmnt_->setInt(field++, t_stratid_);
      write_prep_stmnt_->setString(field++, _pnl_sample_string_);
      write_prep_stmnt_->setString(field++, "N");  // these are fresh results, dont regenerate
      return (write_prep_stmnt_->executeUpdate() > 0);
    } else {
      int t_configid = IsValidConfig(_strat_name_);
      if (t_configid > 0) {
        DeleteResultsForConfigAndDate(t_configid, _date_);
        if (wf_res_write_prep_stmnt_ == nullptr) {
          SetWfResultWriteStatemnt();
        }

        // No need to worry about vector size (as we have already checked it before)

        unsigned int field = 1;
        while (field <= results_table_stats_cols_.size()) {
          write_prep_stmnt_->setString(field, _results_vec_[field - 1]);
          field++;
        }
        wf_res_write_prep_stmnt_->setInt(field++, _date_);
        wf_res_write_prep_stmnt_->setInt(field++, t_stratid_);
        wf_res_write_prep_stmnt_->setString(field++, _pnl_sample_string_);
        wf_res_write_prep_stmnt_->setString(field++, "N");  // these are fresh results, dont regenerate
        return (wf_res_write_prep_stmnt_->executeUpdate() > 0);
      } else {
        std::cerr << "No existing strat matches for " << _strat_name_ << ". Define strat first.\n";
        return false;
      }
    }

  } catch (sql::SQLException &e) {
    std::cerr << "# ERR: SQLException in " << __FILE__;
    std::cerr << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cerr << "# ERR: " << e.what();
    std::cerr << " (MySQL error code: " << e.getErrorCode();
    std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    return false;
  }

  return false;
}

/**
 * Prepare statement for reading results for strategies
 */
void ResultsAccessManager::SetResultReadStatement() {
  if (read_prep_stmnt_ == nullptr) {
    std::string read_query_str_ = "SELECT sname";
    for (auto i = 0u; i < results_table_stats_cols_.size(); i++) {
      read_query_str_ += ", " + results_table_stats_cols_[i];
    }
    read_query_str_ +=
        " FROM strats, results WHERE strats.stratid = results.stratid AND shortcode = ? AND date = ? AND type != 'P'";

    read_prep_stmnt_ = connection_->prepareStatement(read_query_str_);
  }
}

/**
 * Prepare statement for reading strategies for pool timings
 */
void ResultsAccessManager::SetStratReadStatement() {
  if (read_prep_stmnt_ == nullptr) {
    std::string read_query_str_ =
        "SELECT cname FROM wf_configs WHERE shortcode = ? AND start_time = ? AND end_time = ? and type = ?";

    read_prep_stmnt_ = connection_->prepareStatement(read_query_str_);
  }
}

/**
 * Prepare statement for reading strategies for pool timings with pool tag
 */
void ResultsAccessManager::SetStratReadStatementForPool() {
  if (read_prep_stmnt_ == nullptr) {
    std::string read_query_str_ =
        "SELECT cname FROM wf_configs WHERE shortcode = ? AND start_time = ? AND end_time = ? and type = ? and pooltag "
        "= ?";

    read_prep_stmnt_ = connection_->prepareStatement(read_query_str_);
  }
}

/**
 * Set Prepared statement for writing results for strats;
 */
void ResultsAccessManager::SetResultWriteStatement() {
  if (write_prep_stmnt_ == nullptr) {
    std::string insert_query = "INSERT INTO results ( ";

    for (unsigned int field = 0; field < results_table_stats_cols_.size(); field++) {
      insert_query += results_table_stats_cols_[field] + ", ";
    }
    insert_query += "date, stratid, pnl_samples, regenerate ) VALUES ( ";
    for (unsigned int field = 1; field <= results_table_stats_cols_.size() + 3; field++) {
      insert_query += "?, ";
    }
    insert_query += "? )";

    write_prep_stmnt_ = connection_->prepareStatement(insert_query);
  }
}

/**
 * Set prepared statement for reading results for wf_configs
 */
void ResultsAccessManager::SetWfResultReadStatement() {
  if (wf_res_read_prep_stmnt_ == nullptr) {
    std::string read_query_str_ = "SELECT cname";
    for (auto i = 0u; i < results_table_stats_cols_.size(); i++) {
      read_query_str_ += ", " + results_table_stats_cols_[i];
    }
    read_query_str_ +=
        " FROM wf_configs, wf_results WHERE wf_configs.configid = wf_results.configid AND shortcode = ? AND date = "
        "? ";

    wf_res_read_prep_stmnt_ = connection_->prepareStatement(read_query_str_);
  }
}

/**
 * Set prepared statement for writing results for wf_configs;
 */
void ResultsAccessManager::SetWfResultWriteStatemnt() {
  if (wf_res_write_prep_stmnt_ == nullptr) {
    std::string insert_query = "INSERT INTO wf_results ( ";
    for (unsigned int field = 0; field < results_table_stats_cols_.size(); field++) {
      insert_query += results_table_stats_cols_[field] + ", ";
    }

    insert_query += "date, configid, pnl_samples, regenerate ) VALUES ( ";
    for (unsigned int field = 1; field <= results_table_stats_cols_.size() + 3; field++) {
      insert_query += "?, ";
    }
    insert_query += "? )";
    wf_res_write_prep_stmnt_ = connection_->prepareStatement(insert_query);
  }
}

void ResultsAccessManager::SetCfgToStratNameStatement() {
  if (cfg_to_strat_name_ == nullptr) {
    std::string strat_from_cfg_query = "SELECT sname from wf_configs where cname = ?";
    cfg_to_strat_name_ = connection_->prepareStatement(strat_from_cfg_query);
  }
}

// fetch the strategies for the pool timings and insert it to _strats_vec_
int ResultsAccessManager::FetchStrats(std::string &_shortcode_, std::string &_start_time, std::string &_end_time,
                                      std::string &_pool_or_staged_, std::vector<std::string> &_strats_vec_) {
  if (_shortcode_.length() <= 0) {
    // Invalid input => return -1,
    // 0 return value is reserved for case when input was correct but no results in DB for those inputs
    return -1;
  }

  _strats_vec_.clear();

  try {
    if (read_prep_stmnt_ == nullptr) {
      SetStratReadStatement();
    }

    read_prep_stmnt_->setString(1, _shortcode_);
    read_prep_stmnt_->setString(2, _start_time);
    read_prep_stmnt_->setString(3, _end_time);
    read_prep_stmnt_->setString(4, _pool_or_staged_);

    sql::ResultSet *result_set_ = read_prep_stmnt_->executeQuery();
    while (result_set_->next()) {
      _strats_vec_.emplace_back(result_set_->getString(1));
    }
    delete result_set_;
    return _strats_vec_.size();
  } catch (sql::SQLException &e) {
    std::cerr << "# ERR: SQLException in " << __FILE__;
    std::cerr << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cerr << "# ERR: " << e.what();
    std::cerr << " (MySQL error code: " << e.getErrorCode();
    std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    return -2;
  }
}

int ResultsAccessManager::FetchStratsWithPool(std::string &_shortcode_, std::string &_start_time,
                                              std::string &_end_time, std::string &_pool_or_staged_,
                                              std::string _pool_tag_, std::vector<std::string> &_strats_vec_) {
  if (_shortcode_.length() <= 0) {
    // Invalid input => return -1,
    // 0 return value is reserved for case when input was correct but no results in DB for those inputs
    return -1;
  }

  _strats_vec_.clear();

  try {
    if (read_prep_stmnt_ == nullptr) {
      SetStratReadStatementForPool();
    }

    read_prep_stmnt_->setString(1, _shortcode_);
    read_prep_stmnt_->setString(2, _start_time);
    read_prep_stmnt_->setString(3, _end_time);
    read_prep_stmnt_->setString(4, _pool_or_staged_);
    read_prep_stmnt_->setString(5, _pool_tag_);

    sql::ResultSet *result_set_ = read_prep_stmnt_->executeQuery();
    while (result_set_->next()) {
      _strats_vec_.emplace_back(result_set_->getString(1));
    }
    delete result_set_;
    return _strats_vec_.size();
  } catch (sql::SQLException &e) {
    std::cerr << "# ERR: SQLException in " << __FILE__;
    std::cerr << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cerr << "# ERR: " << e.what();
    std::cerr << " (MySQL error code: " << e.getErrorCode();
    std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    return -2;
  }
}

/**
 * Fetch results for a shortcode for a given date in results_vec_
 *
 * @param _shortcode_
 * @param _date_
 * @param _results_vec_
 * @return
 */
int ResultsAccessManager::FetchResults(const std::string &_shortcode_, int _date_,
                                       std::vector<std::vector<std::string>> &_results_vec_) {
  if (_shortcode_.length() <= 0) {
    // Invalid input => return -1,
    // 0 return value is reserved for case when input was correct but no results in DB for those inputs
    return -1;
  }

  _results_vec_.clear();

  try {
    if (read_prep_stmnt_ == nullptr) {
      SetResultReadStatement();
    }

    read_prep_stmnt_->setString(1, _shortcode_);
    read_prep_stmnt_->setInt(2, _date_);

    sql::ResultSet *result_set_ = read_prep_stmnt_->executeQuery();
    while (result_set_->next()) {
      if (result_set_->getInt(2) == 0 && result_set_->getInt(3) == 0) {
        continue;
      }

      _results_vec_.emplace_back(std::vector<std::string>());
      std::vector<std::string> &single_row_vec = _results_vec_.back();
      // sname and results_table_stats_cols_
      for (unsigned int field = 1; field <= results_table_stats_cols_.size() + 1; field++) {
        // getstring is a heavy operation, use only when needed
        // breaking on first nullptr occurrence, this will take care of missing trailing pnl stats
        if (result_set_->isNull(field)) {
          break;
        }

        single_row_vec.emplace_back(result_set_->getString(field));
      }
    }
    delete result_set_;
    return _results_vec_.size();
  } catch (sql::SQLException &e) {
    std::cerr << "# ERR: SQLException in " << __FILE__;
    std::cerr << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cerr << "# ERR: " << e.what();
    std::cerr << " (MySQL error code: " << e.getErrorCode();
    std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    return -2;
  }
}

/**
 * Fetch results for a shortcode for a given date and fill the strats/results maps appropriately
 *
 * @param _return_map_
 * @param _shortcode_
 * @param _date_
 * @param _stratbasename_set_
 * @param _maxloss_per_uts_
 * @return
 */
int ResultsAccessManager::FetchResults(FileToResultLineSetMap &_return_map_, const std::string &_shortcode_, int _date_,
                                       const std::set<std::string> &_stratbasename_set_, int _maxloss_per_uts_) {
  if (_shortcode_.length() <= 0) {
    // Invalid input => return -1,
    // 0 return value is reserved for case when input was correct but no results in DB for those inputs
    return -1;
  }

  try {
    bool is_strat = true;
    int t_return_val_ = 0;

    for (auto &strat : _stratbasename_set_) {
      int configid = IsValidConfig(strat);

      if (configid > 1) {
        is_strat = false;
      }
      break;
    }

    sql::ResultSet *result_set_ = nullptr;
    if (is_strat) {
      if (read_prep_stmnt_ == nullptr) {
        SetResultReadStatement();
      }

      read_prep_stmnt_->setString(1, _shortcode_);
      read_prep_stmnt_->setInt(2, _date_);
      result_set_ = read_prep_stmnt_->executeQuery();
    } else {
      t_return_val_ = 0;
      if (wf_res_read_prep_stmnt_ == nullptr) {
        SetWfResultReadStatement();
      }
      wf_res_read_prep_stmnt_->setString(1, _shortcode_);
      wf_res_read_prep_stmnt_->setInt(2, _date_);
      result_set_ = wf_res_read_prep_stmnt_->executeQuery();
    }
    while (result_set_->next()) {
      std::string this_stratbase_ = result_set_->getString(1);
      if (_stratbasename_set_.empty() || _stratbasename_set_.find(this_stratbase_) != _stratbasename_set_.end()) {
        if (result_set_->getInt(2) == 0 && result_set_->getInt(3) == 0) {
          continue;
        }
        std::vector<double> single_row_vec;
        // sname and results_table_stats_cols
        for (unsigned int field = 2; field <= results_table_stats_cols_.size() + 1; field++) {
          // breaking on first nullptr occurrence, this will take care of missing trailing pnl stats
          if (result_set_->isNull(field)) {
            break;
          }
          single_row_vec.emplace_back(result_set_->getDouble(field));
        }

        if (_return_map_.find(this_stratbase_) == _return_map_.end()) {
          _return_map_[this_stratbase_].strategy_filename_base_ = this_stratbase_;
          _return_map_[this_stratbase_].result_line_vec_.clear();
        }
        _return_map_[this_stratbase_].result_line_vec_.emplace_back(ResultLine(_date_, single_row_vec));
        ResultLine &this_res_line_ = _return_map_[this_stratbase_].result_line_vec_.back();

        if (SanityCheckResultLine(this_res_line_, _shortcode_, this_res_line_.unit_trade_size_)) {
          if (_maxloss_per_uts_ > 0) {
            int t_max_loss_ = _maxloss_per_uts_ * this_res_line_.unit_trade_size_;
            if (-1 * t_max_loss_ > this_res_line_.min_pnl_) {
              this_res_line_.pnl_ = -1 * t_max_loss_;
              this_res_line_.min_pnl_ = -1 * t_max_loss_;
            }
          }
          t_return_val_++;
        } else {
          _return_map_[this_stratbase_].result_line_vec_.pop_back();
        }
      }
    }
    delete result_set_;
    return t_return_val_;
  } catch (sql::SQLException &e) {
    std::cerr << "# ERR: SQLException in " << __FILE__;
    std::cerr << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cerr << "# ERR: " << e.what();
    std::cerr << " (MySQL error code: " << e.getErrorCode();
    std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    return -2;
  }
}

/**
 * Utility used while moving to wf_configs, not actively used now
 *
 * @param configname
 * @param stratname
 * @return
 */
int ResultsAccessManager::FetchStratNameFromConfigName(const std::string &configname, std::string &stratname) {
  try {
    if (cfg_to_strat_name_ == nullptr) {
      SetCfgToStratNameStatement();
    }

    cfg_to_strat_name_->setString(1, configname);
    stratname = configname;
    sql::ResultSet *result_set_ = cfg_to_strat_name_->executeQuery();
    while (result_set_->next()) {
      stratname = result_set_->getString(1);
    }
    return 0;
  } catch (sql::SQLException &e) {
    std::cerr << "# ERR: SQLException in " << __FILE__;
    std::cerr << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cerr << "# ERR: " << e.what();
    std::cerr << " (MySQL error code: " << e.getErrorCode();
    std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    return -2;
  }
}

/**
 * C++ variant of checking if the config is valid or not
 * @param configname
 * @return
 */
int ResultsAccessManager::IsValidConfig(const std::string &configname) {
  SetWriteDB();

  try {
    if (get_configid_prep_stmnt_ == nullptr) {
      get_configid_prep_stmnt_ = connection_->prepareStatement("SELECT configid FROM wf_configs WHERE cname = ?");
    }

    get_configid_prep_stmnt_->setString(1, configname);
    sql::ResultSet *t_sid_result_set_ = get_configid_prep_stmnt_->executeQuery();

    if (t_sid_result_set_->next()) {
      return t_sid_result_set_->getInt("configid");
      delete t_sid_result_set_;
    }
  } catch (sql::SQLException &e) {
    std::cerr << "# ERR: SQLException in " << __FILE__;
    std::cerr << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cerr << "# ERR: " << e.what();
    std::cerr << " (MySQL error code: " << e.getErrorCode();
    std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    return -2;
  }
  return -1;
}

/**
 *
 * @param stratname
 * @return
 */
int ResultsAccessManager::IsValidStrat(const std::string &stratname) {
  int t_stratid_ = -1;
  try {
    // Get stratid
    if (get_stratid_prep_stmnt_ == nullptr) {
      get_stratid_prep_stmnt_ = connection_->prepareStatement("SELECT stratid FROM strats WHERE sname = ?");
    }

    get_stratid_prep_stmnt_->setString(1, stratname);
    sql::ResultSet *t_sid_result_set_ = get_stratid_prep_stmnt_->executeQuery();

    if (t_sid_result_set_->next()) {
      t_stratid_ = t_sid_result_set_->getInt("stratid");
      delete t_sid_result_set_;
    }
  } catch (sql::SQLException &e) {
    std::cerr << "# ERR: SQLException in " << __FILE__;
    std::cerr << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cerr << "# ERR: " << e.what();
    std::cerr << " (MySQL error code: " << e.getErrorCode();
    std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    return -2;
  }
  return t_stratid_;
}

/**
 * Delte results for a strategy for given tradingdate
 *
 * @param stratid
 * @param tradingdate
 * @return
 */
bool ResultsAccessManager::DeleteResultsForStratAndDate(const int stratid, const int tradingdate) {
  try {
    // Delete old results
    if (del_res_prep_stmnt_ == nullptr) {
      del_res_prep_stmnt_ = connection_->prepareStatement("DELETE FROM results WHERE stratid = ? AND date = ?");
    }

    del_res_prep_stmnt_->setInt(1, stratid);
    del_res_prep_stmnt_->setInt(2, tradingdate);
    del_res_prep_stmnt_->executeUpdate();
  } catch (sql::SQLException &e) {
    std::cerr << "# ERR: SQLException in " << __FILE__;
    std::cerr << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cerr << "# ERR: " << e.what();
    std::cerr << " (MySQL error code: " << e.getErrorCode();
    std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    return false;
  }
  return true;
}

/**
 * Config variant for deleting results from table
 * @param configid
 * @param tradingdate
 * @return
 */
bool ResultsAccessManager::DeleteResultsForConfigAndDate(const int configid, const int tradingdate) {
  try {
    if (wf_res_del_prep_stmnt_ == nullptr) {
      wf_res_del_prep_stmnt_ = connection_->prepareStatement("DELETE FROM wf_results WHERE configid = ? and date = ? ");
    }
    wf_res_del_prep_stmnt_->setInt(1, configid);
    wf_res_del_prep_stmnt_->setInt(2, tradingdate);
    del_res_prep_stmnt_->executeUpdate();
  } catch (sql::SQLException &e) {
    std::cerr << "# ERR: SQLException in " << __FILE__;
    std::cerr << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cerr << "# ERR: " << e.what();
    std::cerr << " (MySQL error code: " << e.getErrorCode();
    std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    return false;
  }
  return true;
}

void ResultsAccessManager::InitializeFieldNameVecs() {
  // Add an entry here when any extra stat is added to the results
  results_table_stats_cols_.clear();
  results_table_stats_cols_.emplace_back("pnl");
  results_table_stats_cols_.emplace_back("vol");
  results_table_stats_cols_.emplace_back("supp_per");
  results_table_stats_cols_.emplace_back("best_per");
  results_table_stats_cols_.emplace_back("agg_per");
  results_table_stats_cols_.emplace_back("imp_per");
  results_table_stats_cols_.emplace_back("apos");
  results_table_stats_cols_.emplace_back("median_ttc");
  results_table_stats_cols_.emplace_back("avg_ttc");
  results_table_stats_cols_.emplace_back("med_closed_trd_pnl");
  results_table_stats_cols_.emplace_back("avg_closed_trd_pnl");
  results_table_stats_cols_.emplace_back("std_closed_trd_pnl");
  results_table_stats_cols_.emplace_back("sharpe_closed_trade_pnls_");
  results_table_stats_cols_.emplace_back("fracpos_closed_trd_pnl");
  results_table_stats_cols_.emplace_back("min_pnl");
  results_table_stats_cols_.emplace_back("max_pnl");
  results_table_stats_cols_.emplace_back("drawdown");
  results_table_stats_cols_.emplace_back("max_ttc");
  results_table_stats_cols_.emplace_back("msg_count");
  results_table_stats_cols_.emplace_back("vol_norm_avg_ttc");
  results_table_stats_cols_.emplace_back("otl_hits");
  results_table_stats_cols_.emplace_back("abs_open_pos");
  results_table_stats_cols_.emplace_back("uts");
  results_table_stats_cols_.emplace_back("ptrds");
  results_table_stats_cols_.emplace_back("ttrds");
}

int ResultsAccessManager::FetchDates(int mode_, int min_date_, int max_date_, std::vector<int> &date_vec_) {
  try {
    if (results_fetch_date_stmnt_ == nullptr) {
      results_fetch_date_stmnt_ = connection_->prepareStatement(
          "SELECT date from dates WHERE train_test = ? and date >= ? and date <= ? ORDER BY date");
    }
    results_fetch_date_stmnt_->setInt(1, mode_);
    results_fetch_date_stmnt_->setInt(2, min_date_);
    results_fetch_date_stmnt_->setInt(3, max_date_);

    sql::ResultSet *result_set_ = results_fetch_date_stmnt_->executeQuery();
    while (result_set_->next()) {
      date_vec_.emplace_back(result_set_->getInt("date"));
    }
    delete result_set_;
    return date_vec_.size();
  } catch (sql::SQLException &e) {
    std::cerr << "# ERR: SQLException in " << __FILE__;
    std::cerr << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cerr << "# ERR: " << e.what();
    std::cerr << " (MySQL error code: " << e.getErrorCode();
    std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    return -2;
  }
}
}
