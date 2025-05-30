// =====================================================================================
//
//       Filename:  generate_ors_mktdata_delays_stats.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  10/20/2012 01:46:54 PM
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

#include "baseinfra/LoggedSources/cme_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/eurex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ntp_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/liffe_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/tmx_logged_message_filenamer.hpp"

#define LOGGED_DATA_PREFIX "/NAS1/data/ORSData/"
#define MAX_DELAY_LIMIT 3000000

struct TradeInfo {
  double trade_price_;
  HFSAT::TradeType_t buysell_;
  int trade_qty_;
};

// This class will read from the ORSBInary Log file
class ORSBinReader {
 public:
  ORSBinReader(std::string _shortcode_, int _yyyymmdd_, std::vector<unsigned long>& _mds_trade_events_time_,
               std::vector<struct TradeInfo>& _mds_trade_packets_)
      : shortcode_(_shortcode_),
        yyyymmdd_(_yyyymmdd_),
        mds_trade_events_time_(_mds_trade_events_time_),
        mds_trade_packet_(_mds_trade_packets_),
        last_vector_index_for_mds_event_(0) {}

  void MatchMarketUpdateForTrade(HFSAT::ttime_t trade_time_, double trade_price_, HFSAT::TradeType_t buysell_,
                                 int qty_) {
    int this_unmatched_index_ = 0;
    bool unmatched_ = false;

    for (; last_vector_index_for_mds_event_ < (int)mds_trade_events_time_.size(); last_vector_index_for_mds_event_++) {
      int this_tsec_ = trade_time_.tv_sec;
      int this_usec_ = trade_time_.tv_usec;

      unsigned long this_trade_time_ = ((unsigned long)this_tsec_ * 1000000) + this_usec_;

      if (this_trade_time_ > mds_trade_events_time_[last_vector_index_for_mds_event_]) continue;

      if (unmatched_) {
        if (mds_trade_events_time_[last_vector_index_for_mds_event_] - this_trade_time_ > MAX_DELAY_LIMIT) {
          std::cerr << " Reset Last Unmatched To : " << this_unmatched_index_ << "\t";
          std::cerr << " Our Trade : " << (unsigned long)this_trade_time_ << " " << trade_price_ << " " << (int)buysell_
                    << " " << qty_ << "\t";
          last_vector_index_for_mds_event_ = this_unmatched_index_;
          return;
        }
      }

      if (((int)(mds_trade_packet_[last_vector_index_for_mds_event_].trade_price_ * 100) ==
           (int)(trade_price_ * 100)) &&
          (mds_trade_packet_[last_vector_index_for_mds_event_].trade_qty_ >= qty_) &&
          (buysell_ != mds_trade_packet_[last_vector_index_for_mds_event_].buysell_)) {
        unsigned long trade_market_update_time_ =
            mds_trade_events_time_[last_vector_index_for_mds_event_] - this_trade_time_;

        double ors_trade_time_ = (this_trade_time_) / (double)(1000000);
        double mds_trade_time_ = (mds_trade_events_time_[last_vector_index_for_mds_event_]) / (double)(1000000);
        unsigned int delay_usec_ = trade_market_update_time_;

        std::cerr << " Our Trade : " << (unsigned long)this_trade_time_ << " " << trade_price_ << " " << (int)buysell_
                  << " " << qty_ << "\t";
        std::cerr << " Exch Trade : " << mds_trade_events_time_[last_vector_index_for_mds_event_] << " "
                  << mds_trade_packet_[last_vector_index_for_mds_event_].trade_price_ << " "
                  << "0"
                  << " " << mds_trade_packet_[last_vector_index_for_mds_event_].trade_qty_ << "\t";
        std::cerr << " Delay :  " << trade_market_update_time_ << "\n";

        mds_trade_packet_[last_vector_index_for_mds_event_].trade_qty_ -= qty_;

        printf("%11.6f %11.6f %u\n", ors_trade_time_, mds_trade_time_, delay_usec_);

        trade_mktupdate_delay_vec_.push_back(trade_market_update_time_);

        break;

      } else {
        if (!unmatched_) {
          unmatched_ = true;
          this_unmatched_index_ = last_vector_index_for_mds_event_;
        }

        std::cerr << " Unmatched Our Trade : " << (unsigned long)this_trade_time_ << " " << trade_price_ << " "
                  << (int)buysell_ << " " << qty_ << "\n";
      }
    }
  }

  void dumpStats() {
    unsigned long long total_time_ = 0;
    std::sort(trade_mktupdate_delay_vec_.begin(), trade_mktupdate_delay_vec_.end());

    for (auto i = 0u; i < trade_mktupdate_delay_vec_.size(); i++) {
      total_time_ += trade_mktupdate_delay_vec_[i];
    }

    std::cout << " Sample Size : " << trade_mktupdate_delay_vec_.size() << " "
              << " 10 % : " << trade_mktupdate_delay_vec_[trade_mktupdate_delay_vec_.size() * 10 / 100] << " "
              << " 50 % : " << trade_mktupdate_delay_vec_[trade_mktupdate_delay_vec_.size() / 2] << " "
              << " 75 % : " << trade_mktupdate_delay_vec_[trade_mktupdate_delay_vec_.size() * 3 / 4] << " "
              << " 90 % : " << trade_mktupdate_delay_vec_[trade_mktupdate_delay_vec_.size() * 9 / 10] << " "
              << " 95 % : " << trade_mktupdate_delay_vec_[trade_mktupdate_delay_vec_.size() * 95 / 100] << " "
              << " 99 % : " << trade_mktupdate_delay_vec_[trade_mktupdate_delay_vec_.size() * 99 / 100] << " "
              << " Mean : " << (unsigned long)(total_time_ / trade_mktupdate_delay_vec_.size()) << "\n";
  }

  /// Reader
  void processMsgRecvd() {
    std::map<int, HFSAT::ttime_t> seqd_orders_map_;
    std::map<int, HFSAT::ttime_t> conf_orders_map_;

    HFSAT::ExchangeSymbolManager::SetUniqueInstance(yyyymmdd_);
    const char* t_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);
    std::string location_ =
        HFSAT::TradingLocationUtils::GetTradingLocationName(HFSAT::TradingLocationUtils::GetTradingLocationExch(
            HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, yyyymmdd_)));

    std::ostringstream t_temp_oss_;
    t_temp_oss_ << yyyymmdd_;
    std::string date_ = t_temp_oss_.str();

    std::stringstream ff;
    ff << LOGGED_DATA_PREFIX << location_ << "/" << date_.substr(0, 4) << "/" << date_.substr(4, 2) << "/"
       << date_.substr(6, 2) << "/" << t_exchange_symbol_ << "_" << yyyymmdd_;

    std::string filename_to_read = ff.str();

    while (filename_to_read.find(" ") != std::string::npos) {  // Liffe naming issues
      filename_to_read.replace(filename_to_read.find(" "), 1, "~");
    }

    HFSAT::BulkFileReader bulk_file_reader_;
    bulk_file_reader_.open(filename_to_read.c_str());

    std::cerr << " FileName : " << filename_to_read << "\n";
    std::cerr << " Total Trade Events : " << mds_trade_packet_.size() << "\n";

    if (bulk_file_reader_.is_open()) {
      while (true) {
        HFSAT::GenericORSReplyStruct reply_struct;

        size_t read_length_ =
            bulk_file_reader_.read(reinterpret_cast<char*>(&reply_struct), sizeof(HFSAT::GenericORSReplyStruct));

        if (read_length_ < sizeof(HFSAT::GenericORSReplyStruct)) break;

        char pp[6] = {'\0'};
        sprintf(pp, "%.6f", reply_struct.price_);

        if (reply_struct.orr_type_ == HFSAT::kORRType_Exec) {
          HFSAT::ttime_t this_trade_time_ = reply_struct.time_set_by_server_;
          double trade_price_ = reply_struct.price_;
          HFSAT::TradeType_t buysell_ = reply_struct.buysell_;
          int trade_size_ = reply_struct.size_executed_;

          MatchMarketUpdateForTrade(this_trade_time_, trade_price_, buysell_, trade_size_);
        }
      }

      dumpStats();
    }
  }

 private:
  HFSAT::GenericORSReplyStruct generic_ors_reply_struct_;
  std::string shortcode_;
  int yyyymmdd_;
  bool seqd_conf_summary_;
  bool seqd_conf_plottable_stats_;
  std::vector<unsigned long>& mds_trade_events_time_;
  std::vector<struct TradeInfo>& mds_trade_packet_;
  int last_vector_index_for_mds_event_;
  std::vector<unsigned long> trade_mktupdate_delay_vec_;
};

class MDSTradeEvents {
 public:
  static void ReadMDSTradeEvents(std::string shortcode_, int tradingdate_,
                                 std::vector<unsigned long>& mds_trade_events_time_,
                                 std::vector<struct TradeInfo>& mds_trade_packet_) {
    HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);
    const char* t_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);

    HFSAT::ExchSource_t exch_src_ = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, tradingdate_);
    HFSAT::TradingLocation_t trading_location_file_read_ =
        HFSAT::TradingLocationUtils::GetTradingLocationExch(exch_src_);  // initialize to primary location

    HFSAT::BulkFileReader bulk_file_reader_;

    switch (exch_src_) {
      case HFSAT::kExchSourceCME: {
        std::string t_cme_filename_ =
            HFSAT::CMELoggedMessageFileNamer::GetName(t_exchange_symbol_, tradingdate_, trading_location_file_read_);

        CME_MDS::CMECommonStruct next_event_;
        bulk_file_reader_.open(t_cme_filename_);

        if (!bulk_file_reader_.is_open())
          return;
        else
          while (true) {
            size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(CME_MDS::CMECommonStruct));
            if (available_len_ >= sizeof(CME_MDS::CMECommonStruct)) {  // data not found in file

              if (next_event_.msg_ == CME_MDS::CME_TRADE) {
                unsigned long this_time_ = next_event_.time_.tv_sec * 1000000 + next_event_.time_.tv_usec;
                mds_trade_events_time_.push_back(this_time_);

                struct TradeInfo this_trade_info_;
                this_trade_info_.trade_price_ = next_event_.data_.cme_trds_.trd_px_;
                this_trade_info_.trade_qty_ = next_event_.data_.cme_trds_.trd_qty_;
                this_trade_info_.buysell_ =
                    ((next_event_.data_.cme_trds_.agg_side_ == 1)
                         ? (HFSAT::kTradeTypeBuy)
                         : ((next_event_.data_.cme_trds_.agg_side_ == 2) ? HFSAT::kTradeTypeSell
                                                                         : HFSAT::kTradeTypeNoInfo));

                mds_trade_packet_.push_back(this_trade_info_);
              }
            } else
              break;
          }
      } break;
      case HFSAT::kExchSourceEUREX: {
        std::string t_eurex_filename_ =
            HFSAT::EUREXLoggedMessageFileNamer::GetName(t_exchange_symbol_, tradingdate_, trading_location_file_read_);

        EUREX_MDS::EUREXCommonStruct next_event_;
        bulk_file_reader_.open(t_eurex_filename_);

        if (!bulk_file_reader_.is_open())
          return;
        else
          while (true) {
            size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(EUREX_MDS::EUREXCommonStruct));
            if (available_len_ >= sizeof(EUREX_MDS::EUREXCommonStruct)) {  // data not found in file

              if (next_event_.msg_ == EUREX_MDS::EUREX_TRADE) {
                unsigned long this_time_ = next_event_.time_.tv_sec * 1000000 + next_event_.time_.tv_usec;
                mds_trade_events_time_.push_back(this_time_);
              }
            } else
              break;
          }
      } break;
      case HFSAT::kExchSourceBMFEQ:
      case HFSAT::kExchSourceBMF: {
        std::string t_cme_filename_ = HFSAT::NTPLoggedMessageFileNamer::GetName(
            t_exchange_symbol_, tradingdate_, trading_location_file_read_, false, false);

        NTP_MDS::NTPCommonStruct next_event_;
        bulk_file_reader_.open(t_cme_filename_);

        if (!bulk_file_reader_.is_open())
          return;
        else
          while (true) {
            size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(NTP_MDS::NTPCommonStruct));
            if (available_len_ >= sizeof(NTP_MDS::NTPCommonStruct)) {  // data not found in file

              if (next_event_.msg_ == NTP_MDS::NTP_TRADE) {
                unsigned long this_time_ = next_event_.time_.tv_sec * 1000000 + next_event_.time_.tv_usec;
                mds_trade_events_time_.push_back(this_time_);
              }
            } else
              break;
          }
      } break;
      case HFSAT::kExchSourceTMX: {
        std::string t_tmx_filename_ =
            HFSAT::TMXLoggedMessageFileNamer::GetName(t_exchange_symbol_, tradingdate_, trading_location_file_read_);

        TMX_MDS::TMXCommonStruct next_event_;
        bulk_file_reader_.open(t_tmx_filename_);

        if (!bulk_file_reader_.is_open())
          return;
        else
          while (true) {
            size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(TMX_MDS::TMXCommonStruct));
            if (available_len_ >= sizeof(TMX_MDS::TMXCommonStruct)) {  // data not found in file

              if (next_event_.msg_ == TMX_MDS::TMX_TRADE) {
                unsigned long this_time_ = next_event_.time_.tv_sec * 1000000 + next_event_.time_.tv_usec;
                mds_trade_events_time_.push_back(this_time_);
              }
            } else
              break;
          }
      } break;
      case HFSAT::kExchSourceLIFFE: {
        std::string t_liffe_filename_ =
            HFSAT::LIFFELoggedMessageFileNamer::GetName(t_exchange_symbol_, tradingdate_, trading_location_file_read_);

        LIFFE_MDS::LIFFECommonStruct next_event_;
        bulk_file_reader_.open(t_liffe_filename_);

        if (!bulk_file_reader_.is_open())
          return;
        else
          while (true) {
            size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(LIFFE_MDS::LIFFECommonStruct));
            if (available_len_ >= sizeof(LIFFE_MDS::LIFFECommonStruct)) {  // data not found in file

              if (next_event_.msg_ == LIFFE_MDS::LIFFE_TRADE) {
                unsigned long this_time_ = next_event_.time_.tv_sec * 1000000 + next_event_.time_.tv_usec;
                mds_trade_events_time_.push_back(this_time_);

                struct TradeInfo this_trade_info_;
                this_trade_info_.trade_price_ = next_event_.data_.liffe_trds_.trd_px_;
                this_trade_info_.trade_qty_ = next_event_.data_.liffe_trds_.trd_qty_;
                this_trade_info_.buysell_ =
                    ((next_event_.data_.liffe_trds_.agg_side_ == 'B')
                         ? (HFSAT::kTradeTypeBuy)
                         : ((next_event_.data_.liffe_trds_.agg_side_ == 'S') ? HFSAT::kTradeTypeSell
                                                                             : HFSAT::kTradeTypeNoInfo));

                mds_trade_packet_.push_back(this_trade_info_);
              }
            } else
              break;
          }
      } break;

      default: { } break; }
  }
};

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cout << " USAGE: EXEC <shortcode> <tradedate>" << std::endl;
    std::cout << " STDCOUT Generats Plottable Stats, STDCERR will provide details" << std::endl;
    exit(0);
  }

  std::string shortcode_ = argv[1];
  int tradingdate_ = atoi(argv[2]);

  std::vector<unsigned long> mds_trade_events_time_;
  std::vector<struct TradeInfo> mds_trade_events_vec_;

  MDSTradeEvents::ReadMDSTradeEvents(shortcode_, tradingdate_, mds_trade_events_time_, mds_trade_events_vec_);
  ORSBinReader ors_bin_reader_(shortcode_, tradingdate_, mds_trade_events_time_, mds_trade_events_vec_);

  ors_bin_reader_.processMsgRecvd();

  return 0;
}
