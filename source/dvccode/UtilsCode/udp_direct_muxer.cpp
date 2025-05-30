// =====================================================================================
// 
//       Filename:  udp_direct_muxer.cpp
// 
//    Description:  
// 
//        Version:  1.0
//        Created:  05/16/2022 03:42:04 AM
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


#include "dvccode/Utils/udp_direct_muxer.hpp"

#define DEFAULT_EVENTS_TIMEOUT 100000

namespace HFSAT {
namespace Utils {

  UDPDirectMuxer::UDPDirectMuxer():
    interface_to_udp_direct_zocket_(),
    event_timeout_listener_(NULL),
    nse_zocket_muxer_(HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceNSE, HFSAT::k_MktDataRaw)),
    bse_zocket_muxer_(HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceBSE, HFSAT::k_MktDataRaw))
  {
    interface_to_udp_direct_zocket_[HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceNSE, HFSAT::k_MktDataRaw)] = &nse_zocket_muxer_;
    interface_to_udp_direct_zocket_[HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceBSE, HFSAT::k_MktDataRaw)] = &bse_zocket_muxer_;
  }

  void UDPDirectMuxer::InitializeUDPZocketOverInterface(std::string _interface_){
    return;
    if(std::string::npos != _interface_.find("enp101s0f0")){
//      nse_zocket_muxer_ = interface_to_udp_direct_zocket_[_interface_] ;
    }else{
//      bse_zocket_muxer_ = interface_to_udp_direct_zocket_[_interface_] ;
    }
  }

  UDPDirectMultipleZocket * UDPDirectMuxer::GetUDPZocketForInterface(std::string _interface_){

    if(interface_to_udp_direct_zocket_.end() == interface_to_udp_direct_zocket_.find(_interface_)) return NULL;
    return interface_to_udp_direct_zocket_[_interface_];

  }

  void UDPDirectMuxer::AddEventTimeoutNotifyListener(EventTimeoutListener *_listener_){
    event_timeout_listener_ = _listener_;
  }

  void UDPDirectMuxer::RunMuxerLiveDispatcherWithTimeOut(int64_t _events_timeout_){

    int64_t evt_counter = 0 ;

    while(true){

      nse_zocket_muxer_.ProcessEventsFromZocket();
      bse_zocket_muxer_.ProcessEventsFromZocket();

//      for(auto & itr : interface_to_udp_direct_zocket_){
//        std::cout << " ITR : " << itr.first << std::endl ;
//        (itr.second)->ProcessEventsFromZocket();
//        std::cout << " ITR END : " << itr.first << std::endl ;
//      }

      evt_counter++;

      if(_events_timeout_ == evt_counter){
//        std::cout << "EVENT TIMEOUT : "<< std::endl;
        event_timeout_listener_->OnEventsTimeout();
        evt_counter = 0 ;
      }

    }
  }
}
}
