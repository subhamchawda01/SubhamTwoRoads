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

#include "baseinfra/LoggedSources/cme_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/eurex_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/ose_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/tmx_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/bmf_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/ntp_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/liffe_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/hkex_logged_message_filesource.hpp"

void ParseCommandLineParams(const int argc, const char** argv, std::string& exchange_src_input_,
                            std::string& file_name_, int& begin_secs_from_midnight_, int& end_secs_from_midnight_,
                            bool& compute_notional_) {
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
    if (argc > 5) {
      compute_notional_ = (atoi(argv[5]) != 0);
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
  bool compute_notional_ = false;
  ParseCommandLineParams(argc, (const char**)argv, exchange_src_input_, file_name_to_act_, begin_secs_from_midnight_,
                         end_secs_from_midnight_, compute_notional_);

  HFSAT::BulkFileReader bulk_file_reader_;

  int64_t traded_volume_ = 0;
  int64_t notional_ = 0;
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
  } else if (exchange_src_input_.compare("PUMA") == 0 || exchange_src_input_.compare("BMFEQ") == 0) {
    exchange_src_ = HFSAT::kExchSourceBMFEQ;
  } else if (exchange_src_input_.compare("BSL") == 0) {
    exchange_src_ = HFSAT::kExchSourceLIFFE;
  } else if (exchange_src_input_.compare("NSE") == 0) {
    exchange_src_ = HFSAT::kExchSourceNSE;
  }

  else if (exchange_src_input_.compare("FR2") == 0) {
    if (file_name_to_act_.find("EOBI") == std::string::npos) {
      exchange_src_ = HFSAT::kExchSourceEUREX;
    } else {
      exchange_src_ = HFSAT::kExchSourceEOBI;
    }
  }

  else if (exchange_src_input_.compare("TOR") == 0) {
    exchange_src_ = HFSAT::kExchSourceTMX;
  }

  else if (exchange_src_input_.compare("HK") == 0) {
    exchange_src_ = HFSAT::kExchSourceHONGKONG;
  }

  else if (exchange_src_input_.compare("TOK") == 0) {
    exchange_src_ = HFSAT::kExchSourceJPY;
  } else if (exchange_src_input_.compare("MICEX") == 0) {
    exchange_src_ = HFSAT::kExchSourceMICEX;
  } else if (exchange_src_input_.compare("RTS") == 0) {
    exchange_src_ = HFSAT::kExchSourceRTS;
  } else if (exchange_src_input_.compare("CHIX") == 0) {
    exchange_src_ = HFSAT::kExchSourceBATSCHI;
  } else if (exchange_src_input_.compare("CRT") == 0) {
    exchange_src_ = HFSAT::kExchSourceNASDAQ;
  } else if (exchange_src_input_.compare("CFE") == 0) {
    exchange_src_ = HFSAT::kExchSourceCFE;
  } else if (exchange_src_input_.compare("ESPEED") == 0) {
    exchange_src_ = HFSAT::kExchSourceESPEED;
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

            if (next_event_.msg_ == CME_MDS::CME_TRADE) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                if (compute_notional_) {
                  notional_ += next_event_.data_.cme_trds_.trd_qty_ * next_event_.data_.cme_trds_.trd_px_;
                } else {
                  traded_volume_ += next_event_.data_.cme_trds_.trd_qty_;
                }
              }
            }
          } else
            break;
        }
    } break;
    case HFSAT::kExchSourceEOBI: {
      std::ostringstream file_stream;

      file_stream << file_name_to_act_;
      std::string t_eobi_filename_ = file_stream.str();

      EOBI_MDS::EOBICommonStruct next_event_;
      bulk_file_reader_.open(t_eobi_filename_);

      if (!bulk_file_reader_.is_open()) {
        traded_volume_ = -1;
      } else {
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(EOBI_MDS::EOBICommonStruct));
          if (available_len_ >= sizeof(EOBI_MDS::EOBICommonStruct)) {  // data not found in file

            if (next_event_.data_.order_.action_ == '6') {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                if (compute_notional_) {
                  notional_ += next_event_.data_.order_.size * next_event_.data_.order_.price;
                } else {
                  traded_volume_ += next_event_.data_.order_.size;
                }
              }
            }
          } else
            break;
        }
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
                if (compute_notional_) {
                  notional_ += next_event_.data_.eurex_trds_.trd_qty_ * next_event_.data_.eurex_trds_.trd_px_;
                } else {
                  traded_volume_ += next_event_.data_.eurex_trds_.trd_qty_;
                }
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
                if (compute_notional_) {
                  notional_ += next_event_.data_.tmx_trds_.trd_qty_ * next_event_.data_.tmx_trds_.trd_qty_;
                } else {
                  traded_volume_ += next_event_.data_.tmx_trds_.trd_qty_;
                }
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
                if (compute_notional_) {
                  notional_ += next_event_.data_.bmf_trds_.trd_qty_ * next_event_.data_.bmf_trds_.trd_px_;
                } else {
                  traded_volume_ += next_event_.data_.bmf_trds_.trd_qty_;
                }
              }
            }
          } else
            break;
        }
    } break;
    case HFSAT::kExchSourceBMFEQ:
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
                if (compute_notional_) {
                  notional_ += next_event_.data_.ntp_trds_.trd_qty_ * next_event_.data_.ntp_trds_.trd_px_;
                } else {
                  traded_volume_ += next_event_.data_.ntp_trds_.trd_qty_;
                }
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

    case HFSAT::kExchSourceHONGKONG: {
      std::ostringstream file_stream;

      file_stream << file_name_to_act_;
      std::string t_hkex_filename_ = file_stream.str();

      HKEX_MDS::HKEXCommonStruct next_event_;
      bulk_file_reader_.open(t_hkex_filename_);

      if (!bulk_file_reader_.is_open())
        traded_volume_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HKEX_MDS::HKEXCommonStruct));
          if (available_len_ >= sizeof(HKEX_MDS::HKEXCommonStruct)) {  // data not found in file

            if (next_event_.msg_ == HKEX_MDS::HKEX_TRADE) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                traded_volume_ += next_event_.data_.hkex_trds_.trd_qty_;
              }
            }
          } else
            break;
        }
    } break;

    case HFSAT::kExchSourceJPY: {
      // Price Feed
      std::ostringstream file_stream;

      file_stream << file_name_to_act_;
      std::string t_ose_filename_ = file_stream.str();

      OSE_MDS::OSEPriceFeedCommonStruct next_event_;
      bulk_file_reader_.open(t_ose_filename_);

      if (!bulk_file_reader_.is_open())
        traded_volume_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(OSE_MDS::OSEPriceFeedCommonStruct));
          if (available_len_ >= sizeof(OSE_MDS::OSEPriceFeedCommonStruct)) {  // data not found in file

            if ((uint8_t)next_event_.type_ == 2) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                traded_volume_ += next_event_.size;
              }
            }
          } else
            break;
        }

    } break;

    case HFSAT::kExchSourceMICEX: {
      std::ostringstream file_stream;
      file_stream << file_name_to_act_;
      std::string t_micex_filename_ = file_stream.str();
      MICEX_MDS::MICEXCommonStruct next_event_;
      bulk_file_reader_.open(t_micex_filename_);

      if (!bulk_file_reader_.is_open())
        traded_volume_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(MICEX_MDS::MICEXCommonStruct));
          if (available_len_ >= sizeof(MICEX_MDS::MICEXCommonStruct)) {  // data not found in file

            if (next_event_.msg_ == MICEX_MDS::MICEX_TRADE) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                if (compute_notional_) {
                  notional_ += next_event_.data_.micex_trds_.trd_qty_ * next_event_.data_.micex_trds_.trd_px_;
                } else {
                  traded_volume_ += next_event_.data_.micex_trds_.trd_qty_;
                }
              }
            }
          } else
            break;
        }
    } break;

    case HFSAT::kExchSourceRTS: {
      std::ostringstream file_stream;
      file_stream << file_name_to_act_;
      std::string t_rts_filename_ = file_stream.str();
      RTS_MDS::RTSCommonStruct next_event_;
      bulk_file_reader_.open(t_rts_filename_);

      if (!bulk_file_reader_.is_open())
        traded_volume_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(RTS_MDS::RTSCommonStruct));
          if (available_len_ >= sizeof(RTS_MDS::RTSCommonStruct)) {  // data not found in file

            if (next_event_.msg_ == RTS_MDS::RTS_TRADE) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                traded_volume_ += next_event_.data_.rts_trds_.trd_qty_;
              }
            }
          } else
            break;
        }
    } break;
    case HFSAT::kExchSourceBATSCHI: {
      std::ostringstream file_stream;
      file_stream << file_name_to_act_;
      std::string t_chix_filename_ = file_stream.str();
      HFSAT::BATSCHI_MDS::BATSCHICommonStruct next_event_;
      bulk_file_reader_.open(t_chix_filename_);

      if (!bulk_file_reader_.is_open())
        traded_volume_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HFSAT::BATSCHI_MDS::BATSCHICommonStruct));
          if (available_len_ >= sizeof(HFSAT::BATSCHI_MDS::BATSCHICommonStruct)) {  // data not found in file

            if (next_event_.msg_ == HFSAT::BATSCHI_MDS::BATSCHI_TRADE) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                traded_volume_ += next_event_.data_.batschi_trds_.trd_qty_;
              }
            }
          } else
            break;
        }
    } break;

    case HFSAT::kExchSourceCFE: {
      std::ostringstream file_stream;
      file_stream << file_name_to_act_;
      std::string t_cfe_filename_ = file_stream.str();
      CSM_MDS::CSMCommonStruct next_event_;
      bulk_file_reader_.open(t_cfe_filename_);

      // Hack to ignore spread trades in outright, while considering trades with same trade_condition for spread symbols
      bool is_spread_ = false;

      if (t_cfe_filename_.find("_") != std::string::npos) {
        size_t found_ = t_cfe_filename_.find("_");

        if (t_cfe_filename_.find("_", found_ + 1) != std::string::npos) {
          is_spread_ = true;
        }
      }

      if (!bulk_file_reader_.is_open())
        traded_volume_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(CSM_MDS::CSMCommonStruct));
          if (available_len_ >= sizeof(CSM_MDS::CSMCommonStruct)) {  // data not found in file

            if (next_event_.msg_ == CSM_MDS::CSM_TRADE &&
                (is_spread_ || next_event_.data_.csm_trds_.trade_condition[0] != 'S')) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                traded_volume_ += next_event_.data_.csm_trds_.trd_qty_;
              }
            }
          } else
            break;
        }
    } break;
    case HFSAT::kExchSourceNSE:
    case HFSAT::kExchSourceNSE_CD:
    case HFSAT::kExchSourceNSE_FO: {
      std::ostringstream file_stream;
      file_stream << file_name_to_act_;
      std::string t_nse_filename_ = file_stream.str();
      NSE_MDS::NSEDotexOfflineCommonStruct next_event_;
      bulk_file_reader_.open(t_nse_filename_);

      if (!bulk_file_reader_.is_open())
        traded_volume_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(NSE_MDS::NSEDotexOfflineCommonStruct));
          if (available_len_ >= sizeof(NSE_MDS::NSEDotexOfflineCommonStruct)) {  // data not found in file
            if (next_event_.msg_type == NSE_MDS::MsgType::kNSETrade) {
              if (next_event_.source_time.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.source_time.tv_sec % 86400 < end_secs_from_midnight_) {
                if (compute_notional_) {
                  traded_volume_ +=
                      (next_event_.data.nse_dotex_trade.trade_price * next_event_.data.nse_dotex_trade.trade_quantity);
                } else {
                  traded_volume_ += next_event_.data.nse_dotex_trade.trade_quantity;
                }
              }
            }
          } else
            break;
        }
    } break;
    default: { } break; }

  if (compute_notional_) {
    std::cout << file_name_to_act_ << "\t" << notional_ << std::endl;
  } else {
    std::cout << file_name_to_act_ << "\t" << traded_volume_ << std::endl;
  }
}
