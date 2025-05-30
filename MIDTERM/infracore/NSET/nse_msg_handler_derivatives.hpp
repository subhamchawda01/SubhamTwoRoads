// =====================================================================================
//
//       Filename:  nse_msg_handler_derivatives.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  06/18/2014 08:30:37 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 353, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#pragma once
#include "infracore/NSET/nse_msg_handler.hpp"
#include "infracore/NSET/NSETemplates/SystemInformationResponseDerivatives.hpp"
#include "infracore/NSET/NSETemplates/OrderResponsesDerivatives.hpp"
#include "infracore/NSET/NSETemplates/OrderEntryDerivatives.hpp"
#include "infracore/NSET/NSETemplates/LogonResponseDerivatives.hpp"
#include "infracore/NSET/NSETemplates/TradeConfirmationResponseDerivatives.hpp"
#include "infracore/NSET/NSETemplates/LogonRequestDerivatives.hpp"
#include "infracore/NSET/NSETemplates/OrderChangeRequestDerivatives.hpp"
#include "infracore/NSET/NSETemplates/SystemInformationDerivatives.hpp"
#include "infracore/NSET/NSETemplates/UpdateLocalDatabaseRequestDerivatives.hpp"

namespace HFSAT {
namespace NSE {

/*
 * This implements the interface for derivatives structures. All the message templates which differ
 * from CM are initialized here. This will be helpful in engine to be able to make generic requests and responses.
 */

class NseMsgHandlerDerivatives : public NseMsgHandler {
 public:
  void InitialiseMsgStructures() {
    logon_request_ = new NSELogonRequestDerivatives();
    system_info_request_ = new NSESystemInfoRequestDerivatives();
    system_info_response_ = new SystemInfoResponseDerivatives();
    logon_response_ = new NSELogonResponseDerivatives();
    new_order_ = new OrderEntryRequestDerivatives();
    cancel_order_ = new OrderCancelRequestDerivatives();
    modify_order_ = new OrderModifyRequestDerivatives();
    order_response_ = new OrderResponseDerivatives();
    trade_conf_response_ = new TradeConfirmationResponseDerivatives();
    update_local_db_request_ = new NSEUpdateLocalDatabaseRequestDerivatives();
  }
  NseMsgHandlerDerivatives() { InitialiseMsgStructures(); }
};
}
}
