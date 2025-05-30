// =====================================================================================
//
//       Filename:  get_approximate_ors_exchange_message_count.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  10/16/2014 07:54:22 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <signal.h>
#include <getopt.h>
#include <map>
#include <algorithm>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/file_utils.hpp"  // To create the directory
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/ors_defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"

#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CDef/ors_messages.hpp"

#define LOGGED_DATA_PREFIX "/NAS1/data/ORSData/"

class GenerateApproximateORSExchangeMessageCount {
 public:
  static int32_t GetMessageCountStats(const std::string &_shortcode_, const int32_t &_yyyymmdd_,
                                      const time_t &_unix_start_time_, const time_t &_unix_end_time_,
                                      const bool &_time_frame_given_) {
    int32_t exchange_message_count = 0;
    std::string logged_data_filename = "";

    if (std::string::npos == _shortcode_.find("/")) {
      HFSAT::ExchangeSymbolManager::SetUniqueInstance(_yyyymmdd_);
      const char *exchange_symbol = HFSAT::ExchangeSymbolManager::GetExchSymbol(_shortcode_);

      std::string location =
          HFSAT::TradingLocationUtils::GetTradingLocationName(HFSAT::TradingLocationUtils::GetTradingLocationExch(
              HFSAT::SecurityDefinitions::GetContractExchSource(_shortcode_, _yyyymmdd_)));

      std::ostringstream date_str;
      date_str << _yyyymmdd_;
      std::string date = date_str.str();

      std::ostringstream logged_data_filename_stream;
      logged_data_filename_stream << LOGGED_DATA_PREFIX << location << "/" << date.substr(0, 4) << "/"
                                  << date.substr(4, 2) << "/" << date.substr(6, 2) << "/" << exchange_symbol << "_"
                                  << _yyyymmdd_;
      logged_data_filename = logged_data_filename_stream.str();

      if (std::string::npos != logged_data_filename.find(" ")) {
        logged_data_filename.replace(logged_data_filename.find(" "), 1, "~");
      }

    } else {
      logged_data_filename = _shortcode_;
    }

    HFSAT::BulkFileReader bulk_file_reader;
    bulk_file_reader.open(logged_data_filename.c_str());

    if (!bulk_file_reader.is_open()) {
      std::cerr << "FAILED TO OPEN FILE FOR COMPUTING STATS : " << logged_data_filename << "\n";
      exit(-1);
    }

    while (true) {
      HFSAT::GenericORSReplyStruct reply_struct;

      int32_t read_size =
          bulk_file_reader.read(reinterpret_cast<char *>(&reply_struct), sizeof(HFSAT::GenericORSReplyStruct));
      if (read_size < (int32_t)sizeof(HFSAT::GenericORSReplyStruct)) break;

      if (_time_frame_given_ && (reply_struct.time_set_by_server_.tv_sec < _unix_start_time_)) continue;
      if (_time_frame_given_ && (reply_struct.time_set_by_server_.tv_sec > _unix_end_time_)) break;

      if (HFSAT::kORRType_Conf == reply_struct.orr_type_) exchange_message_count++;
      if (HFSAT::kORRType_Cxld == reply_struct.orr_type_) exchange_message_count++;
      if (HFSAT::kORRType_CxlRejc == reply_struct.orr_type_ &&
          HFSAT::kCxlRejectReasonTooLate == reply_struct.size_executed_)
        exchange_message_count++;
      if (HFSAT::kORRType_Rejc == reply_struct.orr_type_ &&
          HFSAT::kCxlRejectReasonTooLate == reply_struct.size_executed_)
        exchange_message_count++;
    }

    bulk_file_reader.close();

    return exchange_message_count;
  }
};

int main(int argc, char *argv[]) {
  if (3 != argc && 5 != argc) {
    std::cerr << "USAGE : <exec> <SHORTCODE/FILE> <DATE> <OPT:START_TIME> <OPT:END_TIME> \n";
    std::cerr << "EXAMPLE : FGBM_0 20141015 EST_800 EST_1600 \n";
    std::cerr << "EXAMPLE : /NAS1/data/ORSData/FR2/2014/10/15/FGBM201412_20141015.gz 20141015 EST_800 EST_1600 \n";
    std::cerr.flush();

    exit(-1);
  }

  std::string shortcode = "";
  int32_t yyyymmdd = 0;
  int32_t start_time = -1;
  int32_t end_time = -1;
  bool time_frame_given = false;
  time_t unixtime_start = (time_t)0;
  time_t unixtime_end = (time_t)0;

  shortcode = argv[1];
  yyyymmdd = atoi(argv[2]);

  if (5 == argc) {
    const char *start_time_str = argv[3];
    const char *end_time_str = argv[4];

    if ((strncmp(start_time_str, "EST_", 4) == 0) || (strncmp(start_time_str, "CST_", 4) == 0) ||
        (strncmp(start_time_str, "CET_", 4) == 0) || (strncmp(start_time_str, "BRT_", 4) == 0) ||
        (strncmp(start_time_str, "UTC_", 4) == 0) || (strncmp(start_time_str, "KST_", 4) == 0) ||
        (strncmp(start_time_str, "HKT_", 4) == 0) || (strncmp(start_time_str, "MSK_", 4) == 0) ||
        (strncmp(start_time_str, "IST_", 4) == 0) || (strncmp(start_time_str, "JST_", 4) == 0) ||
        (strncmp(start_time_str, "BST_", 4) == 0)) {
      start_time = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(yyyymmdd, atoi(start_time_str + 4), start_time_str);
    } else {
      start_time = atoi(start_time_str);
    }

    if ((strncmp(end_time_str, "EST_", 4) == 0) || (strncmp(end_time_str, "CST_", 4) == 0) ||
        (strncmp(end_time_str, "CET_", 4) == 0) || (strncmp(end_time_str, "BRT_", 4) == 0) ||
        (strncmp(end_time_str, "UTC_", 4) == 0) || (strncmp(end_time_str, "KST_", 4) == 0) ||
        (strncmp(end_time_str, "HKT_", 4) == 0) || (strncmp(end_time_str, "MSK_", 4) == 0) ||
        (strncmp(end_time_str, "IST_", 4) == 0) || (strncmp(end_time_str, "JST_", 4) == 0) ||
        (strncmp(end_time_str, "BST_", 4) == 0)) {
      end_time = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(yyyymmdd, atoi(end_time_str + 4), end_time_str);
    } else {
      end_time = atoi(end_time_str);
    }

    struct tm timeinfo = {0};

    timeinfo.tm_year = (yyyymmdd / 10000) - 1900;
    timeinfo.tm_mon = (yyyymmdd / 100) % 100 - 1;
    timeinfo.tm_mday = (yyyymmdd % 100);

    unixtime_start = mktime(&timeinfo);
    unixtime_end = mktime(&timeinfo);

    start_time = (start_time / 100) * 3600 + (start_time % 100) * 60;
    end_time = (end_time / 100) * 3600 + (end_time % 100) * 60;

    unixtime_start += start_time;
    unixtime_end += end_time;

    time_frame_given = true;
  }

  std::cout << GenerateApproximateORSExchangeMessageCount::GetMessageCountStats(shortcode, yyyymmdd, unixtime_start,
                                                                                unixtime_end, time_frame_given)
            << "\n";

  return EXIT_SUCCESS;
}  // ----------  end of function main  ----------
