/**
   \file liffe_trade_time_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551

   Created on: Sep 21, 2011
   Author: piyush

 */

#ifndef BASE_MARKETADAPTER_LIFFE_TRADE_TIME_MANAGER_H
#define BASE_MARKETADAPTER_LIFFE_TRADE_TIME_MANAGER_H

#define LIFFE_TRADE_TIME_INFO_FILE "/spare/local/files/LIFFE/liffe-trd-hours.txt"

#include <fstream>
#include <cstring>
#include <iostream>
#include <stdlib.h>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"

namespace HFSAT {

class LiffeTradeTimeManager {
 private:
  int* start_times;
  int* end_times;
  int size;
  int yyyymmdd;

  LiffeTradeTimeManager(const LiffeTradeTimeManager&);

  LiffeTradeTimeManager(const HFSAT::SecurityNameIndexer& sec_indexer, int YYYYMMDD) {
    size = sec_indexer.GetNumSecurityId();
    start_times = (int*)calloc(size, sizeof(int));
    end_times = (int*)calloc(size, sizeof(int));
    yyyymmdd = YYYYMMDD;

    parseTimeFile(sec_indexer);
  }

  void parseTimeFile(const HFSAT::SecurityNameIndexer& sec_indexer) {
    std::fstream file((LIFFE_TRADE_TIME_INFO_FILE), std::ofstream::in);

    if (!file || !file.is_open()) {
      fprintf(stderr, "Could not open file %s in liffe_trade_time_manager\n", LIFFE_TRADE_TIME_INFO_FILE);
      exit(1);
    }
    char line[1024];
    char* ticker;
    memset(line, 0, sizeof(line));
    while (file.getline(line, sizeof(line))) {
      ticker = NULL;
      ticker = strtok(line, "\n\t ");
      if (ticker == NULL || strstr(ticker, "#") != NULL) continue;

      char* tmp1 = strtok(NULL, "\n\t ");
      char* tmp2 = strtok(NULL, "\n\t ");
      char* time_zone = strtok(NULL, "\n\t ");

      char* t1 = strtok(tmp1, ":");
      char* t2 = strtok(NULL, ":");
      int st = (atoi(t1) * 60 + atoi(t2)) * 60;

      t1 = strtok(tmp2, ":");
      t2 = strtok(NULL, ":");
      int et = (atoi(t1) * 60 + atoi(t2)) * 60;

      char* short_code = ticker;

      if (!(HFSAT::ExchangeSymbolManager::CheckIfContractSpecExists(short_code))) continue;
      const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(short_code);

      if (exchange_symbol_ == NULL) {
        continue;
      }

      int sec_id = sec_indexer.GetIdFromSecname(exchange_symbol_);
      if (sec_id >= 0) {
        std::string tz = (time_zone == NULL) ? "" : std::string(time_zone);
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
        }

        start_times[sec_id] = st + offset;
        end_times[sec_id] = et + offset;

        //            std::cerr << " Shortcode : " << exchange_symbol_ << " Start Time : "  << start_times [sec_id ] <<
        //            " End Time : " << end_times[sec_id] << " Security Id : " << sec_id << "\n" ;
      }
      memset(line, 0, sizeof(line));
    }

    file.close();
  }

 public:
  static LiffeTradeTimeManager& GetUniqueInstance(const HFSAT::SecurityNameIndexer& sec_indexer, int YYYYMMDD = -1) {
    static LiffeTradeTimeManager uniqueinstance_ = LiffeTradeTimeManager(sec_indexer, YYYYMMDD);
    return uniqueinstance_;
  }

  inline bool hasTradingStarted(int sec_id, int seconds_elapased_today) {
    if (sec_id < 0 || sec_id >= size) return true;

    return (seconds_elapased_today > start_times[sec_id]);
  }

  inline bool hasTradingClosed(int sec_id, int seconds_elapased_today) {
    if (sec_id < 0 || sec_id >= size) return true;

    return (seconds_elapased_today > end_times[sec_id]);
  }

  // time zone not taken care of.
  inline bool isValidTimeToTrade(int sec_id, int seconds_elapased_today) {
    if (sec_id < 0 || sec_id >= size) return true;
    return (start_times[sec_id] <= seconds_elapased_today && seconds_elapased_today <= end_times[sec_id]);
  }
};
}

#endif /* BASE_MARKETADAPTER_LIFFE_TRADE_TIME_MANAGER_H */
