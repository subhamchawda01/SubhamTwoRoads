/**
   \file ose_trade_time_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551

   Created on: Sep 21, 2011
   Author: piyush

 */

#ifndef BASE_MARKETADAPTER_OSE_TRADE_TIME_MANAGER_H
#define BASE_MARKETADAPTER_OSE_TRADE_TIME_MANAGER_H

#define OSE_TRADE_TIME_INFO_FILE "/spare/local/files/OSE/ose-trd-hours.txt"

#include <fstream>
#include <cstring>
#include <iostream>
#include <stdlib.h>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"

namespace HFSAT {

class OseTradeTimeManager {
 private:
  int* start_times;
  int* end_times;
  int* break_start_times;
  int* break_end_times;
  int* second_break_start_times;
  int* second_break_end_times;
  int size;
  int yyyymmdd;

  OseTradeTimeManager(const OseTradeTimeManager&);

  OseTradeTimeManager(HFSAT::SecurityNameIndexer& sec_indexer, int YYYYMMDD) {
    size = sec_indexer.GetNumSecurityId();
    start_times = (int*)calloc(size, sizeof(int));
    end_times = (int*)calloc(size, sizeof(int));
    yyyymmdd = YYYYMMDD;
    break_start_times = (int*)calloc(size, sizeof(int));
    break_end_times = (int*)calloc(size, sizeof(int));

    second_break_start_times = (int*)calloc(size, sizeof(int));
    second_break_end_times = (int*)calloc(size, sizeof(int));

    parseTimeFile(sec_indexer);
  }

  void parseTimeFile(HFSAT::SecurityNameIndexer& sec_indexer) {
    std::fstream file(OSE_TRADE_TIME_INFO_FILE, std::ofstream::in);

    if (!file || !file.is_open()) {
      fprintf(stderr, "Could not open file %s in ose_trade_time_manager\n", OSE_TRADE_TIME_INFO_FILE);
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
      char* tmp3 = strtok(NULL, "\n\t ");
      char* tmp4 = strtok(NULL, "\n\t ");
      char* tmp5 = strtok(NULL, "\n\t ");
      char* tmp6 = strtok(NULL, "\n\t ");

      char* t1 = strtok(tmp1, ":");
      char* t2 = strtok(NULL, ":");
      int st = (atoi(t1) * 60 + atoi(t2)) * 60;

      t1 = strtok(tmp2, ":");
      t2 = strtok(NULL, ":");
      int et = (atoi(t1) * 60 + atoi(t2)) * 60;

      t1 = strtok(tmp3, ":");
      t2 = strtok(NULL, ":");
      int bs = (atoi(t1) * 60 + atoi(t2)) * 60;

      t1 = strtok(tmp4, ":");
      t2 = strtok(NULL, ":");
      int be = (atoi(t1) * 60 + atoi(t2)) * 60;

      t1 = strtok(tmp5, ":");
      t2 = strtok(NULL, ":");
      int sbs = (atoi(t1) * 60 + atoi(t2)) * 60;

      t1 = strtok(tmp6, ":");
      t2 = strtok(NULL, ":");
      int sbe = (atoi(t1) * 60 + atoi(t2)) * 60;

      char* short_code = ticker;
      if (!(HFSAT::ExchangeSymbolManager::CheckIfContractSpecExists(short_code))) continue;
      const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(short_code);

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
        }

        start_times[sec_id] = st + offset;
        end_times[sec_id] = et + offset;
        break_start_times[sec_id] = bs + offset;
        break_end_times[sec_id] = be + offset;
        second_break_start_times[sec_id] = sbs + offset;
        second_break_end_times[sec_id] = sbe + offset;
      }
      memset(line, 0, sizeof(line));
    }

    file.close();
  }

 public:
  static OseTradeTimeManager& GetUniqueInstance(HFSAT::SecurityNameIndexer& sec_indexer, int YYYYMMDD = -1) {
    // std::cout << "inside this" <<std::endl;
    static OseTradeTimeManager uniqueinstance_ = OseTradeTimeManager(sec_indexer, YYYYMMDD);
    return uniqueinstance_;
  }

  // time zone not taken care of.
  inline bool isValidTimeToTrade(int sec_id, int seconds_elapased_today) {
    if (sec_id < 0 || sec_id >= size) return true;
    return (
        start_times[sec_id] <= seconds_elapased_today && seconds_elapased_today <= end_times[sec_id] &&
        !(break_start_times[sec_id] <= seconds_elapased_today && seconds_elapased_today <= break_end_times[sec_id]) &&
        !(second_break_start_times[sec_id] <= seconds_elapased_today &&
          seconds_elapased_today <= second_break_end_times[sec_id]));
  }

  inline bool isExchClosed(int sec_id, int seconds_elapased_today) {
    if (sec_id < 0 || sec_id >= size) {
      return false;
    } else {
      return (seconds_elapased_today > end_times[sec_id]);
    }
  }
};
}

#endif /* BASE_MARKETADAPTER_OSE_TRADE_TIME_MANAGER_H */
