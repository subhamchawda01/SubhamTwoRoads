/**
   \file Tools/ors_delay_calc.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

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
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/ors_defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"

#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CDef/ors_messages.hpp"

#define LOGGED_DATA_PREFIX "/NAS1/data/ORSData/"

#include "baseinfra/LoggedSources/ors_conf_market_data_delay_computer.hpp"

class ORSBinReader {
 public:
  ORSBinReader(std::string _shortcode_, int _yyyymmdd_, bool summary_, bool plottable_stats_,
               std::string _ors_bin_file_)
      : shortcode_(_shortcode_),
        yyyymmdd_(_yyyymmdd_),
        seqd_conf_summary_(summary_),
        seqd_conf_plottable_stats_(plottable_stats_),
        ors_bin_filename_(_ors_bin_file_) {
    ors_conf_market_delay_computer_ = new HFSAT::ORSConfMarketDelayComputer(shortcode_, yyyymmdd_);
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

    if (ors_bin_filename_ != "") {
      filename_to_read = ors_bin_filename_;
    }

    bulk_file_reader_.open(filename_to_read.c_str());

    if (bulk_file_reader_.is_open()) {
      while (true) {
        HFSAT::GenericORSReplyStruct reply_struct;

        size_t read_length_ =
            bulk_file_reader_.read(reinterpret_cast<char*>(&reply_struct), sizeof(HFSAT::GenericORSReplyStruct));

        if (read_length_ < sizeof(HFSAT::GenericORSReplyStruct)) break;

        switch (reply_struct.orr_type_) {
          case HFSAT::kORRType_Seqd: {
            if (seqd_orders_map_.find(reply_struct.server_assigned_order_sequence_) == seqd_orders_map_.end()) {
              seqd_orders_map_[reply_struct.server_assigned_order_sequence_] = reply_struct.time_set_by_server_;
            }

          } break;

          case HFSAT::kORRType_Conf: {
            HFSAT::ttime_t this_accurate_conf_market_delay_(0, 0);
            this_accurate_conf_market_delay_ = (ors_conf_market_delay_computer_->getAccurateConfToMarketUpdateDelay(
                reply_struct.time_set_by_server_, reply_struct.price_, reply_struct.buysell_,
                reply_struct.size_remaining_));

            std::cout << seqd_orders_map_[reply_struct.server_assigned_order_sequence_].tv_sec << " "
                      << seqd_orders_map_[reply_struct.server_assigned_order_sequence_].tv_usec << " "
                      << this_accurate_conf_market_delay_.tv_sec << " " << this_accurate_conf_market_delay_.tv_usec
                      << " " << std::endl;

          } break;

          default: { } break; }
      }
    }
  }

 private:
  HFSAT::GenericORSReplyStruct generic_ors_reply_struct;
  std::string shortcode_;
  int yyyymmdd_;
  bool seqd_conf_summary_;
  bool seqd_conf_plottable_stats_;
  std::string ors_bin_filename_;
  HFSAT::ORSConfMarketDelayComputer* ors_conf_market_delay_computer_;
};

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cout << " USAGE: EXEC <shortcode> <tradedate>" << std::endl;
    std::cout << " STDCOUT will contain the entire output, please redirect" << std::endl;
    exit(0);
  }

  std::string shortcode_ = argv[1];
  int tradingdate_ = atoi(argv[2]);

  ORSBinReader common_logger(shortcode_, tradingdate_, false, false, "");

  common_logger.processMsgRecvd();
  return 0;
}
