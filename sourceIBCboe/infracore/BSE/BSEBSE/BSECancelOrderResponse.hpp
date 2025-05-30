// =====================================================================================
//
//       Filename:  BSECancelOrderResponse.hpp
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

class BSECancelOrderResponse {
 private:
  DeleteOrderResponseT bse_cancel_order_response_;

 public:
  BSECancelOrderResponse() { memset((void*)(&bse_cancel_order_response_), 0, sizeof(DeleteOrderResponseT)); }
  ~BSECancelOrderResponse() {}

  DeleteOrderResponseT *ProcessCancelOrderResponse(char const *msg_ptr) {
    bse_cancel_order_response_.MessageHeaderOut.BodyLen = *((uint32_t *)(msg_ptr + 0));
    bse_cancel_order_response_.MessageHeaderOut.TemplateID = *((uint16_t *)(msg_ptr + 4));
    bse_cancel_order_response_.ResponseHeaderMECompT.RequestTime = *((uint64_t *)(msg_ptr + 8));
    bse_cancel_order_response_.ResponseHeaderMECompT.RequestOut = *((uint64_t *)(msg_ptr + 16));
    bse_cancel_order_response_.ResponseHeaderMECompT.TrdRegTSTimeIn = *((uint64_t *)(msg_ptr + 24));
    bse_cancel_order_response_.ResponseHeaderMECompT.TrdRegTSTimeOut = *((uint64_t *)(msg_ptr + 32));
    bse_cancel_order_response_.ResponseHeaderMECompT.ResponseIn = *((uint64_t *)(msg_ptr + 40));
    bse_cancel_order_response_.ResponseHeaderMECompT.SendingTime = *((uint64_t *)(msg_ptr + 48));
    bse_cancel_order_response_.ResponseHeaderMECompT.MsgSeqNum = *((uint32_t *)(msg_ptr + 56));
    bse_cancel_order_response_.ResponseHeaderME.PartitionID = *((uint16_t *)(msg_ptr + 60));
    bse_cancel_order_response_.ResponseHeaderME.ApplID = *((uint8_t *)(msg_ptr + 62));
    memcpy((void*)bse_cancel_order_response_.ResponseHeaderME.ApplMsgID,
           (void*)(msg_ptr + 63),LEN_APPL_MSGID);
    bse_cancel_order_response_.ResponseHeaderMECompT.LastFragment = *((uint8_t *)(msg_ptr + 79));
    bse_cancel_order_response_.OrderID = *((uint64_t *)(msg_ptr + 80));
    bse_cancel_order_response_.ClOrdID = *((uint64_t *)(msg_ptr + 88));
    bse_cancel_order_response_.OrigClOrdID = *((uint64_t *)(msg_ptr + 96));
    bse_cancel_order_response_.SecurityID = *((int64_t *)(msg_ptr + 104));
    bse_cancel_order_response_.ExecID = *((uint64_t *)(msg_ptr + 112));
    bse_cancel_order_response_.CumQty = *((int32_t *)(msg_ptr + 120));
    bse_cancel_order_response_.CxlQty = *((int32_t *)(msg_ptr + 124));
    memcpy((void*)bse_cancel_order_response_.OrdStatus,
           (void*)(msg_ptr + 128),LEN_ORD_STATUS);
    memcpy((void*)bse_cancel_order_response_.ExecType,
           (void*)(msg_ptr + 129),LEN_EXEC_TYPE);
    bse_cancel_order_response_.ExecRestatementReason = *((uint16_t *)(msg_ptr + 130));
    bse_cancel_order_response_.ProductComplex = *((uint8_t *)(msg_ptr + 132));

    return &bse_cancel_order_response_;
  }

};
}
}
