/**
   \file BMFEP/bmfep_fix_container.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
 */

#pragma once

#include "infracore/BasicOrderRoutingServer/defines.hpp"

#include "infracore/BMFEP/BMFEPFIX/BMFEPFIXTypes.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPMessageDefs.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPLogon.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPLogout.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPHeartbeat.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPTestRequest.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPNewOrder.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPCancelOrder.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPCancelReplace.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPResendRequest.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPSeqReset.hpp"
#include "infracore/BMFEP/BMFEPFIX/BMFEPExecutionReport.hpp"

namespace HFSAT {
namespace ORS {

class BMFEPFixContainer {
 private:
 public:
  BMFEPFIX::Logon logon;
  BMFEPFIX::Logout logout;
  BMFEPFIX::Heartbeat heartbeat;
  BMFEPFIX::Heartbeat test_request_heartbeat_;
  BMFEPFIX::TestRequest test_request;
  BMFEPFIX::ExecutionReport exec_report;
  BMFEPFIX::ResendRequest resend_request;
  BMFEPFIX::SeqReset seq_reset;

  BMFEPFIX::NewOrder* new_order_map[DEF_MAX_SEC_ID];
  BMFEPFIX::CancelOrder* cancel_order_map[DEF_MAX_SEC_ID];
  BMFEPFIX::CancelReplace* cancel_replace_map[DEF_MAX_SEC_ID];

  BMFEPFixContainer(bool make_optimize_assumptions);
  ~BMFEPFixContainer();
};
}
}
