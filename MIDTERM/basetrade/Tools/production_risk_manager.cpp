// =====================================================================================
//
//       Filename:  production_risk_monitor.cpp
//
//    Description:  Central exec for prod risk monitoring (runs on NY)
//
//        Version:  1.0
//        Created:  03/18/2016 05:55:07 PM
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

#include "dvctrade/RiskManager/risk_manager.hpp"

int main() {
  // Load IP/port from config file
  HFSAT::TagMappingListener tag_map_listener(PNL_SERVER_PORT);
  tag_map_listener.run();

  HFSAT::ClientListener risk_server(SERVER_PORT);
  risk_server.run();

  risk_server.stop();
  tag_map_listener.stop();
  return 0;
}
