// =====================================================================================
//
//       Filename:  BSEConnectionGatewayRequest.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  11/04/2012 12:12:14 PM
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

#include "infracore/BSE/BSEBSE/BSEMessageDefs.hpp"

namespace HFSAT {
namespace BSE {

class BSEConnectionGatewayRequest {
 public:
  GatewayRequestT bse_connection_gateway_request_;

 public:
  BSEConnectionGatewayRequest() {
    // initialize connection gateway request message
    memset((void*)(&bse_connection_gateway_request_), 0, sizeof(GatewayRequestT));

    // fill up the Messageheader
    bse_connection_gateway_request_.MessageHeaderIn.BodyLen = sizeof(GatewayRequestT);
    bse_connection_gateway_request_.MessageHeaderIn.TemplateID = TID_GATEWAY_REQUEST;

    // fill up the RequestHeader
    bse_connection_gateway_request_.RequestHeader.MsgSeqNum = 1;
    memcpy((void*)(&bse_connection_gateway_request_.DefaultCstmApplVerID), ETI_INTERFACE_VERSION,
           sizeof(ETI_INTERFACE_VERSION));

    // fill up the connection gateway request struct
    // No static fields here
  }

  // These fields would be read from ORS Config File
  void setBSEConnectionGatewayRequestStaticFields(const uint32_t _party_session_id_, const char* _password_,
                                                  const uint8_t _password_fill_length_) {
    bse_connection_gateway_request_.PartyIDSessionID = _party_session_id_;
    memcpy((void*)(bse_connection_gateway_request_.Password), (void*)(_password_), _password_fill_length_);
  }

  // get MsgLength to write
  int getBSEConnectionGatewayRequestMsgLength() { return (sizeof(GatewayRequestT)); }

  GatewayRequestT *ProcessGatewayRequest(char const *msg_ptr) {
    bse_connection_gateway_request_.MessageHeaderIn.BodyLen = *((uint32_t *)(msg_ptr + 0));
    bse_connection_gateway_request_.MessageHeaderIn.TemplateID = *((uint16_t *)(msg_ptr + 4));
    bse_connection_gateway_request_.RequestHeader.MsgSeqNum = *((uint32_t *)(msg_ptr + 16));
    bse_connection_gateway_request_.PartyIDSessionID = *((uint32_t *)(msg_ptr + 24));
    memcpy((void*)bse_connection_gateway_request_.DefaultCstmApplVerID,
           (void*)(msg_ptr + 28),LEN_DEFAULT_CSTM_APPL_VERID);
    memcpy((void*)bse_connection_gateway_request_.Password,
           (void*)(msg_ptr + 58),LEN_PASSWORD);

    return &bse_connection_gateway_request_;
  }

};
}
}
