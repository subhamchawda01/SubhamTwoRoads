// =====================================================================================
//
//       Filename:  BSECancelOrderSingleResponse.hpp
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

class BSECancelOrderSingleResponse {
 private:
  DeleteOrderNRResponseT bse_cancel_order_single_short_response_;

 public:
  BSECancelOrderSingleResponse() { memset((void*)(&bse_cancel_order_single_short_response_), 0, sizeof(DeleteOrderNRResponseT)); }
  ~BSECancelOrderSingleResponse() {}

  DeleteOrderNRResponseT *ProcessCancelOrderSingleShortResponse(char const *msg_ptr) {
    bse_cancel_order_single_short_response_.MessageHeaderOut.BodyLen = *((uint32_t *)(msg_ptr + 0));
    bse_cancel_order_single_short_response_.MessageHeaderOut.TemplateID = *((uint16_t *)(msg_ptr + 4));
    bse_cancel_order_single_short_response_.NRResponseHeaderME.RequestTime = *((uint64_t *)(msg_ptr + 8));
    bse_cancel_order_single_short_response_.NRResponseHeaderME.RequestOut = *((uint64_t *)(msg_ptr + 16));
    bse_cancel_order_single_short_response_.NRResponseHeaderME.TrdRegTSTimeIn = *((uint64_t *)(msg_ptr + 24));
    bse_cancel_order_single_short_response_.NRResponseHeaderME.TrdRegTSTimeOut = *((uint64_t *)(msg_ptr + 32));
    bse_cancel_order_single_short_response_.NRResponseHeaderME.ResponseIn = *((uint64_t *)(msg_ptr + 40));
    bse_cancel_order_single_short_response_.NRResponseHeaderME.SendingTime = *((uint64_t *)(msg_ptr + 48));
    bse_cancel_order_single_short_response_.NRResponseHeaderME.MsgSeqNum = *((uint32_t *)(msg_ptr + 56));
    bse_cancel_order_single_short_response_.NRResponseHeaderME.LastFragment = *((uint8_t *)(msg_ptr + 60));
    bse_cancel_order_single_short_response_.OrderID = *((uint64_t *)(msg_ptr + 64));
    bse_cancel_order_single_short_response_.ClOrdID = *((uint64_t *)(msg_ptr + 72));
    bse_cancel_order_single_short_response_.OrigClOrdID = *((uint64_t *)(msg_ptr + 80));
    bse_cancel_order_single_short_response_.SecurityID = *((int64_t *)(msg_ptr + 88));
    bse_cancel_order_single_short_response_.ExecID = *((uint64_t *)(msg_ptr + 96));
    bse_cancel_order_single_short_response_.CumQty = *((int32_t *)(msg_ptr + 104));
    bse_cancel_order_single_short_response_.CxlQty = *((int32_t *)(msg_ptr + 108));
    memcpy((void*)bse_cancel_order_single_short_response_.OrdStatus,
           (void*)(msg_ptr + 112),LEN_ORD_STATUS);
    memcpy((void*)bse_cancel_order_single_short_response_.ExecType,
           (void*)(msg_ptr + 113),LEN_EXEC_TYPE);
    bse_cancel_order_single_short_response_.ExecRestatementReason = *((uint16_t *)(msg_ptr + 114));
    bse_cancel_order_single_short_response_.ProductComplex = *((uint8_t *)(msg_ptr + 116));

    return &bse_cancel_order_single_short_response_;
  }

};
}
}
