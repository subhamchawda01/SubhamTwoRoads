/**
   \file MinuteBar/minute_bar_smv_source.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   India
   +91 80 4190 3551
 */
#pragma once

#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/signals.hpp"
#include <inttypes.h>

#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/ttime.hpp"

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"

#include "dvccode/Utils/bulk_file_reader.hpp"

#include "dvccode/ExternalData/historical_dispatcher.hpp"
#include "baseinfra/LoggedSources/minute_bar_logged_message_filesource.hpp"
#include "baseinfra/MinuteBar/minute_bar_data_market_view_manager.hpp"
#include "baseinfra/MinuteBar/shortcode_minute_bar_smv_map.hpp"

class MinuteBarSMVSource {
  std::vector<std::string> source_shortcode_vec_;
  std::vector<std::string> dep_shortcode_vec_;
  std::string dep_shortcode_;
  int tradingdate_;
  HFSAT::DebugLogger dbglogger_;
  HFSAT::Watch watch_;
  //  HFSAT::StrategyDesc *strategy_desc_;
  std::vector<const char *> exchange_symbol_vec_;
  HFSAT::SecIDMinuteBarSMVMap &sid_to_smv_ptr_map_;
  HFSAT::TradingLocation_t dep_trading_location_;
  std::string log_file_name_;
  std::vector<HFSAT::ExchSource_t> sid_to_exch_source_map_;

  int datagen_start_utc_hhmm_;
  int datagen_end_utc_hhmm_;
  int global_datagen_start_utc_yymmdd_;
  int global_datagen_end_utc_yymmdd_;

  unsigned int num_sec_id_;

  HFSAT::HistoricalDispatcher historical_dispatcher_;

 public:
  MinuteBarSMVSource(std::vector<std::string> source_shortcode_vec, int tradingdate_, bool live_trading_ = false)
      : source_shortcode_vec_(source_shortcode_vec),
        dep_shortcode_vec_(),
        dep_shortcode_("NULL"),
        tradingdate_(tradingdate_),
        dbglogger_(1024000, 1),
        watch_(dbglogger_, tradingdate_),
        //        strategy_desc_(),
        sid_to_smv_ptr_map_(HFSAT::SecIDMinuteBarSMVMap::GetUniqueInstance()),
        dep_trading_location_(HFSAT::kTLocCHI),
        log_file_name_("/spare/local/logs/alllogs/temp_log.log"),
        sid_to_exch_source_map_(),
        datagen_start_utc_hhmm_(-1),
        datagen_end_utc_hhmm_(-1),
        global_datagen_start_utc_yymmdd_(-1),
        global_datagen_end_utc_yymmdd_(-1),
        num_sec_id_(1) {}

  void SetDbgloggerFileName(std::string filename_) { log_file_name_ = filename_; }

  void SetSourceShortcodes(std::vector<std::string> &source_shortcode_vec) {
    source_shortcode_vec_ = source_shortcode_vec;
  }

  void SetDepShortcodeVector(std::vector<std::string> &dep_shortcode_vec) { dep_shortcode_vec_ = dep_shortcode_vec; }

  void SetDepShortcode(std::string dep_shortcode) { dep_shortcode_ = dep_shortcode; }

  void SetStartEndTime(int datagen_start_utc_hhmm, int datagen_end_utc_hhmm) {
    datagen_start_utc_hhmm_ = datagen_start_utc_hhmm;
    datagen_end_utc_hhmm_ = datagen_end_utc_hhmm;
  }

  void SetStartEndUTCDate(int datagen_start_utc_yymmdd, int datagen_end_utc_yymmdd) {
    global_datagen_start_utc_yymmdd_ = datagen_start_utc_yymmdd;
    global_datagen_end_utc_yymmdd_ = datagen_end_utc_yymmdd;
  }

  //  void SetStrategyDesc(HFSAT::StrategyDesc *strategy_desc) { strategy_desc_ = strategy_desc; }

  // Checks if any of the instruments are NSE symbols and if so adds NSE contract specs
  void CheckAndAddNSEDefinitions(std::vector<std::string> &t_shortcode_vec_) {
    bool is_nse_present_ = false;
    for (auto i = 0u; i < t_shortcode_vec_.size(); i++) {
      if (strncmp(t_shortcode_vec_[i].c_str(), "NSE_", 4) == 0) {
        is_nse_present_ = true;
      }
    }
    if (is_nse_present_) {
      HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadNSESecurityDefinitions();
    }
  }
  // Checks if any of the instruments are BSE symbols and if so adds BSE contract specs
  void CheckAndAddBSEDefinitions(std::vector<std::string> &t_shortcode_vec_) {
    bool is_bse_present_ = false;
    for (auto i = 0u; i < t_shortcode_vec_.size(); i++) {
      if (strncmp(t_shortcode_vec_[i].c_str(), "BSE_", 4) == 0) {
        is_bse_present_ = true;
      }
    }
    if (is_bse_present_) {
      HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadBSESecurityDefinitions();
    }
  }

  // returns true if given shortcode corresponds to an HK equity
  bool IsHKEquity(std::string _shortcode) { return _shortcode.substr(0, 3) == "HK_"; }

  // Checks if any of the instruments are HK stocks and if so adds HK stocks contract specs
  void CheckAndAddHKStocksDefinitions(std::vector<std::string> &shortcode_vec) {
    bool is_hk_equities_present = false;
    for (auto i = 0u; i < shortcode_vec.size(); i++) {
      if (IsHKEquity(shortcode_vec[i])) {
        is_hk_equities_present = true;
      }
    }
    if (is_hk_equities_present) {
      HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadHKStocksSecurityDefinitions();
    }
  }

  void Initialize() {
    // setup DebugLogger
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << log_file_name_;
    std::string logfilename_ = t_temp_oss_.str();
    dbglogger_.OpenLogFile(logfilename_.c_str(), std::ofstream::out);
    dbglogger_.AddLogLevel(BOOK_ERROR);
    dbglogger_.AddLogLevel(BOOK_INFO);

    HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);
    HFSAT::SecurityNameIndexer &sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();

    CheckAndAddNSEDefinitions(source_shortcode_vec_);
    CheckAndAddBSEDefinitions(source_shortcode_vec_);
    CheckAndAddHKStocksDefinitions(source_shortcode_vec_);

    if (dep_shortcode_ == "NULL") {
      dep_shortcode_ = source_shortcode_vec_[0];
    }

    dep_trading_location_ = HFSAT::kTLocCHI;
    if (dep_shortcode_.compare("NONAME") != 0) {
      dep_trading_location_ = HFSAT::TradingLocationUtils::GetTradingLocationExch(
          HFSAT::SecurityDefinitions::GetContractExchSource(dep_shortcode_, tradingdate_));
    }

    // Get exchange symbols corresponding to the shortcodes of interest
    // Add exchange symbols to SecurityNameIndexer

    for (auto i = 0u; i < source_shortcode_vec_.size(); i++) {
      if (!sec_name_indexer_.HasString(source_shortcode_vec_[i])) {
        const char *exchange_symbol = HFSAT::ExchangeSymbolManager::GetExchSymbol(source_shortcode_vec_[i]);
        sec_name_indexer_.AddString(exchange_symbol, source_shortcode_vec_[i]);
        exchange_symbol_vec_.push_back(exchange_symbol);
      }
    }

    std::vector<bool> sid_is_dependant_map_;
    // Set sid_to_exch_source maps
    for (auto i = 0u; i < sec_name_indexer_.NumSecurityId(); i++) {
      std::string shortcode = sec_name_indexer_.GetShortcodeFromId(i);

      sid_is_dependant_map_.push_back(
          HFSAT::VectorUtils::LinearSearchValue(dep_shortcode_vec_, sec_name_indexer_.GetShortcodeFromId(i)));

      HFSAT::ExchSource_t exch_source = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode, tradingdate_);
      sid_to_exch_source_map_.push_back(exch_source);
    }

    HFSAT::SecIDMinuteBarSMVMap &sid_to_smv_ptr_map = HFSAT::SecIDMinuteBarSMVMap::GetUniqueInstance();
    HFSAT::ShortcodeMinuteBarSMVMap &shortcode_smv_map_ = HFSAT::ShortcodeMinuteBarSMVMap::GetUniqueInstance();

    // Making all the smvs and adding to sid_to_smv_map
    for (unsigned int security_id = 0; security_id < sec_name_indexer_.NumSecurityId(); security_id++) {
      std::string shortcode = sec_name_indexer_.GetShortcodeFromId(security_id);
      const char *exchange_symbol = sec_name_indexer_.GetSecurityNameFromId(security_id);

      HFSAT::MinuteBarSecurityMarketView *smv =
          new HFSAT::MinuteBarSecurityMarketView(dbglogger_, watch_, sec_name_indexer_, shortcode, exchange_symbol,
                                                 security_id, sid_to_exch_source_map_[security_id]);
      sid_to_smv_ptr_map.AddEntry(security_id, smv);  // add to security_id_ to SMV* map
      shortcode_smv_map_.AddEntry(shortcode, smv);    // add to shortcode_ to SMV* map
    }

    num_sec_id_ = sec_name_indexer_.NumSecurityId();

    InitializeMinuteBarInterafce(sid_to_smv_ptr_map, sec_name_indexer_);

  }  // Init ends

  void InitializeMinuteBarInterafce(HFSAT::SecIDMinuteBarSMVMap &sid_to_smv_ptr_map,
                                    HFSAT::SecurityNameIndexer &sec_name_indexer) {
    HFSAT::MinuteBarDataMarketViewManager *minute_bar_market_view_manager =
        new HFSAT::MinuteBarDataMarketViewManager(dbglogger_, watch_, sec_name_indexer);

    std::map<std::string, HFSAT::MinuteBarLoggedMessageFileSource *> shortcode_minute_bar_logged_message_filesource_map;

    for (unsigned int secid = 0; secid < sec_name_indexer.NumSecurityId(); secid++) {
      const char *exchange_symbol_ = sec_name_indexer.GetSecurityNameFromId(secid);
      std::string shortcode_ = sec_name_indexer.GetShortcodeFromId(secid);

      if (shortcode_minute_bar_logged_message_filesource_map.find(shortcode_) ==
          shortcode_minute_bar_logged_message_filesource_map.end()) {
        shortcode_minute_bar_logged_message_filesource_map[shortcode_] = new HFSAT::MinuteBarLoggedMessageFileSource(
            dbglogger_, sec_name_indexer, tradingdate_, secid, exchange_symbol_, dep_trading_location_);
        shortcode_minute_bar_logged_message_filesource_map[shortcode_]->SetMinuteBarDataListener(
            minute_bar_market_view_manager);
        shortcode_minute_bar_logged_message_filesource_map[shortcode_]->SetExternalTimeListener(&watch_);

        historical_dispatcher_.AddExternalDataListener(shortcode_minute_bar_logged_message_filesource_map[shortcode_]);
      }
    }
  }

  void Seek(HFSAT::ttime_t seek_time = HFSAT::ttime_t(0, 0)) {
    // If seek time is given as proper input use that, else use the date, hhmm given as input
    if (seek_time.tv_sec > 0) {
      historical_dispatcher_.SeekHistFileSourcesTo(seek_time);
      if (HFSAT::DateTime::Get_UTC_YYYYMMDD_from_ttime(seek_time) != tradingdate_) {
        watch_.ResetWatch(HFSAT::DateTime::Get_UTC_YYYYMMDD_from_ttime(seek_time));
      }
      return;
    }
#define MINUTES_TO_PREP 30
    // To only process data starting MINUTES_TO_PREP minutes before datagen_start_utc_hhmm_
    int event_process_start_utc_hhmm_ = datagen_start_utc_hhmm_;
    {
      event_process_start_utc_hhmm_ =
          ((event_process_start_utc_hhmm_ / 100) * 60) + (event_process_start_utc_hhmm_ % 100);
      event_process_start_utc_hhmm_ = std::max(0, event_process_start_utc_hhmm_ - MINUTES_TO_PREP);
      event_process_start_utc_hhmm_ =
          (event_process_start_utc_hhmm_ % 60) + ((event_process_start_utc_hhmm_ / 60) * 100);
    }
#undef MINUTES_TO_PREP

    // In all the file sources, read events till
    // we reach the first event after the specified ttime_t
    historical_dispatcher_.SeekHistFileSourcesTo(HFSAT::ttime_t(
        HFSAT::DateTime::GetTimeUTC(global_datagen_start_utc_yymmdd_, event_process_start_utc_hhmm_), 0));

    if (global_datagen_start_utc_yymmdd_ != tradingdate_) {
      watch_.ResetWatch(global_datagen_start_utc_yymmdd_);
    }
  }

  void Run(HFSAT::ttime_t end_time = HFSAT::ttime_t(0, 0)) {
    // // start event loop
    try {
      if (end_time.tv_sec > 0) {
        historical_dispatcher_.RunHist(end_time);
      } else {
        if (global_datagen_end_utc_yymmdd_ == -1 || datagen_end_utc_hhmm_ == -1) {
          historical_dispatcher_.RunHist();
        } else {
          historical_dispatcher_.RunHist(
              HFSAT::ttime_t(HFSAT::DateTime::GetTimeUTC(global_datagen_end_utc_yymmdd_, datagen_end_utc_hhmm_), 0));
        }
      }
    } catch (int e) {
      std::cerr << "Unable to run historical dispatcher\n";
    }

    historical_dispatcher_.DeleteSources();
  }

  HFSAT::MinuteBarSecurityMarketView *getSMV() { return sid_to_smv_ptr_map_.GetSecurityMarketView(0); }

  HFSAT::SecIDMinuteBarSMVMap &getSMVMap() { return sid_to_smv_ptr_map_; }

  unsigned int getNumSecId() { return num_sec_id_; }

  HFSAT::TradingLocation_t getDepTradingLocation() { return dep_trading_location_; }

  HFSAT::HistoricalDispatcher &getHistoricalDispatcher() { return historical_dispatcher_; }

  HFSAT::Watch &getWatch() { return watch_; }

  HFSAT::DebugLogger &getLogger() { return dbglogger_; }

  const char *getExchangleSymbol() { return exchange_symbol_vec_[0]; }

  void cleanup() { historical_dispatcher_.DeleteSources(); }

  ~MinuteBarSMVSource() { cleanup(); };
};
