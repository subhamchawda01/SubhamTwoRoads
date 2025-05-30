// =====================================================================================
//
//       Filename:  ose_l1_data_generator_via_shm_pricefeed.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  02/22/2013 07:26:33 PM
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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <getopt.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <map>

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/Utils/mds_logger.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/Utils/multicast_receiver_socket.hpp"

std::string exch_ = "";

class LiveMDSLogger {
 protected:
  bool set_time;  // set time by logger. we already expect the m-casted data to have time, but we want the logger
                  // time-stamp to be set explicitly

  HFSAT::MulticastReceiverSocket mcast_receiver_socket_;
  HFSAT::MulticastSenderSocket mcast_sender_socket_;

 public:
  LiveMDSLogger(std::string exch_, bool set_time_, std::string _pf_bcast_ip_, int _pf_bcast_port_,
                std::string _bcast_ip_, int _bcast_port_)
      : set_time(set_time_),
        mcast_receiver_socket_(_pf_bcast_ip_, _pf_bcast_port_,
                               HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceJPY,
                                                                                              HFSAT::k_MktDataLive)),
        mcast_sender_socket_(_bcast_ip_, _bcast_port_, HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(
                                                           HFSAT::kExchSourceJPY, HFSAT::k_MktDataLive))

  {}

  ~LiveMDSLogger() {}

  void runLogger() {
    OSE_MDS::OSECombinedCommonStruct cstr_;
    OSE_MDS::OSEPLCommonStruct ose_l1_event_;

    memset((void*)&cstr_, 0, sizeof(OSE_MDS::OSECombinedCommonStruct));

    while (true) {
      int read_length = mcast_receiver_socket_.ReadN(sizeof(OSE_MDS::OSECombinedCommonStruct), &cstr_);

      if (read_length < (int32_t)sizeof(OSE_MDS::OSECombinedCommonStruct)) {
        std::cerr << " Expected : " << sizeof(OSE_MDS::OSECombinedCommonStruct) << " Received : " << read_length
                  << "\n";
        exit(1);
      }

      if (((OSE_MDS::FEED_NEW == cstr_.feed_msg_type_ || OSE_MDS::FEED_CHANGE == cstr_.feed_msg_type_) &&
           (1 == cstr_.price_feed_level_)) ||
          (OSE_MDS::FEED_TRADE == cstr_.feed_msg_type_)) {
        memset((void*)(&ose_l1_event_), 0, sizeof(OSE_MDS::OSEPLCommonStruct));
        memcpy((void*)(&ose_l1_event_.time_), (void*)(&cstr_.time_), sizeof(struct timeval));
        memcpy((void*)(ose_l1_event_.contract_), (void*)(cstr_.contract_), OSE_NEW_MDS_CONTRACT_TEXT_SIZE);

        memcpy((void*)(&ose_l1_event_.price), (void*)(&cstr_.price_), sizeof(cstr_.price_));
        memcpy((void*)(&ose_l1_event_.size), (void*)(&cstr_.agg_size_), sizeof(cstr_.agg_size_));
        memcpy((void*)(&ose_l1_event_.order_count_), (void*)(&cstr_.order_count_), sizeof(cstr_.order_count_));

        if (OSE_MDS::FEED_TRADE == cstr_.feed_msg_type_)
          ose_l1_event_.type_ = 2;
        else
          ose_l1_event_.type_ = cstr_.type_ - 1;

        mcast_sender_socket_.WriteN(sizeof(OSE_MDS::OSEPLCommonStruct), &ose_l1_event_);
      }
    }
  }

  void closeFiles() {}
};

void* logger;

/// signal handling
void sighandler(int signum) {
  if (logger != NULL) {
    if (exch_.compare("OSEPriceFeed") == 0) {
      ((LiveMDSLogger*)logger)->closeFiles();
    }
  }
  exit(0);
}

int main(int argc, char** argv) {
  /// set signal handler .. add other signals later
  struct sigaction sigact;
  sigact.sa_handler = sighandler;
  sigaction(SIGINT, &sigact, NULL);

  std::string pf_bcast_ip_ = "";
  int pf_bcast_port_ = -1;

  std::string bcast_ip_ = "";
  int bcast_port_ = -1;

  if (argc < 5) {
    std::cerr << " Usage : < exec > < pf_bcast_ip > < pf_bcast_port > < bcast_ip > < bcast_port > \n";
    exit(-1);
  }

  pf_bcast_ip_ = argv[1];
  pf_bcast_port_ = atoi(argv[2]);

  bcast_ip_ = argv[3];
  bcast_port_ = atoi(argv[4]);

  // true is for settime
  logger = (void*)(new LiveMDSLogger("OSEPriceFeed", true, pf_bcast_ip_, pf_bcast_port_, bcast_ip_, bcast_port_));
  ((LiveMDSLogger*)logger)->runLogger();

  return 0;
}
