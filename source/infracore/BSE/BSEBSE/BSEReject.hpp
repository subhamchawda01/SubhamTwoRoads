// =====================================================================================
//
//       Filename:  BSEReject.hpp
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

class BSEReject {
 private:
  RejectT bse_reject_;

 public:
   BSEReject() { memset((void*)(&bse_reject_), 0, sizeof(RejectT)); }
  ~BSEReject() {}

  RejectT *ProcessReject(char const *msg_ptr) {
    //char VarTextTemp[LEN_VAR_TEXT + 1];
    bse_reject_.MessageHeaderOut.BodyLen = *((uint32_t *)(msg_ptr + 0));
    bse_reject_.MessageHeaderOut.TemplateID = *((uint16_t *)(msg_ptr + 4));
    bse_reject_.NRResponseHeaderME.RequestTime = *((uint64_t *)(msg_ptr + 8));
    bse_reject_.NRResponseHeaderME.RequestOut = *((uint64_t *)(msg_ptr + 16));
    bse_reject_.NRResponseHeaderME.TrdRegTSTimeIn = *((uint64_t *)(msg_ptr + 24));
    bse_reject_.NRResponseHeaderME.TrdRegTSTimeOut = *((uint64_t *)(msg_ptr + 32));
    bse_reject_.NRResponseHeaderME.ResponseIn = *((uint64_t *)(msg_ptr + 40));
    bse_reject_.NRResponseHeaderME.SendingTime = *((uint64_t *)(msg_ptr + 48));
    bse_reject_.NRResponseHeaderME.MsgSeqNum = *((uint32_t *)(msg_ptr + 56));
    bse_reject_.NRResponseHeaderME.LastFragment = *((uint8_t *)(msg_ptr + 60));
    bse_reject_.SessionRejectReason = *((uint32_t *)(msg_ptr + 64));
    bse_reject_.VarTextLen = *((uint16_t *)(msg_ptr + 68));
    bse_reject_.SessionStatus = *((uint8_t *)(msg_ptr + 70));
    memcpy((void*)(bse_reject_.VarText), (void*)(msg_ptr + 72), LEN_VAR_TEXT);
    //memcpy((void*)(bse_reject_.VarText), (void*)(VarTextTemp), LEN_VAR_TEXT);

    return &bse_reject_;
  }
};
}
}
