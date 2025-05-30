// =====================================================================================
//
//       Filename:  BSEConnectionGatewayResponse.hpp
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

class BSEConnectionGatewayResponse {
 private:
  GatewayResponseT bse_connection_gateway_response_;

 public:
  BSEConnectionGatewayResponse() { memset((void*)(&bse_connection_gateway_response_), 0, sizeof(GatewayResponseT)); }
  ~BSEConnectionGatewayResponse() {}

  GatewayResponseT *ProcessGatewayResponse(char const *msg_ptr) {
    bse_connection_gateway_response_.MessageHeaderOut.BodyLen = *((uint32_t *)(msg_ptr + 0));
    bse_connection_gateway_response_.MessageHeaderOut.TemplateID = *((uint16_t *)(msg_ptr + 4));
    bse_connection_gateway_response_.ResponseHeader.RequestTime = *((uint64_t *)(msg_ptr + 8));
    bse_connection_gateway_response_.ResponseHeader.SendingTime = *((uint64_t *)(msg_ptr + 16));
    bse_connection_gateway_response_.ResponseHeader.MsgSeqNum = *((uint32_t *)(msg_ptr + 24));
    bse_connection_gateway_response_.GatewayID = *((uint32_t *)(msg_ptr + 32));
    bse_connection_gateway_response_.GatewaySubID = *((uint32_t *)(msg_ptr + 36));
    bse_connection_gateway_response_.SecondaryGatewayID = *((uint32_t *)(msg_ptr + 40));
    bse_connection_gateway_response_.SecondaryGatewaySubID = *((uint32_t *)(msg_ptr + 44));
    bse_connection_gateway_response_.SessionMode = *((uint8_t *)(msg_ptr + 48));
    bse_connection_gateway_response_.TradSesMode = *((uint8_t *)(msg_ptr + 49));
    memcpy((void *)bse_connection_gateway_response_.cryptographic_key, (void *)(msg_ptr + 50), 32);
    memcpy((void *)bse_connection_gateway_response_.cryptographic_iv, (void *)(msg_ptr + 82), 16);

    return &bse_connection_gateway_response_;
  }

};
}
}
