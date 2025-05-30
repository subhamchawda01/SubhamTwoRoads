// =====================================================================================
// 
//       Filename:  signal_data_into_control_msg_broadcaster.hpp
// 
//    Description:  
// 
//        Version:  1.0
//        Created:  Monday 17 April 2023 11:02:49  UTC
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


#pragma once

#include <iostream>
#include <cstdlib>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <fstream>
#include <signal.h>
#include <getopt.h>
#include <map>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/signal_msg.hpp"
#include "dvccode/CDef/control_messages.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#define SIGNAL_DATA_SENDER_COFNIG_FILEPATH "/home/pengine/prod/live_configs/"

class SignalDataSender{

  private: 
    HFSAT::MulticastSenderSocket *mcast_sender_socket_;

  public:
    SignalDataSender(){

      char this_hostname_[64];
      memset((void*)this_hostname_, 0, sizeof(this_hostname_));
      gethostname(this_hostname_, sizeof(this_hostname_) - 1);

      std::string signal_data_sender_config_file = std::string(SIGNAL_DATA_SENDER_COFNIG_FILEPATH) + std::string(this_hostname_) + "_signal_data_sender_config.txt";

      if(false == HFSAT::FileUtils::ExistsAndReadable(signal_data_sender_config_file)){
        std::cerr << "File : " << signal_data_sender_config_file << " Either Doesn't Exists or Not Readable" << std::endl;
        std::exit(-1);
      }

      std::ifstream sds_file;
      sds_file.open(signal_data_sender_config_file.c_str(),std::ifstream::in);
      char line_buffer[1024];
      while (sds_file.good()) {
        sds_file.getline(line_buffer, 1024);

        std::string line = line_buffer;
        if (line.find("#") != std::string::npos) continue;  // comments

        HFSAT::PerishableStringTokenizer pst(line_buffer, 1024);
        std::vector<char const *> const &tokens = pst.GetTokens();

        if (tokens.size() < 3) {
          std::cerr << "MALFORMED LINE : " << line << std::endl;
          continue;
        }

        std::string mcast_out_ip = std::string(tokens[0]);
        int32_t mcast_out_port = std::atoi(tokens[1]);
        std::string out_interface = std::string(tokens[2]);
        std::cout << " Created Multicast Sender Socket FOr Signal : " << mcast_out_ip << " " << mcast_out_port << " " << out_interface << std::endl;
        mcast_sender_socket_ = new HFSAT::MulticastSenderSocket(mcast_out_ip, mcast_out_port, out_interface);
        break;

      }

    }

    int32_t DispatchSignalDataOverMulticast(HFSAT::IVCurveData *data){
      return mcast_sender_socket_->WriteN(sizeof(HFSAT::IVCurveData), data);
    }

};
