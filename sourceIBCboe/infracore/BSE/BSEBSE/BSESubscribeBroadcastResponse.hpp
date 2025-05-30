// =====================================================================================
//
//       Filename:  BSESubscribeBroadcastResponse.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  11/04/2012 12:36:04 PM
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

class BSESubscribeBroadcastResponse {
 private:
  SubscribeResponseT bse_user_subscribe_response_;

 public:
   BSESubscribeBroadcastResponse() { memset((void*)(&bse_user_subscribe_response_), 0, sizeof(SubscribeResponseT)); }
  ~BSESubscribeBroadcastResponse() {}

  SubscribeResponseT *ProcessSubscribeBroadcastResponse(char const *msg_ptr) {
    bse_user_subscribe_response_.MessageHeaderOut.BodyLen = (uint32_t *)(msg_ptr + 0);
    bse_user_subscribe_response_.MessageHeaderOut.TemplateID = (uint16_t *)(msg_ptr + 4);
    bse_user_subscribe_response_.ResponseHeader.ResponseTime = (uint64_t *)(msg_ptr + 8);
    bse_user_subscribe_response_.ResponseHeader.SendingTime = (uint64_t *)(msg_ptr + 16);
    bse_user_subscribe_response_.ResponseHeader.MsgSeqNum = (uint32_t *)(msg_ptr + 24);
    bse_user_subscribe_response_.ApplSubID = (uint32_t *)(msg_ptr + 32);

    return &bse_user_subscribe_response_;
  }
};
}
}
