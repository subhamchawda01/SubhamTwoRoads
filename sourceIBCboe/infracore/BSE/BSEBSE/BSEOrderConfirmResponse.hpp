// =====================================================================================
//
//       Filename:  BSEOrderConfirmResponse.hpp 
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

class BSEOrderConfirmResponse {
 private:
  GwOrderAcknowledgementT bse_order_confirm_response_;

 public:
  BSEOrderConfirmResponse() { memset((void*)(&bse_order_confirm_response_), 0, sizeof(GwOrderAcknowledgementT)); }
  ~BSEOrderConfirmResponse() {}

  GwOrderAcknowledgementT *ProcessOrderConfirmResponse(char const *msg_ptr) {
    bse_order_confirm_response_.MessageHeaderOut.BodyLen = *((uint32_t *)(msg_ptr + 0));
    bse_order_confirm_response_.MessageHeaderOut.TemplateID = *((uint16_t *)(msg_ptr + 4));
    bse_order_confirm_response_.ResponseHeader.RequestTime = *((uint64_t *)(msg_ptr + 8));
    bse_order_confirm_response_.ResponseHeader.SendingTime = *((uint64_t *)(msg_ptr + 16));
    bse_order_confirm_response_.ResponseHeader.MsgSeqNum = *((uint32_t *)(msg_ptr + 24));
    bse_order_confirm_response_.PrimaryOrderID = *((uint64_t *)(msg_ptr + 32));
    bse_order_confirm_response_.ClOrdID = *((uint64_t *)(msg_ptr + 40));
    bse_order_confirm_response_.MessageTag = *((int32_t *)(msg_ptr + 48));

    return &bse_order_confirm_response_;
  }

};
}
}
