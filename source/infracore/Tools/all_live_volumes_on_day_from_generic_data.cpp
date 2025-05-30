// =====================================================================================
//
//       Filename:  all_live_volumes_on_day_from_generic_data.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  06/13/2014 01:10:42 PM
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

#include <iostream>
#include <stdlib.h>
#include <sstream>
#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"

void ParseCommandLineParams(const int argc, const char** argv, std::string& exchange_src_input_,
                            std::string& file_name_, int& begin_secs_from_midnight_, int& end_secs_from_midnight_) {
  // expect :
  // 1. $0 shortcode date_YYYYMMDD [ start_tm HHMM ] [ end_tm HHMM ]
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " LOCATION  FILE_NAME [ start_tm_utc_hhmm ] [ end_tm_utc_hhmm ]" << std::endl;
    std::cerr << "Example: "
              << " EXEC CHI  /NAS1/data/CMELoggedData/CHI/2011/09/12/ZTU1_20110912" << std::endl;
    std::cerr << "Example: "
              << " EXEC FR2  /NAS1/data/EUREXLoggedData/FR2/2011/09/12/FILENAME" << std::endl;
    exit(0);
  } else {
    exchange_src_input_ = argv[1];
    // input_date_ = ( argv[2] ) ;
    file_name_ = argv[2];
    if (argc > 3) {
      begin_secs_from_midnight_ = (atoi(argv[3]) / 100) * 60 * 60 + (atoi(argv[3]) % 100) * 60;
    }
    if (argc > 4) {
      end_secs_from_midnight_ = (atoi(argv[4]) / 100) * 60 * 60 + (atoi(argv[4]) % 100) * 60;
    }
  }
}

/// input arguments : input_date
int main(int argc, char** argv) {
  std::string exchange_src_input_ = "";
  std::string file_name_to_act_ = "";
  // std::string input_date_ = "";
  int begin_secs_from_midnight_ = 0;
  int end_secs_from_midnight_ = 24 * 60 * 60;
  ParseCommandLineParams(argc, (const char**)argv, exchange_src_input_, file_name_to_act_, begin_secs_from_midnight_,
                         end_secs_from_midnight_);

  HFSAT::BulkFileReader bulk_file_reader_;

  int traded_volume_ = 0;

  HFSAT::ExchSource_t exchange_src_ = HFSAT::kExchSourceInvalid;
  if (exchange_src_input_.compare("CME") == 0) {
    exchange_src_ = HFSAT::kExchSourceCME;
  } else if (exchange_src_input_.compare("NTP") == 0) {
    exchange_src_ = HFSAT::kExchSourceNTP;
  } else if (exchange_src_input_.compare("LIFFE") == 0) {
    exchange_src_ = HFSAT::kExchSourceLIFFE;
  } else if (exchange_src_input_.compare("EUREX") == 0) {
    exchange_src_ = HFSAT::kExchSourceEUREX;
  } else if (exchange_src_input_.compare("HKEX") == 0) {
    exchange_src_ = HFSAT::kExchSourceHONGKONG;
  } else if (exchange_src_input_.compare("OSE") == 0) {
    exchange_src_ = HFSAT::kExchSourceJPY;
  } else if (exchange_src_input_.compare("MICEX") == 0) {
    exchange_src_ = HFSAT::kExchSourceMICEX;
  } else if (exchange_src_input_.compare("RTS") == 0) {
    exchange_src_ = HFSAT::kExchSourceRTS;
  } else if (exchange_src_input_.compare("CHIX") == 0) {
    exchange_src_ = HFSAT::kExchSourceBATSCHI;
  } else if (exchange_src_input_.compare("CFE") == 0) {
    exchange_src_ = HFSAT::kExchSourceCFE;
  } else if (exchange_src_input_.compare("QUINCY") == 0) {
    exchange_src_ = HFSAT::kExchSourceQUINCY;
  }

  std::ostringstream file_stream;
  file_stream << file_name_to_act_;
  std::string t_generic_filename_ = file_stream.str();
  HFSAT::MDS_MSG::GenericMDSMessage next_event_;

  bulk_file_reader_.open(t_generic_filename_.c_str());

  if (!bulk_file_reader_.is_open()) {
    traded_volume_ = -1;
  }

  while (true) {
    size_t available_len = bulk_file_reader_.read(&next_event_, sizeof(HFSAT::MDS_MSG::GenericMDSMessage));

    if (available_len < sizeof(HFSAT::MDS_MSG::GenericMDSMessage)) break;

    if (next_event_.time_.tv_sec % 86400 < begin_secs_from_midnight_ &&
        next_event_.time_.tv_sec % 86400 >= end_secs_from_midnight_)
      continue;

    // Exit from this point
    if (next_event_.GetExchangeSourceFromGenericStruct() != exchange_src_) {
      return 0;
    }

    switch (exchange_src_) {
      case HFSAT::kExchSourceCME: {
        if (CME_MDS::CME_TRADE == next_event_.generic_data_.cme_data_.msg_)
          traded_volume_ += next_event_.generic_data_.cme_data_.data_.cme_trds_.trd_qty_;

      } break;

      case HFSAT::kExchSourceEOBI:
      case HFSAT::kExchSourceEUREX: {
        if (EUREX_MDS::EUREX_TRADE == next_event_.generic_data_.eurex_data_.msg_)
          traded_volume_ += next_event_.generic_data_.eurex_data_.data_.eurex_trds_.trd_qty_;

      } break;

      case HFSAT::kExchSourceNTP: {
        if (NTP_MDS::NTP_TRADE == next_event_.generic_data_.ntp_data_.msg_)
          traded_volume_ += next_event_.generic_data_.ntp_data_.data_.ntp_trds_.trd_qty_;

      } break;

      case HFSAT::kExchSourceLIFFE: {
        if (LIFFE_MDS::LIFFE_TRADE == next_event_.generic_data_.liffe_data_.msg_)
          traded_volume_ += next_event_.generic_data_.liffe_data_.data_.liffe_trds_.trd_qty_;

      } break;

      case HFSAT::kExchSourceHONGKONG: {
        if (HKEX_MDS::HKEX_TRADE == next_event_.generic_data_.hkex_data_.msg_)
          traded_volume_ += next_event_.generic_data_.hkex_data_.data_.hkex_trds_.trd_qty_;

      } break;

      case HFSAT::kExchSourceJPY: {
        if (OSE_MDS::FEED_TRADE == next_event_.generic_data_.ose_cf_data_.feed_msg_type_)
          traded_volume_ += next_event_.generic_data_.ose_cf_data_.agg_size_;

      } break;

      case HFSAT::kExchSourceMICEX: {
        if (MICEX_MDS::MICEX_TRADE == next_event_.generic_data_.micex_data_.msg_)
          traded_volume_ += next_event_.generic_data_.micex_data_.data_.micex_trds_.trd_qty_;

      } break;

      case HFSAT::kExchSourceRTS: {
        if (RTS_MDS::RTS_TRADE == next_event_.generic_data_.rts_data_.msg_)
          traded_volume_ += next_event_.generic_data_.rts_data_.data_.rts_trds_.trd_qty_;

      } break;

      case HFSAT::kExchSourceCFE: {
        if (CSM_MDS::CSM_TRADE == next_event_.generic_data_.csm_data_.msg_ &&
            'S' != next_event_.generic_data_.csm_data_.data_.csm_trds_.trade_condition[0])
          traded_volume_ += next_event_.generic_data_.csm_data_.data_.csm_trds_.trd_qty_;

      } break;

      default: { } break; }
  }

  std::cout << file_name_to_act_ << "\t" << traded_volume_ << std::endl;

  return 0;
}
