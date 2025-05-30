// =====================================================================================
//
//       Filename:  nse_msg_handler_cash_market.hpp
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
#include "infracore/NSET/NSETemplates/SystemInformationResponseCashMarket.hpp"
#include "infracore/NSET/NSETemplates/OrderResponsesCashMarket.hpp"
#include "infracore/NSET/NSETemplates/OrderEntryCashMarket.hpp"
#include "infracore/NSET/NSETemplates/LogonResponseCashMarket.hpp"
#include "infracore/NSET/NSETemplates/TradeConfirmationResponseCashMarket.hpp"
#include "infracore/NSET/NSETemplates/LogonRequestCashMarket.hpp"
#include "infracore/NSET/NSETemplates/OrderChangeRequestCashMarket.hpp"
#include "infracore/NSET/NSETemplates/SystemInformationCashMarket.hpp"
#include "infracore/NSET/NSETemplates/UpdateLocalDatabaseRequestCashMarket.hpp"

namespace HFSAT {
namespace NSE {

/*
 * This implements the interface for cash market structures. All the message templates which differ
 * from FO are initialised here. This will be helpful in engine to be able to make generic requests and responses.
 */

class NseMsgHandlerCashMarket : public NseMsgHandler {
 public:
  void InitialiseMsgStructures() {
    logon_request_ = new NSELogonRequestCashMarket();
    system_info_request_ = new NSESystemInfoRequestCashMarket();
    system_info_response_ = new SystemInfoResponseCashMarket();
    logon_response_ = new NSELogonResponseCashMarket();
    new_order_ = new OrderEntryRequestCashMarket();
    cancel_order_ = new OrderCancelRequestCashMarket();
    modify_order_ = new OrderModifyRequestCashMarket();
    order_response_ = new OrderResponseCashMarket();
    trade_conf_response_ = new TradeConfirmationResponseCashMarket();
    update_local_db_request_ = new NSEUpdateLocalDatabaseRequestCashMarket();
  }
  NseMsgHandlerCashMarket() { InitialiseMsgStructures(); }
};
}
}
