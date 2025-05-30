/**
    \file Tools/get_vol_on_day.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717

 */

#include <iostream>
#include <stdlib.h>

#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/chix_mds_defines.hpp"
#include "dvccode/CDef/nasdaq_mds_defines.hpp"
#include "baseinfra/LoggedSources/chix_l1_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/cme_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/eurex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/tmx_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/bmf_logged_message_filenamer.hpp"
#include "baseinfra/MarketAdapter/book_init_utils.hpp"

void ParseCommandLineParams(const int argc, const char** argv, std::string& shortcode_, int& input_date_,
                            int& begin_secs_from_midnight_, int& end_secs_from_midnight_) {
  // expect :
  // 1. $0 shortcode date_YYYYMMDD [ start_tm HHMM ] [ end_tm HHMM ]
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " shortcode input_date_YYYYMMDD [ start_tm_utc_hhmm ] [ end_tm_utc_hhmm ]"
              << std::endl;
    exit(0);
  } else {
    shortcode_ = argv[1];
    input_date_ = atoi(argv[2]);

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
  std::string shortcode_ = "";
  int input_date_ = 20110101;
  int begin_secs_from_midnight_ = 0;
  int end_secs_from_midnight_ = 24 * 60 * 60;
  ParseCommandLineParams(argc, (const char**)argv, shortcode_, input_date_, begin_secs_from_midnight_,
                         end_secs_from_midnight_);

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date_);
  const char* t_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);

  HFSAT::TradingLocation_t trading_location_file_read_;
  HFSAT::BulkFileReader bulk_file_reader_;

  int num_trades_ = 0;

  HFSAT::ExchSource_t exch_src_ = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_);
  trading_location_file_read_ =
      HFSAT::TradingLocationUtils::GetTradingLocationExch(exch_src_);  // initialize to primary location

  switch (exch_src_) {
    case HFSAT::kExchSourceCME: {
      std::string t_cme_filename_ =
          HFSAT::CMELoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);

      CME_MDS::CMECommonStruct next_event_;
      bulk_file_reader_.open(t_cme_filename_);

      if (!bulk_file_reader_.is_open())
        num_trades_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(CME_MDS::CMECommonStruct));
          if (available_len_ >= sizeof(CME_MDS::CMECommonStruct)) {  // data not found in file

            if (next_event_.msg_ == CME_MDS::CME_TRADE) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                num_trades_++;
              }
            }
          } else
            break;
        }
    } break;
    case HFSAT::kExchSourceEUREX: {
      std::string t_eurex_filename_ =
          HFSAT::EUREXLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);
      if (HFSAT::UseEOBIData(trading_location_file_read_, input_date_, shortcode_)) {
        t_eurex_filename_ = HFSAT::EOBIPriceFeedLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_,
                                                                                trading_location_file_read_);
      }

      EUREX_MDS::EUREXCommonStruct next_event_;
      bulk_file_reader_.open(t_eurex_filename_);

      if (!bulk_file_reader_.is_open())
        num_trades_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(EUREX_MDS::EUREXCommonStruct));
          if (available_len_ >= sizeof(EUREX_MDS::EUREXCommonStruct)) {  // data not found in file

            if (next_event_.msg_ == EUREX_MDS::EUREX_TRADE) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                num_trades_++;
              }
            }
          } else
            break;
        }
    } break;
    case HFSAT::kExchSourceTMX: {
      std::string t_tmx_filename_ =
          HFSAT::TMXLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);

      TMX_MDS::TMXCommonStruct next_event_;
      bulk_file_reader_.open(t_tmx_filename_);

      if (!bulk_file_reader_.is_open())
        num_trades_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(TMX_MDS::TMXCommonStruct));
          if (available_len_ >= sizeof(TMX_MDS::TMXCommonStruct)) {  // data not found in file

            if (next_event_.msg_ == TMX_MDS::TMX_TRADE) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                num_trades_++;
              }
            }
          } else
            break;
        }
    } break;
    case HFSAT::kExchSourceBMF: {
      std::string t_cme_filename_ =
          HFSAT::BMFLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);

      BMF_MDS::BMFCommonStruct next_event_;
      bulk_file_reader_.open(t_cme_filename_);

      if (!bulk_file_reader_.is_open())
        num_trades_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(BMF_MDS::BMFCommonStruct));
          if (available_len_ >= sizeof(BMF_MDS::BMFCommonStruct)) {  // data not found in file

            if (next_event_.msg_ == BMF_MDS::BMF_TRADE) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                num_trades_++;
              }
            }
          } else
            break;
        }
    } break;
    case HFSAT::kExchSourceNASDAQ: {
      std::string t_nasdaq_filename_ =
          HFSAT::NASDAQLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);

      HFSAT::NASDAQ_MDS::NASDAQCommonStruct next_event_;
      bulk_file_reader_.open(t_nasdaq_filename_);

      if (!bulk_file_reader_.is_open())
        num_trades_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HFSAT::NASDAQ_MDS::NASDAQCommonStruct));
          if (available_len_ >= sizeof(HFSAT::NASDAQ_MDS::NASDAQCommonStruct)) {  // data not found in file
            bool intermediate = next_event_.nts_.msgType_ >= 16;
            if (intermediate) next_event_.nts_.msgType_ = next_event_.nts_.msgType_ - 16;
            if (next_event_.nts_.msgType_ == HFSAT::NASDAQ_MDS::EXEC) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                num_trades_++;
              }
            }
          } else
            break;
        }
    } break;
    case HFSAT::kExchSourceBATSCHI: {
      std::string t_chix_filename_ =
          HFSAT::CHIXLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);
      HFSAT::BATSCHI_MDS::BATSCHICommonStruct next_event_;
      bulk_file_reader_.open(t_chix_filename_);
      if (!bulk_file_reader_.is_open())
        num_trades_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HFSAT::BATSCHI_MDS::BATSCHICommonStruct));
          if (available_len_ >= sizeof(HFSAT::BATSCHI_MDS::BATSCHICommonStruct)) {  // data not found in file
            if (next_event_.msg_ == HFSAT::OrderExecchiX || next_event_.msg_ == HFSAT::OrderExecWithPriceSize) {
              {
                if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                    next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                  num_trades_++;
                }
              }
            } else
              break;
          }
        }
    } break;
    case HFSAT::kExchSourceHONGKONG: {
      std::string t_hkex_filename_ =
          HFSAT::HKEXLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);

      HKEX_MDS::HKEXCommonStruct next_event_;
      bulk_file_reader_.open(t_hkex_filename_);

      if (!bulk_file_reader_.is_open())
        num_trades_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HKEX_MDS::HKEXCommonStruct));
          if (available_len_ >= sizeof(HKEX_MDS::HKEXCommonStruct)) {  // data not found in file
            if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
              if (next_event_.msg_ == HKEX_MDS::HKEX_TRADE) {
                num_trades_++;
              }
            }

          } else
            break;
        }
    } break;
    case HFSAT::kExchSourceJPY: {
      std::string t_ose_filename_ =
          HFSAT::OSELoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);
      OSE_MDS::OSECommonStruct next_event_;
      bulk_file_reader_.open(t_ose_filename_);

      if (!bulk_file_reader_.is_open())
        num_trades_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HFSAT::BATSCHI_MDS::BATSCHICommonStruct));
          if (available_len_ >= sizeof(HFSAT::BATSCHI_MDS::BATSCHICommonStruct)) {
            if (next_event_.msg_ == OSE_MDS::OSE_TRADE) {
              {
                if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                    next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                  num_trades_++;
                }
              }
            }
          } else
            break;
        }
    } break;
    default: { } break; }

  char print_secname_[24] = {0};
  strcpy(print_secname_, t_exchange_symbol_);
  for (size_t i = 0; i < strlen(print_secname_); ++i) {
    if (print_secname_[i] == ' ') {  // Liffe symbols have space, which is bad for the post processing script
      print_secname_[i] = '~';
    }
  }
  std::cout << shortcode_ << ' ' << print_secname_ << ' ' << num_trades_ << std::endl;
}
