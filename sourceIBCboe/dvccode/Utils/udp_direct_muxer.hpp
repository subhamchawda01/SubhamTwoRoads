// =====================================================================================
// 
//       Filename:  udp_direct_muxer.hpp
// 
//    Description:  
// 
//        Version:  1.0
//        Created:  05/14/2022 05:26:21 PM
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

#include <zf/zf.h>
#include <zf/zf_utils.h>

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <vector>
#include <sstream>

#include "dvccode/Utils/tcp_direct_client_zocket_with_logging.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"

namespace HFSAT {
namespace Utils {

  class UDPDirectMuxer{

    private :

      std::map<std::string, UDPDirectMultipleZocket *> interface_to_udp_direct_zocket_; 
      EventTimeoutListener *event_timeout_listener_;

      UDPDirectMultipleZocket nse_zocket_muxer_;
      UDPDirectMultipleZocket bse_zocket_muxer_;

      UDPDirectMuxer(UDPDirectMuxer const &disabled_copy_constructor) = delete;

    public : 

      UDPDirectMuxer();

      static UDPDirectMuxer &GetUniqueInstance(){
        static UDPDirectMuxer unique_instance;
        return unique_instance;
      }

      void InitializeUDPZocketOverInterface(std::string _interface_);
      UDPDirectMultipleZocket * GetUDPZocketForInterface(std::string _interface_);
      void AddEventTimeoutNotifyListener(EventTimeoutListener *_listener_);

      void RunMuxerLiveDispatcherWithTimeOut(int64_t _timeout_);
  };

}
}
