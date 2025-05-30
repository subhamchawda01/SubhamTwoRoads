/**
   \file Tools/mds_logger.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
*/
#include <iostream>
#include <stdlib.h>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"

#include "baseinfra/LoggedSources/cme_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/eurex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ntp_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/liffe_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/tmx_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/hkex_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/hkomd_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/ose_l1_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/rts_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/micex_logged_message_filesource.hpp"

#include "dvccode/Utils/bulk_file_reader.hpp"

int start_time_ = 0;
int end_time_ = 0;

class CMEMDSLogReader {
 public:
  static void ReadMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_) {
    CME_MDS::CMECommonStruct next_event_;
    double bid_price_ = 0.0;
    double ask_price_ = 0.0;
    int bid_size_ = 0;
    double ask_size_ = 0;
    int bid_num_ords_ = 0;
    double ask_num_ords_ = 0;
    int endtime_ = 0;
    int starttime_ = 0;

    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(CME_MDS::CMECommonStruct));
        if (available_len_ < sizeof(next_event_)) break;
        HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;
        starttime_ = next_event_timestamp_.tv_sec;

        if (starttime_ < start_time_ || start_time_ > end_time_) continue;

        if (endtime_ == 0) endtime_ = starttime_ + 60;

        if (next_event_.msg_ == CME_MDS::CME_TRADE) {
          continue;
        }

        if (next_event_.data_.cme_dels_.level_ == 1) {
          bid_price_ = next_event_.data_.cme_dels_.type_ == '0' ? next_event_.data_.cme_dels_.price_ : bid_price_;
          bid_size_ = next_event_.data_.cme_dels_.type_ == '0' ? next_event_.data_.cme_dels_.size_ : bid_size_;
          bid_num_ords_ =
              next_event_.data_.cme_dels_.type_ == '0' ? next_event_.data_.cme_dels_.num_ords_ : bid_num_ords_;
          ask_price_ = next_event_.data_.cme_dels_.type_ == '1' ? next_event_.data_.cme_dels_.price_ : ask_price_;
          ask_size_ = next_event_.data_.cme_dels_.type_ == '1' ? next_event_.data_.cme_dels_.size_ : ask_size_;
          ask_num_ords_ =
              next_event_.data_.cme_dels_.type_ == '1' ? next_event_.data_.cme_dels_.num_ords_ : ask_num_ords_;

          if (starttime_ >= endtime_) {
            endtime_ = starttime_ + 60;
            if (bid_price_ > 0 && ask_price_ > 0)
              std::cout << starttime_ << " " << (double)(bid_price_ + ask_price_) / 2 << "\n";
          }
        }
      }
      bulk_file_reader_.close();
    }
  }
};

class EUREXMDSLogReader {
 public:
  static void ReadMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_) {
    EUREX_MDS::EUREXCommonStruct next_event_;
    double bid_price_ = 0.0;
    double ask_price_ = 0.0;
    int bid_size_ = 0;
    double ask_size_ = 0;
    int bid_num_ords_ = 0;
    double ask_num_ords_ = 0;
    int endtime_ = 0;
    int starttime_ = 0;

    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(EUREX_MDS::EUREXCommonStruct));
        if (available_len_ < sizeof(next_event_)) break;

        HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;
        starttime_ = next_event_timestamp_.tv_sec;
        if (starttime_ < start_time_ || start_time_ > end_time_) continue;

        if (endtime_ == 0) endtime_ = starttime_ + 60;

        if (next_event_.msg_ == EUREX_MDS::EUREX_TRADE) {
          continue;
        }

        if (next_event_.data_.eurex_dels_.level_ == 1) {
          bid_price_ = next_event_.data_.eurex_dels_.type_ == '2' ? next_event_.data_.eurex_dels_.price_ : bid_price_;
          bid_size_ = next_event_.data_.eurex_dels_.type_ == '2' ? next_event_.data_.eurex_dels_.size_ : bid_size_;
          bid_num_ords_ =
              next_event_.data_.eurex_dels_.type_ == '2' ? next_event_.data_.eurex_dels_.num_ords_ : bid_num_ords_;
          ask_price_ = next_event_.data_.eurex_dels_.type_ == '1' ? next_event_.data_.eurex_dels_.price_ : ask_price_;
          ask_size_ = next_event_.data_.eurex_dels_.type_ == '1' ? next_event_.data_.eurex_dels_.size_ : ask_size_;
          ask_num_ords_ =
              next_event_.data_.eurex_dels_.type_ == '1' ? next_event_.data_.eurex_dels_.num_ords_ : ask_num_ords_;

          if (starttime_ >= endtime_) {
            endtime_ = starttime_ + 60;
            if (bid_price_ > 0 && ask_price_ > 0)
              std::cout << starttime_ << " " << (double)(bid_price_ + ask_price_) / 2 << "\n";
          }
        }
      }
      bulk_file_reader_.close();
    }
  }
};

class NTPMDSLogReader {
 public:
  static void ReadMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_) {
    NTP_MDS::NTPCommonStruct next_event_;
    double bid_price_ = 0.0;
    double ask_price_ = 0.0;
    int bid_size_ = 0;
    double ask_size_ = 0;

    int endtime_ = 0;
    int starttime_ = 0;

    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(NTP_MDS::NTPCommonStruct));
        if (available_len_ < sizeof(next_event_)) break;
        HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;

        starttime_ = next_event_timestamp_.tv_sec;

        if (starttime_ < start_time_ || start_time_ > end_time_) continue;

        if (endtime_ == 0) endtime_ = starttime_ + 60;

        if (next_event_.msg_ == NTP_MDS::NTP_TRADE) {
          continue;
        }

        if (next_event_.data_.ntp_dels_.level_ == 1) {
          bid_price_ = next_event_.data_.ntp_dels_.type_ == '0' ? next_event_.data_.ntp_dels_.price_ : bid_price_;
          bid_size_ = next_event_.data_.ntp_dels_.type_ == '0' ? next_event_.data_.ntp_dels_.size_ : bid_size_;
          ask_price_ = next_event_.data_.ntp_dels_.type_ == '1' ? next_event_.data_.ntp_dels_.price_ : ask_price_;
          ask_size_ = next_event_.data_.ntp_dels_.type_ == '1' ? next_event_.data_.ntp_dels_.size_ : ask_size_;

          if (starttime_ >= endtime_) {
            endtime_ = starttime_ + 60;
            if (bid_price_ > 0 && ask_price_ > 0)
              std::cout << starttime_ << " " << (double)(bid_price_ + ask_price_) / 2 << "\n";
          }
        }
      }
      bulk_file_reader_.close();
    }
  }
};

class LIFFEMDSLogReader {
 public:
  static void ReadMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_) {
    LIFFE_MDS::LIFFECommonStruct next_event_;
    double bid_price_ = 0.0;
    double ask_price_ = 0.0;
    int bid_size_ = 0;
    double ask_size_ = 0;

    int endtime_ = 0;
    int starttime_ = 0;

    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(LIFFE_MDS::LIFFECommonStruct));
        if (available_len_ < sizeof(next_event_)) break;
        HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;

        starttime_ = next_event_timestamp_.tv_sec;

        if (starttime_ < start_time_ || start_time_ > end_time_) continue;

        if (endtime_ == 0) endtime_ = starttime_ + 10;

        if (next_event_.msg_ == LIFFE_MDS::LIFFE_TRADE) {
          continue;
        }

        if (next_event_.data_.liffe_dels_.level_ == 1) {
          if (next_event_.data_.liffe_dels_.type_ == '1') {
            bid_price_ = next_event_.data_.liffe_dels_.price_ > 0 && next_event_.data_.liffe_dels_.price_ < 1e6
                             ? next_event_.data_.liffe_dels_.price_
                             : bid_price_;
            bid_size_ = next_event_.data_.liffe_dels_.size_ > 0 && next_event_.data_.liffe_dels_.size_ < 1e6
                            ? next_event_.data_.liffe_dels_.size_
                            : bid_size_;
          } else if (next_event_.data_.liffe_dels_.type_ == '2') {
            ask_price_ = next_event_.data_.liffe_dels_.price_ > 0 && next_event_.data_.liffe_dels_.price_ < 1e6
                             ? next_event_.data_.liffe_dels_.price_
                             : ask_price_;
            ask_size_ = next_event_.data_.liffe_dels_.size_ > 0 && next_event_.data_.liffe_dels_.size_ < 1e6
                            ? next_event_.data_.liffe_dels_.size_
                            : ask_size_;
          }

          if (starttime_ >= endtime_) {
            endtime_ = starttime_ + 10;
            if (bid_price_ > 0 && ask_price_ > 0)
              std::cout << starttime_ << " " << (double)(bid_price_ + ask_price_) / 2 << "\n";
          }
        }
      }
      bulk_file_reader_.close();
    }
  }
};

class RTSMDSLogReader {
 public:
  static void ReadMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_) {
    RTS_MDS::RTSCommonStruct next_event_;
    double bid_price_ = 0.0;
    double ask_price_ = 0.0;
    int bid_size_ = 0;
    double ask_size_ = 0;

    int starttime_ = 0;

    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(RTS_MDS::RTSCommonStruct));
        if (available_len_ < sizeof(next_event_)) break;
        //            HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;
        HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;
        starttime_ = next_event_timestamp_.tv_sec;

        if (starttime_ < start_time_ || start_time_ > end_time_) continue;

        if (next_event_.msg_ == RTS_MDS::RTS_TRADE) {
          continue;
        }

        if (next_event_.data_.rts_dels_.level_ == 1) {
          bid_price_ = next_event_.data_.rts_dels_.type_ == '0' ? next_event_.data_.rts_dels_.price_ : bid_price_;
          bid_size_ = next_event_.data_.rts_dels_.type_ == '0' ? next_event_.data_.rts_dels_.size_ : bid_size_;
          ask_price_ = next_event_.data_.rts_dels_.type_ == '1' ? next_event_.data_.rts_dels_.price_ : ask_price_;
          ask_size_ = next_event_.data_.rts_dels_.type_ == '1' ? next_event_.data_.rts_dels_.size_ : ask_size_;

          if ((bid_price_ > 0 && bid_price_ < 9000000) && (ask_price_ > 0 && ask_price_ < 9000000)) {
            std::cout << starttime_ << " " << (double)(bid_price_ + ask_price_) / 2 << "\n";
          }
        }
      }
      bulk_file_reader_.close();
    }
  }
};

class MICEXMDSLogReader {
 public:
  static void ReadMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_) {
    MICEX_MDS::MICEXCommonStruct next_event_;
    // double bid_price_ = 0.0; double ask_price_ = 0.0;
    // int bid_size_ = 0; double ask_size_ = 0;

    int starttime_ = 0;

    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(MICEX_MDS::MICEXCommonStruct));
        if (available_len_ < sizeof(next_event_)) break;
        HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;
        starttime_ = next_event_timestamp_.tv_sec;
        if (starttime_ < start_time_ || start_time_ > end_time_) continue;

        if (next_event_.msg_ == MICEX_MDS::MICEX_TRADE) {
          if (next_event_.data_.micex_trds_.trd_px_ > 0 && next_event_.data_.micex_trds_.trd_px_ < 9000000) {
            std::cout << "TRADE " << next_event_.time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                      << next_event_.time_.tv_usec << "\t" << next_event_.data_.micex_trds_.trd_qty_ << "\t"
                      << next_event_.data_.micex_trds_.trd_px_ << "\n";
          }

          continue;
        }

        // if (next_event_.data_.rts_dels_.level_ == 1) {
        //   bid_price_ = next_event_.data_.rts_dels_.type_ == '0' ? next_event_.data_.rts_dels_.price_ : bid_price_;
        //   bid_size_ = next_event_.data_.rts_dels_.type_ == '0' ? next_event_.data_.rts_dels_.size_ : bid_size_;
        //   ask_price_ = next_event_.data_.rts_dels_.type_ == '1' ? next_event_.data_.rts_dels_.price_ : ask_price_;
        //   ask_size_ = next_event_.data_.rts_dels_.type_ == '1' ? next_event_.data_.rts_dels_.size_ : ask_size_;

        //   if ( ( bid_price_ > 0 && bid_price_ < 9000000 ) &&
        //   	   ( ask_price_ > 0 && ask_price_ < 9000000 ) )
        // 	{
        // 	  std::cout << "L1    " << next_event_.time_.tv_sec << "." << std::setw ( 6 ) << std::setfill( '0' ) <<
        // next_event_.time_.tv_usec << "\t" << bid_size_ << "\t1\t" << bid_price_ << "\t" << ask_price_ << "\t1\t" <<
        // ask_size_ << "\n";
        // 	}
        // }
      }
      bulk_file_reader_.close();
    }
  }
};

class TMXMDSLogReader {
 public:
  static void ReadMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_) {
    TMX_MDS::TMXCommonStruct next_event_;
    double bid_price_ = 0.0;
    double ask_price_ = 0.0;
    int starttime_ = 0;
    int endtime_ = 0;

    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(TMX_MDS::TMXCommonStruct));
        if (available_len_ < sizeof(next_event_)) break;

        HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;
        starttime_ = next_event_timestamp_.tv_sec;

        if (starttime_ < start_time_ || start_time_ > end_time_) continue;

        if (next_event_.msg_ == TMX_MDS::TMX_TRADE) {
          continue;
        }

        if (next_event_.msg_ == TMX_MDS::TMX_BOOK) {
          bid_price_ = next_event_.data_.tmx_books_.bid_pxs_[0];
          ask_price_ = next_event_.data_.tmx_books_.ask_pxs_[0];

          for (int i = 0; i < 5; i++) {
            if (next_event_.data_.tmx_books_.bid_pxs_[i]) {
              bid_price_ = next_event_.data_.tmx_books_.bid_pxs_[i];
              break;
            }
          }

          for (int i = 0; i < 5; i++) {
            if (next_event_.data_.tmx_books_.ask_pxs_[i]) {
              ask_price_ = next_event_.data_.tmx_books_.ask_pxs_[i];
              break;
            }
          }

          if (starttime_ >= endtime_) {
            endtime_ = starttime_ + 60;
            if (bid_price_ > 0 && ask_price_ > 0)
              std::cout << starttime_ << " " << (double)(bid_price_ + ask_price_) / 2 << "\n";
          }
        }
      }
      bulk_file_reader_.close();
    }
  }
};

class HKEXMDSLogReader {
 public:
  static void ReadMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_) {
    HKEX_MDS::HKEXCommonStruct next_event_;
    double bid_price_ = 0.0;
    double ask_price_ = 0.0;
    int bid_size_ = 0;
    int ask_size_ = 0;
    int starttime_ = 0;

    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HKEX_MDS::HKEXCommonStruct));
        if (available_len_ < sizeof(next_event_)) break;

        HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;
        starttime_ = next_event_timestamp_.tv_sec;
        if (starttime_ < start_time_ || start_time_ > end_time_) continue;

        if (next_event_.msg_ == HKEX_MDS::HKEX_BOOK) {
          if (next_event_.data_.hkex_books_.side_ == 1) {  // Bid
            bid_price_ =
                (next_event_.data_.hkex_books_.pxs_[0] != 0) ? next_event_.data_.hkex_books_.pxs_[0] : bid_price_;
            bid_size_ =
                (next_event_.data_.hkex_books_.demand_[0] != 0) ? next_event_.data_.hkex_books_.demand_[0] : bid_size_;
          } else {
            ask_size_ =
                (next_event_.data_.hkex_books_.demand_[0] != 0) ? next_event_.data_.hkex_books_.demand_[0] : ask_size_;
            ask_price_ =
                (next_event_.data_.hkex_books_.pxs_[0] != 0) ? next_event_.data_.hkex_books_.pxs_[0] : ask_price_;
          }
          std::cout << starttime_ << " " << (double)(bid_price_ + ask_price_) / 2 << "\n";
        }

        if (next_event_.msg_ == HKEX_MDS::HKEX_TRADE) {
          continue;
        }
      }
      bulk_file_reader_.close();
    }
  }
};

class HKOMDPFMDSLogReader {
 public:
  static void ReadMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_) {
    HKOMD_MDS::HKOMDPFCommonStruct next_event_;
    double bid_price_ = 0.0;
    double ask_price_ = 0.0;
    int bid_size_ = 0;
    int ask_size_ = 0;
    int starttime_ = 0;

    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HKOMD_MDS::HKOMDPFCommonStruct));
        if (available_len_ < sizeof(next_event_)) break;

        HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;
        starttime_ = next_event_timestamp_.tv_sec;
        if (starttime_ < start_time_ || start_time_ > end_time_) continue;

        if (next_event_.msg_ == HKOMD_MDS::HKOMD_PF_DELTA) {
          if (next_event_.data_.delta_.side_ == 0) {  // Bid
            bid_price_ = (next_event_.data_.delta_.price_ != 0) ? next_event_.data_.delta_.price_ : bid_price_;
            bid_size_ = (next_event_.data_.delta_.quantity_ != 0) ? next_event_.data_.delta_.quantity_ : bid_size_;
          } else {
            ask_size_ = (next_event_.data_.delta_.quantity_ != 0) ? next_event_.data_.delta_.quantity_ : ask_size_;
            ask_price_ = (next_event_.data_.delta_.price_ != 0) ? next_event_.data_.delta_.price_ : ask_price_;
          }
          std::cout << starttime_ << " " << (double)(bid_price_ + ask_price_) / 2 << "\n";
        }

        if (next_event_.msg_ == HKOMD_MDS::HKOMD_PF_TRADE) {
          continue;
        }
      }
      bulk_file_reader_.close();
    }
  }
};

class OSEL1MDSLogReader {
 public:
  static void ReadMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_) {
    OSE_MDS::OSEPLCommonStruct next_event_;
    double bid_price_ = 0.0;
    double ask_price_ = 0.0;
    int bid_size_ = 0;
    int ask_size_ = 0;
    int bid_ords_ = 0;
    int ask_ords_ = 0;

    int starttime_;

    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(OSE_MDS::OSEPLCommonStruct));
        if (available_len_ < sizeof(next_event_)) break;

        if (next_event_.size <= 0) continue;

        HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;
        starttime_ = next_event_timestamp_.tv_sec;
        if (starttime_ < start_time_ || start_time_ > end_time_) continue;

        switch (next_event_.get_buy_sell_trade()) {
          case OSE_MDS::kL1BUY: {
            bid_price_ = next_event_.price;
            bid_size_ = next_event_.size;
            bid_ords_ = next_event_.order_count_;
          } break;
          case OSE_MDS::kL1SELL: {
            ask_price_ = next_event_.price;
            ask_size_ = next_event_.size;
            ask_ords_ = next_event_.order_count_;
          } break;

          case OSE_MDS::kL1TRADE: {
            continue;
          } break;
        }

        if (bid_price_ > 0 && ask_price_ > 0 && bid_size_ > 0 && ask_size_ > 0 && bid_ords_ > 0 && ask_ords_ > 0) {
          std::cout << starttime_ << " " << (double)(bid_price_ + ask_price_) / 2 << "\n";
        }
      }
      bulk_file_reader_.close();
    }
  }
};

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cout << " USAGE: " << argv[0] << " shortcode tradingdate start_time end_time " << std::endl;
    exit(0);
  }

  std::string shortcode_ = argv[1];
  unsigned int tradingdate_ = atoi(argv[2]);

  if (argc >= 4) {
    start_time_ = atoi(argv[3]);
    end_time_ = atoi(argv[4]);
  }

  HFSAT::ExchSource_t _this_exch_source_ = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, tradingdate_);

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);
  const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);

  HFSAT::BulkFileReader reader;

  switch (_this_exch_source_) {
    case HFSAT::kExchSourceCME: {
      HFSAT::TradingLocation_t trading_location_ = HFSAT::kTLocCHI;
      std::string filename_ =
          HFSAT::CMELoggedMessageFileNamer::GetName(exchange_symbol_, tradingdate_, trading_location_);
      reader.open(filename_.c_str());
      CMEMDSLogReader::ReadMDSStructs(reader);
    } break;
    case HFSAT::kExchSourceEUREX: {
      HFSAT::TradingLocation_t trading_location_ = HFSAT::kTLocFR2;
      std::string filename_ =
          HFSAT::EUREXLoggedMessageFileNamer::GetName(exchange_symbol_, tradingdate_, trading_location_);
      reader.open(filename_.c_str());
      EUREXMDSLogReader::ReadMDSStructs(reader);
    } break;
    case HFSAT::kExchSourceBMF:
    case HFSAT::kExchSourceNTP: {
      HFSAT::TradingLocation_t trading_location_ = HFSAT::kTLocBMF;
      std::string filename_ =
          HFSAT::NTPLoggedMessageFileNamer::GetName(exchange_symbol_, tradingdate_, trading_location_);
      reader.open(filename_.c_str());
      NTPMDSLogReader::ReadMDSStructs(reader);
    } break;
    case HFSAT::kExchSourceLIFFE: {
      HFSAT::TradingLocation_t trading_location_ = HFSAT::kTLocBSL;
      std::string filename_ =
          HFSAT::LIFFELoggedMessageFileNamer::GetName(exchange_symbol_, tradingdate_, trading_location_);
      reader.open(filename_.c_str());
      LIFFEMDSLogReader::ReadMDSStructs(reader);
    } break;
    case HFSAT::kExchSourceTMX: {
      HFSAT::TradingLocation_t trading_location_ = HFSAT::kTLocTMX;
      std::string filename_ =
          HFSAT::TMXLoggedMessageFileNamer::GetName(exchange_symbol_, tradingdate_, trading_location_);
      reader.open(filename_.c_str());
      TMXMDSLogReader::ReadMDSStructs(reader);
    } break;
    case HFSAT::kExchSourceHONGKONG: {
      HFSAT::TradingLocation_t trading_location_ = HFSAT::kTLocHK;
      if (tradingdate_ >= 20141204) {
        std::string filename_ =
            HFSAT::HKOMDCPFLoggedMessagefileNamer::GetName(exchange_symbol_, tradingdate_, trading_location_);
        reader.open(filename_.c_str());
        HKOMDPFMDSLogReader::ReadMDSStructs(reader);
        break;
      }
      std::string filename_ =
          HFSAT::HKEXLoggedMessageFileNamer::GetName(exchange_symbol_, tradingdate_, trading_location_);
      reader.open(filename_.c_str());
      HKEXMDSLogReader::ReadMDSStructs(reader);
    } break;
    case HFSAT::kExchSourceJPY: {
      HFSAT::TradingLocation_t trading_location_ =
          HFSAT::kTLocHK;  // Setting this to HK on purpose for L1 data from HK.
      std::string filename_ =
          HFSAT::OSEL1LoggedMessageFileNamer::GetName(exchange_symbol_, tradingdate_, trading_location_);
      reader.open(filename_.c_str());
      OSEL1MDSLogReader::ReadMDSStructs(reader);
    } break;
    case HFSAT::kExchSourceRTS: {
      HFSAT::TradingLocation_t trading_location_ = HFSAT::kTLocOTK;
      std::string filename_ =
          HFSAT::RTSLoggedMessageFileNamer::GetName(exchange_symbol_, tradingdate_, trading_location_);
      reader.open(filename_.c_str());
      RTSMDSLogReader::ReadMDSStructs(reader);
    } break;

    case HFSAT::kExchSourceMICEX:
    case HFSAT::kExchSourceMICEX_CR:
    case HFSAT::kExchSourceMICEX_EQ: {
      HFSAT::TradingLocation_t trading_location_ = HFSAT::kTLocOTK;
      std::string filename_ =
          HFSAT::MICEXLoggedMessageFileNamer::GetName(exchange_symbol_, tradingdate_, trading_location_);
      reader.open(filename_.c_str());
      MICEXMDSLogReader::ReadMDSStructs(reader);
    } break;
    default: { } break; }

  return 0;
}
