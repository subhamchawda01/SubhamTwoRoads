// =====================================================================================
//
//       Filename:  BSENewOrderSingleShortResponse.hpp
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

class BSENewOrderSingleShortResponse {
 private:
  NewOrderNRResponseT bse_new_order_single_short_response_;

 public:
  BSENewOrderSingleShortResponse() { memset((void*)(&bse_new_order_single_short_response_), 0, sizeof(NewOrderNRResponseT)); }
  ~BSENewOrderSingleShortResponse() {}

  NewOrderNRResponseT *ProcessNewOrderSingleShortResponse(char const *msg_ptr) {
    bse_new_order_single_short_response_.MessageHeaderOut.BodyLen = *((uint32_t *)(msg_ptr + 0));
    bse_new_order_single_short_response_.MessageHeaderOut.TemplateID = *((uint16_t *)(msg_ptr + 4));
    bse_new_order_single_short_response_.NRResponseHeaderME.RequestTime = *((uint64_t *)(msg_ptr + 8));
    bse_new_order_single_short_response_.NRResponseHeaderME.RequestOut = *((uint64_t *)(msg_ptr + 16));
    bse_new_order_single_short_response_.NRResponseHeaderME.TrdRegTSTimeIn = *((uint64_t *)(msg_ptr + 24));
    bse_new_order_single_short_response_.NRResponseHeaderME.TrdRegTSTimeOut = *((uint64_t *)(msg_ptr + 32));
    bse_new_order_single_short_response_.NRResponseHeaderME.ResponseIn = *((uint64_t *)(msg_ptr + 40));
    bse_new_order_single_short_response_.NRResponseHeaderME.SendingTime = *((uint64_t *)(msg_ptr + 48));
    bse_new_order_single_short_response_.NRResponseHeaderME.MsgSeqNum = *((uint32_t *)(msg_ptr + 56));
    bse_new_order_single_short_response_.NRResponseHeaderME.LastFragment = *((uint8_t *)(msg_ptr + 60));
    bse_new_order_single_short_response_.OrderID = *((uint64_t *)(msg_ptr + 64));
    bse_new_order_single_short_response_.ClOrdID = *((uint64_t *)(msg_ptr + 72));
    bse_new_order_single_short_response_.SecurityID = *((int64_t *)(msg_ptr + 80));
    bse_new_order_single_short_response_.PriceMkToLimitPx = *((int64_t *)(msg_ptr + 88));
    bse_new_order_single_short_response_.Yield = *((int64_t *)(msg_ptr + 96));
    bse_new_order_single_short_response_.UnderlyingDirtyPrice = *((int64_t *)(msg_ptr + 104));
    bse_new_order_single_short_response_.ExecID = *((uint64_t *)(msg_ptr + 112));
    bse_new_order_single_short_response_.ActivityTime = *((uint64_t *)(msg_ptr + 120));
    memcpy((void*)bse_new_order_single_short_response_.OrdStatus,
           (void*)(msg_ptr + 142),LEN_ORD_STATUS);
    memcpy((void*)bse_new_order_single_short_response_.ExecType,
           (void*)(msg_ptr + 143),LEN_EXEC_TYPE);
    bse_new_order_single_short_response_.ExecRestatementReason = *((uint16_t *)(msg_ptr + 144));
    bse_new_order_single_short_response_.ProductComplex = *((uint8_t *)(msg_ptr + 146));

    return &bse_new_order_single_short_response_;
  }

};
}
}
