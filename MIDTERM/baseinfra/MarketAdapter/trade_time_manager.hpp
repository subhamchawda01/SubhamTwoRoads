/**
   \file hkex_trade_time_manager.hpp
   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
   Created on: Sep 21, 2011
   Author: piyush
 */

#ifndef BASE_MARKETADAPTER_TRADE_TIME_MANAGER_H
#define BASE_MARKETADAPTER_TRADE_TIME_MANAGER_H

#include <fstream>
#include <cstring>
#include <iostream>
#include <stdlib.h>
#include <string>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CDef/security_definitions.hpp"

namespace HFSAT {

class TradeTimeManager {
 private:
  int* start_times;
  int* end_times;
  int** break_start_times_;
  int** break_end_times_;
  int* number_of_breaks_;
  bool* trade_time_exists_;
  int size;
  int yyyymmdd;
  std::map<const char*, int> filename_to_id_map_;

 public:
  std::map<const char*, std::string> exchange_to_break_time_file_map_;

  TradeTimeManager(const TradeTimeManager&);

  TradeTimeManager(const HFSAT::SecurityNameIndexer& sec_indexer, int YYYYMMDD) {
    size = sec_indexer.GetNumSecurityId();
    start_times = (int*)calloc(size, sizeof(int));
    end_times = (int*)calloc(size, sizeof(int));
    number_of_breaks_ = (int*)calloc(size, sizeof(int));
    break_start_times_ = (int**)calloc(size, sizeof(int*));
    break_end_times_ = (int**)calloc(size, sizeof(int*));
    trade_time_exists_ = new bool[size];

    for (int i = 0; i < size; i++) {
      number_of_breaks_[i] = 0;
      trade_time_exists_[i] = false;
    }

    yyyymmdd = YYYYMMDD;
    std::vector<std::string> filevec_;
    filevec_.push_back((std::string("/spare/local/files/HKEX/hkex-trd-hours.txt")));
    exchange_to_break_time_file_map_["HKEX"] = std::string("/spare/local/files/HKEX/hkex-trd-hours.txt");
    filename_to_id_map_["/spare/local/files/HKEX/hkex-trd-hours.txt"] = 9;

    filevec_.push_back((std::string("/spare/local/files/LIFFE/liffe-trd-hours.txt")));
    exchange_to_break_time_file_map_["LIFFE"] = std::string("/spare/local/files/LIFFE/liffe-trd-hours.txt");
    filename_to_id_map_["/spare/local/files/LIFFE/liffe-trd-hours.txt"] = 13;

    filevec_.push_back((std::string("/spare/local/files/BMF/bmf-trd-hours.txt")));
    exchange_to_break_time_file_map_["BMF"] = std::string("/spare/local/files/BMF/bmf-trd-hours.txt");
    filename_to_id_map_["/spare/local/files/BMF/bmf-trd-hours.txt"] = 3;

    filevec_.push_back((std::string("/spare/local/files/NSE/nse-trd-hours.txt")));
    exchange_to_break_time_file_map_["NSE"] = std::string("/spare/local/files/NSE/nse-trd-hours.txt");
    filename_to_id_map_["/spare/local/files/NSE/nse-trd-hours.txt"] = 39;

    filevec_.push_back((std::string("/spare/local/files/ASX/asx-trd-hours.txt")));
    exchange_to_break_time_file_map_["ASX"] = std::string("/spare/local/files/ASX/asx-trd-hours.txt");
    filename_to_id_map_["/spare/local/files/ASX/asx-trd-hours.txt"] = 35;

    filevec_.push_back((std::string("/spare/local/files/SGX/sgx-trd-hours.txt")));
    exchange_to_break_time_file_map_["SGX"] = std::string("/spare/local/files/SGX/sgx-trd-hours.txt");
    filename_to_id_map_["/spare/local/files/SGX/sgx-trd-hours.txt"] = 36;

    filevec_.push_back((std::string("/spare/local/files/OSE/ose-trd-hours.txt")));
    exchange_to_break_time_file_map_["OSE"] = std::string("/spare/local/files/OSE/ose-trd-hours.txt");
    filename_to_id_map_["/spare/local/files/OSE/ose-trd-hours.txt"] = 22;

    filevec_.push_back((std::string("/spare/local/files/MICEX/micex-trd-hours.txt")));
    exchange_to_break_time_file_map_["MICEX"] = std::string("/spare/local/files/MICEX/micex-trd-hours.txt");
    filename_to_id_map_["/spare/local/files/MICEX/micex-trd-hours.txt"] = 17;

    filevec_.push_back((std::string("/spare/local/files/RTS/rts-trd-hours.txt")));
    exchange_to_break_time_file_map_["RTS"] = std::string("/spare/local/files/RTS/rts-trd-hours.txt");
    filename_to_id_map_["/spare/local/files/RTS/rts-trd-hours.txt"] = 14;

    filevec_.push_back((std::string("/spare/local/files/CME/cme-trd-hours.txt")));
    exchange_to_break_time_file_map_["CME"] = std::string("/spare/local/files/CME/cme-trd-hours.txt");
    filename_to_id_map_["/spare/local/files/CME/cme-trd-hours.txt"] = 1;

    for (unsigned i = 0; i < filevec_.size(); i++) {
      parseTimeFile(sec_indexer, filevec_[i].c_str());
    }
  }

  ~TradeTimeManager() {
    for (auto id = 0; id < size; id++) {
      free(break_start_times_[id]);
      free(break_end_times_[id]);
    }

    free(number_of_breaks_);
    free(break_start_times_);
    free(break_end_times_);
    delete trade_time_exists_;
  }
  void parseTimeFile(const HFSAT::SecurityNameIndexer& sec_indexer, const char* trade_time_filename_) {
    std::fstream file(trade_time_filename_, std::ofstream::in);

    // check for the condition below if the file required is needed by trade time manager

    std::vector<int> relevant_exchange_vec_;
    // std::string jgbl=std::string("JGBL_0");

    std::string this_schortcode_("JGBL_0");
    for (int i = 0; i < size; i++) {
      this_schortcode_ = sec_indexer.GetShortcodeFromId(i);
      const ExchSource_t this_exch_source_ = SecurityDefinitions::GetContractExchSource(this_schortcode_, yyyymmdd);
      relevant_exchange_vec_.push_back(this_exch_source_);
    }

    if (!file || !file.is_open()) {
      if (std::find(relevant_exchange_vec_.begin(), relevant_exchange_vec_.end(),
                    filename_to_id_map_[trade_time_filename_]) == relevant_exchange_vec_.end()) {
        fprintf(stderr, "Could not open file %s in trade_time_manager\n", trade_time_filename_);
        exit(1);
      } else {
        return;
      }
    }

    char line[1024];
    memset(line, 0, sizeof(line));

    while (file.getline(line, sizeof(line))) {
      PerishableStringTokenizer st_(line, sizeof(line));
      std::vector<const char*> tokens = st_.GetTokens();
      std::vector<std::string> tokens_;
      for (auto token : tokens) {
        tokens_.push_back(token);
      }

      if (tokens_.size() < 4) {
        continue;
      }

      if (tokens_[0].compare("#") == 0) {
        continue;
      }

      // shc_ start_time_ end_time_ tz_ fbst_ fbet_ sbst_ sbet_ ....

      if (!(HFSAT::ExchangeSymbolManager::CheckIfContractSpecExists(tokens_[0])) && (tokens_[0].substr(0, 4) != "NSE_"))
        continue;

      if (tokens_[0].substr(0, 4) == "NSE_") {
        // Different handling for NSE, setting product class wise times
        char segment = NSE_FO_SEGMENT_MARKING;
        if (tokens_[0] == "NSE_FO") {
          segment = NSE_FO_SEGMENT_MARKING;
        } else if (tokens_[0] == "NSE_CD") {
          segment = NSE_CD_SEGMENT_MARKING;
        } else if (tokens_[0] == "NSE_EQ") {
          segment = NSE_EQ_SEGMENT_MARKING;
        }

        for (unsigned index = 0; index < sec_indexer.NumSecurityId(); index++) {
          std::string secname = sec_indexer.GetShortcodeFromId(index);
          if (NSESecurityDefinitions::GetSegmentTypeFromShortCode(secname) == segment) {
            tokens_[0] = secname.c_str();
            SetBreaksForSecID(index, tokens_);
          }
        }
      } else {
        if (HFSAT::HolidayManager::GetUniqueInstance().GetProductStartDate(tokens_[0]) > yyyymmdd) {
          continue;
        }
        const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(tokens_[0]);
        int sec_id = sec_indexer.GetIdFromSecname(exchange_symbol_);
        SetBreaksForSecID(sec_id, tokens_);
      }
      memset(line, 0, sizeof(line));
    }

    file.close();
  }

  void SetBreaksForSecID(int sec_id, const std::vector<std::string>& tokens_) {
    if (sec_id >= 0) {
      // Read information for this
      char this_token_[32];
      strcpy(this_token_, tokens_[1].c_str());
      char* t1 = strtok(this_token_, ":");
      char* t2 = strtok(NULL, ":");
      int st = (atoi(t1) * 60 + atoi(t2)) * 60;

      strcpy(this_token_, tokens_[2].c_str());
      t1 = strtok(this_token_, ":");
      t2 = strtok(NULL, ":");
      int et = (atoi(t1) * 60 + atoi(t2)) * 60;

      char time_zone_[32];
      strcpy(time_zone_, tokens_[3].c_str());

      std::string tz = (time_zone_ == NULL) ? "" : std::string(time_zone_);
      int32_t offset = 0;
      if (yyyymmdd == -1) {
        timeval tv;
        gettimeofday(&tv, NULL);
        struct tm t_ = *localtime(&tv.tv_sec);
        yyyymmdd = (1900 + t_.tm_year) * 10000 + (1 + t_.tm_mon) * 100 + t_.tm_mday;
      }

      // always assume EST else 0 offset i.e. UTC
      if (tz == "EST") {
        offset = HFSAT::DateTime::GetTimeMidnightEST(yyyymmdd) - HFSAT::DateTime::GetTimeMidnightUTC(yyyymmdd);
      } else if (tz == "LON") {
        offset = HFSAT::DateTime::GetTimeMidnightLON(yyyymmdd) - HFSAT::DateTime::GetTimeMidnightUTC(yyyymmdd);
      } else if (tz == "PAR") {
        offset = HFSAT::DateTime::GetTimeMidnightPAR(yyyymmdd) - HFSAT::DateTime::GetTimeMidnightUTC(yyyymmdd);
      } else if (tz == "AMS") {
        offset = HFSAT::DateTime::GetTimeMidnightAMS(yyyymmdd) - HFSAT::DateTime::GetTimeMidnightUTC(yyyymmdd);
      } else if (tz == "BRT") {
        offset = HFSAT::DateTime::GetTimeMidnightBRT(yyyymmdd) - HFSAT::DateTime::GetTimeMidnightUTC(yyyymmdd);
      } else if (tz == "MSK") {
        offset = HFSAT::DateTime::GetTimeMidnightMSK(yyyymmdd) - HFSAT::DateTime::GetTimeMidnightUTC(yyyymmdd);
      } else if (tz == "CET") {
        offset = HFSAT::DateTime::GetTimeMidnightCET(yyyymmdd) - HFSAT::DateTime::GetTimeMidnightUTC(yyyymmdd);
      } else if (tz == "IST") {
        offset = HFSAT::DateTime::GetTimeMidnightIST(yyyymmdd) - HFSAT::DateTime::GetTimeMidnightUTC(yyyymmdd);
      } else if (tz == "AST") {
        offset = HFSAT::DateTime::GetTimeMidnightAEST(yyyymmdd) - HFSAT::DateTime::GetTimeMidnightUTC(yyyymmdd);
      } else if (tz == "JST") {
        offset = HFSAT::DateTime::GetTimeMidnightJST(yyyymmdd) - HFSAT::DateTime::GetTimeMidnightUTC(yyyymmdd);
      } else if (tz == "SGT") {
        offset = HFSAT::DateTime::GetTimeMidnightSGT(yyyymmdd) - HFSAT::DateTime::GetTimeMidnightUTC(yyyymmdd);
      }

      start_times[sec_id] = st + offset;
      end_times[sec_id] = et + offset;
      if (tz == "JST") {
        start_times[sec_id] += 86400;  // Handling of previous day issue
        end_times[sec_id] += 86400;
      }

      if (tz == "AST") {
        start_times[sec_id] += 86400;  // Handling of previous day issue
        end_times[sec_id] += 86400;
      }
      if (tz == "SGT") {
        start_times[sec_id] += 86400;  // Handling of previous day issue
        end_times[sec_id] += 86400;
      }
      // std::cerr << exchange_symbol_ << " tz: " << tz <<  " off: "<< offset <<  "  st: " << start_times[ sec_id ] <<
      // " et: " << end_times[ sec_id ] << " " ;
      unsigned int index_ = 4;
      number_of_breaks_[sec_id] = ((tokens_.size() - 4) / 2);
      break_start_times_[sec_id] = (int*)calloc(number_of_breaks_[sec_id], sizeof(int));
      break_end_times_[sec_id] = (int*)calloc(number_of_breaks_[sec_id], sizeof(int));

      for (int i = 0; i < number_of_breaks_[sec_id]; i++) {
        break_start_times_[sec_id][i] = 0;
        break_end_times_[sec_id][i] = 24 * 3600;
      }

      trade_time_exists_[sec_id] = true;
      int break_index_ = 0;
      while (index_ < tokens_.size()) {
        strcpy(this_token_, tokens_[index_].c_str());
        t1 = strtok(this_token_, ":");
        t2 = strtok(NULL, ":");
        int bst_ = (atoi(t1) * 60 + atoi(t2)) * 60;
        index_++;
        strcpy(this_token_, tokens_[index_].c_str());
        t1 = strtok(this_token_, ":");
        t2 = strtok(NULL, ":");
        int bet_ = (atoi(t1) * 60 + atoi(t2)) * 60;
        break_start_times_[sec_id][break_index_] = bst_ + offset;
        break_end_times_[sec_id][break_index_] = bet_ + offset;
        break_index_++;
        index_++;
      }

      // std::cerr << "\n";
    }
  }

 public:
  static TradeTimeManager& GetUniqueInstance(const HFSAT::SecurityNameIndexer& sec_indexer, int YYYYMMDD = -1) {
    static TradeTimeManager uniqueinstance_ = TradeTimeManager(sec_indexer, YYYYMMDD);
    return uniqueinstance_;
  }

  int** get_break_start_times_() { return break_start_times_; }
  int** get_break_end_times_() { return break_end_times_; }

  // time zone not taken care of.
  inline bool isValidTimeToTrade(int sec_id, int seconds_elapased_today) {
    bool can_trade_;
    if (sec_id < 0 || sec_id >= size || !trade_time_exists_[sec_id]) return true;

    if (start_times[sec_id] <= end_times[sec_id]) {
      can_trade_ = (start_times[sec_id] <= seconds_elapased_today && seconds_elapased_today <= end_times[sec_id]);
    } else {
      can_trade_ = !(start_times[sec_id] >= seconds_elapased_today && seconds_elapased_today >= end_times[sec_id]);
    }
    for (int i = 0; i < number_of_breaks_[sec_id]; i++) {
      can_trade_ = can_trade_ &&
                   !((break_start_times_[sec_id][i] <= seconds_elapased_today) &&
                     (break_end_times_[sec_id][i] >= seconds_elapased_today));
      if (!can_trade_) {
        return can_trade_;
      }
    }

    return can_trade_;
  }

  inline int GetStartTime(int sec_id) {
    if (sec_id < 0 || sec_id >= size) return -1;
    return start_times[sec_id];
  }
  bool BreakTimeFileExists(const char* exchange_name_) {
    std::string fileName = exchange_to_break_time_file_map_[exchange_name_];
    std::ifstream infile(fileName);
    return infile.good();
  }
};
}

#endif /* BASE_MARKETADAPTER_TRADE_TIME_MANAGER_H */
