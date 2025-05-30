/**
    \file Tools/get_volume_on_day.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717

*/

#include <iostream>
#include <stdlib.h>
#include <sstream>
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

#include "baseinfra/LoggedSources/cme_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/eurex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/tmx_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/bmf_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ntp_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/liffe_logged_message_filenamer.hpp"

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
  HFSAT::DebugLogger dbglogger_(1024000, 1);
  // setup DebugLogger
  {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "/spare/local/logs/alllogs/all_volume__log." << file_name_to_act_ << "."
                << HFSAT::DateTime::GetCurrentIsoDateLocal();
    std::string logfilename_ = t_temp_oss_.str();
    dbglogger_.OpenLogFile(logfilename_.c_str(), std::ofstream::out);
  }

  // std::string yyyy = input_date_.substr(0,4);
  // std::string mm = input_date_.substr(4,2);
  // std::string dd = input_date_.substr(6,2);
  HFSAT::ExchSource_t exchange_src_ = HFSAT::kExchSourceInvalid;
  if (exchange_src_input_.compare("CHI") == 0) {
    exchange_src_ = HFSAT::kExchSourceCME;
  } else if (exchange_src_input_.compare("BMF") == 0) {
    exchange_src_ = HFSAT::kExchSourceBMF;
  } else if (exchange_src_input_.compare("NTP") == 0) {
    exchange_src_ = HFSAT::kExchSourceNTP;
  } else if (exchange_src_input_.compare("LIFFE") == 0) {
    exchange_src_ = HFSAT::kExchSourceLIFFE;
  }

  else if (exchange_src_input_.compare("FR2") == 0) {
    exchange_src_ = HFSAT::kExchSourceEUREX;
  }

  else if (exchange_src_input_.compare("TOR") == 0) {
    exchange_src_ = HFSAT::kExchSourceTMX;
  }

  switch (exchange_src_) {
    case HFSAT::kExchSourceCME: {
      std::ostringstream file_stream;
      file_stream << file_name_to_act_;
      std::string t_cme_filename_ = file_stream.str();
      CME_MDS::CMECommonStruct next_event_;
      bulk_file_reader_.open(t_cme_filename_);

      if (!bulk_file_reader_.is_open())
        traded_volume_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(CME_MDS::CMECommonStruct));
          if (available_len_ >= sizeof(CME_MDS::CMECommonStruct)) {  // data not found in file

            if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
              dbglogger_ << next_event_.ToString();
              dbglogger_.DumpCurrentBuffer();
            }
            if (next_event_.msg_ == CME_MDS::CME_TRADE) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                traded_volume_ += next_event_.data_.cme_trds_.trd_qty_;
              }
            }
          } else
            break;
        }
    } break;
    case HFSAT::kExchSourceEUREX: {
      std::ostringstream file_stream;

      file_stream << file_name_to_act_;
      std::string t_eurex_filename_ = file_stream.str();

      EUREX_MDS::EUREXCommonStruct next_event_;
      bulk_file_reader_.open(t_eurex_filename_);

      if (!bulk_file_reader_.is_open())
        traded_volume_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(EUREX_MDS::EUREXCommonStruct));
          if (available_len_ >= sizeof(EUREX_MDS::EUREXCommonStruct)) {  // data not found in file

            if (next_event_.msg_ == EUREX_MDS::EUREX_TRADE) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                traded_volume_ += next_event_.data_.eurex_trds_.trd_qty_;
              }
            }
          } else
            break;
        }
    } break;
    case HFSAT::kExchSourceTMX: {
      std::ostringstream file_stream;

      file_stream << file_name_to_act_;
      std::string t_tmx_filename_ = file_stream.str();

      TMX_MDS::TMXCommonStruct next_event_;
      bulk_file_reader_.open(t_tmx_filename_);

      if (!bulk_file_reader_.is_open())
        traded_volume_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(TMX_MDS::TMXCommonStruct));
          if (available_len_ >= sizeof(TMX_MDS::TMXCommonStruct)) {  // data not found in file

            if (next_event_.msg_ == TMX_MDS::TMX_TRADE) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                traded_volume_ += next_event_.data_.tmx_trds_.trd_qty_;
              }
            }
          } else
            break;
        }
    } break;
    case HFSAT::kExchSourceBMF: {
      std::ostringstream file_stream;

      file_stream << file_name_to_act_;
      std::string t_cme_filename_ = file_stream.str();

      BMF_MDS::BMFCommonStruct next_event_;
      bulk_file_reader_.open(t_cme_filename_);

      if (!bulk_file_reader_.is_open())
        traded_volume_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(BMF_MDS::BMFCommonStruct));
          if (available_len_ >= sizeof(BMF_MDS::BMFCommonStruct)) {  // data not found in file

            if (next_event_.msg_ == BMF_MDS::BMF_TRADE) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                traded_volume_ += next_event_.data_.bmf_trds_.trd_qty_;
              }
            }
          } else
            break;
        }
    } break;
    case HFSAT::kExchSourceNTP: {
      std::ostringstream file_stream;

      file_stream << file_name_to_act_;
      std::string t_cme_filename_ = file_stream.str();

      NTP_MDS::NTPCommonStruct next_event_;
      bulk_file_reader_.open(t_cme_filename_);

      if (!bulk_file_reader_.is_open())
        traded_volume_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(NTP_MDS::NTPCommonStruct));
          if (available_len_ >= sizeof(NTP_MDS::NTPCommonStruct)) {  // data not found in file

            if (next_event_.msg_ == NTP_MDS::NTP_TRADE) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                traded_volume_ += next_event_.data_.ntp_trds_.trd_qty_;
              }
            }
          } else
            break;
        }
    } break;

    case HFSAT::kExchSourceLIFFE: {
      std::ostringstream file_stream;

      file_stream << file_name_to_act_;
      std::string t_cme_filename_ = file_stream.str();

      LIFFE_MDS::LIFFECommonStruct next_event_;
      bulk_file_reader_.open(t_cme_filename_);

      if (!bulk_file_reader_.is_open())
        traded_volume_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(LIFFE_MDS::LIFFECommonStruct));
          if (available_len_ >= sizeof(LIFFE_MDS::LIFFECommonStruct)) {  // data not found in file

            if (next_event_.msg_ == LIFFE_MDS::LIFFE_TRADE) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                traded_volume_ += next_event_.data_.liffe_trds_.trd_qty_;
              }
            }
          } else
            break;
        }
    } break;

    default: { } break; }

  std::cout << file_name_to_act_ << "\t" << traded_volume_ << std::endl;
}
