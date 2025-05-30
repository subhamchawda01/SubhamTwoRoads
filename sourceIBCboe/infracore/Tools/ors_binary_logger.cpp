/**
   \file Tools/ors_binary_logger.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551

*/
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <fstream>
#include <signal.h>
#include <getopt.h>
#include <map>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/file_utils.hpp"  // To create the directory
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CDef/defines.hpp"

#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/CDef/ors_messages.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"

#define LOGGED_DATA_PREFIX "/spare/local/ORSBCAST_MULTISHM/"

std::string exch_ = "";

void print_usage(const char* prg_name) {
  printf("This is the ORS Binary Data Logger \n");
  printf("Usage:%s --exch <EXCHANGE> --ip <bcast_address> --port <bcast_port> \n", prg_name);
}

static struct option options_[] = {{"help", no_argument, 0, 'h'},
                                   {"exch", required_argument, 0, 'a'},
                                   {"ip", required_argument, 0, 'b'},
                                   {"port", required_argument, 0, 'c'},
                                   {0, 0, 0, 0}};

class ORSBinLogger {
 public:
  ORSBinLogger(std::string ip, int port, std::string t_exchange_name)
      : ip_(ip), port_(port), exchange_name_(t_exchange_name) {
    socket_ = new HFSAT::MulticastReceiverSocket(
        ip_, port_, HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(HFSAT::k_ORS_LS));
    if (socket_ == NULL) {
      std::cout << " Failed to get the Receiver socket. ";
    }
  }

  void processMsgRecvd() {
    while (true) {
      int num_bytes = socket_->ReadN(sizeof(HFSAT::GenericORSReplyStructLiveProShm), (void*)&generic_ors_reply_struct_);

      if (num_bytes >= (int)sizeof(HFSAT::GenericORSReplyStructLiveProShm)) {
        // Write to the file
        std::string symbol_got(generic_ors_reply_struct_.symbol_);
        if (exchange_name_ == "LIFFE") {
          std::replace(symbol_got.begin(), symbol_got.end(), ' ', '~');
        }
        // std::cout << " Symbol got for message: "<< symbol_got << " : " << generic_ors_reply_struct_.symbol_;
        // Find the ofstream and write it
        if (exchange_symbol_to_ors_bcast_log_map_.find(symbol_got) == exchange_symbol_to_ors_bcast_log_map_.end()) {
          // Add to the map
          //		std::cout << " Added : "<< symbol_got << std::endl;
          AddToMap(symbol_got);
          // First add then write to file
        }
        std::ofstream* file_streamer = exchange_symbol_to_ors_bcast_log_map_[symbol_got];
        file_streamer->write((char*)(&generic_ors_reply_struct_), num_bytes);
        file_streamer->flush();
      }
    }
  }

  void AddToMap(std::string exchange_symbol) {
    time_t m_time_t;
    time(&m_time_t);
    struct tm m_tm;
    localtime_r(&m_time_t, &m_tm);
    unsigned int yyyymmdd = (1900 + m_tm.tm_year) * 10000 + (1 + m_tm.tm_mon) * 100 + m_tm.tm_mday;
    char date_str[9] = {'\0'};
    sprintf(date_str, "%d", yyyymmdd);
    std::string filename = LOGGED_DATA_PREFIX + exchange_name_ + "/" + exchange_symbol + "_" + date_str;
    std::cout << "Total Filename :" << filename << std::endl;
    std::ofstream* new_file = new std::ofstream();

    HFSAT::FileUtils::MkdirEnclosing(filename);

    new_file->open(filename.c_str(), std::ofstream::binary | std::ofstream::app | std::ofstream::ate);
    if (!new_file->is_open()) {
      fprintf(stderr, "ors_bcast_binary could not open file %s for writing \n", filename.c_str());
      exit(-1);
    }
    exchange_symbol_to_ors_bcast_log_map_[exchange_symbol] = new_file;
  }

 private:
  HFSAT::MulticastReceiverSocket* socket_;
  std::string ip_;
  int port_;
  HFSAT::GenericORSReplyStructLiveProShm generic_ors_reply_struct_;
  std::string exchange_name_;
  std::map<std::string, std::ofstream*> exchange_symbol_to_ors_bcast_log_map_;
};

void sighandler(int signum) { exit(0); }
int main(int argc, char** argv) {
  //-------------- START OPTIONS--------------
  int c;
  int hflag = 0;
  /// TODO currently picks up ip/port from commandline .. can be changed to networkinfo class later
  std::string ip_ = "";
  int port_ = -1;

  /// parse input options
  while (1) {
    int option_index = 0;
    c = getopt_long(argc, argv, "", options_, &option_index);
    if (c == -1) break;
    switch (c) {
      case 'h':
        hflag = 1;
        break;

      case 'a':
        exch_ = optarg;
        break;

      case 'b':
        ip_ = optarg;
        break;

      case 'c':
        port_ = atoi(optarg);
        break;

      case '?':
        if (optopt != 'h') {
          fprintf(stderr, "Option %c requires an argument .. will exit \n", optopt);
          print_usage(argv[0]);
          exit(-1);
        }
        break;

      default:
        fprintf(stderr, "Weird option specified .. no handling yet \n");
        break;
    }
  }

  if (hflag || exch_.empty() || ip_.empty() || port_ == -1) {
    print_usage(argv[0]);
    exit(-1);
  }
  ///--------------END OPTIONS------------------------

  //@Default Case is CME
  int global_port = 17107;
  std::string global_ip = "225.2.3.2";
  std::string exchange_name = "CME";

  // Trading location
  exchange_name = exch_;
  global_port = port_;
  global_ip = ip_;

  std::cout << "  Exch: " << exchange_name << " Port: " << global_port << " IP : " << global_ip << std::endl;

  // std::string exchange_name; // Get from command
  ORSBinLogger common_logger(global_ip, global_port, exchange_name);

  common_logger.processMsgRecvd();

  struct sigaction sigact;
  sigact.sa_handler = sighandler;
  sigaction(SIGINT, &sigact, NULL);
}
