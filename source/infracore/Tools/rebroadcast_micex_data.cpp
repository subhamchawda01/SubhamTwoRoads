/*
 * =====================================================================================
 *
 *       Filename:  rebroadcast_mos_data.cpp
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
#include "dvccode/CDef/mds_messages.hpp"

int main(int argc, char** argv) {
  HFSAT::NetworkAccountInfoManager network_account_info_manager_;

  HFSAT::DataInfo data_info_mkt_data = network_account_info_manager_.GetSrcDataInfo(HFSAT::kExchSourceRTS, "Si_0");
  std::string receive_ip_ = data_info_mkt_data.bcast_ip_;
  int receive_port_ = data_info_mkt_data.bcast_port_;

  std::string rebroadcast_ip_1 = receive_ip_;
  int rebroadcast_port_1 = receive_port_ + 1;

  std::string rebroadcast_ip_2 = receive_ip_;
  int rebroadcast_port_2 = receive_port_ + 2;

  HFSAT::MulticastReceiverSocket* multicast_receiver_socket_ = new HFSAT::MulticastReceiverSocket(
      receive_ip_, receive_port_,
      HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceBMF, HFSAT::k_MktDataLive));

  HFSAT::MulticastSenderSocket* multicast_sender_socket_1 = new HFSAT::MulticastSenderSocket(
      rebroadcast_ip_1, rebroadcast_port_1,
      HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceBMF, HFSAT::k_MktDataLive));

  HFSAT::MulticastSenderSocket* multicast_sender_socket_2 = new HFSAT::MulticastSenderSocket(
      rebroadcast_ip_2, rebroadcast_port_2,
      HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceBMF, HFSAT::k_MktDataLive));
  //  HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceMICEX, HFSAT::k_MktDataMcast)

  HFSAT::MDS_MSG::GenericMDSMessage cstr_;

  std::cerr << " Receive : " << receive_ip_ << "  " << receive_port_ << " "
            << HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceBMF,
                                                                              HFSAT::k_MktDataLive)
            << "\n";
  std::cerr << " Send 1: " << rebroadcast_ip_1 << " " << rebroadcast_port_1 << " "
            << HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceBMF,
                                                                              HFSAT::k_MktDataLive)
            << "\n";
  std::cerr << " Send 2: " << rebroadcast_ip_2 << " " << rebroadcast_port_2 << " "
            << HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceBMF,
                                                                              HFSAT::k_MktDataLive)
            << "\n";

  int cstr_struct_bytes_ = 0;
  int micex_struct_bytes_ = 0;
  int rts_struct_bytes_ = 0;

  cstr_struct_bytes_ = sizeof(HFSAT::MDS_MSG::GenericMDSMessage);
  micex_struct_bytes_ = sizeof(MICEX_MDS::MICEXCommonStruct);
  rts_struct_bytes_ = sizeof(RTS_MDS::RTSCommonStruct);

  // TODO a single threaded rebroadcaster can fill up onload stack, while it's writing to socket, check data flow

  while (true) {
    int ret_val_ = multicast_receiver_socket_->ReadN(cstr_struct_bytes_, &cstr_);

    if (ret_val_ == -1) {
      std::cerr << " Read Error : " << strerror(errno) << "\n";
      break;

    } else if (ret_val_ == 0) {
      std::cerr << "Socket Closed \n";
      break;

    } else if (ret_val_ != cstr_struct_bytes_) {
      std::cerr << " Malformatted Data Read : " << ret_val_ << " Bytes \n";
      break;

    } else {
      if (cstr_.mds_msg_exch_ == HFSAT::MDS_MSG::MICEX) {
        multicast_sender_socket_1->WriteN(micex_struct_bytes_, &(cstr_.generic_data_.micex_data_));
      } else if (cstr_.mds_msg_exch_ == HFSAT::MDS_MSG::RTS) {
        multicast_sender_socket_2->WriteN(rts_struct_bytes_, &(cstr_.generic_data_.rts_data_));
      } else {
        std::cerr << "Should not get this exchange data " << cstr_.mds_msg_exch_ << "\n";
      }
    }
  }

  delete multicast_receiver_socket_;
  delete multicast_sender_socket_1;
  delete multicast_sender_socket_2;

  return 0;
}
