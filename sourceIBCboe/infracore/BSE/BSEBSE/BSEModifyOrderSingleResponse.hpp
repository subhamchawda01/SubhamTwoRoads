// =====================================================================================
//
//       Filename:  BSEModifyOrderSingleResponse.hpp
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

class BSEModifyOrderSingleResponse {
 private:
  ModifyOrderResponseT bse_modify_order_single_response_;

 public:
  BSEModifyOrderSingleResponse() { memset((void*)(&bse_modify_order_single_response_), 0, sizeof(ModifyOrderResponseT)); }
  ~BSEModifyOrderSingleResponse() {}

  ModifyOrderResponseT *ProcessModifyOrderSingleResponse(char const *msg_ptr) {
    bse_modify_order_single_response_.MessageHeaderOut.BodyLen = *((uint32_t *)(msg_ptr + 0));
    bse_modify_order_single_response_.MessageHeaderOut.TemplateID = *((uint16_t *)(msg_ptr + 4));
    bse_modify_order_single_response_.ResponseHeaderME.RequestTime = *((uint64_t *)(msg_ptr + 8));
    bse_modify_order_single_response_.ResponseHeaderME.RequestOut = *((uint64_t *)(msg_ptr + 16));
    bse_modify_order_single_response_.ResponseHeaderME.TrdRegTSTimeIn = *((uint64_t *)(msg_ptr + 24));
    bse_modify_order_single_response_.ResponseHeaderME.TrdRegTSTimeOut = *((uint64_t *)(msg_ptr + 32));
    bse_modify_order_single_response_.ResponseHeaderME.ResponseIn = *((uint64_t *)(msg_ptr + 40));
    bse_modify_order_single_response_.ResponseHeaderME.SendingTime = *((uint64_t *)(msg_ptr + 48));
    bse_modify_order_single_response_.ResponseHeaderME.MsgSeqNum = *((uint32_t *)(msg_ptr + 56));
    bse_modify_order_single_response_.ResponseHeaderME.PartitionID = *((uint16_t *)(msg_ptr + 60));
    bse_modify_order_single_response_.ResponseHeaderME.ApplID = *((uint8_t *)(msg_ptr + 62));
    memcpy((void*)bse_new_order_single_response_.ResponseHeaderME.ApplMsgID,
           (void*)(msg_ptr + 63),LEN_APPL_MSGID);
    bse_modify_order_single_response_.ResponseHeaderME.LastFragment = *((uint8_t *)(msg_ptr + 79));
    bse_modify_order_single_response_.OrderID = *((uint64_t *)(msg_ptr + 80));
    bse_modify_order_single_response_.ClOrdID = *((uint64_t *)(msg_ptr + 88));
    bse_modify_order_single_response_.OrigClOrdID = *((uint64_t *)(msg_ptr + 96));
    bse_modify_order_single_response_.SecurityID = *((int64_t *)(msg_ptr + 104));
    bse_modify_order_single_response_.ExecID = *((uint64_t *)(msg_ptr + 112));
    bse_modify_order_single_response_.PriceMkToLimitPx = *((int64_t *)(msg_ptr + 120));
    bse_modify_order_single_response_.Yield = *((int64_t *)(msg_ptr + 128));
    bse_modify_order_single_response_.UnderlyingDirtyPrice = *((int64_t *)(msg_ptr + 136));
    bse_modify_order_single_response_.ActivityTime = *((uint64_t *)(msg_ptr + 152));
    bse_modify_order_single_response_.LeavesQty = *((int32_t *)(msg_ptr + 172));
    bse_modify_order_single_response_.CumQty = *((int32_t *)(msg_ptr + 176));
    bse_modify_order_single_response_.CxlQty = *((int32_t *)(msg_ptr + 180));
    memcpy((void*)bse_modify_order_single_response_.OrdStatus,
           (void*)(msg_ptr + 186),LEN_ORD_STATUS);
    memcpy((void*)bse_modify_order_single_response_.ExecType,
           (void*)(msg_ptr + 187),LEN_EXEC_TYPE);
    bse_modify_order_single_response_.ExecRestatementReason = *((uint16_t *)(msg_ptr + 188));
    bse_modify_order_single_response_.ProductComplex = *((uint8_t *)(msg_ptr + 190));

    return &bse_modify_order_single_response_;
  }

};
}
}
