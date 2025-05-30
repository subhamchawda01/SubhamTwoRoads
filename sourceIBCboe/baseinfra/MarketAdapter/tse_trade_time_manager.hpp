// =====================================================================================
//
//       Filename:  tse_trade_time_manager.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  05/29/2013 07:50:06 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 353, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#ifndef BASE_MARKETADAPTER_TSE_TRADE_TIME_MANAGER_H
#define BASE_MARKETADAPTER_TSE_TRADE_TIME_MANAGER_H

#define TSE_TRADE_TIME_INFO_FILE "/spare/local/files/TSE/tse-trd-hours.txt"

#include <fstream>
#include <cstring>
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <map>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"

#define MAX_TIME_STORE_LIMIT 10

namespace HFSAT {

class TseTradeTimeManager {
 private:
  int* start_times;
  int* end_times;
  int size;

  std::vector<int*> security_id_to_start_time_;
  std::vector<int*> security_id_to_end_time_;

  std::map<int, int> security_id_to_time_pointer_;

  TseTradeTimeManager(const TseTradeTimeManager&);

  TseTradeTimeManager(const HFSAT::SecurityNameIndexer& sec_indexer)
      : security_id_to_start_time_(), security_id_to_end_time_(), security_id_to_time_pointer_() {
    size = sec_indexer.GetNumSecurityId();

    for (int security_counter_ = 0; security_counter_ < size; security_counter_++) {
      int* start_time_for_this_security_ = new int[MAX_TIME_STORE_LIMIT];
      int* end_time_for_this_security_ = new int[MAX_TIME_STORE_LIMIT];

      security_id_to_start_time_.push_back(start_time_for_this_security_);
      security_id_to_end_time_.push_back(end_time_for_this_security_);

      security_id_to_time_pointer_[security_counter_] = 0;
    }

    start_times = (int*)calloc(size, sizeof(int));
    end_times = (int*)calloc(size, sizeof(int));

    parseTimeFile(sec_indexer);
  }

  void parseTimeFile(const HFSAT::SecurityNameIndexer& sec_indexer) {
    std::fstream file((TSE_TRADE_TIME_INFO_FILE), std::ofstream::in);

    if (!file || !file.is_open()) {
      fprintf(stderr, "Could not open file %s in tse_trade_time_manager\n", TSE_TRADE_TIME_INFO_FILE);
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
      const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(short_code);

      int sec_id = sec_indexer.GetIdFromSecname(exchange_symbol_);
      if (sec_id >= 0) {
        std::string tz = (time_zone == NULL) ? "" : std::string(time_zone);
        int32_t offset = 0;

        // always assume EST else 0 offset i.e. UTC
        if (tz == "EST") {
          timeval tv;
          gettimeofday(&tv, NULL);
          struct tm t_ = *localtime(&tv.tv_sec);
          int yyyymmdd = (1900 + t_.tm_year) * 10000 + (1 + t_.tm_mon) * 100 + t_.tm_mday;
          offset = HFSAT::DateTime::GetTimeMidnightEST(yyyymmdd) - HFSAT::DateTime::GetTimeMidnightUTC(yyyymmdd);
        } else if (tz == "LON") {
          timeval tv;
          gettimeofday(&tv, NULL);
          struct tm t_ = *localtime(&tv.tv_sec);
          int yyyymmdd = (1900 + t_.tm_year) * 10000 + (1 + t_.tm_mon) * 100 + t_.tm_mday;
          offset = HFSAT::DateTime::GetTimeMidnightLON(yyyymmdd) - HFSAT::DateTime::GetTimeMidnightUTC(yyyymmdd);
        } else if (tz == "PAR") {
          timeval tv;
          gettimeofday(&tv, NULL);
          struct tm t_ = *localtime(&tv.tv_sec);
          int yyyymmdd = (1900 + t_.tm_year) * 10000 + (1 + t_.tm_mon) * 100 + t_.tm_mday;
          offset = HFSAT::DateTime::GetTimeMidnightPAR(yyyymmdd) - HFSAT::DateTime::GetTimeMidnightUTC(yyyymmdd);
        } else if (tz == "AMS") {
          timeval tv;
          gettimeofday(&tv, NULL);
          struct tm t_ = *localtime(&tv.tv_sec);
          int yyyymmdd = (1900 + t_.tm_year) * 10000 + (1 + t_.tm_mon) * 100 + t_.tm_mday;
          offset = HFSAT::DateTime::GetTimeMidnightAMS(yyyymmdd) - HFSAT::DateTime::GetTimeMidnightUTC(yyyymmdd);
        }

        start_times[sec_id] = st + offset;
        end_times[sec_id] = et + offset;

        (security_id_to_start_time_[sec_id])[security_id_to_time_pointer_[sec_id]] = st + offset;
        (security_id_to_end_time_[sec_id])[security_id_to_time_pointer_[sec_id]] = et + offset;

        security_id_to_time_pointer_[sec_id]++;

        // std::cerr << " Shortcode : " << exchange_symbol_ << " Start Time : "  << start_times [sec_id ] << " End Time
        // : " << end_times[sec_id] << " Security Id : " << sec_id << "\n" ;
      }
      memset(line, 0, sizeof(line));
    }

    file.close();
  }

 public:
  static TseTradeTimeManager& GetUniqueInstance(const HFSAT::SecurityNameIndexer& sec_indexer) {
    static TseTradeTimeManager uniqueinstance_ = TseTradeTimeManager(sec_indexer);
    return uniqueinstance_;
  }

  // time zone not taken care of.
  inline bool isValidTimeToTrade(int sec_id, int seconds_elapased_today) {
    if (sec_id < 0 || sec_id >= size) return true;

    for (int time_store_counter_ = 0; time_store_counter_ < security_id_to_time_pointer_[sec_id];
         time_store_counter_++) {
      if (seconds_elapased_today >= security_id_to_start_time_[sec_id][time_store_counter_] &&
          seconds_elapased_today <= security_id_to_end_time_[sec_id][time_store_counter_])
        return true;
    }

    return false;
  }
};
}

#endif /* BASE_MARKETADAPTER_TSE_TRADE_TIME_MANAGER_H */
