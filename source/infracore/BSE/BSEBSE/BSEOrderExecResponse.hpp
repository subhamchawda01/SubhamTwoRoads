// =====================================================================================
//
//       Filename:  BSEOrderExecResponse.hpp
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

class BSEOrderExecResponse {
 private:
  OrderExecResponseT bse_order_exec_response_;

 public:
   BSEOrderExecResponse() { memset((void*)(&bse_order_exec_response_), 0, sizeof(OrderExecResponseT)); }
  ~BSEOrderExecResponse() {}

  OrderExecResponseT *ProcessOrderExecResponse(char const *msg_ptr) {
    bse_order_exec_response_.MessageHeaderOut.BodyLen = *((uint32_t *)(msg_ptr + 0));
    bse_order_exec_response_.MessageHeaderOut.TemplateID = *((uint16_t *)(msg_ptr + 4));
    bse_order_exec_response_.ResponseHeaderME.RequestTime = *((uint64_t *)(msg_ptr + 8));
    bse_order_exec_response_.ResponseHeaderME.RequestOut = *((uint64_t *)(msg_ptr + 16));
    bse_order_exec_response_.ResponseHeaderME.TrdRegTSTimeIn = *((uint64_t *)(msg_ptr + 24));
    bse_order_exec_response_.ResponseHeaderME.TrdRegTSTimeOut = *((uint64_t *)(msg_ptr + 32));
    bse_order_exec_response_.ResponseHeaderME.ResponseIn = *((uint64_t *)(msg_ptr + 40));
    bse_order_exec_response_.ResponseHeaderME.SendingTime = *((uint64_t *)(msg_ptr + 48));
    bse_order_exec_response_.ResponseHeaderME.MsgSeqNum = *((uint32_t *)(msg_ptr + 56));
    bse_order_exec_response_.ResponseHeaderME.PartitionID = *((uint16_t *)(msg_ptr + 60));
    bse_order_exec_response_.ResponseHeaderME.ApplID = *((uint8_t *)(msg_ptr + 62));
    memcpy((void*)bse_order_exec_response_.ResponseHeaderME.ApplMsgID,
           (void*)(msg_ptr + 63),LEN_APPL_MSGID);
    bse_order_exec_response_.ResponseHeaderME.LastFragment = *((uint8_t *)(msg_ptr + 79));
    bse_order_exec_response_.OrderID = *((uint64_t *)(msg_ptr + 80));
    bse_order_exec_response_.ClOrdID = *((uint64_t *)(msg_ptr + 88));
    bse_order_exec_response_.OrigClOrdID = *((uint64_t *)(msg_ptr + 96));
    bse_order_exec_response_.SecurityID = *((int64_t *)(msg_ptr + 104));
    bse_order_exec_response_.ExecID = *((uint64_t *)(msg_ptr + 112));
    bse_order_exec_response_.TrdRegTSEntryTime = *((uint64_t *)(msg_ptr + 120));
    bse_order_exec_response_.TrdRegTSTimePriority = *((uint64_t *)(msg_ptr + 128));
    bse_order_exec_response_.ActivityTime = *((uint64_t *)(msg_ptr + 136));
    bse_order_exec_response_.MarketSegmentID = *((int32_t *)(msg_ptr + 156));
    bse_order_exec_response_.LeavesQty = *((int32_t *)(msg_ptr + 160));
    bse_order_exec_response_.CumQty = *((int32_t *)(msg_ptr + 164));
    bse_order_exec_response_.CxlQty = *((int32_t *)(msg_ptr + 168));
    bse_order_exec_response_.NoLegExecs = *((uint16_t *)(msg_ptr + 174));
    bse_order_exec_response_.ExecRestatementReason = *((uint16_t *)(msg_ptr + 176));
    bse_order_exec_response_.ProductComplex = *((uint8_t *)(msg_ptr + 178));
    memcpy((void*)bse_order_exec_response_.OrdStatus,
           (void*)(msg_ptr + 179),LEN_ORD_STATUS);
    memcpy((void*)bse_order_exec_response_.ExecType,
           (void*)(msg_ptr + 180),LEN_EXEC_TYPE);
    bse_order_exec_response_.Triggered = *((uint8_t *)(msg_ptr + 181));
    bse_order_exec_response_.NoFills = *((uint8_t *)(msg_ptr + 183));
    memcpy((void*)bse_order_exec_response_.AlgoID,
           (void*)(msg_ptr + 184),LEN_ALGOID);
    bse_order_exec_response_.FillsGrp[0].FillPx = *((int64_t *)(msg_ptr + 200));

    return &bse_order_exec_response_;
  }
};
}
}
