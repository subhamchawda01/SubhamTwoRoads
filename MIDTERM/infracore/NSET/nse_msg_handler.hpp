// =====================================================================================
//
//       Filename:  nse_msg_handler.hpp
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

// Response Templates
#include "infracore/NSET/NSETemplates/ResponseHeader.hpp"
#include "infracore/NSET/NSETemplates/ResponsePacket.hpp"
#include "infracore/NSET/NSETemplates/LogonResponse.hpp"
#include "infracore/NSET/NSETemplates/SystemInformationResponse.hpp"
#include "infracore/NSET/NSETemplates/ResponseInnerHeader.hpp"
#include "infracore/NSET/NSETemplates/SecurityMasterUpdateDBResponse.hpp"
#include "infracore/NSET/NSETemplates/SecurityStatusUpdateInfoUpdateDBResponse.hpp"
#include "infracore/NSET/NSETemplates/ParticipantUpdateInfoUpdateDBResponse.hpp"
#include "infracore/NSET/NSETemplates/IndexMapDetailsUpdateDBResponse.hpp"
#include "infracore/NSET/NSETemplates/IndexUpdateInfoUpdateDBResponse.hpp"
#include "infracore/NSET/NSETemplates/InstrumentUpdateInfoUpdateDBResponse.hpp"
#include "infracore/NSET/NSETemplates/OrderResponses.hpp"
#include "infracore/NSET/NSETemplates/TradeConfirmationResponse.hpp"
#include "infracore/NSET/NSETemplates/OrderResponsesNonTR.hpp"
#include "infracore/NSET/NSETemplates/SpreadOrderResponses.hpp"
#include "infracore/NSET/NSETemplates/GRResponse.hpp"
#include "infracore/NSET/NSETemplates/BoxLoginResponse.hpp"
#include "infracore/NSET/NSETemplates/BoxSignOffResponse.hpp"

// Request Templates
#include "infracore/NSET/NSETemplates/RequestHeader.hpp"
#include "infracore/NSET/NSETemplates/RequestPacket.hpp"
#include "infracore/NSET/NSETemplates/LogonRequest.hpp"
#include "infracore/NSET/NSETemplates/LogonRequestDerivatives.hpp"
#include "infracore/NSET/NSETemplates/LogoffRequest.hpp"
#include "infracore/NSET/NSETemplates/SystemInformation.hpp"
#include "infracore/NSET/NSETemplates/UpdateLocalDatabaseRequest.hpp"
#include "infracore/NSET/NSETemplates/OrderEntry.hpp"
#include "infracore/NSET/NSETemplates/OrderChangeRequest.hpp"
#include "infracore/NSET/NSETemplates/SpreadOrderEntry.hpp"
#include "infracore/NSET/NSETemplates/Heartbeat.hpp"
#include "infracore/NSET/NSETemplates/GRRequest.hpp"
#include "infracore/NSET/NSETemplates/BoxLoginRequest.hpp"

namespace HFSAT {
namespace NSE {

/*
 * This serves as an interface to handle various message requests and responses from exchange in all segment type.
 * The FO and cash segment message structures have slight differences and to be able to use a single NSEEngine,
 * we have carved out the handler interface.
 */

class NseMsgHandler {
 public:
  // Header msg handlers
  NSEPacketResponse packet_response_;
  ResponseHeader response_header_;
  ResponseInnerHeader response_inner_header_;

  // Heartbear msg
  NSEHeartbeat heartbeat_request_;

  // Logon msg handlers
  NSELogonRequest* logon_request_;
  NSEGRRequest* gr_request_;
  NSEBoxLoginRequest* boxlogin_request_;
  NSESystemInfoRequest* system_info_request_;
  NSEUpdateLocalDatabaseRequest* update_local_db_request_;
  NSELogonResponse* logon_response_;
  NSEGRResponse* gr_response_;
  NSEBoxLoginResponse* boxlogin_response_;
  NSEBoxSignOffResponse* boxsignoff_response_;
  SystemInfoResponse* system_info_response_;

  // Logout msg handlers
  LogoffRequest logout_request_;

  // Order Requests handlers
  OrderEntryRequest* new_order_;
  OrderCancelRequest* cancel_order_;
  OrderModifyRequest* modify_order_;
  SpreadOrderEntryRequest spread_new_order_;
  SpreadOrderCancelRequest spread_cancel_order_;
  SpreadOrderCancelRequest spread_modify_order_;

  // Order Response handlers
  OrderResponse* order_response_;
  TradeConfirmationResponse* trade_conf_response_;
  OrderResponseNonTR order_response_non_tr_;
  SpreadOrderResponse spread_order_response_;

  // Initiliasing all common structures in constructor here.
  // All other structures with different definition will be initialised in corresponding derived class
  NseMsgHandler()
      : packet_response_(),
        response_header_(),
        response_inner_header_(),
        heartbeat_request_(),
        logout_request_(),
        spread_new_order_(),
        spread_cancel_order_(),
        spread_modify_order_(),
        order_response_non_tr_(),
        spread_order_response_() { 

          gr_request_ = new NSEGRRequest();
          gr_response_ = new NSEGRResponse();

          boxlogin_request_ = new NSEBoxLoginRequest();
          boxlogin_response_ = new NSEBoxLoginResponse();

          boxsignoff_response_ = new NSEBoxSignOffResponse();
  }
  virtual ~NseMsgHandler() {}

  // Should be implemented by each derived class. It initializes all the different structures in CM & FO
  virtual void InitialiseMsgStructures() = 0;
};
}
}
