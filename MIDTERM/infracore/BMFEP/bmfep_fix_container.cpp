/**
   \file BMFEP/bmfep_fix_container.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
 */

#include "infracore/BMFEP/bmfep_fix_container.hpp"

namespace HFSAT {
namespace ORS {

BMFEPFixContainer::BMFEPFixContainer(bool make_optimize_assumptions)
    : logon(make_optimize_assumptions),
      logout(make_optimize_assumptions),
      heartbeat(make_optimize_assumptions),
      test_request_heartbeat_(make_optimize_assumptions),
      test_request(make_optimize_assumptions),
      exec_report(make_optimize_assumptions),
      resend_request(make_optimize_assumptions),
      seq_reset(make_optimize_assumptions) {}

BMFEPFixContainer::~BMFEPFixContainer() {
  for (int i = 0; i < DEF_MAX_SEC_ID; i++) {
    if (new_order_map[i]) {
      delete new_order_map[i];
      new_order_map[i] = nullptr;
    }

    if (cancel_order_map[i]) {
      delete cancel_order_map[i];
      cancel_order_map[i] = nullptr;
    }

    if (cancel_replace_map[i]) {
      delete cancel_replace_map[i];
      cancel_replace_map[i] = nullptr;
    }
  }
}

}  // ORS
}  // HFSAT
