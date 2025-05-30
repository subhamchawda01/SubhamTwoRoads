/**
    \file Tools/get_avg_l1sz_on_day.cpp

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
#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/Utils/signals.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "baseinfra/LoggedSources/eobi_price_feed_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/cme_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/liffe_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/eurex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/tmx_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ntp_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/hkex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/hkomd_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ose_l1_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/rts_logged_message_filenamer.hpp"
#include "baseinfra/MarketAdapter/book_init_utils.hpp"

// To be used in termination handler
std::string global_shortcode_ = "";
int global_input_date_ = 0;
int global_begin_secs_from_midnight_ = 0;
int global_end_secs_from_midnight_ = 0;

void termination_handler(int signum) {
  if (signum == SIGTERM || signum == SIGFPE) {  // On a segfault inform , so this is picked up and fixed.
    char hostname_[128];
    hostname_[127] = '\0';
    gethostname(hostname_, 127);

    std::string email_string_ = "";
    std::string email_address_ = "";
    {
      std::ostringstream t_oss_;

      t_oss_ << "get_avg_l1sz_on_day received " << SimpleSignalString(signum) << " on " << hostname_ << "\n";
      t_oss_ << "shortcode_= " << global_shortcode_ << "\n"
             << "input_date_= " << global_input_date_ << "\n"
             << "begin_secs_from_midnight_= " << global_begin_secs_from_midnight_ << "\n"
             << "end_secs_from_midnight_= " << global_end_secs_from_midnight_ << "\n\n";

      email_string_ = t_oss_.str();
    }

    HFSAT::Email email_;
    email_.setSubject(email_string_);

    {  // Not sure if others want to receive these emails.
      email_address_ = "nseall@tworoads.co.in";
    }

    email_.addRecepient(email_address_);
    email_.addSender(email_address_);
    email_.content_stream << email_string_ << "<br/>";

    email_.sendMail();

    abort();
  }

  exit(0);
}

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
      begin_secs_from_midnight_ = std::max(1, (atoi(argv[3]) / 100) * 60 * 60 + (atoi(argv[3]) % 100) * 60);
    }
    if (argc > 4) {
      end_secs_from_midnight_ = (atoi(argv[4]) / 100) * 60 * 60 + (atoi(argv[4]) % 100) * 60;
    }
  }
  global_shortcode_ = shortcode_;
  global_input_date_ = input_date_;
  global_begin_secs_from_midnight_ = begin_secs_from_midnight_;
  global_end_secs_from_midnight_ = end_secs_from_midnight_;
}

/// input arguments : input_date
int main(int argc, char** argv) {
  // signal ( SIGFPE , termination_handler );
  // signal ( SIGTERM , termination_handler );
  std::string shortcode_ = "";
  int input_date_ = 20110101;
  int begin_secs_from_midnight_ = 1;
  int end_secs_from_midnight_ = 24 * 60 * 60;
  ParseCommandLineParams(argc, (const char**)argv, shortcode_, input_date_, begin_secs_from_midnight_,
                         end_secs_from_midnight_);

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date_);
  const char* t_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);

  HFSAT::TradingLocation_t trading_location_file_read_;
  HFSAT::BulkFileReader bulk_file_reader_;

  long long count_ = 0;
  long long sum_l1_szs_ = 0;
  int bid_sz_ = 0;
  int ask_sz_ = 0;

  int last_sfm_ = 0;

  HFSAT::ExchSource_t exch_src_ = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, input_date_);
  trading_location_file_read_ =
      HFSAT::TradingLocationUtils::GetTradingLocationExch(exch_src_);  // initialize to primary location
  if ((exch_src_ == HFSAT::kExchSourceHONGKONG && trading_location_file_read_ == HFSAT::kTLocHK &&
       (input_date_ >= 20141204)) ||
      ((exch_src_ == HFSAT::kExchSourceHONGKONG) && (trading_location_file_read_ == HFSAT::kTLocJPY) &&
       (input_date_ >= 20150121))) {
    exch_src_ = HFSAT::kExchSourceHKOMDCPF;
  }

  switch (exch_src_) {
    case HFSAT::kExchSourceCME: {
      std::string t_cme_filename_ =
          HFSAT::CMELoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);

      CME_MDS::CMECommonStruct next_event_;
      bulk_file_reader_.open(t_cme_filename_);

      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(CME_MDS::CMECommonStruct));
        if (available_len_ >= sizeof(CME_MDS::CMECommonStruct)) {  // data not found in file

          if (next_event_.msg_ == CME_MDS::CME_DELTA) {
            if (next_event_.data_.cme_dels_.level_ == 1) {
              int t_sfm_ = next_event_.time_.tv_sec % 86400;
              if (t_sfm_ > begin_secs_from_midnight_ && t_sfm_ < end_secs_from_midnight_ && t_sfm_ > last_sfm_) {
                if (next_event_.data_.cme_dels_.type_ == 0) {
                  bid_sz_ = next_event_.data_.cme_dels_.size_;
                } else {
                  ask_sz_ = next_event_.data_.cme_dels_.size_;
                }
                sum_l1_szs_ += (bid_sz_ + ask_sz_) / 2;
                count_++;
                last_sfm_ = t_sfm_;
              }
            }
          }
        } else
          break;
      }
    } break;

    case HFSAT::kExchSourceLIFFE: {
      std::string t_liffe_filename_ =
          HFSAT::LIFFELoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);
      LIFFE_MDS::LIFFECommonStruct next_event_;
      bulk_file_reader_.open(t_liffe_filename_);
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(LIFFE_MDS::LIFFECommonStruct));
        if (available_len_ >= sizeof(LIFFE_MDS::LIFFECommonStruct)) {  // data not found in file

          if (next_event_.msg_ == LIFFE_MDS::LIFFE_DELTA) {
            if (next_event_.data_.liffe_dels_.level_ == 1) {
              int t_sfm_ = next_event_.time_.tv_sec % 86400;
              if (t_sfm_ > begin_secs_from_midnight_ && t_sfm_ < end_secs_from_midnight_ && t_sfm_ > last_sfm_) {
                if (next_event_.data_.liffe_dels_.type_ == '1') {
                  bid_sz_ = next_event_.data_.liffe_dels_.size_;
                } else if (next_event_.data_.liffe_dels_.type_ == '2') {
                  ask_sz_ = next_event_.data_.liffe_dels_.size_;
                }
                sum_l1_szs_ += (bid_sz_ + ask_sz_) / 2;
                count_++;
                last_sfm_ = t_sfm_;
              }
            }
          }
        } else
          break;
      }

    } break;

    case HFSAT::kExchSourceEUREX: {
      std::string t_eurex_filename_ = "";

      if (HFSAT::UseEOBIData(trading_location_file_read_, input_date_, shortcode_)) {
        t_eurex_filename_ = HFSAT::EOBIPriceFeedLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_,
                                                                                trading_location_file_read_);
      } else {
        t_eurex_filename_ =
            HFSAT::EUREXLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);
      }
      EUREX_MDS::EUREXCommonStruct next_event_;
      bulk_file_reader_.open(t_eurex_filename_);

      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(EUREX_MDS::EUREXCommonStruct));
        if (available_len_ >= sizeof(EUREX_MDS::EUREXCommonStruct)) {  // data not found in file

          if (next_event_.msg_ == EUREX_MDS::EUREX_DELTA) {
            if (next_event_.data_.eurex_dels_.level_ == 1) {
              int t_sfm_ = next_event_.time_.tv_sec % 86400;
              if (t_sfm_ > begin_secs_from_midnight_ && t_sfm_ < end_secs_from_midnight_ && t_sfm_ > last_sfm_) {
                if (next_event_.data_.eurex_dels_.type_ == 2) {
                  bid_sz_ = next_event_.data_.eurex_dels_.size_;
                } else {
                  ask_sz_ = next_event_.data_.eurex_dels_.size_;
                }
                sum_l1_szs_ += (bid_sz_ + ask_sz_) / 2;
                count_++;
                last_sfm_ = t_sfm_;
              }
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

      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(TMX_MDS::TMXCommonStruct));
        if (available_len_ >= sizeof(TMX_MDS::TMXCommonStruct)) {  // data not found in file

          if (next_event_.msg_ == TMX_MDS::TMX_BOOK) {
            int t_sfm_ = next_event_.time_.tv_sec % 86400;
            if (t_sfm_ > begin_secs_from_midnight_ && t_sfm_ < end_secs_from_midnight_ && t_sfm_ > last_sfm_) {
              if (next_event_.data_.tmx_books_.bid_szs_[0] > 0 && next_event_.data_.tmx_books_.num_bid_ords_[0]) {
                bid_sz_ = next_event_.data_.tmx_books_.bid_szs_[0];
              } else if (next_event_.data_.tmx_books_.ask_szs_[0] > 0 &&
                         next_event_.data_.tmx_books_.num_ask_ords_[0]) {
                ask_sz_ = next_event_.data_.tmx_books_.ask_szs_[0];
              }
              sum_l1_szs_ += (bid_sz_ + ask_sz_) / 2;
              count_++;
              last_sfm_ = t_sfm_;
            }
          }
        } else
          break;
      }
    } break;
    case HFSAT::kExchSourceBMF: {
      std::string t_cme_filename_ =
          HFSAT::NTPLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);

      NTP_MDS::NTPCommonStruct next_event_;
      bulk_file_reader_.open(t_cme_filename_);

      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(NTP_MDS::NTPCommonStruct));
        if (available_len_ >= sizeof(NTP_MDS::NTPCommonStruct)) {
          if (next_event_.msg_ == NTP_MDS::NTP_DELTA) {
            if (next_event_.data_.ntp_dels_.level_ == 1) {
              int t_sfm_ = next_event_.time_.tv_sec % 86400;
              if (t_sfm_ > begin_secs_from_midnight_ && t_sfm_ < end_secs_from_midnight_ && t_sfm_ > last_sfm_) {
                if (next_event_.data_.ntp_dels_.type_ == 0) {
                  bid_sz_ = next_event_.data_.ntp_dels_.size_;
                } else {
                  ask_sz_ = next_event_.data_.ntp_dels_.size_;
                }
                sum_l1_szs_ += (bid_sz_ + ask_sz_) / 2;
                count_++;
                last_sfm_ = t_sfm_;
              }
            }
          }
        } else
          break;
      }
    } break;
    case HFSAT::kExchSourceHONGKONG: {
      std::string t_hk_filename_ =
          HFSAT::HKEXLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);
      HKEX_MDS::HKEXCommonStruct next_event_;
      bulk_file_reader_.open(t_hk_filename_);
      if (!bulk_file_reader_.is_open()) {
        sum_l1_szs_ = -1;
        count_ = 1;
      } else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HKEX_MDS::HKEXCommonStruct));
          if (available_len_ >= sizeof(HKEX_MDS::HKEXCommonStruct)) {  // data not found in file

            if (next_event_.msg_ == HKEX_MDS::HKEX_BOOK) {
              int t_sfm_ = next_event_.time_.tv_sec % 86400;
              if (t_sfm_ > begin_secs_from_midnight_ && t_sfm_ < end_secs_from_midnight_ && t_sfm_ > last_sfm_) {
                sum_l1_szs_ += next_event_.data_.hkex_books_.demand_[0];
                count_++;
                last_sfm_ = t_sfm_;
              }
            }
          } else
            break;
        }
    } break;
    case HFSAT::kExchSourceHKOMDCPF:
    case HFSAT::kExchSourceHKOMDPF: {
      std::string t_hk_filename_ =
          HFSAT::HKOMDCPFLoggedMessagefileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);
      HKOMD_MDS::HKOMDPFCommonStruct next_event_;
      bulk_file_reader_.open(t_hk_filename_);
      if (!bulk_file_reader_.is_open()) {
        sum_l1_szs_ = -1;
        count_ = 1;
      } else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HKOMD_MDS::HKOMDPFCommonStruct));
          if (available_len_ >= sizeof(HKOMD_MDS::HKOMDPFCommonStruct)) {  // data not found in file

            if (next_event_.msg_ == HKOMD_MDS::HKOMD_PF_DELTA) {
              if (next_event_.data_.delta_.level_ == 1) {
                int t_sfm_ = next_event_.time_.tv_sec % 86400;
                if (t_sfm_ > begin_secs_from_midnight_ && t_sfm_ < end_secs_from_midnight_ && t_sfm_ > last_sfm_) {
                  sum_l1_szs_ += next_event_.data_.delta_.quantity_;
                  count_++;
                  last_sfm_ = t_sfm_;
                }
              }
            }
          } else
            break;
        }
    } break;
    case HFSAT::kExchSourceJPY: {
      std::string t_ose_filename_ =
          HFSAT::OSEL1LoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);
      OSE_MDS::OSEPLCommonStruct next_event_;
      bulk_file_reader_.open(t_ose_filename_);
      if (!bulk_file_reader_.is_open()) {
        sum_l1_szs_ = -1;
        count_ = 1;
      } else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(OSE_MDS::OSEPLCommonStruct));
          if (available_len_ >= sizeof(OSE_MDS::OSEPLCommonStruct)) {
            if (next_event_.get_buy_sell_trade() != OSE_MDS::kL1TRADE) {
              int t_sfm_ = next_event_.time_.tv_sec % 86400;
              if (t_sfm_ > begin_secs_from_midnight_ && t_sfm_ < end_secs_from_midnight_ && t_sfm_ > last_sfm_) {
                sum_l1_szs_ += next_event_.size;
                count_++;
                last_sfm_ = t_sfm_;
              }
            }

          } else
            break;
        }
    } break;
    case HFSAT::kExchSourceRTS: {
      std::string t_rts_filename_ =
          HFSAT::RTSLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);

      RTS_MDS::RTSCommonStruct next_event_;
      bulk_file_reader_.open(t_rts_filename_);

      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(RTS_MDS::RTSCommonStruct));
        if (available_len_ >= sizeof(RTS_MDS::RTSCommonStruct)) {  // data not found in file

          if (next_event_.msg_ == RTS_MDS::RTS_DELTA) {
            if (next_event_.data_.rts_dels_.level_ == 1) {
              int t_sfm_ = next_event_.time_.tv_sec % 86400;
              if (t_sfm_ > begin_secs_from_midnight_ && t_sfm_ < end_secs_from_midnight_ && t_sfm_ > last_sfm_) {
                if (next_event_.data_.rts_dels_.type_ == '0') {
                  bid_sz_ = next_event_.data_.rts_dels_.size_;
                } else {
                  ask_sz_ = next_event_.data_.rts_dels_.size_;
                }
                sum_l1_szs_ += (bid_sz_ + ask_sz_) / 2;
                count_++;
                last_sfm_ = t_sfm_;
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
  std::cout << shortcode_ << ' ' << print_secname_ << ' ' << (sum_l1_szs_ / count_) << std::endl;
}
