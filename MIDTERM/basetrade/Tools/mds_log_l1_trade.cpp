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
#include "baseinfra/LoggedSources/eobi_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/eobi_price_feed_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ntp_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/liffe_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/tmx_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/hkex_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/hkomd_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/ose_l1_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/ose_pricefeed_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/rts_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/micex_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/cfe_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/ice_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/nse_logged_message_filenamer.hpp"

#include "dvccode/Utils/bulk_file_reader.hpp"

class CMEMDSLogReader {
 public:
  static void ReadMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_) {
    CME_MDS::CMECommonStruct next_event_;
    double bid_price_ = 0.0;
    double ask_price_ = 0.0;
    int bid_size_ = 0;
    int ask_size_ = 0;
    int bid_ords_ = 0;
    int ask_ords_ = 0;

    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(CME_MDS::CMECommonStruct));
        if (available_len_ < sizeof(next_event_)) break;
        //            HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;

        if (next_event_.msg_ == CME_MDS::CME_TRADE) {
          std::cout << "TRADE " << next_event_.time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                    << next_event_.time_.tv_usec << "\t" << next_event_.data_.cme_trds_.trd_qty_ << "\t"
                    << next_event_.data_.cme_trds_.trd_px_ << "\t" << next_event_.data_.cme_trds_.agg_side_ << "\n";
          continue;
        }

        if (next_event_.data_.cme_dels_.level_ == 1) {
          bid_price_ = (next_event_.data_.cme_dels_.type_ == '0' && next_event_.data_.cme_dels_.action_ < '2')
                           ? next_event_.data_.cme_dels_.price_
                           : bid_price_;
          bid_size_ = (next_event_.data_.cme_dels_.type_ == '0' && next_event_.data_.cme_dels_.action_ < '2')
                          ? next_event_.data_.cme_dels_.size_
                          : bid_size_;
          bid_ords_ = (next_event_.data_.cme_dels_.type_ == '0' && next_event_.data_.cme_dels_.action_ < '2')
                          ? next_event_.data_.cme_dels_.num_ords_
                          : bid_ords_;

          ask_price_ = (next_event_.data_.cme_dels_.type_ == '1' && next_event_.data_.cme_dels_.action_ < '2')
                           ? next_event_.data_.cme_dels_.price_
                           : ask_price_;
          ask_size_ = (next_event_.data_.cme_dels_.type_ == '1' && next_event_.data_.cme_dels_.action_ < '2')
                          ? next_event_.data_.cme_dels_.size_
                          : ask_size_;
          ask_ords_ = (next_event_.data_.cme_dels_.type_ == '1' && next_event_.data_.cme_dels_.action_ < '2')
                          ? next_event_.data_.cme_dels_.num_ords_
                          : ask_ords_;

          std::cout << "L1    " << next_event_.time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                    << next_event_.time_.tv_usec << "\t" << bid_size_ << "\t" << bid_ords_ << "\t" << bid_price_ << "\t"
                    << ask_price_ << "\t" << ask_ords_ << "\t" << ask_size_ << "\n";
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
    int ask_size_ = 0;
    int bid_num_ords_ = 0;
    int ask_num_ords_ = 0;
    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(EUREX_MDS::EUREXCommonStruct));
        if (available_len_ < sizeof(next_event_)) break;
        //            HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;

        if (next_event_.msg_ == EUREX_MDS::EUREX_TRADE) {
          std::cout << "TRADE " << next_event_.time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                    << next_event_.time_.tv_usec << "\t" << next_event_.data_.eurex_trds_.trd_qty_ << "\t"
                    << next_event_.data_.eurex_trds_.trd_px_ << "\t" << next_event_.data_.eurex_trds_.agg_side_ << "\n";
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

          std::cout << "L1    " << next_event_.time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                    << next_event_.time_.tv_usec << "\t" << bid_size_ << "\t" << bid_num_ords_ << "\t" << bid_price_
                    << "\t" << ask_price_ << "\t" << ask_num_ords_ << "\t" << ask_size_ << "\n";
        }
      }
      bulk_file_reader_.close();
    }
  }
};

class EOBIMDSLogReader {
 public:
  static void ReadMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_) {
    EOBI_MDS::EOBICommonStruct next_event_;

    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(EOBI_MDS::EOBICommonStruct));
        if (available_len_ < sizeof(next_event_)) break;
        //            HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;
        if (next_event_.msg_ == EOBI_MDS::EOBI_ORDER) {
          // exec_summary is used to get trades
          if (next_event_.data_.order_.action_ == '6') {
            std::cout << "TRADE " << next_event_.time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                      << next_event_.time_.tv_usec << "\t" << next_event_.data_.order_.size << "\t"
                      << next_event_.data_.order_.price << "\t" << next_event_.data_.order_.side << "\n";
            continue;
          }
          //		if ( next_event_.data_or
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
    int ask_size_ = 0;
    int bid_ords_ = 0;
    int ask_ords_ = 0;

    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(NTP_MDS::NTPCommonStruct));
        if (available_len_ < sizeof(next_event_)) break;
        //            HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;

        if (next_event_.msg_ == NTP_MDS::NTP_TRADE) {
          // std::cerr << "TRADE FLAGS " << next_event_.data_.ntp_trds_.flags_[0] << " " <<
          // next_event_.data_.ntp_trds_.flags_[1] << " " << next_event_.data_.ntp_trds_.flags_[2] << "\n" ;
          if (next_event_.data_.ntp_trds_.flags_[0] == 'X') {
            std::cout << "CROSSED_TRADE " << next_event_.time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                      << next_event_.time_.tv_usec << "\t" << next_event_.data_.ntp_trds_.trd_qty_ << "\t"
                      << next_event_.data_.ntp_trds_.trd_px_ << "\t" << next_event_.data_.ntp_trds_.buyer_ << "\t"
                      << next_event_.data_.ntp_trds_.seller_ << "\n";
          } else {
            std::cout << "TRADE " << next_event_.time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                      << next_event_.time_.tv_usec << "\t" << next_event_.data_.ntp_trds_.trd_qty_ << "\t"
                      << next_event_.data_.ntp_trds_.trd_px_ << "\n";
          }
          continue;
        }

        if (next_event_.data_.ntp_dels_.level_ == 1 && !next_event_.data_.ntp_dels_.intermediate_) {
          bid_price_ = (next_event_.data_.ntp_dels_.type_ == '0' && next_event_.data_.ntp_dels_.action_ < 2)
                           ? next_event_.data_.ntp_dels_.price_
                           : bid_price_;
          bid_size_ = (next_event_.data_.ntp_dels_.type_ == '0' && next_event_.data_.ntp_dels_.action_ < 2)
                          ? next_event_.data_.ntp_dels_.size_
                          : bid_size_;
          bid_ords_ = (next_event_.data_.ntp_dels_.type_ == '0' && next_event_.data_.ntp_dels_.action_ < 2)
                          ? next_event_.data_.ntp_dels_.num_ords_
                          : bid_ords_;

          ask_price_ = (next_event_.data_.ntp_dels_.type_ == '1' && next_event_.data_.ntp_dels_.action_ < 2)
                           ? next_event_.data_.ntp_dels_.price_
                           : ask_price_;
          ask_size_ = (next_event_.data_.ntp_dels_.type_ == '1' && next_event_.data_.ntp_dels_.action_ < 2)
                          ? next_event_.data_.ntp_dels_.size_
                          : ask_size_;
          ask_ords_ = (next_event_.data_.ntp_dels_.type_ == '1' && next_event_.data_.ntp_dels_.action_ < 2)
                          ? next_event_.data_.ntp_dels_.num_ords_
                          : ask_ords_;

          std::cout << "L1    " << next_event_.time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                    << next_event_.time_.tv_usec << "\t" << bid_size_ << "\t" << bid_ords_ << "\t" << bid_price_ << "\t"
                    << ask_price_ << "\t" << ask_ords_ << "\t" << ask_size_ << "\n";
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

    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(LIFFE_MDS::LIFFECommonStruct));
        if (available_len_ < sizeof(next_event_)) break;
        //            HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;

        if (next_event_.msg_ == LIFFE_MDS::LIFFE_TRADE) {
          if (next_event_.data_.liffe_trds_.trd_px_ > 0 && next_event_.data_.liffe_trds_.trd_px_ < 90000) {
            std::cout << "TRADE " << next_event_.time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                      << next_event_.time_.tv_usec << "\t" << next_event_.data_.liffe_trds_.trd_qty_ << "\t"
                      << next_event_.data_.liffe_trds_.trd_px_ << "\n";
          }

          continue;
        }

        if (next_event_.data_.liffe_dels_.level_ == 1) {
          bid_price_ = next_event_.data_.liffe_dels_.type_ == '2' ? next_event_.data_.liffe_dels_.price_ : bid_price_;
          bid_size_ = next_event_.data_.liffe_dels_.type_ == '2' ? next_event_.data_.liffe_dels_.size_ : bid_size_;
          ask_price_ = next_event_.data_.liffe_dels_.type_ == '1' ? next_event_.data_.liffe_dels_.price_ : ask_price_;
          ask_size_ = next_event_.data_.liffe_dels_.type_ == '1' ? next_event_.data_.liffe_dels_.size_ : ask_size_;

          if ((bid_price_ > 0 && bid_price_ < 90000) && (ask_price_ > 0 && ask_price_ < 90000)) {
            std::cout << "L1    " << next_event_.time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                      << next_event_.time_.tv_usec << "\t" << bid_size_ << "\t1\t" << bid_price_ << "\t" << ask_price_
                      << "\t1\t" << ask_size_ << "\n";
          }
        }
      }
      bulk_file_reader_.close();
    }
  }
};

class ICEMDSLogReader {
 public:
  static void ReadMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_) {
    ICE_MDS::ICECommonStruct next_event_;
    double bid_price_ = 0.0;
    double ask_price_ = 0.0;
    int bid_size_ = 0;
    double ask_size_ = 0;

    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(ICE_MDS::ICECommonStruct));
        if (available_len_ < sizeof(next_event_)) break;
        //            HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;

        if (next_event_.msg_ == ICE_MDS::ICE_TRADE) {
          if (next_event_.data_.ice_trds_.price_ > 0 && next_event_.data_.ice_trds_.price_ < 90000) {
            std::cout << "TRADE " << next_event_.time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                      << next_event_.time_.tv_usec << "\t" << next_event_.data_.ice_trds_.size_ << "\t"
                      << next_event_.data_.ice_trds_.price_ << "\n";
          }

          continue;
        }

        if (next_event_.data_.ice_pls_.level_ == 1) {
          bid_price_ = next_event_.data_.ice_pls_.side_ == '1' ? next_event_.data_.ice_pls_.price_ : bid_price_;
          bid_size_ = next_event_.data_.ice_pls_.side_ == '1' ? next_event_.data_.ice_pls_.size_ : bid_size_;
          ask_price_ = next_event_.data_.ice_pls_.side_ == '2' ? next_event_.data_.ice_pls_.price_ : ask_price_;
          ask_size_ = next_event_.data_.ice_pls_.side_ == '2' ? next_event_.data_.ice_pls_.size_ : ask_size_;

          if ((bid_price_ > 0 && bid_price_ < 90000) && (ask_price_ > 0 && ask_price_ < 90000)) {
            std::cout << "L1    " << next_event_.time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                      << next_event_.time_.tv_usec << "\t" << bid_size_ << "\t1\t" << bid_price_ << "\t" << ask_price_
                      << "\t1\t" << ask_size_ << "\n";
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

    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(RTS_MDS::RTSCommonStruct));
        if (available_len_ < sizeof(next_event_)) break;
        //            HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;

        if (next_event_.msg_ == RTS_MDS::RTS_TRADE) {
          if (next_event_.data_.rts_trds_.trd_px_ > 0 && next_event_.data_.rts_trds_.trd_px_ < 9000000) {
            std::cout << "TRADE " << next_event_.time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                      << next_event_.time_.tv_usec << "\t" << next_event_.data_.rts_trds_.trd_qty_ << "\t"
                      << next_event_.data_.rts_trds_.trd_px_ << "\n";
          }

          continue;
        }

        if (next_event_.data_.rts_dels_.level_ == 1) {
          bid_price_ = next_event_.data_.rts_dels_.type_ == '0' ? next_event_.data_.rts_dels_.price_ : bid_price_;
          bid_size_ = next_event_.data_.rts_dels_.type_ == '0' ? next_event_.data_.rts_dels_.size_ : bid_size_;
          ask_price_ = next_event_.data_.rts_dels_.type_ == '1' ? next_event_.data_.rts_dels_.price_ : ask_price_;
          ask_size_ = next_event_.data_.rts_dels_.type_ == '1' ? next_event_.data_.rts_dels_.size_ : ask_size_;

          if ((bid_price_ > 0 && bid_price_ < 9000000) && (ask_price_ > 0 && ask_price_ < 9000000)) {
            std::cout << "L1    " << next_event_.time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                      << next_event_.time_.tv_usec << "\t" << bid_size_ << "\t1\t" << bid_price_ << "\t" << ask_price_
                      << "\t1\t" << ask_size_ << "\n";
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

    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(MICEX_MDS::MICEXCommonStruct));
        if (available_len_ < sizeof(next_event_)) break;

        if (next_event_.msg_ == MICEX_MDS::MICEX_TRADE) {
          if (next_event_.data_.micex_trds_.trd_px_ > 0 && next_event_.data_.micex_trds_.trd_px_ < 9000000) {
            std::cout << "TRADE " << next_event_.time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                      << next_event_.time_.tv_usec << "\t" << next_event_.data_.micex_trds_.trd_qty_ << "\t"
                      << next_event_.data_.micex_trds_.trd_px_ << "\n";
          }

          continue;
        }
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
    int bid_size_ = 0;
    int ask_size_ = 0;
    int bid_ords_ = 0;
    int ask_ords_ = 0;

    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(TMX_MDS::TMXCommonStruct));
        if (available_len_ < sizeof(next_event_)) break;
        //            HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;

        if (next_event_.msg_ == TMX_MDS::TMX_BOOK) {
          bid_price_ =
              (next_event_.data_.tmx_books_.bid_szs_[0] != 0) ? next_event_.data_.tmx_books_.bid_pxs_[0] : bid_price_;
          bid_size_ =
              (next_event_.data_.tmx_books_.bid_szs_[0] != 0) ? next_event_.data_.tmx_books_.bid_szs_[0] : bid_size_;
          bid_ords_ = (next_event_.data_.tmx_books_.num_bid_ords_[0] != 0)
                          ? next_event_.data_.tmx_books_.num_bid_ords_[0]
                          : bid_ords_;

          ask_ords_ = (next_event_.data_.tmx_books_.num_ask_ords_[0] != 0)
                          ? next_event_.data_.tmx_books_.num_ask_ords_[0]
                          : ask_ords_;
          ask_size_ =
              (next_event_.data_.tmx_books_.ask_szs_[0] != 0) ? next_event_.data_.tmx_books_.ask_szs_[0] : ask_size_;
          ask_price_ =
              (next_event_.data_.tmx_books_.ask_szs_[0] != 0) ? next_event_.data_.tmx_books_.ask_pxs_[0] : ask_price_;

          std::cout << "L1    " << next_event_.time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                    << next_event_.time_.tv_usec << "\t" << bid_size_ << "\t" << bid_ords_ << "\t" << bid_price_ << "\t"
                    << ask_price_ << "\t" << ask_ords_ << "\t" << ask_size_ << "\n";
        }

        if (next_event_.msg_ == TMX_MDS::TMX_TRADE) {
          std::cout << "TRADE " << next_event_.time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                    << next_event_.time_.tv_usec << "\t" << next_event_.data_.tmx_trds_.trd_qty_ << "\t"
                    << next_event_.data_.tmx_trds_.trd_px_ << "\n";
          continue;
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
    int bid_ords_ = 0;
    int ask_ords_ = 0;

    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HKEX_MDS::HKEXCommonStruct));
        if (available_len_ < sizeof(next_event_)) break;
        //            HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;

        if (next_event_.msg_ == HKEX_MDS::HKEX_BOOK) {
          if (next_event_.data_.hkex_books_.side_ == 1) {  // Bid
            bid_price_ =
                (next_event_.data_.hkex_books_.pxs_[0] != 0) ? next_event_.data_.hkex_books_.pxs_[0] : bid_price_;
            bid_size_ =
                (next_event_.data_.hkex_books_.demand_[0] != 0) ? next_event_.data_.hkex_books_.demand_[0] : bid_size_;
            bid_ords_ = 1;  // HKEX no order-count info.
          } else {
            ask_ords_ = 1;  // HKEX no order-count info.
            ask_size_ =
                (next_event_.data_.hkex_books_.demand_[0] != 0) ? next_event_.data_.hkex_books_.demand_[0] : ask_size_;
            ask_price_ =
                (next_event_.data_.hkex_books_.pxs_[0] != 0) ? next_event_.data_.hkex_books_.pxs_[0] : ask_price_;
          }

          std::cout << "L1    " << next_event_.time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                    << next_event_.time_.tv_usec << "\t" << bid_size_ << "\t" << bid_ords_ << "\t" << bid_price_ << "\t"
                    << ask_price_ << "\t" << ask_ords_ << "\t" << ask_size_ << "\n";
        }

        if (next_event_.msg_ == HKEX_MDS::HKEX_TRADE) {
          std::cout << "TRADE " << next_event_.time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                    << next_event_.time_.tv_usec << "\t" << next_event_.data_.hkex_trds_.trd_qty_ << "\t"
                    << next_event_.data_.hkex_trds_.trd_px_ << "\n";
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
    int bid_ords_ = 0;
    int ask_ords_ = 0;

    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HKOMD_MDS::HKOMDPFCommonStruct));
        if (available_len_ < sizeof(next_event_)) break;
        //            HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;

        if (next_event_.msg_ == HKOMD_MDS::HKOMD_PF_DELTA) {
          if (next_event_.data_.delta_.side_ == 0) {  // Bid
            bid_price_ = (next_event_.data_.delta_.price_ != 0) ? next_event_.data_.delta_.price_ : bid_price_;
            bid_size_ = (next_event_.data_.delta_.quantity_ != 0) ? next_event_.data_.delta_.quantity_ : bid_size_;
            bid_ords_ = (next_event_.data_.delta_.num_orders_ != 0) ? next_event_.data_.delta_.num_orders_ : bid_ords_;
          } else {
            ask_size_ = (next_event_.data_.delta_.quantity_ != 0) ? next_event_.data_.delta_.quantity_ : ask_size_;
            ask_price_ = (next_event_.data_.delta_.price_ != 0) ? next_event_.data_.delta_.price_ : ask_price_;
            ask_ords_ = (next_event_.data_.delta_.num_orders_ != 0) ? next_event_.data_.delta_.num_orders_ : ask_ords_;
          }

          std::cout << "L1    " << next_event_.time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                    << next_event_.time_.tv_usec << "\t" << bid_size_ << "\t" << bid_ords_ << "\t" << bid_price_ << "\t"
                    << ask_price_ << "\t" << ask_ords_ << "\t" << ask_size_ << "\n";
        }

        if (next_event_.msg_ == HKOMD_MDS::HKOMD_PF_TRADE) {
          std::cout << "TRADE " << next_event_.time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                    << next_event_.time_.tv_usec << "\t" << next_event_.data_.trade_.quantity_ << "\t"
                    << next_event_.data_.trade_.price_ << "\n";
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

    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(OSE_MDS::OSEPLCommonStruct));
        if (available_len_ < sizeof(next_event_)) break;

        if (next_event_.size <= 0) continue;

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
            std::cout << "TRADE " << next_event_.time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                      << next_event_.time_.tv_usec << "\t" << next_event_.size << "\t" << next_event_.price << "\n";
          } break;
        }

        if (bid_price_ > 0 && ask_price_ > 0 && bid_size_ > 0 && ask_size_ > 0 && bid_ords_ > 0 && ask_ords_ > 0) {
          std::cout << "L1    " << next_event_.time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                    << next_event_.time_.tv_usec << "\t" << bid_size_ << "\t" << bid_ords_ << "\t" << bid_price_ << "\t"
                    << ask_price_ << "\t" << ask_ords_ << "\t" << ask_size_ << "\n";
        }
      }
      bulk_file_reader_.close();
    }
  }
};

class OSEPFL1MDSLogReader {
 public:
  static void ReadMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_) {
    OSE_MDS::OSEPriceFeedCommonStruct next_event_;
    double bid_price_ = 0.0;
    double ask_price_ = 0.0;
    int bid_size_ = 0;
    int ask_size_ = 0;
    int bid_ords_ = 0;
    int ask_ords_ = 0;

    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(OSE_MDS::OSEPriceFeedCommonStruct));
        if (available_len_ < sizeof(next_event_)) break;

        if (next_event_.size <= 0) continue;

        switch (next_event_.get_buy_sell_trade()) {
          case OSE_MDS::kL1BUY: {
            if (next_event_.price_level_ == 1) {
              bid_price_ = next_event_.price;
              bid_size_ = next_event_.size;
              bid_ords_ = next_event_.order_count_;
            }
          } break;
          case OSE_MDS::kL1SELL: {
            if (next_event_.price_level_ == 1) {
              ask_price_ = next_event_.price;
              ask_size_ = next_event_.size;
              ask_ords_ = next_event_.order_count_;
            }
          } break;

          case OSE_MDS::kL1TRADE: {
            std::cout << "TRADE " << next_event_.time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                      << next_event_.time_.tv_usec << "\t" << next_event_.size << "\t" << next_event_.price << "\n";
          } break;
        }

        if (bid_price_ > 0 && ask_price_ > 0 && bid_size_ > 0 && ask_size_ > 0 && bid_ords_ > 0 && ask_ords_ > 0 &&
            next_event_.price_level_ == 1) {
          std::cout << "L1    " << next_event_.time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                    << next_event_.time_.tv_usec << "\t" << bid_size_ << "\t" << bid_ords_ << "\t" << bid_price_ << "\t"
                    << ask_price_ << "\t" << ask_ords_ << "\t" << ask_size_ << "\n";
        }
      }
      bulk_file_reader_.close();
    }
  }
};

class TSEMDSLogReader {
 public:
  static void ReadMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_) {
    std::cout << "TSEMDS\n";
    if (bulk_file_reader_.is_open()) {
      bulk_file_reader_.close();
    }
  }
};

class CFEMDSLogReader {
 public:
  static void ReadMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_) {
    CSM_MDS::CSMCommonStruct next_event_;

    double bid_price_ = 0.0;
    double ask_price_ = 0.0;
    int bid_size_ = 0;
    int ask_size_ = 0;
    int bid_ords_ = 0;
    int ask_ords_ = 0;

    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(CSM_MDS::CSMCommonStruct));
        if (available_len_ < sizeof(next_event_)) break;

        if (next_event_.msg_ == CSM_MDS::CSM_TRADE) {
          std::cout << "TRADE " << next_event_.time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                    << next_event_.time_.tv_usec << "\t" << next_event_.data_.csm_trds_.trd_qty_ << "\t"
                    << next_event_.data_.csm_trds_.trd_px_ << "\n";
          continue;
        }

        if (1 == next_event_.data_.csm_dels_.level_ && next_event_.data_.csm_dels_.size_[0] != 0) {
          bid_price_ = ('0' == next_event_.data_.csm_dels_.type_) ? next_event_.data_.csm_dels_.price_ : bid_price_;
          bid_size_ = ('0' == next_event_.data_.csm_dels_.type_) ? next_event_.data_.csm_dels_.size_[0] : bid_size_;
          bid_ords_ = 0;

          ask_price_ = ('1' == next_event_.data_.csm_dels_.type_) ? next_event_.data_.csm_dels_.price_ : ask_price_;
          ask_size_ = ('1' == next_event_.data_.csm_dels_.type_) ? next_event_.data_.csm_dels_.size_[0] : ask_size_;
          ask_ords_ = 0;

          std::cout << "L1    " << next_event_.time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                    << next_event_.time_.tv_usec << "\t" << bid_size_ << "\t" << bid_ords_ << "\t" << bid_price_ << "\t"
                    << ask_price_ << "\t" << ask_ords_ << "\t" << ask_size_ << "\n";
        }
      }
      bulk_file_reader_.close();
    }
  }
};

// since we dont have price feed data, we are just printing out trade messages, use mkt_trade_logger otherwise
class NSEMDSLogReader {
 public:
  static void ReadMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_) {
    NSE_MDS::NSEDotexOfflineCommonStruct next_event_;

    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(NSE_MDS::NSEDotexOfflineCommonStruct));
        if (available_len_ < sizeof(next_event_)) break;

        if (next_event_.msg_type == NSE_MDS::MsgType::kNSETrade) {
          std::cout << "TRADE " << next_event_.source_time.tv_sec << "." << std::setw(6) << std::setfill('0')
                    << next_event_.source_time.tv_usec << "\t" << next_event_.data.nse_dotex_trade.trade_quantity
                    << "\t" << next_event_.data.nse_dotex_trade.trade_price << "\n";
          continue;
        }
      }
      bulk_file_reader_.close();
    }
  }
};

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cout << " USAGE: " << argv[0] << " shortcode tradingdate" << std::endl;
    exit(0);
  }

  std::string shortcode_ = argv[1];
  unsigned int tradingdate_ = atoi(argv[2]);

  if (strncmp(shortcode_.c_str(), "NSE_", 4) == 0) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadNSESecurityDefinitions();
  }
  HFSAT::ExchSource_t _this_exch_source_ = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, tradingdate_);

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);
  const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);

  HFSAT::BulkFileReader reader;

  if (_this_exch_source_ == HFSAT::kExchSourceHONGKONG && tradingdate_ > 20141204) {
    _this_exch_source_ = HFSAT::kExchSourceHKOMDPF;
  }

  switch (_this_exch_source_) {
    case HFSAT::kExchSourceCME: {
      HFSAT::TradingLocation_t trading_location_ = HFSAT::kTLocCHI;
      std::string filename_ =
          HFSAT::CMELoggedMessageFileNamer::GetName(exchange_symbol_, tradingdate_, trading_location_);
      reader.open(filename_.c_str());
      CMEMDSLogReader::ReadMDSStructs(reader);
    } break;
    /*    case HFSAT::kExchSourceEUREX :
    {
      HFSAT::TradingLocation_t trading_location_ = HFSAT::kTLocFR2;
      std::string filename_ = HFSAT::EUREXLoggedMessageFileNamer::GetName (exchange_symbol_, tradingdate_,
    trading_location_);
      reader.open(filename_.c_str ());
      EUREXMDSLogReader::ReadMDSStructs(reader);
    }
    break; */
    case HFSAT::kExchSourceEUREX: {
      HFSAT::TradingLocation_t trading_location_ = HFSAT::kTLocFR2;
      std::string filename_ =
          HFSAT::EUREXLoggedMessageFileNamer::GetName(exchange_symbol_, tradingdate_, trading_location_);
      if (tradingdate_ >= 20131228) {
        filename_ =
            HFSAT::EOBIPriceFeedLoggedMessageFileNamer::GetName(exchange_symbol_, tradingdate_, trading_location_);
      }
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
    case HFSAT::kExchSourceICE: {
      HFSAT::TradingLocation_t trading_location_ = HFSAT::kTLocBSL;
      std::string filename_ =
          HFSAT::ICELoggedMessageFileNamer::GetName(exchange_symbol_, tradingdate_, trading_location_);
      reader.open(filename_.c_str());
      ICEMDSLogReader::ReadMDSStructs(reader);
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
      std::string filename_ =
          HFSAT::HKEXLoggedMessageFileNamer::GetName(exchange_symbol_, tradingdate_, trading_location_);
      reader.open(filename_.c_str());
      HKEXMDSLogReader::ReadMDSStructs(reader);
    } break;
    case HFSAT::kExchSourceHKOMDPF: {
      HFSAT::TradingLocation_t trading_location_ = HFSAT::kTLocHK;
      std::string filename_ =
          HFSAT::HKOMDCPFLoggedMessagefileNamer::GetName(exchange_symbol_, tradingdate_, trading_location_);
      reader.open(filename_.c_str());
      HKOMDPFMDSLogReader::ReadMDSStructs(reader);
    } break;
    case HFSAT::kExchSourceJPY: {
      // HFSAT::TradingLocation_t trading_location_ = HFSAT::kTLocHK; // Setting this to HK on purpose for L1 data from
      // HK.
      // std::string filename_ = HFSAT::OSEL1LoggedMessageFileNamer::GetName ( exchange_symbol_ , tradingdate_ ,
      // trading_location_ );
      // reader.open ( filename_.c_str ( ) );
      // OSEL1MDSLogReader::ReadMDSStructs ( reader );
      HFSAT::TradingLocation_t trading_location_ = HFSAT::kTLocJPY;
      std::string filename_ =
          HFSAT::OSEPriceFeedLoggedMessageFileNamer::GetName(exchange_symbol_, tradingdate_, trading_location_);
      reader.open(filename_.c_str());
      OSEPFL1MDSLogReader::ReadMDSStructs(reader);
    } break;
    case HFSAT::kExchSourceRTS: {
      HFSAT::TradingLocation_t trading_location_ = HFSAT::kTLocM1;
      std::string filename_ =
          HFSAT::RTSLoggedMessageFileNamer::GetName(exchange_symbol_, tradingdate_, trading_location_);
      reader.open(filename_.c_str());
      RTSMDSLogReader::ReadMDSStructs(reader);
    } break;
    case HFSAT::kExchSourceMICEX:
    case HFSAT::kExchSourceMICEX_CR:
    case HFSAT::kExchSourceMICEX_EQ: {
      HFSAT::TradingLocation_t trading_location_ = HFSAT::kTLocM1;
      std::string filename_ =
          HFSAT::MICEXLoggedMessageFileNamer::GetName(exchange_symbol_, tradingdate_, trading_location_);
      reader.open(filename_.c_str());
      MICEXMDSLogReader::ReadMDSStructs(reader);
    } break;
    case HFSAT::kExchSourceCFE: {
      HFSAT::TradingLocation_t trading_location_ = HFSAT::kTLocCHI;
      std::string filename_ =
          HFSAT::CFELoggedMessageFileNamer::GetName(exchange_symbol_, tradingdate_, trading_location_);
      reader.open(filename_.c_str());
      CFEMDSLogReader::ReadMDSStructs(reader);
    } break;
    case HFSAT::kExchSourceNSE: {
      HFSAT::TradingLocation_t trading_location_ = HFSAT::kTLocNSE;
      std::string filename_ =
          HFSAT::NSELoggedMessageFileNamer::GetName(exchange_symbol_, tradingdate_, trading_location_);
      reader.open(filename_.c_str());
      NSEMDSLogReader::ReadMDSStructs(reader);
    } break;

    default: { } break; }

  return 0;
}
