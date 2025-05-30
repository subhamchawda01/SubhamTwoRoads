/**
    \file Tools/GC_volume_reader.cpp ..Reads the volumes of the gold futures

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717

*/

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <algorithm>

#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/hybrid_sec.hpp"

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"

#include "baseinfra/LoggedSources/common_logged_message_filenamer.hpp"

void ParseCommandLineParams(const int argc, const char** argv, int& input_date_, int& begin_secs_from_midnight_,
                            int& end_secs_from_midnight_) {
  // expect :
  // 1. $0 date_YYYYMMDD [ start_tm HHMM ] [ end_tm HHMM ]
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " input_date_YYYYMMDD [ start_tm_utc_hhmm ] [ end_tm_utc_hhmm ]" << std::endl;
    exit(0);
  } else {
    input_date_ = atoi(argv[1]);

    // Convert HHMM inputs to seconds.
    if (argc > 2) {
      begin_secs_from_midnight_ = (atoi(argv[2]) / 100) * 60 * 60 + (atoi(argv[2]) % 100) * 60;
    }
    if (argc > 3) {
      end_secs_from_midnight_ = (atoi(argv[3]) / 100) * 60 * 60 + (atoi(argv[3]) % 100) * 60;
    }
  }
}

bool comparator(unsigned long i, unsigned long j) { return (i > j); }

int main(int argc, char** argv) {
  int input_date_ = 20110101;
  int begin_secs_from_midnight_ = 0;
  int end_secs_from_midnight_ = 24 * 60 * 60;
  ParseCommandLineParams(argc, (const char**)argv, input_date_, begin_secs_from_midnight_, end_secs_from_midnight_);

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date_);

  // Sure to capture the 5 highest traded volumes in these
  std::map<int, const char*> GC_symbols_map;
  std::map<int, const char*> volume_to_symbol;

  GC_symbols_map[0] = "GCU1";
  GC_symbols_map[1] = "GCV1";
  GC_symbols_map[2] = "GCZ1";
  GC_symbols_map[3] = "GCG2";
  GC_symbols_map[4] = "GCJ2";
  GC_symbols_map[5] = "GCQ1";
  GC_symbols_map[6] = "GCQ2";
  GC_symbols_map[7] = "GCV2";
  GC_symbols_map[8] = "GCZ2";
  GC_symbols_map[9] = "GCZ3";
  GC_symbols_map[10] = "GCZ4";
  GC_symbols_map[11] = "GCZ5";
  GC_symbols_map[12] = "GCZ6";

  for (unsigned int ii = 0; ii < 13; ii++) {
    HFSAT::TradingLocation_t trading_location_file_read_;
    HFSAT::BulkFileReader bulk_file_reader_;

    int traded_volume_ = 0;

    const char* t_exchange_symbol_ = GC_symbols_map[ii];

    std::string t_cme_filename_ = HFSAT::CommonLoggedMessageFileNamer::GetName(
        HFSAT::kExchSourceCME, t_exchange_symbol_, input_date_, trading_location_file_read_);
    CME_MDS::CMECommonStruct next_event_;
    bulk_file_reader_.open(t_cme_filename_);

    if (!bulk_file_reader_.is_open()) {
      traded_volume_ = -1;
    } else {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(CME_MDS::CMECommonStruct));
        if (available_len_ >= sizeof(CME_MDS::CMECommonStruct)) {  // data found in file
          if (next_event_.msg_ == CME_MDS::CME_TRADE) {  // This is a trade message, update the total_traded_volume_
                                                         // based on this trd vol, if this falls within our time range.
            if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&  // 86400 = 24 hrs * 60 mins * 60 secs.
                next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
              traded_volume_ += next_event_.data_.cme_trds_.trd_qty_;
            }
          }
        } else {  // Read error.
          break;
        }
      }
    }

    volume_to_symbol[traded_volume_] = t_exchange_symbol_;
  }

  std::map<int, const char*>::reverse_iterator it;
  for (it = volume_to_symbol.rbegin(); it != volume_to_symbol.rend(); ++it) {
    std::cout << "Volume = " << it->first << "   Sym : " << it->second << std::endl;
  }
}
