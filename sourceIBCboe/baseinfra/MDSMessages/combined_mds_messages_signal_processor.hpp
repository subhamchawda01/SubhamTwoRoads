// =====================================================================================
//
//       Filename:  combined_mds_messages_signal_processor.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  02/03/2014 09:29:53 AM
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

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/ExternalData/simple_external_data_live_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"

#include "dvccode/Utils/multicast_receiver_socket.hpp"

#include "dvccode/CDef/signal_msg.hpp"
#include "dvccode/ORSMessages/control_message_listener.hpp"
#include "dvccode/CDef/mds_shm_interface_defines.hpp"
#include "dvccode/CDef/mds_messages.hpp"

namespace HFSAT {

class CombinedMDSMessagesSignalProcessor {

 protected:
  std::map<std::string, std::vector<SignalDataListener*>> basename_to_signal_data_listener_map_;

 public:
  CombinedMDSMessagesSignalProcessor()
      : basename_to_signal_data_listener_map_(){
  }

  inline void AddSignalDataListener(const std::string& _basename_, SignalDataListener* _new_listener_){
    VectorUtils::UniqueVectorAdd(basename_to_signal_data_listener_map_[_basename_], _new_listener_);
  } 

  inline void RemoveSignalDataListener(const std::string& _basename_, SignalDataListener* _this_listener_){
    VectorUtils::UniqueVectorRemove(basename_to_signal_data_listener_map_[_basename_], _this_listener_);
  }

  inline void ProcessSignalEvent(HFSAT::IVCurveData * _iv_curve_data_){
    if(basename_to_signal_data_listener_map_.find(_iv_curve_data_->basename) != basename_to_signal_data_listener_map_.end()){
      for( auto & vec_itr : basename_to_signal_data_listener_map_[_iv_curve_data_->basename]){
        vec_itr->OnSignalDataUpdate(_iv_curve_data_->basename, *_iv_curve_data_);
      }
    }
  }

};
}
