// =====================================================================================
//
//       Filename:  network_account_interface_manager.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  02/12/2015 11:59:18 AM
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

#include "dvccode/TradingInfo/network_account_interface_manager.hpp"

namespace HFSAT {

NetworkAccountInterfaceManager* NetworkAccountInterfaceManager::inst = NULL;

NetworkAccountInterfaceManager& NetworkAccountInterfaceManager::instance() {
  if (inst == NULL) {
    inst = new NetworkAccountInterfaceManager();
  }
  return *inst;
}

void NetworkAccountInterfaceManager::RemoveInstance() {
  if (NULL != inst) {
    delete inst;
    inst = NULL;
  }
}
}
