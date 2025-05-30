// =====================================================================================
//
//       Filename:  control_hub_listener.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  08/17/2015 09:50:00 AM
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

namespace HFSAT {
namespace Utils {

class ControlHubListener {
 public:
  virtual void OnControlHubConnectionFailure(){};     // currently not using this
  virtual void OnControlHubConnectionResumption(){};  // and this as well
  virtual void OnControlHubDisconnectionShutdown(){};
  virtual ~ControlHubListener(){};
};
}
}
