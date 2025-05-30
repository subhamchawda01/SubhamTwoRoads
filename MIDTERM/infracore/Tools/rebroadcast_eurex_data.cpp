/*
 * =====================================================================================
 *
 *       Filename:  rebroadcast_eurex_data.cpp
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  07/05/2012 10:29:27 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011 (),
 *   Organization:
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <iostream>
#include <errno.h>
#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage <exec> <bcast_ip> <bcast_port> \n";
    exit(1);
  }

  std::string rebroadcast_ip_ = argv[1];
  int rebroadcast_port_ = atoi(argv[2]);

  HFSAT::NetworkAccountInfoManager network_account_info_manager_;

  HFSAT::DataInfo data_info_ = network_account_info_manager_.GetSrcDataInfo(HFSAT::kExchSourceEUREX, "FESX_0");

  HFSAT::MulticastReceiverSocket* multicast_receiver_socket_ = new HFSAT::MulticastReceiverSocket(
      data_info_.bcast_ip_, data_info_.bcast_port_,
      HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceEUREX, HFSAT::k_MktDataLive));

  HFSAT::MulticastSenderSocket* multicast_sender_socket_ = new HFSAT::MulticastSenderSocket(
      rebroadcast_ip_, rebroadcast_port_,
      HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceEUREX, HFSAT::k_MktDataMcast));

  EUREX_MDS::EUREXLSCommonStruct eurex_struct_;

  std::cerr << " Receive : " << data_info_.bcast_ip_ << "  " << data_info_.bcast_port_ << " "
            << HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceEUREX,
                                                                              HFSAT::k_MktDataLive)
            << "\n";
  std::cerr << " Send : " << rebroadcast_ip_ << " " << rebroadcast_port_ << " "
            << HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceEUREX,
                                                                              HFSAT::k_MktDataMcast)
            << "\n";

  int eurex_struct_bytes_ = 0;

  eurex_struct_bytes_ = sizeof(EUREX_MDS::EUREXLSCommonStruct);

  // TODO a single threaded rebroadcaster can fill up onload stack, while it's writing to socket, check data flow

  while (true) {
    int ret_val_ = multicast_receiver_socket_->ReadN(eurex_struct_bytes_, &eurex_struct_);

    if (ret_val_ == -1) {
      std::cerr << " Read Error : " << strerror(errno) << "\n";
      break;

    } else if (ret_val_ == 0) {
      std::cerr << "Socket Closed \n";
      break;

    } else if (ret_val_ != eurex_struct_bytes_) {
      std::cerr << " Malformatted Data Read : " << ret_val_ << " Bytes \n";
      break;

    } else {
      multicast_sender_socket_->WriteN(eurex_struct_bytes_, &eurex_struct_);
    }
  }

  delete multicast_receiver_socket_;
  delete multicast_sender_socket_;

  return 0;
}
