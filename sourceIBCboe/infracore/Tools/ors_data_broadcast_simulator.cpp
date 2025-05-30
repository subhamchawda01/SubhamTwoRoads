/**
   \file Tools/ors_data_brodcast_simulator.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

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

#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CDef/ors_messages.hpp"

#define LOGGED_DATA_PREFIX "/NAS1/data/ORSData/"

// This class will read from the ORSBInary Log file
class ORSBinReader {
 public:
  ORSBinReader(std::string _shortcode_, int _yyyymmdd_, std::string ip_, int port_, std::string interface_)
      : shortcode_(_shortcode_),
        yyyymmdd_(_yyyymmdd_),
        ors_ip_(ip_),
        ors_port_(port_),
        ors_interface_(interface_),
        mcast_sender_socket_(ip_, port_, interface_) {}

  /// Reader
  void processMsgRecvd() {
    std::map<int, HFSAT::ttime_t> seqd_orders_map_;
    std::map<int, HFSAT::ttime_t> conf_orders_map_;

    HFSAT::ExchangeSymbolManager::SetUniqueInstance(yyyymmdd_);
    const char* t_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);
    std::string location_ =
        HFSAT::TradingLocationUtils::GetTradingLocationName(HFSAT::TradingLocationUtils::GetTradingLocationExch(
            HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, yyyymmdd_)));

    HFSAT::ExchSource_t _this_exch_source_ = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, yyyymmdd_);
    std::string exchange_ = ExchSourceStringForm(_this_exch_source_);

    //    std::string location_ = HFSAT::TradingLocationUtils::GetTradingLocationName(
    //    HFSAT::TradingLocationUtils::GetTradingLocationExch ( HFSAT::SecurityDefinitions::GetContractExchSource (
    //    shortcode_ )) );

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

    std::cout << " File : " << filename_to_read << "\n";

    HFSAT::BulkFileReader bulk_file_reader_;
    bulk_file_reader_.open(filename_to_read.c_str());

    unsigned long last_packet_send_time_ = 0;

    if (bulk_file_reader_.is_open()) {
      while (true) {
        HFSAT::GenericORSReplyStruct reply_struct;

        size_t read_length_ =
            bulk_file_reader_.read(reinterpret_cast<char*>(&reply_struct), sizeof(HFSAT::GenericORSReplyStruct));

        if (read_length_ < sizeof(HFSAT::GenericORSReplyStruct)) break;

        if (reply_struct.orr_type_ != HFSAT::kORRType_CxlRejc) continue;

        if (last_packet_send_time_ != 0) {
          unsigned int wait_time_ =
              (reply_struct.time_set_by_server_.tv_sec * 1000000 + reply_struct.time_set_by_server_.tv_usec) -
              last_packet_send_time_;

          std::cerr << " Wait Time : " << wait_time_ << "\n";

          int sec = (wait_time_ / 1000000);
          int usec = (wait_time_ % 1000000);

          if (sec || usec) {
            for (; sec > 0; sec--) {
              usleep(99999);
            }

            usleep(usec);
          }
        }

        last_packet_send_time_ =
            reply_struct.time_set_by_server_.tv_sec * 1000000 + reply_struct.time_set_by_server_.tv_usec;

        reply_struct.time_set_by_server_ = HFSAT::GetTimeOfDay();

        mcast_sender_socket_.WriteN(sizeof(HFSAT::GenericORSReplyStruct), (void*)&reply_struct);
      }
    }
  }

 private:
  HFSAT::GenericORSReplyStruct generic_ors_reply_struct_;
  std::string shortcode_;
  int yyyymmdd_;
  std::string ors_ip_;
  int ors_port_;
  std::string ors_interface_;

  HFSAT::MulticastSenderSocket mcast_sender_socket_;
};

int main(int argc, char** argv) {
  std::string dir = "";
  std::string shortcode_ = "";
  int yyyymmdd_ = 0;
  std::string ip_ = "";
  int port_ = 0;
  std::string interface_ = "";

  if (argc < 6) {
    std::cout << "Usage : SHORTCODE YYYYMMDD BCAST_IP BCAST_PORT INTERFACE" << std::endl;
    exit(0);
  } else {
    shortcode_ = std::string(argv[1]);
    yyyymmdd_ = atoi(argv[2]);
    ip_ = argv[3];
    port_ = atoi(argv[4]);
    interface_ = argv[5];
  }

  ORSBinReader common_logger(shortcode_, yyyymmdd_, ip_, port_, interface_);

  common_logger.processMsgRecvd();
}
